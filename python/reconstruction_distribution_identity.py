"""Pinned native identities that tolerate only trailing signature replacement."""

import hashlib
import json
import os
from pathlib import Path
import re
import stat
import struct

SCHEMA = 'RECONSTRUCTION_TESTFLIGHT_NATIVE_IDENTITIES_V1'
REGISTRY_RELATIVE_PATH = 'reconstruction_production/DISTRIBUTION_IDENTITIES.json'
_BUNDLE_ID_SETTINGS = (
    'WORLDSWITHIN_PRODUCTION_BUNDLE_ID',
    'WORLDSWITHIN_BENCH_BUNDLE_ID',
    'WORLDSWITHIN_PRODUCT_COPY_BUNDLE_ID',
)
_BUNDLE_ID = re.compile(r'[A-Za-z0-9-]+(?:\.[A-Za-z0-9-]+)+')


def configured_bundle_id(setting):
    value = os.environ.get(setting, '')
    if not _BUNDLE_ID.fullmatch(value):
        return None
    if any(os.environ.get(other) == value for other in _BUNDLE_ID_SETTINGS if other != setting):
        return None
    return value


def _sha(value):
    return isinstance(value, str) and re.fullmatch(r'[0-9a-f]{64}', value) is not None


def canonical_identity(data):
    """Hash the pre-signature bytes, normalizing the evidenced signing-size fields.

    This does not authenticate Apple's signature. The trusted registry digest
    supplies the identity pin. Unsupported or malformed images fail closed.
    """
    data = bytes(data)
    if len(data) < 32:
        raise ValueError('Truncated Mach-O header')
    magic, cpu, _, _, ncmds, sizeofcmds, _, _ = struct.unpack_from('<8I', data)
    if magic != 0xfeedfacf or cpu != 0x100000c:
        raise ValueError('Only thin little-endian arm64 Mach-O is supported')
    commands_end = 32 + sizeofcmds
    if commands_end > len(data) or ncmds > sizeofcmds // 8:
        raise ValueError('Invalid Mach-O load-command bounds')
    cursor = 32
    linkedit = None
    signature = None
    segments = []
    encryption_seen = False
    for _ in range(ncmds):
        if cursor + 8 > commands_end:
            raise ValueError('Truncated Mach-O load command')
        command, size = struct.unpack_from('<II', data, cursor)
        if size < 8 or size % 8 or cursor + size > commands_end:
            raise ValueError('Invalid Mach-O load-command size')
        if command == 0x19:
            if size < 72:
                raise ValueError('Truncated segment command')
            name = data[cursor + 8:cursor + 24]
            offset, length = struct.unpack_from('<QQ', data, cursor + 40)
            sections = struct.unpack_from('<I', data, cursor + 64)[0]
            if size != 72 + 80 * sections or offset + length > len(data):
                raise ValueError('Invalid segment bounds')
            if length:
                segments.append((offset, offset + length))
            if name == b'__LINKEDIT' + b'\0' * 6:
                if linkedit is not None:
                    raise ValueError('Duplicate __LINKEDIT segment')
                vm_size = struct.unpack_from('<Q', data, cursor + 32)[0]
                if vm_size != ((length + 16383) // 16384) * 16384:
                    raise ValueError('Unsupported __LINKEDIT virtual size')
                linkedit = (offset, offset + length, cursor + 48)
        elif command == 0x1d:
            if size != 16 or signature is not None:
                raise ValueError('Invalid code-signature command')
            offset, length = struct.unpack_from('<II', data, cursor + 8)
            signature = (offset, length, cursor + 12)
        elif command == 0x2c:
            if size != 24 or encryption_seen:
                raise ValueError('Invalid encryption command')
            encryption_seen = True
            offset, length, cryptid, pad = struct.unpack_from('<4I', data, cursor + 8)
            if cryptid != 0 or pad != 0 or offset + length > len(data):
                raise ValueError('Encrypted or malformed native image')
        cursor += size
    if cursor != commands_end or linkedit is None or signature is None:
        raise ValueError('Missing or inconsistent Mach-O commands')
    segments.sort()
    if any(left[1] > right[0] for left, right in zip(segments, segments[1:])):
        raise ValueError('Overlapping Mach-O segments')
    offset, length, size_field = signature
    if (length == 0 or offset < commands_end or offset + length != len(data)
            or offset < linkedit[0] or offset + length != linkedit[1]):
        raise ValueError('Code signature must be the exact trailing __LINKEDIT range')
    canonical = bytearray(data[:offset])
    canonical[linkedit[2]:linkedit[2] + 8] = b'\0' * 8
    canonical_vm_size = ((offset - linkedit[0] + 16383) // 16384) * 16384
    struct.pack_into('<Q', canonical, linkedit[2] - 16, canonical_vm_size)
    canonical[size_field:size_field + 4] = b'\0' * 4
    return dict(canonical_sha256=hashlib.sha256(canonical).hexdigest(),
                canonical_length=offset, cpu_type=cpu)


def _canonical_regular(path):
    path = Path(path).absolute()
    if path != path.resolve(strict=True) or not stat.S_ISREG(path.lstat().st_mode):
        raise ValueError('Native identity paths must be canonical regular files')
    return path


def verify_native_binary(path, expected_sha256, *, expected_bytes=None,
                         expected_registry_sha256, registry_path):
    """Preserve exact raw verification first, then try the pinned native registry."""
    path = Path(path)
    data = path.read_bytes()
    if (hashlib.sha256(data).hexdigest() == expected_sha256
            and (expected_bytes is None or len(data) == expected_bytes)):
        return
    if not _sha(expected_sha256) or not _sha(expected_registry_sha256):
        raise ValueError('Invalid native identity pin')
    path = _canonical_regular(path)
    app = next((parent for parent in path.parents if parent.name.endswith('.app')), None)
    if app is None or registry_path is None:
        raise ValueError('Native image has no application registry')
    registry_path = _canonical_regular(registry_path)
    if registry_path != app / REGISTRY_RELATIVE_PATH:
        raise ValueError('Unexpected native identity registry path')
    registry_data = registry_path.read_bytes()
    if hashlib.sha256(registry_data).hexdigest() != expected_registry_sha256:
        raise ValueError('Native identity registry digest mismatch')
    try:
        registry = json.loads(registry_data)
        if registry['schema'] != SCHEMA or not isinstance(registry['entries'], dict):
            raise ValueError('Unsupported native identity registry')
        row = registry['entries'][path.relative_to(app).as_posix()]
        baseline_bytes = row['baseline_bytes']
        canonical_length = row['canonical_length']
        if (not isinstance(baseline_bytes, int) or isinstance(baseline_bytes, bool)
                or baseline_bytes <= 0 or not isinstance(canonical_length, int)
                or isinstance(canonical_length, bool) or canonical_length <= 0
                or row['baseline_sha256'] != expected_sha256
                or (expected_bytes is not None and baseline_bytes != expected_bytes)
                or not _sha(row['canonical_sha256'])):
            raise ValueError('Native identity baseline mismatch')
        identity = canonical_identity(data)
        if (identity['canonical_sha256'] != row['canonical_sha256']
                or identity['canonical_length'] != canonical_length):
            raise ValueError('Native canonical identity mismatch')
    except (KeyError, TypeError, json.JSONDecodeError) as error:
        raise ValueError('Invalid native identity registry') from error


                                                                         
PINNED_REGISTRY_SHA256 = 'fc3655dc890efe93e21bb6b89c7d092e5c36e93953227ce576adb0a7e7428447'


def require_bundle_identity(bundle):
    import plistlib
    import sys
    root = Path(bundle)
    if (not root.is_absolute() or root.suffix != '.app'
            or root != root.resolve(strict=True) or not root.is_dir()
            or not stat.S_ISDIR(root.lstat().st_mode)):
        raise ValueError('Canonical application directory required')
    executable = Path(sys.executable).resolve(strict=True)
    if not executable.is_file() or root != executable.parent:
        raise ValueError('Application root differs from current process')
    info_path = _canonical_regular(root / 'Info.plist')
    info = plistlib.loads(info_path.read_bytes())
    allowed = {configured_bundle_id(setting) for setting in _BUNDLE_ID_SETTINGS[:2]}
    allowed.discard(None)
    if info.get('CFBundleIdentifier') not in allowed:
        raise ValueError('Unapproved application identity')
    registry_path = _canonical_regular(root / REGISTRY_RELATIVE_PATH)
    if hashlib.sha256(registry_path.read_bytes()).hexdigest() != PINNED_REGISTRY_SHA256:
        raise ValueError('Native identity registry digest mismatch')
    return True


def native_digest(path):
    path = _canonical_regular(path)
    app = next((parent for parent in path.parents if parent.name.endswith('.app')), None)
    if app is None:
        raise ValueError('Native image outside application')
    registry_path = _canonical_regular(app / REGISTRY_RELATIVE_PATH)
    data = registry_path.read_bytes()
    if hashlib.sha256(data).hexdigest() != PINNED_REGISTRY_SHA256:
        raise ValueError('Native identity registry digest mismatch')
    registry = json.loads(data)
    if registry.get('schema') != SCHEMA or not isinstance(registry.get('entries'), dict):
        raise ValueError('Invalid pinned native registry schema')
    row = registry['entries'][path.relative_to(app).as_posix()]
    verify_native_binary(path, row['baseline_sha256'],
        expected_bytes=row['baseline_bytes'], expected_registry_sha256=PINNED_REGISTRY_SHA256,
        registry_path=registry_path)
    return row['baseline_sha256']


def native_matches(path, expected_sha256):
    try:
        return native_digest(path) == expected_sha256
    except (ValueError, OSError, KeyError, TypeError):
        return False


def native_verify(root, row):
    relative = row['path']
    parts = relative.split('/')
    if not parts or any(part in ('', '.', '..') for part in parts) or relative.startswith('/'):
        raise ValueError('Noncanonical native row path')
    path = _canonical_regular(Path(root) / relative)
    registry_path = _canonical_regular(Path(root) / REGISTRY_RELATIVE_PATH)
    if native_digest(path) != row['sha256']:
        raise ValueError('Native row logical identity mismatch')
    registry = json.loads(registry_path.read_bytes())
    if registry['entries'][relative]['baseline_bytes'] != row['bytes']:
        raise ValueError('Native row baseline length mismatch')
    verify_native_binary(path, row['sha256'], expected_bytes=row['bytes'],
        expected_registry_sha256=PINNED_REGISTRY_SHA256, registry_path=registry_path)
    return path
