from pathlib import Path
import contextlib, ctypes as C, hashlib, json, mmap, os, struct, sys, threading, time, copy, platform, stat, shlex

def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(65536), b''):
            h.update(block)
    return h.hexdigest()

def inside(root, relative):
    p = Path(relative)
    if p.is_absolute() or not p.parts or any((x in ('', '.', '..') for x in relative.split('/'))) or ('\\' in relative):
        raise ValueError('canonical Documents-relative path required')
    result = Path(root) / p
    if result.resolve() != result:
        raise ValueError('symlink/noncanonical path rejected')
    return result

def verify(root, row):
    p = inside(root, row['path'])
    if not p.is_file() or p.stat().st_size != row['bytes']:
        raise ValueError('immutable input byte length mismatch: ' + row['path'])
    if row.get('sha256') is not None:
        if digest(p) != row['sha256']:
            raise ValueError('immutable input SHA mismatch: ' + row['path'])
    elif 'frame_sha256' in row:
        frame_bytes = row['frame_bytes']
        if row['bytes'] != frame_bytes * len(row['frame_sha256']):
            raise ValueError('frame hashes do not cover complete bank')
        with p.open('rb') as f:
            for expected in row['frame_sha256']:
                h = hashlib.sha256()
                left = frame_bytes
                while left:
                    block = f.read(min(65536, left))
                    if not block:
                        raise ValueError('short bank')
                    h.update(block)
                    left -= len(block)
                if h.hexdigest() != expected:
                    raise ValueError('actual native per-ref SHA mismatch')
    else:
        raise ValueError('no measured input SHA/frame SHA identity')
    return p

def save(path, obj):
    with Path(path).open('x') as f:
        json.dump(obj, f, indent=2)
        f.write('\n')

@contextlib.contextmanager
def mapping(path, writable=False, byte_length=None):
    mode = 'x+b' if writable else 'rb'
    with Path(path).open(mode) as f:
        if writable:
            f.truncate(byte_length)
        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_WRITE if writable else mmap.ACCESS_READ)
        view = memoryview(mm)
        try:
            yield view
        finally:
            view.release()
            if writable:
                mm.flush()
            mm.close()

class MemoryObserver:
    """Existing task_info layout; asynchronous 100ms plus call boundaries.
    Native operations cannot be preempted by this Python observer. A breach is
    latched and prevents the next call; root external controller owns hard stop.
    """

    def __init__(self, out, snapshot=None):
        self.phase = 'PREPARING'
        self.stop = threading.Event()
        self.failure = None
        self.rows = 0
        self.peak = 0
        self.last = None
        self.max_gap_ns = 0
        self.stream = (out / 'MEMORY_100MS.jsonl').open('x')
        self.lock = threading.RLock()
        self.snapshot = snapshot or self._ios_snapshot
        self.thread = threading.Thread(target=self.loop, daemon=True)

    def _ios_snapshot(self):
        lib = C.CDLL('/usr/lib/libSystem.B.dylib')
        task = C.c_uint.in_dll(lib, 'mach_task_self_').value
        fn = lib.task_info
        fn.argtypes = [C.c_uint, C.c_int, C.POINTER(C.c_uint32), C.POINTER(C.c_uint32)]
        fn.restype = C.c_int
        vm = (C.c_uint32 * 256)()
        n = C.c_uint32(256)
        rc = fn(task, 22, vm, C.byref(n))
        if rc or n.value < 44:
            raise RuntimeError('TASK_VM_INFO rc/count invalid')
        raw = bytes(vm)
        phys = int.from_bytes(raw[144:152], 'little')
        lifetime = int.from_bytes(raw[168:176], 'little')
        basic = (C.c_uint32 * 12)()
        bn = C.c_uint32(12)
        brc = fn(task, 20, basic, C.byref(bn))
        if brc or bn.value != 12 or phys <= 0 or (lifetime < phys):
            raise RuntimeError('TASK_BASIC/physical invalid')
        return {'phys_footprint_bytes': phys, 'lifetime_peak_bytes': lifetime, 'rss_bytes': int.from_bytes(bytes(basic)[8:16], 'little'), 'vm_rc': rc, 'rss_rc': brc}

    def sample(self, boundary='periodic'):
        with self.lock:
            return self._sample(boundary)

    def _sample(self, boundary):
        row = self.snapshot()
        now = time.monotonic_ns()
        if self.last is not None:
            self.max_gap_ns = max(self.max_gap_ns, now - self.last)
        self.last = now
        self.rows += 1
        self.peak = max(self.peak, row['phys_footprint_bytes'])
        row.update(monotonic_ns=now, epoch_ns=time.time_ns(), stage=self.phase, boundary=boundary)
        self.stream.write(json.dumps(row) + '\n')
        self.stream.flush()
        if row['phys_footprint_bytes'] > 2000000000:
            self.failure = 'whole-process sampled physical >2,000,000,000B'
        return row

    def loop(self):
        try:
            while not self.stop.wait(0.1):
                self.sample()
        except BaseException as e:
            self.failure = repr(e)

    def begin(self):
        self.sample('observer_start')
        self.thread.start()

    def boundary(self, phase, kind):
        self.phase = phase
        self.sample(kind)
        if self.failure:
            raise RuntimeError(self.failure)

    def finish(self):
        self.stop.set()
        self.thread.join()
        self.sample('observer_stop')
        self.stream.close()
        return {'samples': self.rows, 'max_gap_ns': self.max_gap_ns, 'sampled_phys_peak_bytes': self.peak, 'failure': self.failure, 'coverage': '100ms Python-thread samples plus boundaries; GIL/native stalls can miss peaks; root external observer required; lifetime peak is not run-local'}
