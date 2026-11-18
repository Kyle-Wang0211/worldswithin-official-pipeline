"""Original StageH CPython transport, W row-worker anchors. No UI or MVS entry."""
from pathlib import Path
from reconstruction_fusion.resume_historical_gate import required_digest
import ast,ctypes,gc,hashlib,io,json,os,resource,sys,threading,time,traceback,types
HERE=Path(__file__).resolve().parent
MODE='RECONSTRUCTION_ROW_RUNTIME_ANCHORS_V1'
SOURCE_SHA='36e0a60bec53f7cb6a535245a536470dad1d0b510bf4f89acdce104560a29637'
PARENT='Documents/reconstruction_phone32_parent'
SUCCESS={2:'PASS_PHONE_BASELINE2_ORACLE_CREATED_NOT_PARITY',32:'PASS_PHONE_ANCHORS_EXACT_32'}
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def save(path,value):
 path=Path(path)
 with path.open('w')as f:json.dump(value,f,indent=2);f.write('\n');f.flush();os.fsync(f.fileno())
def contained(root,relative,*,file=False):
 root=Path(root);p=Path(relative)
 if p.is_absolute()or not p.parts or any(s in ('','.','..')for s in relative.split('/')):raise ValueError('canonical relative path required')
 path=root/p
 if path.resolve()!=path or not path.is_relative_to(root):raise ValueError('path escape/symlink')
 if file and not path.is_file():raise ValueError('regular file required: '+relative)
 return path
def checked(root,row,*,content=True):
 p=contained(root,row['relative'],file=True)
 if p.stat().st_size!=row['bytes']or(content and sha(p)!=row['sha256']):raise ValueError('immutable input changed: '+row['relative'])
 return p

def admission(control,active,binding):
 inputs_digest = required_digest('WORLDSWITHIN_DIAGNOSTIC_CAMERA_INPUTS_SHA256')
 admission_digest = required_digest('WORLDSWITHIN_DIAGNOSTIC_CAMERA_ADMISSION_SHA256')
 required_digest('WORLDSWITHIN_DIAGNOSTIC_CAMERA_SHA256')
 if control.get('schema')!='RECONSTRUCTION_SEPARATE_BENCH_FUSION_RESUME_DIAGNOSTIC_V1'or control.get('resume_id')!='fusion_resume_diagnostic_v1'or control.get('anchor_mode')!=MODE:raise ValueError('original worker envelope identity required')
 if set(active)!={'schema','workers'}or active['schema']!=MODE or type(active['workers'])is not int or active['workers']not in(2,32):raise ValueError('exact2/32 selector required')
 w=active['workers']
 if set(control['anchor_arms'])!={'2','32'}:raise ValueError('exact two frozen arms required')
 arm=control['anchor_arms'][str(w)]
 expected_phase='baseline2_make_oracle'if w==2 else'candidate32_compare_phone2'
 if arm['workers']!=w or arm['phase']!=expected_phase or (arm['refs']!=[0,48,96]or any(type(x)is not int for x in arm['refs'])):raise ValueError('worker/phase/anchors identity')
 if arm['output_relative']!=PARENT+f'/z_rows{w}_anchor_v1':raise ValueError('fixed fresh app-owned attempt required')
 if type(control['physical_limit_B'])is not int or type(control['wall_limit_s'])is not int or control['physical_limit_B']!=1073741824 or control['wall_limit_s']!=1200:raise ValueError('fixed resource guards required')
 if binding['source_sha256']!=SOURCE_SHA or set(binding['libraries'])!={'2','32'}:raise ValueError('frozen W source/libraries required')
 row=binding['libraries'][str(w)]
 if row['worker_count']!=w or row['source_sha256']!=SOURCE_SHA or row['bundle_relative']!=f'Frameworks/libreconstructionFusionRows{w}.dylib'or not isinstance(row['signed_sha256'],str)or len(row['signed_sha256'])!=64:raise ValueError('signed native row binding required')
 if control['inputs']['sha256']!=inputs_digest or control['input_admission']['sha256']!=admission_digest:raise ValueError('bound origin identities required')
 return w,arm,row

