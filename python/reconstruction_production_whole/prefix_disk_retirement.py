"""Exact-file, same-run retirement after native release and final source checks.

No tree deletion, retry, numerical work, or inference about an unfinished owner.
A durable receipt precedes every unlink; failure keeps all remaining inputs.
"""
import hashlib
import json
import os
from pathlib import Path
import stat


def _require(ok, message):
    if not ok:
        raise ValueError(message)


def _canonical(path):
    path = Path(path)
    _require(path.is_absolute() and path.resolve(strict=True) == path,
             'retirement requires existing canonical nonsymlink path')
                                                                             
                                                                 
    for part in (path, *path.parents):
        _require(not part.is_symlink(), 'retirement symlink forbidden')
    return path


def _identity(info):
    return (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns)


class PrefixRetirement:
    def __init__(self, root, run_id, *, additional_roots=()):
        self.root = _canonical(root)
        _require(self.root.is_dir() and self.root != Path('/'), 'private actual run root required')
        _require(isinstance(run_id, str) and run_id and '/' not in run_id,
                 'same-run identity required')
        _require(self.root.stat().st_uid == os.getuid() and
                 self.root.stat().st_mode & 0o077 == 0, 'private current-owner run root required')
        self.run_id = run_id
        self.roots = [self.root]
        for other in additional_roots:
            other = _canonical(other)
            _require(other != self.root.parent and other.is_relative_to(self.root.parent)
                     and other.is_dir(), 'additional generated root must share private run output')
            self.roots.append(other)
        self.ledger = self.root/'PREFIX_RETIREMENT.jsonl'
        with self.ledger.open('x') as stream:
            stream.write(json.dumps(dict(event='RUN_BOUND', run_id=run_id,
                root=str(self.root), roots=list(map(str, self.roots))))+'\n')
            stream.flush(); os.fsync(stream.fileno())
        self.root_id = _identity(self.root.stat())[:2]
        self.ledger_id = _identity(self.ledger.stat())[:2]
        self.files = {}
        self.completed = set()
        self.reference_next = {0: 0, 1: 0}
        self.feature_payload_bytes = 0
        self.retired_feature_payload_bytes = 0

    def _check(self, path):
        _require(_identity(_canonical(self.root).stat())[:2] == self.root_id,
                 'actual run root replaced')
        _require(_identity(_canonical(self.ledger).stat())[:2] == self.ledger_id,
                 'same-run ledger replaced')
        path = _canonical(path)
        _require(path != self.ledger and any(path != r and path.is_relative_to(r) for r in self.roots),
                 'outside registered generated run roots')
        _require(stat.S_ISREG(path.lstat().st_mode) and path.stat().st_nlink == 1,
                 'only private regular single-link generated files may retire')
        return path

    def add_generated_root(self, root):
        root = _canonical(root)
                                                                             
                                                                         
        run_root = self.root.parent.parent
        allowed_sibling = root in (run_root/'original_fixture', run_root/'inputs')
        _require(root.is_dir() and root != self.root.parent and
                 (root.is_relative_to(self.root.parent) or allowed_sibling),
                 "additional generated root must belong to this exact run")
        self.roots.append(root)

    def register(self, path, *, consumers, producer_sha256=None, producer_manifest_sha256=None):
        path = self._check(path)
        _require(path not in self.files and consumers and all(type(c) is str for c in consumers),
                 'unique explicit file and last readers required')
        self.files[path] = dict(identity=_identity(path.stat()), consumers=tuple(consumers),
                               producer_sha256=producer_sha256,
                               producer_manifest_sha256=producer_manifest_sha256)
        if producer_sha256 is not None:
            self.feature_payload_bytes += self.files[path]['identity'][2]

    def complete(self, event):
        self.completed.add(event)

    def _record(self, row):
        with self.ledger.open('a') as stream:
            stream.write(json.dumps(dict(row, run_id=self.run_id), sort_keys=True)+'\n')
            stream.flush(); os.fsync(stream.fileno())

    def retire_features_after_reference(self, pass_index, ref, *, producer_pending):
        """Only call from actual wrapper phase2, after Run/finite/hash/write/receipt.

        Pinned mmapcore closes each HostMaterialHalf after CopyTensors; BankIndex
        holds paths only. Native GPU slots own independent uploaded buffers.
        """
        _require(type(pass_index) is int and pass_index in (0, 1) and
                 type(ref) is int and ref == self.reference_next[pass_index],
                 "actual ordered per-pass reference completion required")
        _require(pass_index == 0 or self.reference_next[0] > ref, "LOW must precede HIGH")
        _require(producer_pending is False, "feature producer owner still active")
        self.reference_next[pass_index] += 1
        self.complete(f"feature:{pass_index}:{ref}")
        ready = {p: e for p, e in self.files.items() if
                 all(c.startswith(f"feature:{pass_index}:") for c in e["consumers"]) and
                 set(e["consumers"]) <= self.completed}
        return self._retire_entries(ready, "MVS_REFERENCE_RETURNED")

    def retire_remaining_features_after_mvs(self, *, owner_state, producer_pending):
                                                                                
                                                                                 
        _require(owner_state == 'MVS_COMPLETED_ENV_DEVICE_RETAINED' and producer_pending is False,
                 'successful full MVS return required for remaining feature payloads')
        remaining = {p: e for p, e in self.files.items()
                     if all(c.startswith('feature:') for c in e['consumers'])}
        for entry in remaining.values():
            self.completed.update(entry['consumers'])
        receipt = self._retire_entries(remaining, 'MVS_ALL_FEATURE_READERS_CLOSED')
        receipt.update(manifest_payload_bytes=self.feature_payload_bytes,
                       total_feature_unlinked_bytes=self.retired_feature_payload_bytes,
                       already_unlinked_before_full_return_bytes=
                           self.retired_feature_payload_bytes-receipt['released_bytes'])
        self._record(dict(receipt, event='FEATURE_BANKS_ALL_RETIRED'))
        return receipt

    def retire_mvs_inputs(self, *, owner_state, producer_pending):
                                                                              
                                                                              
        _require(owner_state == 'MVS_COMPLETED_ENV_DEVICE_RETAINED' and producer_pending is False,
                 'successful full MVS return and input validation required')
        ready = {p: e for p, e in self.files.items()
                 if e['consumers'] == ('mvs_input_validation',)}
        return self._retire_entries(ready, 'MVS_INPUTS_CLOSED')

    def retire_fusion_rgb(self, *, owner_state, producer_pending):
        _require(owner_state == 'DESTROYED_SUFFIX_READY' and producer_pending is False,
                 'successful Fusion emit and closed RGB streams required')
        ready = {p: e for p, e in self.files.items()
                 if e['consumers'] == ('fusion_rgb_emit',)}
        return self._retire_entries(ready, 'FUSION_RGB_EMIT_CLOSED')

    def retire_cer_sources(self, *, owner_state, producer_pending):
                                                                         
                                                                                  
                                                                     
        _require(owner_state == 'MVS_COMPLETED_ENV_DEVICE_RETAINED' and producer_pending is False,
                 'successful full MVS and closed CER mappings required')
        self.complete('cer')
        ready = {p: e for p, e in self.files.items() if e['consumers'] == ('cer',)}
        return self._retire_entries(ready, 'CER_RETURNED_MAPPINGS_CLOSED')

    def retire(self, *, owner_state, producer_pending):
        _require(owner_state == 'DESTROYED_SUFFIX_READY' and producer_pending is False,
                 'successful destroy0 and released feature owner required')
        return self._retire_entries(dict(self.files), owner_state)

    def _retire_entries(self, entries, owner_state):
                                                                           
        for path, entry in entries.items():
            self._check(path)
            _require(set(entry['consumers']) <= self.completed, 'future consumer still active')
            _require(_identity(path.stat()) == entry['identity'], 'generated file changed before retirement')
        freed = 0
        for path, entry in entries.items():
            self._check(path)
            fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW)
            parent_fd = None
            parent_mode = None
            feature_object_retired = False
            try:
                _require(_identity(os.fstat(fd)) == entry['identity'], 'file replaced during retirement')
                known_sha = entry.get('producer_sha256')
                if known_sha is None:
                    h = hashlib.sha256()
                    with os.fdopen(fd, 'rb', closefd=False) as stream:
                        for block in iter(lambda: stream.read(1024*1024), b''):
                            h.update(block)
                    known_sha = h.hexdigest()
                                                                              
                                                                               
                                                                             
                _require(_identity(os.fstat(fd)) == entry['identity'], 'file changed before retirement receipt')
                receipt = dict(event='PREPARED', path=str(path), bytes=entry['identity'][2],
                    sha256=known_sha, consumers=entry['consumers'], owner_state=owner_state,
                    producer_manifest_sha256=entry.get('producer_manifest_sha256'),
                    sha256_source='VERIFIED_PRODUCER_MANIFEST' if entry.get('producer_sha256') else 'READ_THIS_RETIREMENT')
                self._record(receipt)
                self._check(path)
                _require(_identity(path.stat()) == entry['identity'], 'file replaced before unlink')
                                                                             
                                                                                 
                parent = _canonical(path.parent)
                parent_mode = stat.S_IMODE(parent.stat().st_mode)
                if not parent_mode & stat.S_IWUSR:
                    _require(path.name == 'features.f16' and len(parent.name) == 64 and
                             all(c in '0123456789abcdef' for c in parent.name) and
                             all(c.startswith('feature:') for c in entry['consumers']) and
                             parent.stat().st_uid == os.getuid(), 'only owned sealed feature object may unfreeze')
                    parent_fd = os.open(parent, os.O_RDONLY | os.O_NOFOLLOW | os.O_DIRECTORY)
                    os.fchmod(parent_fd, 0o700)
                path.unlink()
                freed += entry['identity'][2]
                if entry.get('producer_sha256') is not None:
                    self.retired_feature_payload_bytes += entry['identity'][2]
                self._record(dict(event='UNLINKED', path=str(path), bytes=entry['identity'][2]))
                del self.files[path]
                if parent_fd is not None:
                                                                                
                                                                               
                                                                              
                    seal = self._check(parent/'seal.txt')
                    seal_bytes = seal.stat().st_size
                    seal.unlink()
                    self._record(dict(event='FEATURE_SEAL_RETIRED', path=str(seal),
                                      bytes=seal_bytes, owner_state=owner_state))
                    parent.rmdir()                                                 
                    feature_object_retired = True
            finally:
                try:
                    if parent_fd is not None:
                        try:
                            if not feature_object_retired: os.fchmod(parent_fd, parent_mode)
                        finally: os.close(parent_fd)
                finally:
                    os.close(fd)
        event = {'DESTROYED_SUFFIX_READY':'PREFIX_VALIDATED_DESTROYED',
                 'MVS_REFERENCE_RETURNED':'FEATURE_LAST_CONSUMERS_RETURNED',
                 'CER_RETURNED_MAPPINGS_CLOSED':'CER_SOURCE_CONFIDENCE_RETIRED',
                 'MVS_INPUTS_CLOSED':'MVS_INPUT_GRAY_RETIRED',
                 'FUSION_RGB_EMIT_CLOSED':'FUSION_RGB_RETIRED',
                 'MVS_ALL_FEATURE_READERS_CLOSED':'REMAINING_FEATURE_BANKS_RETIRED'}[owner_state]
        result = dict(event=event, released_bytes=freed, files=len(entries), ledger=str(self.ledger))
        self._record(result)
        return result


def feature_reference_consumers(neighbors):
    """Exact self + nine ordered original neighbors; no locality assumptions."""
    n = len(neighbors)
    consumers = [[] for _ in range(n)]
    for ref, neighbors_for_ref in enumerate(neighbors):
        views = (ref,) + tuple(neighbors_for_ref)
        _require(len(views) == 10 and len(set(views)) == 10 and
                 all(type(v) is int and 0 <= v < n for v in views),
                 'actual ordered self + nine neighbor IDs required')
        for view in views:
            consumers[view].append(ref)
    return tuple(tuple(refs) for refs in consumers)
