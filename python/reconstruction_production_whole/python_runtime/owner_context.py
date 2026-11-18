"""Single existing-App process owner. No autorun, device IO, allocator or math."""
from pathlib import Path
import ctypes as C,hashlib,json,os,time
from .phone_owner_calls import RetainedWholeOwner
CURRENT=None
def digest(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(65536),b''):h.update(b)
 return h.hexdigest()
def inside(root,relative):
 p=Path(relative)
 if p.is_absolute()or any(x in ('','.','..')for x in relative.split('/')):raise ValueError('canonical relative asset')
 p=Path(root)/p
 if p.resolve()!=p:raise ValueError('noncanonical asset')
 return p
def verify_signed(docs,row):
 p=inside(docs,row['device_path']);h=row.get('signed_sha256');n=row.get('signed_bytes')
 if type(h)is not str or len(h)!=64 or type(n)is not int or n<=0:raise ValueError('phoneowner must bind actual signed hash/bytes; unsignedMac is insufficient')
 if p.stat().st_size!=n or digest(p)!=h:raise ValueError('actual signed artifact mismatch '+row['device_path'])
 return p
class MemorySnapshot(C.Structure):
 _fields_=[(k,C.c_uint32)for k in ('abi','struct_size','valid_bits','event')]+[(k,C.c_uint64)for k in ('sequence','monotonic_ns','device_identity','request_B','managed_estimated_B','buffer_estimated_B','texture_estimated_B','metal_device_allocated_B','process_phys_footprint_B','process_resident_B','allocation_requests','cache_hits','releases','refreshes','rejections','peak_managed_estimated_B','peak_metal_device_allocated_B','peak_process_phys_footprint_B')]+[('rejection_domain',C.c_uint32),('reserved',C.c_uint32)]
assert C.sizeof(MemorySnapshot)==168
class Borrow(C.Structure):
 pass