def dispatch(control,active,binding,run):
 """Actual worker dispatch seam; CPU tests inject only the numerical callback."""
 w,arm,row=admission(control,active,binding)
 value=run(w,arm,row)
 if not isinstance(value,dict)or value.get('status')!=SUCCESS[w]or value.get('workers')!=w or value.get('refs')!=[0,48,96]or value.get('full_pipeline_complete')is not False:raise RuntimeError('selected anchors worker failed/invalid completion')
 return value

def _verify_origin(control,container):
                                                                                  
 import reconstruction_fusion_resume_entry as prior
 from reconstruction_fusion import q_fullchain_fusion_driver as provider
 if sha(prior.__file__)!='90e53f17046082dfce2f9b4c98d1429bb763be3bb03b61e1ac39386bada2d1ea':raise ValueError('original resume transport changed')
 lineage=json.loads((Path(provider.__file__).parent/'BINDING.json').read_text())['resume_cer_allowed_origin']
 required={'request':prior.REQUEST_SHA,'input_binding':prior.BINDING_SHA,'pipeline':lineage['origin_pipeline_receipt_sha256'],'q_failure':lineage['origin_Q_FUSION_receipt_sha256'],'harness':lineage['origin_harness_receipt_sha256']}
 descriptors=control['origin_files']
 if set(descriptors)!=set(required):raise ValueError('exact five original source records required')
 actual={}
 for name,digest in required.items():
  if descriptors[name]['sha256']!=digest:raise ValueError('original receipt digest differs')
  actual[name]=json.loads(checked(container,descriptors[name]).read_text())
 prior.verify_origin(actual['request'],actual['input_binding'],actual['pipeline'],actual['q_failure'],lineage)
 if actual['harness']['run_id']!='fusion_resume_origin'or actual['harness']['status']!='STOPPED':raise ValueError('original failed harness must remain preserved')

def execute(control_path,current_container,bundle_root):
 container=Path(current_container);bundle=Path(bundle_root)
 if container.resolve()!=container or bundle.resolve()!=bundle:raise ValueError('canonical existing Bench container/bundle required')
 parent=contained(container,PARENT)
 if not parent.is_dir()or not os.access(parent,os.W_OK):raise ValueError('existing app-created writable parent required; no host mkdir workaround')
 control=json.loads(Path(control_path).read_text());binding_path=HERE/'BINDING.json'
 if sha(binding_path)!=control['binding_sha256']:raise ValueError('exact source-bound package binding required')
 binding=json.loads(binding_path.read_text())
 for name,digest in binding['python_files'].items():
  if Path(name).name!=name or sha(HERE/name)!=digest:raise ValueError('signed helper source changed: '+name)
 active_path=parent/'Z_ACTIVE_ARM.json';active=json.loads(active_path.read_text());w,arm,library=admission(control,active,binding)
 _verify_origin(control,container)
 out=contained(container,arm['output_relative'])
 if out.exists()or out.is_symlink():raise ValueError('once-only attempt already exists')
                                                                                
 out.mkdir(mode=0o700);save(out/'STARTED.json',{'workers':w,'control_sha256':sha(control_path),'entry_sha256':sha(__file__),'epoch_ns':time.time_ns()})
 active_path.rename(out/'ACTIVE_CONSUMED.json')
 def run(workers,selected,lib):return _run_anchors(control,binding,workers,container,bundle,out,lib)
 try:
  result=dispatch(control,active,binding,run);result['control_sha256']=sha(control_path);save(out/'RESULT.json',result);save(parent/'Z_DISPATCH_RESULT.json',result);return result
 except BaseException as error:
  result={'status':'FAILED_PRESERVE','workers':w,'full_pipeline_complete':False,'error':repr(error),'traceback':traceback.format_exc(),'control_sha256':sha(control_path)};save(out/'RESULT.json',result);save(parent/'Z_DISPATCH_RESULT.json',result);raise

