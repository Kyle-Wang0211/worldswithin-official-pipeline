"""Private count20 fresh-CER GPUFusion IO adapter; existing iOS Python owner only.
All source inputs read-only; bounded pread, never full CER-bank mapping.
"""
from pathlib import Path
from reconstruction_distribution_identity import native_digest
import os,sys,json,hashlib,ctypes as C,ast,time,gc,threading,traceback
SOURCE_SHA='b29a8abfb40f6957dbf119bd567ef701437e8d8208d4cc32d9947a882723ceab'
N=1536*1152;PLANE=N*4
_PENDING_OWNERS=[]

U=C.c_uint32;Q=C.c_uint64;P=C.c_void_p;RefCB=C.CFUNCTYPE(None,P);EnterCB=C.CFUNCTYPE(C.c_int,P)
class Snapshot(C.Structure):_fields_=[('size',U),('abi',U),('flags',U),('reserved',U)]+[(k,Q)for k in ['physical','available','metal','backing','monotonic_ns']]
SampleCB=C.CFUNCTYPE(C.c_int,P,C.POINTER(Snapshot))
class Config(C.Structure):_fields_=[(k,U)for k in ['size','abi','width','height','audit','timeout']]+[('owner',P),('retain',RefCB),('release',RefCB),('enter',EnterCB),('leave',RefCB),('sample',SampleCB),('sdk',C.c_char_p),('identity',C.c_char_p)]
DepthCB=C.CFUNCTYPE(C.c_int,P,U,C.POINTER(C.c_float),C.c_size_t);PhotoCB=C.CFUNCTYPE(C.c_int,P,U,C.POINTER(U),C.c_size_t);PairCB=C.CFUNCTYPE(C.c_int,P,U,U,C.POINTER(C.c_float));EmitCB=C.CFUNCTYPE(C.c_int,P,U,C.POINTER(C.c_uint16),C.POINTER(U),C.c_size_t,U,C.POINTER(U))
class Reference(C.Structure):_fields_=[('ref',U),('dmax',C.c_float),('dmin',C.c_float),('count',U),('sources',C.POINTER(U))]
class Input(C.Structure):_fields_=[('size',U),('abi',U),('frames',U),('refs_count',U),('refs',C.POINTER(Reference)),('owner',P),('retain',RefCB),('release',RefCB),('depth',DepthCB),('photo',PhotoCB),('pair',PairCB),('emit',EmitCB),('allCER',U),('manifest',C.c_char_p)]
def digest(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for x in iter(lambda:f.read(65536),b''):h.update(x)
 return h.hexdigest()
def inside(docs,rel):
 p=Path(rel)
 if p.is_absolute()or not p.parts or any(x in ('','.','..')for x in rel.split('/'))or'\\'in rel:raise ValueError('canonical relative path required')
 p=docs/p
 if p.resolve()!=p:raise ValueError('symlink/noncanonical source')
 return p
def verify(docs,r):
 p=inside(docs,r['path'])
 if p.stat().st_size!=r['bytes']or digest(p)!=r['sha256']:raise ValueError('input bytes/SHA mismatch '+r['path'])
 return p
def pread_exact(fd,n,off):
 data=os.pread(fd,n,off)
 if len(data)!=n:raise ValueError('bounded input short read')
 return data
def reject_other_runtime_images(paths,intended,passive=None):
 passive=passive or {};verified=[]
 for p in paths:
  name=Path(p).name
  if any(x in name.lower()for x in ['qmvsOrt'.lower(),'onnxruntime','webgpu_dawn','libdawn','s6segmentsrt'])and str(Path(p))!=str(intended):
   if p in passive:
    if native_digest(p)!=passive[p]:raise RuntimeError('fixed App framework SHA mismatch: '+p)
    verified.append({'path':p,'sha256':passive[p]});continue
   raise RuntimeError('otherORT/Dawn image already loaded; require fresh sole-runtime Bench process: '+p)
 return verified
def image_paths():
 lib=C.CDLL('/usr/lib/libSystem.B.dylib');n=lib._dyld_image_count;n.restype=U;f=lib._dyld_get_image_name;f.argtypes=[U];f.restype=C.c_char_p
 return [f(i).decode()for i in range(n())if f(i)]
def wait_thermal(thermal,phase,pause_path,sleep=time.sleep):
                                                                               
                                                                              
 while True:
  state=C.c_int(-1);up=C.c_double();epoch=C.c_double();memory=C.c_ulonglong()
  rc=thermal(C.byref(state),C.byref(up),C.byref(epoch),C.byref(memory))
  if rc or state.value not in (0,1,2,3):raise RuntimeError('thermal snapshot failed')
  if state.value!=3:return state.value
  with Path(pause_path).open('a')as f:f.write(json.dumps({'phase':phase,'thermal':3,'monotonic_ns':time.monotonic_ns()})+'\n')
  sleep(1)
def fresh_budget_snapshot(m0,rt,ptr):
 rc=m0.reconstruction_m0_observer_boundary(b'FUSION97_NATIVE_BUDGET')
 if rc:return 6
 rc=rt.reconstruction_fusion_post_sample_os_v1(ptr)
 if rc:return rc
 ptr.contents.flags|=4;return 0
def source_scope(source,np,CM,D,W,H):
 if digest(source)!=SOURCE_SHA:raise ValueError('original selected Fusion source drift')
 tree=ast.parse(source.read_text());tree.body=[n for n in tree.body if isinstance(n,ast.FunctionDef)and n.name in ['cam','sees']]
 if {n.name for n in tree.body}!={'cam','sees'}:raise ValueError('original functions missing')
 scope={'np':np,'CM':CM,'D':D,'W':W,'H':H};exec(compile(tree,str(source),'exec'),scope);return scope
def qualification_scope(path,np):
 if digest(path)!='b5e6ae535f1287ba18f0fd9d7455cd7d36bcdc7cb6bebc4613480c98b3d00cfb':raise ValueError('current accepted whole-shape qualification source drift')
 tree=ast.parse(path.read_text());tree.body=[n for n in tree.body if isinstance(n,ast.FunctionDef)and n.name in ['eligible','qualified_sources']]
 if len(tree.body)!=2:raise ValueError('accepted qualifier functions missing')
 q={'np':np};exec(compile(tree,str(path),'exec'),q);return q
class RefDepth:
 def __init__(self,np,fd):self.np=np;self.fd=fd;self.ref=None;self.value=None
 def __getitem__(self,ref):
  if ref!=self.ref:self.value=self.np.frombuffer(pread_exact(self.fd,PLANE,ref*PLANE),'<f4').reshape(1152,1536);self.ref=ref
  return self.value
 def clear(self):self.value=None;self.ref=None

def validate_ordered_input_binding(binding,docs,root):
 count=binding["handoff"]["count"]
 if type(count)is not int or count<=0:raise ValueError("actual ordered N10..97 required")
 H,ctrl,B=binding['handoff'],binding['control'],binding['bindings']
 if (H['count'],H['width'],H['height'])!=(count,1536,1152)or ctrl['frame_count']!=count or ctrl['ref_order']!=list(range(count)):raise ValueError('exact20refs AND20sources, not20x97')
 if len(ctrl['frame_id_f64_bits'])!=count or len(set(ctrl['frame_id_f64_bits']))!=count:raise ValueError('original20 frameidentity')
 from .memory_policy import validated_policy
 validated_policy(ctrl,ctrl.get('identity_scope'))
 if H['frame_id_f64_bits']!=ctrl['frame_id_f64_bits']:raise ValueError('frameidentity')
 m=json.loads(verify(docs,ctrl['mvs_receipt']).read_text())
 if m['status']!='MVS_ONLY_COMPLETED'or m['completed_reference_runs']!=2*count or m['inputs']['count']!=count or [x['sha256']for x in m['inputs']['models']]!=ctrl['model_sha256']:raise ValueError('freshcomplete20 MVSlineage')
 if root==docs or not root.is_relative_to(docs)or root!=inside(docs,ctrl['output_relative']):raise ValueError('currentprivatepostroot')
 if len(H['CER_banks'])!=4 or any(x['bytes']!=count*PLANE for x in H['CER_banks'])or H['camera']['bytes']!=count*144 or ctrl['fusion_rgb']['bytes']!=count*N*3:raise ValueError('complete20CER/camera/RGB')
 for i,x in enumerate(H['CER_banks']):
  if inside(docs,x['path'])!=Path(ctrl['work_absolute'])/(['depth','conf0','conf1','conf2'][i]+'.f32'):raise ValueError('freshsamepostCER only')
 if H['CER_stage_returned']is not True:raise ValueError('CER actualreturn barrier required')
 return H,ctrl,B

def run(source,docs,bundle,root,*,bound_inputs):
 import numpy as np
 if np.__version__!='1.26.4':raise RuntimeError('pinned existing numpy1.26.4 required')
 source=Path(source).resolve();docs=Path(docs).resolve();bundle=Path(bundle).resolve();root=Path(root).resolve()
 H,ctrl,B=validate_ordered_input_binding(bound_inputs,docs,root);count=H["count"]
 HANDOFF_SHA=hashlib.sha256(json.dumps(bound_inputs,sort_keys=True,separators=(',',':')).encode()).hexdigest();CONTROL_SHA=HANDOFF_SHA
 out=root/'gpu_fusion20';out.mkdir(mode=0o700);retained=Path(ctrl['retained_absolute']);retained.mkdir(parents=True);save=lambda n,x:(out/n).write_text(json.dumps(x,indent=2)+'\n')
 mvs=json.loads(verify(docs,ctrl['mvs_receipt']).read_text())
 runtime=verify(bundle,B['runtime']);m0path=verify(bundle,B['M0']);fixed=reject_other_runtime_images(image_paths(),runtime,ctrl['passive_runtime_images']);save('FIXED_APP_FRAMEWORKS.json',fixed)
 m0=C.CDLL(str(m0path));m0.reconstruction_m0_observer_boundary.argtypes=[C.c_char_p];m0.reconstruction_m0_observer_boundary.restype=C.c_int
 thermal_path=verify(bundle,B['thermal']);thermal_lib=C.CDLL(str(thermal_path));thermal=thermal_lib.reconstruction_thermal_snapshot_v1
 thermal.argtypes=[C.POINTER(C.c_int),C.POINTER(C.c_double),C.POINTER(C.c_double),C.POINTER(C.c_ulonglong)];thermal.restype=C.c_int
 def boundary(phase):
  wait_thermal(thermal,phase,out/'THERMAL_PAUSE.jsonl')
  rc=m0.reconstruction_m0_observer_boundary(phase.encode())
  if rc:raise RuntimeError('real native M0 not active/failed/cap exceeded rc='+str(rc))
 boundary('FUSION20_INPUT_PREFLIGHT')
 t0=time.perf_counter();banks=[verify(docs,r)for r in H['CER_banks']]
 if any(r['bytes']!=count*PLANE for r in H['CER_banks']):raise ValueError('CER full20 barrier/shape')
 CM=np.fromfile(verify(docs,H['camera']),'<f4').reshape(count,36)
 rgb_path=verify(docs,ctrl['fusion_rgb'])
 if not np.isfinite(CM).all()or not np.all(CM[:,26]==1536)or not np.all(CM[:,27]==1152):raise ValueError('actual HIGH cameras')
                                                                             
                                                                             
 pending_context=False;fd=[os.open(p,os.O_RDONLY)for p in banks];depth=RefDepth(np,fd[0]);original=verify(bundle,ctrl['original_fusion']);scope=source_scope(original,np,CM,depth,1536,1152);qualifier=qualification_scope(verify(bundle,ctrl['qualification_source']),np)
                                                                            
                                                                          
 for rel in ['Frameworks/RECONSTRUCTIONPythonExtensions/reconstruction_row_runtime/native_hook.py','Frameworks/RECONSTRUCTIONPythonExtensions/reconstruction_fusion/BINDING.json']:
  if digest(bundle/rel)!=ctrl['python_bindings'][rel]:raise ValueError('current signed qualifier binding drift')
 from reconstruction_row_runtime import native_hook
 oldbinding=json.loads((bundle/'Frameworks/RECONSTRUCTIONPythonExtensions/reconstruction_fusion/BINDING.json').read_text())['raw_rows32']
 native_hook.configure({'rows_library_relative':oldbinding['rows_library_relative'],'signed_rows_library_sha256':oldbinding['signed_rows_library_sha256'],'worker_count':oldbinding['worker_count']})
                                                                       
                                                                           
                                                                         
 import reconstruction_original_dense_raw_driver as original_driver
 _,cv2,_=original_driver.dependencies(fusion=True)
 from reconstruction_fusion.fusion_runtime_guard import FusionRuntimeGuard,NativeRuntime
 native_binding={'rows_library_relative':oldbinding['rows_library_relative'],'signed_rows_library_sha256':oldbinding['signed_rows_library_sha256'],'worker_count':oldbinding['worker_count']}
 guard_report={};runtime_guard=FusionRuntimeGuard(NativeRuntime(original_driver,np,cv2,native_binding),guard_report,lambda:save('ORIGINAL_RUNTIME_GUARD.json',guard_report));runtime_guard.initialize();guard_restored=False
 scalar=native_hook.Scalar();native_qualify=scalar.qualifier(qualifier['qualified_sources'])
 refs=[];mats=[];cameras=[]
 try:
  for f in range(count):
   boundary('FUSION20_ORIGINAL_SEES_'+str(f));d=depth[f]
   if not np.isfinite(d).all():raise ValueError('nonfinite actual CER depth')
   if not qualifier['eligible'](CM,d):raise RuntimeError('actual input outside already-accepted whole-shape qualifier domain; stop beforeGPU, no approximatefallback')
   sources=native_qualify(f,count,1536,1152,d,scope['cam'])
   if scalar.stats['sees_fallback']:raise RuntimeError('same actual20 input unexpectedly fell outside existing native qualifier; stop ownbinding investigation beforeGPU')
   Kr,Er=scope['cam'](f);cameras.append((Kr,Er));rows=[]
   for s in range(count):
    Ks,Es=scope['cam'](s);rows.append(np.concatenate([np.linalg.inv(Kr).ravel(),(Es@np.linalg.inv(Er)).ravel(),Ks.ravel(),np.linalg.inv(Ks).ravel(),(Er@np.linalg.inv(Es)).ravel(),Kr.ravel(),Er.ravel(),-Es[:3,:3].T@Es[:3,3],-Er[:3,:3].T@Er[:3,3],[np.cos(np.radians(3.))]]).astype('<f4'))
   matrices=np.stack(rows);assert matrices.shape==(count,91)and np.isfinite(matrices).all();mats.append(matrices.tobytes());refs.append({'ref':f,'sources':sources,'dmin':float(CM[f,24])*(1+1e-5),'dmax':float(CM[f,25])*(1-1e-5)});depth.clear();del d,rows,matrices;gc.collect();save('PREPARED_PROGRESS.json',refs)
  depth.clear();del scope;gc.collect();boundary('FUSION20_PREPARED_SCRATCH_RELEASED')
  registration={'input_handoff_sha256':HANDOFF_SHA,'control_sha256':CONTROL_SHA,'source_sha256':SOURCE_SHA,'qualification_provider':'same current-phone native_hook.Scalar.qualifier(visibility_cache_v2.qualified_sources), F64 smallproducts/fullshape/order unchanged','qualification_native_stats':dict(scalar.stats),'qualification_sha256':'b5e6ae535f1287ba18f0fd9d7455cd7d36bcdc7cb6bebc4613480c98b3d00cfb','runtime':B['runtime'],'M0':B['M0'],'models':ctrl['model_sha256'],'ref_order':list(range(count)),'prepared_refs':refs,'oldQ_sha256':None,'thermal':{'source':'original v17 critical-only wait states0/1/2continue,3pause','library_sha256':digest(thermal_path),'pause_log':'THERMAL_PAUSE.jsonl','leases':'FD/owner/window retained duringpause; Cwatch staysactive'},'prepare_s':time.perf_counter()-t0,'scope':'samePID originalMVS20/currentCER/GPU20 viaretainedwholeowner; original F32adaptation warnings unchanged; no performanceclaim untilwholetrace','memory':'bounded pread plane7,077,888B; no mapping of four2.746GB banks; all global-sees work released beforeGPU; real M0 + wholeApp2GB'};save('RUN_CONTRACT.json',registration)
  from .owner_context import get
  ctx=get();rt=ctx.rt;rt.reconstruction_fusion_existing_owner_runtime_identity_v1.restype=C.c_char_p
  rt.reconstruction_fusion_post_create_v1.argtypes=[C.POINTER(Config),C.POINTER(P)];rt.reconstruction_fusion_post_run_v1.argtypes=[P,C.POINTER(Input)]
  for n in ['cancel','drain','destroy']:getattr(rt,'reconstruction_fusion_post_'+n+'_v1').argtypes=[P]
  rt.reconstruction_fusion_post_sample_os_v1.argtypes=[C.POINTER(Snapshot)]
  host_refs=[0];data_refs=[0];phase=threading.Lock();errors=[];output=[]
  @RefCB
  def hold(_):host_refs[0]+=1
  @RefCB
  def drop(_):host_refs[0]-=1
  @EnterCB
  def enter(_):return 0 if phase.acquire(False)else 7
  @RefCB
  def leave(_):phase.release()
  @SampleCB
  def sample(_,ptr):
   try:
    wait_thermal(thermal,'FUSION20_NATIVE_BUDGET',out/'THERMAL_PAUSE.jsonl')
    return fresh_budget_snapshot(m0,rt,ptr)
   except BaseException as e:errors.append({'callback':'budget','error':repr(e)});return 6
  @RefCB
  def data_hold(_):data_refs[0]+=1
  @RefCB
  def data_drop(_):data_refs[0]-=1
  def safe(fn):
   def cb(*a):
    try:return fn(*a)
    except BaseException as e:
     errors.append({'callback':fn.__name__,'error':repr(e),'traceback':traceback.format_exc()})
     try:save('CALLBACK_ERRORS.json',errors)
     except BaseException:pass
     return 1
   return cb
  @DepthCB
  @safe
  def read_depth(_,s,dst,n):
   if n!=N or s>=count:raise ValueError('depth index/plane')
   raw=pread_exact(fd[0],PLANE,s*PLANE);C.memmove(dst,raw,PLANE);return 0
  @PhotoCB
  @safe
  def read_photo(_,f,dst,n):
   if n!=N or f>=count:raise ValueError('photo index/plane')
   mask=np.ones(N,bool)
   for i,threshold in enumerate([.3,.5,.5]):
    conf=np.frombuffer(pread_exact(fd[i+1],PLANE,f*PLANE),'<f4')
    if not np.isfinite(conf).all():raise ValueError('CER confidence nonfinite')
    mask &=conf>threshold
   np.ctypeslib.as_array(dst,shape=(N,))[:]=mask;return 0
  @PairCB
  @safe
  def read_pair(_,f,s,dst):
   if f>=count or s>=count:raise ValueError('pair index')
   raw=mats[f][s*364:(s+1)*364];C.memmove(dst,raw,364);return 0
  @EmitCB
  @safe
  def emit(_,f,dst,counts,n,kept,hist):
   if n!=N or f!=len(output)or data_refs[0]!=1:raise ValueError('ref order/data lease')
   boundary('FUSION20_EMIT_'+str(f));raw=C.string_at(dst,N*2);name=f'{f:08d}.d16';p=retained/name;p.write_bytes(raw)
                                                                            
                                                                            
   Kr,Er=cameras[f];camera=np.concatenate([Kr.ravel(),Er.ravel()]).astype('<f8').tobytes()
   if len(camera)!=200:raise ValueError('camera K9+E16 layout')
   (retained/f'{f:08d}.camera.f64le').write_bytes(camera)
   with rgb_path.open('rb')as rgbstream:
    rgbstream.seek(f*N*3);rgb=rgbstream.read(N*3)
   if len(rgb)!=N*3:raise ValueError('same20 RGB short')
   (retained/f'{f:08d}.col.u8').write_bytes(rgb)
   meta={'schema':1,'pack_index':f,'original_index':f,'frame_id_f64_bits':ctrl['frame_id_f64_bits'][f],'width':1536,'height':1152,'depth_scale':5000,'d16':name,'d16_bytes':3538944,'camera':f'{f:08d}.camera.f64le','camera_bytes':200,'camera_layout':'K9_then_E16_row_major_f64le'}
   (retained/f'{f:08d}.json').write_text(json.dumps(meta,separators=(',',':'))+'\n')
   row={'ref':f,'d16_sha256':hashlib.sha256(raw).hexdigest(),'camera_SHA':hashlib.sha256(camera).hexdigest(),'kept':int(kept),'hist':[hist[i]for i in range(11)],'comparison':'USER_ADOPTED_GPU_FUSION; noCPUreference rerun'};output.append(row);save('OUTPUT_PROGRESS.json',output);return 0
  arrays=[(U*len(r['sources']))(*r['sources'])for r in refs];rr=(Reference*count)(*[Reference(r['ref'],r['dmax'],r['dmin'],len(r['sources']),arrays[r['ref']])for r in refs]);inp=Input(C.sizeof(Input),1,count,count,rr,P(1),data_hold,data_drop,read_depth,read_photo,read_pair,emit,1,HANDOFF_SHA.encode());cfg=Config(C.sizeof(Config),1,1536,1152,0,30000,P(1),hold,drop,enter,leave,sample,b'09f60549925d00e4f3972a9d623fe4aa6a1766c5cec308f8eda6ab2ca794aa47',rt.reconstruction_fusion_existing_owner_runtime_identity_v1());post=P();create_rc=None;run_rc=None;destroy_rc=None;t=time.perf_counter()
  try:
                                                                                  
   actual=ctx.fusion_same_runtime(rt,inp,[locals().copy()])
   create_rc=actual['create_rc'];run_rc=actual['run_rc'];destroy_rc=actual['destroy_rc']
   save('NATIVE_CREATE.json',{'rc':create_rc,'scope':'actual earlierwholeownercreate beforeMVS; no freshPostCreate','pid':os.getpid()})
   save('NATIVE_RUN.json',{'rc':run_rc,'completed_refs':len(output),'native_scope_wall_s':time.perf_counter()-t,'errors':errors})
   save('DESTROY.json',{'rc':destroy_rc,'host_refs':actual['host_refs'],'data_refs':data_refs[0],'identity':actual['actual_identity_witness']})
   if run_rc or len(output)!=count or errors:raise RuntimeError('retained native Fusion20 incomplete')
  except BaseException:
   if ctx.owner.owner:
    pending_context=True;_PENDING_OWNERS.append(locals().copy());os.environ['RECONSTRUCTION_FUSION97_NATIVE_PENDING']='1'
    save('PENDING_OWNERS.json',{'state':ctx.owner.state,'callbacks_module_data_FDs_retained':True,'Cwatch_must_continue':True,'recovery':'same-owner actualwholeDestroy only; no postAPI fallback/no forceclear'})
   raise
  finally:
   if not pending_context:boundary('FUSION20_FINAL_RETURN')
  if host_refs[0]or data_refs[0]:raise RuntimeError('lease not released')
  (retained/'order.json').write_text(json.dumps({'order':list(range(count)),'kept_total':sum(x['kept']for x in output)})+'\n')
  save('RESULT.json',{'status':'PHONE_INDEPENDENT_FUSION20_NATIVE_RETURNED0_QUALITY_AND_MEMORY_ADJUDICATION_PENDING','pid':os.getpid(),'create_rc':create_rc,'run_rc':run_rc,'destroy_rc':destroy_rc,'refs':output,'prepare_s':registration['prepare_s'],'native_scope_wall_s':time.perf_counter()-t,'original_reference_wall_s_cleanup_concurrent_notfair':None,'fullpipeline_speed_or_2GBpass':False,'mvs_or_CER_rerun':False,'wholephysical2GBguard':'real M0 continuous watch/latched + native OS fresh conservative admission; root must adjudicate trace coverage/peak'})
 finally:
  if not guard_restored and not pending_context:runtime_guard.restore()
  if not pending_context:
   for f in fd:os.close(f)