Ref=C.CFUNCTYPE(None,C.c_void_p)
Borrow._fields_=[('size',C.c_uint32),('abi',C.c_uint32)]+[(k,C.c_void_p)for k in ('owner','instance','device','procs')]+[('procbytes',C.c_size_t),('sdk',C.c_char_p),('retain',Ref),('release',Ref)]
class Session:
 def __init__(self,rt,co,provider,m0,thermal,root,Request,*,production=False):
  self.rt=rt;self.co=co;self.provider=provider;self.m0=m0;self.thermal=thermal;self.root=Path(root);self.events=[];self.host_refs=0;self.exclusive=False;self.MVS_device=None;self.last_fusion=None
  rt.reconstruction_fusion_post_sample_os_v1.argtypes=[C.POINTER(provider.Snapshot)];rt.reconstruction_fusion_post_sample_os_v1.restype=C.c_int
  rt.reconstruction_s1_memory_snapshot.argtypes=[C.POINTER(MemorySnapshot),C.c_size_t];rt.reconstruction_s1_memory_snapshot.restype=C.c_int
  rt.reconstruction_s1_device_caps_json.argtypes=[C.c_char_p,C.c_size_t];rt.reconstruction_s1_device_caps_json.restype=C.c_int
  rt.reconstruction_fusion_existing_owner_borrow_v1.argtypes=[C.POINTER(Borrow)];rt.reconstruction_fusion_existing_owner_borrow_v1.restype=C.c_int
  @provider.RefCB
  def retain(_):self.host_refs+=1
  @provider.RefCB
  def release(_):self.host_refs-=1
  @provider.EnterCB
  def enter(_):
   if self.exclusive:return 7
   self.exclusive=True;return 0
  @provider.RefCB
  def leave(_):self.exclusive=False
  @provider.SampleCB
  def sample(_,ptr):
   try:
    provider.wait_thermal(thermal,'WHOLE_NATIVE_BUDGET',self.root/'THERMAL_PAUSE.jsonl')
    return provider.fresh_budget_snapshot(m0,rt,ptr)
   except BaseException as e:self.record_noexcept({'stage':'BUDGET_CALLBACK_ERROR','error':repr(e)});return 6
  rt.reconstruction_fusion_existing_owner_runtime_identity_v1.restype=C.c_char_p
  config=provider.Config(C.sizeof(provider.Config),1,1536,1152,0,30000,C.c_void_p(1),retain,release,enter,leave,sample,b'09f60549925d00e4f3972a9d623fe4aa6a1766c5cec308f8eda6ab2ca794aa47',rt.reconstruction_fusion_existing_owner_runtime_identity_v1())
  self.owner=RetainedWholeOwner(rt,co,config,Request,provider.Input,self.boundary,self.record,[m0,thermal,retain,release,enter,leave,sample],production=production)
 def record(self,row):
  row=dict(row,monotonic_ns=time.monotonic_ns());self.events.append(row);(self.root/'OWNER_EVENTS.json').write_text(json.dumps(self.events,indent=2)+'\n')
 def record_noexcept(self,row):
  try:self.record(row)
  except BaseException:pass                                                      
 def boundary(self,label):
  self.provider.wait_thermal(self.thermal,label,self.root/'THERMAL_PAUSE.jsonl')
  rc=self.m0.reconstruction_m0_observer_boundary(label.encode())
  if rc:raise RuntimeError('continuous external M0 failed/cap '+str(rc))
 def snapshot(self,label):
  s=MemorySnapshot();s.abi=1;s.struct_size=C.sizeof(s);rc=self.rt.reconstruction_s1_memory_snapshot(C.byref(s),C.sizeof(s))
  if rc or not s.sequence or not s.device_identity:raise RuntimeError('actual MVS device identity unavailable')
  if self.MVS_device is None:self.MVS_device=int(s.device_identity)
  if int(s.device_identity)!=self.MVS_device:raise RuntimeError('MVS realDawn device changed')
  raw=C.create_string_buffer(65536);rc=self.rt.reconstruction_s1_device_caps_json(raw,len(raw))
  if rc:raise RuntimeError('actual runtime caps unavailable')
  caps=json.loads(raw.value);fifo=caps['bounded_submit']
  if fifo['max_inflight_policy']!=4 or fifo['invalid_policy']or fifo['failed_latch']:
   self.record_noexcept({'stage':'MVS_FIFO_POLICY_REJECTED','label':label,'expected_max_inflight_policy':4,'actual_max_inflight_policy':fifo['max_inflight_policy'],'invalid_policy':fifo['invalid_policy'],'failed_latch':fifo['failed_latch'],'caps':caps})
   raise RuntimeError('actual FIFO4 configuration drift: '+json.dumps(fifo,sort_keys=True))
  self.record({'stage':'ACTUAL_MVS_DEVICE_WITNESS','label':label,'device_identity':self.MVS_device,'snapshot_sequence':int(s.sequence),'snapshot_monotonic_ns':int(s.monotonic_ns),'snapshot_is_last_allocator_event_not_freshOS':True,'caps':caps})
 def fusion_same_runtime(self,rt,input_,owners):
  if C.cast(rt.OrtGetApiBase,C.c_void_p).value!=C.cast(self.rt.OrtGetApiBase,C.c_void_p).value:raise RuntimeError('different ORT in FusionIO adapter')
  self.owner.fusion(input_,owners)
  event=self.events[-1]
  if event['stage']!='WHOLE_DAWN_GPU_FUSION_RETURN':raise RuntimeError('missing actual Fusion native return')
  b=Borrow();b.size=C.sizeof(b);b.abi=1
  rc=self.rt.reconstruction_fusion_existing_owner_borrow_v1(C.byref(b))
  if rc:raise RuntimeError('actual retained device witness borrow failed')
  try:
   witness={'stage':'ACTUAL_FUSION_DEVICE_WITNESS','device_identity':int(b.device),'instance_identity':int(b.instance),'MVS_device_identity':self.MVS_device,'native_r2_actual_fusion_rc':event['rc'],'native_r2_enforces_saved_device_AND_instance_equal':event['device_identity_checked_natively'],'MVS_instance_raw_value_not_exported_by33ABI':True}
   if int(b.device)!=self.MVS_device or not b.instance:raise RuntimeError('actual MVS/Fusion device mismatch')
  finally:
   if b.owner:b.release(b.owner);b.owner=None
  self.record(witness);self.last_fusion=event
  self.owner.destroy()
  destroy=self.events[-1]
  if destroy['stage']!='WHOLE_DAWN_DESTROY' or destroy['rc']!=0 or self.host_refs or self.exclusive:raise RuntimeError('wholeowner did not close before suffix')
  create=next(r for r in self.events if r['stage']=='WHOLE_DAWN_CREATE')
  return {'create_rc':create['rc'],'run_rc':event['rc'],'destroy_rc':destroy['rc'],'host_refs':self.host_refs,'actual_identity_witness':witness}
def get():
 if CURRENT is None:raise RuntimeError('wholeowner not bound; no freshPost fallback')
 return CURRENT