class ResourceGuard:
 def __init__(self,out,physical_limit,wall_limit):
  self.out=out;self.limit=physical_limit;self.wall=wall_limit;self.start=time.monotonic();self.rows=[];self.error=None;self.event=threading.Event();self.thread=threading.Thread(target=self.loop,daemon=True)
  self.stream=(out/'RESOURCES.jsonl').open('x');self.system=ctypes.CDLL('/usr/lib/libSystem.B.dylib');self.task=ctypes.c_uint.in_dll(self.system,'mach_task_self_').value
  self.system.task_info.argtypes=[ctypes.c_uint,ctypes.c_int,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];self.system.task_info.restype=ctypes.c_int
 def sample(self):
  info=(ctypes.c_uint32*256)();count=ctypes.c_uint32(256);rc=self.system.task_info(self.task,22,info,ctypes.byref(count))
  if rc or count.value<44:raise RuntimeError('TASK_VM_INFO failed/short count')
  raw=bytes(info);physical=int.from_bytes(raw[144:152],'little');peak=int.from_bytes(raw[168:176],'little')
  basic=(ctypes.c_uint32*12)();n=ctypes.c_uint32(12);rc=self.system.task_info(self.task,20,basic,ctypes.byref(n))
  if rc or n.value!=12 or physical<=0 or peak<physical:raise RuntimeError('invalid physical/RSS observation')
  resident=int.from_bytes(bytes(basic)[8:16],'little');row={'monotonic_ns':time.monotonic_ns(),'epoch_ns':time.time_ns(),'physical_B':physical,'lifetime_physical_peak_B':peak,'resident_B':resident,'elapsed_s':time.monotonic()-self.start,'valid':resident>0}
  if not row['valid']or physical>self.limit or peak>self.limit or row['elapsed_s']>self.wall:raise RuntimeError('resource guard breached: '+repr(row))
  self.rows.append(row);self.stream.write(json.dumps(row)+'\n');self.stream.flush();return row
 def loop(self):
  try:
   while not self.event.wait(.1):self.sample()
  except BaseException as error:self.error=repr(error);save(self.out/'HARD_GUARD.json',{'error':self.error,'stop_requested':True})
 def begin(self):self.sample();self.thread.start()
 def check(self):
  if self.error:raise RuntimeError(self.error)
  self.sample()
 def finish(self):
  self.event.set()
  if self.thread.ident is not None:self.thread.join(timeout=1)
  self.check();self.stream.close()
  if self.thread.is_alive():raise RuntimeError('sampler did not stop')
  gaps=[b['monotonic_ns']-a['monotonic_ns']for a,b in zip(self.rows,self.rows[1:])]
  if len(self.rows)<3 or max(gaps,default=0)>500000000:raise RuntimeError('resource sample coverage invalid')
  return {'valid':True,'samples':len(self.rows),'max_gap_ns':max(gaps,default=0),'physical_peak_B':max(r['physical_B']for r in self.rows),'RSS_peak_B':max(r['resident_B']for r in self.rows),'wall_s':time.monotonic()-self.start,'scope':'same app process; lifetime peak separately retained in JSONL; root external stop required if native call hangs'}

def volume_guard(out):
 volume=os.statvfs(out)
 if volume.f_bavail*volume.f_frsize<1073741824:raise RuntimeError('phone free space below1GiB before Fusion')
 if sum(p.stat().st_size for p in Path(out).rglob('*')if p.is_file())>2147483648:raise RuntimeError('owned output data exceeded2GiB')

def _run_anchors(control,binding,w,container,bundle,out,library):
 camera_digest = required_digest('WORLDSWITHIN_DIAGNOSTIC_CAMERA_SHA256')
 import numpy as np,cv2,PIL
 from PIL import Image
 import reconstruction_original_dense_raw_driver as orig
 from reconstruction_row_runtime import native_hook
 from reconstruction_row_runtime.window_io import WindowLoads,LazyDepth
 from reconstruction_row_runtime.fusion_runtime_guard import FusionRuntimeGuard,NativeRuntime
 if sys.version_info[:3]!=(3,10,21)or np.__version__!='1.26.4'or cv2.__version__!='4.11.0'or PIL.__version__!='12.3.0':raise RuntimeError('pinned original runtime required')
 if sha(orig.__file__)!='06a1b3b95021888a470108fb34a853e0756992bd42a6d8a93bedd50984f96956':raise ValueError('original transport source changed')
 p=json.loads(checked(container,control['inputs']).read_text());a=json.loads(checked(container,control['input_admission']).read_text())
 if p['shape']!=[97,1152,1536]or p['refs']!=[0,48,96]or p['cameras']['sha256']!=camera_digest or not a['root_reviewed']:raise ValueError('bound reference anchors required')
 identities={k:[{q:r[q]for q in('bytes','sha256')}for r in p[k]]if k in('banks','jpegs')else{q:p[k][q]for q in('bytes','sha256')}for k in('banks','cameras','frame_ids','jpegs')}
 expected=a['input_identity'];actual_banks=[dict(plane=r['plane'],**v)for r,v in zip(p['banks'],identities['banks'])];identities['banks']=actual_banks
 if identities!=expected:raise ValueError('reference admission content differs')
 native_path=contained(bundle,library['bundle_relative'],file=True)
 if sha(native_path)!=library['signed_sha256']:raise ValueError('signed library changed')
 local_binding={'rows_library_relative':library['bundle_relative'],'signed_rows_library_sha256':library['signed_sha256'],'worker_count':w};native_hook.configure(local_binding)
 report={'status':'PREPARING','workers':w,'refs':[0,48,96],'full_pipeline_complete':False,'frames':[],'files':{},'counts':{},'hist':[0]*11,'source_sha256':SOURCE_SHA,'camera_sha256':camera_digest,'binding_sha256':control['binding_sha256'],'scope':'phone Fusion anchors only; fresh R3phone2 oracle before32; original97 source inputs preserved','runtime_guard':{}}
 checkpoint=lambda:save(out/'RESULT.json',report)
 runtime=FusionRuntimeGuard(NativeRuntime(orig,np,cv2,local_binding),report['runtime_guard'],checkpoint);guard=ResourceGuard(out,control['physical_limit_B'],control['wall_limit_s']);loads=None
 try:
  volume_guard(out);runtime.initialize();guard.begin()
                                                                             
  loads=WindowLoads(np,p['banks'],lambda r:checked(container,r,content=False),orig._B)
  cameras=memoryview(checked(container,p['cameras']).read_bytes());ids=memoryview(checked(container,p['frame_ids']).read_bytes());jpegs=tuple(str(checked(container,row))for row in p['jpegs']);ret=out/'retained';ret.mkdir()
  oracle=None
  if w==32:
   base=contained(container,PARENT+'/z_rows2_anchor_v1',file=False);prior=json.loads((base/'RESULT.json').read_text());oracle=json.loads((base/'ORACLE.json').read_text())
   if prior['status']!=SUCCESS[2]or prior['control_sha256']!=sha(parent_control_path(container))or prior['binding_sha256']!=control['binding_sha256']or prior['refs']!=[0,48,96]or oracle['frames']!=prior['frames']:raise ValueError('successful same-package phone2 oracle required')
   report['oracle_sha256']=sha(base/'ORACLE.json')
  class Adapter(orig.FusionArrayIO):
   def savez(self,path,**arrays):
    super().savez(path,**arrays);f=int(Path(path).stem);local=sys._getframe(1).f_locals;row={'frame':f,'intermediate':{},'files':{}}
    for name in('gs','acc','d_avg','photo'):row['intermediate'][name]=hashlib.sha256(memoryview(np.ascontiguousarray(local[name])).cast('B')).hexdigest()
    for file in sorted(ret.glob(f'{f:08d}.*')):
     row['files'][file.name]=sha(file)
     if oracle and file.read_bytes()!=(base/'retained'/file.name).read_bytes():raise RuntimeError('retained byte difference')
    if len(row['files'])!=4:raise RuntimeError('four retained outputs required')
    report['frames'].append(row)
  adapter=Adapter(np,loads,cameras,str(ret),ids)
  def opened(path,mode='r',*args,**kw):
   if path==orig._FX+'/frames.json'and mode=='r':return io.StringIO(json.dumps({'count':97,'width':1536,'height':1152}))
   raise RuntimeError(('unexpected original IO',path))
  scope={'np':adapter,'Image':orig.OriginalJPEGImageIO(Image,jpegs),'json':json,'open':opened,'FX':orig._FX,'B':orig._B,'OUT':'/reconstruction-original-raw/output','a':types.SimpleNamespace(geo_pixel=.125,mask_thres=[2],sat_tol=1e-5,min_tri_angle=3.)}
  code=orig.source_codes();fe={'np':np,'cv2':cv2};exec(code['filter'],fe);scope['check_geometric_consistency']=fe['check_geometric_consistency'];src=Path(orig.__file__).parent;fusion=(src/'selected_original_fuse_verbatim_sat_angle.py').read_bytes();filters=(src/'selected_original_filter.py').read_bytes();span=orig._SPANS['fusion_setup'];setup=ast.parse(b''.join(fusion.splitlines(keepends=True)[span['lo']-1:span['hi']]));setup.body=[n for n in setup.body if not(isinstance(n,ast.Assign)and any(isinstance(t,ast.Name)and t.id=='D'for t in n.targets))];exec(compile(setup,'<W-identical-botharms-lazy-storage>','exec'),scope);exec(code['fusion_defs'],scope);scope['D']=LazyDepth(loads);state=native_hook.install(scope,fusion,filters,pinned_runtime=True,mode='both');stage_start=time.monotonic();cpu_start=time.process_time()
  for f in(0,48,96):
   volume_guard(out);guard.check();i,kept,hist=scope['one'](f)
   if i!=f:raise RuntimeError('wrong original reference')
   row=report['frames'][-1];row.update(counts={str(k):v for k,v in kept.items()},hist=hist.tolist())
   if oracle and row!=next(x for x in oracle['frames']if x['frame']==f):raise RuntimeError('intermediate/count/hist mismatch')
   report['files'].update(row['files']);report['counts'][str(f)]=row['counts'];report['hist']=[x+y for x,y in zip(report['hist'],row['hist'])];loads.clear();gc.collect();checkpoint();guard.check()
  values=(ctypes.c_uint64*4)();state.lib.pw_neon_rows_stats(values);geom=state.stats['geometry_native']
  if state.stats['geometry_fallback']or state.stats['sees_fallback']or list(values)!=[geom,0,geom*1152*1536//2,geom*w]:raise RuntimeError('native coverage/fallback mismatch')
  if oracle and(report['files']!=oracle['files']or report['counts']!=oracle['counts']or report['hist']!=oracle['hist']or state.stats!=oracle['native_stats']):raise RuntimeError('aggregate/native comparison differs')
  report.update(stage_seconds=time.monotonic()-stage_start,cpu_seconds=time.process_time()-cpu_start,native_stats=dict(state.stats),row_counters=list(values),resources=guard.finish());guard=None
  if len(report['files'])!=12:raise RuntimeError('exact12 output files required')
  report['status']=SUCCESS[w]
  if w==2:save(out/'ORACLE.json',dict(frames=report['frames'],files=report['files'],counts=report['counts'],hist=report['hist'],native_stats=report['native_stats']))
  return report
 finally:
  if loads is not None:loads.clear()
  try:
   if guard is not None:guard.finish()
  finally:runtime.restore()

def parent_control_path(container):return Path(container)/PARENT/'CONTROL.json'
