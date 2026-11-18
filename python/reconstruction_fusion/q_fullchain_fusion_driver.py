"""Same C/Python raw fuse_pack ABI; selected original function code remains intact.

Only a per-call exec binding installs an admitted implementation after the exact
original definitions load. No mutation of the original driver's globals/module.
"""
from pathlib import Path
import builtins,ctypes,hashlib,json,sys,time,types,traceback
HERE=Path(__file__).resolve().parent
CER_DISPATCH={'owned_calls':0,'original_calls':0}
DRIVER_SHA='06a1b3b95021888a470108fb34a853e0756992bd42a6d8a93bedd50984f96956'

def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb')as stream:
  for block in iter(lambda:stream.read(1<<20),b''):h.update(block)
 return h.hexdigest()

def compare_historical_cer(rows,expected):
 """Metadata-only gate on already authenticated current-run bank SHA rows."""
 planes=('depth','conf0','conf1','conf2')
 def indexed(values):
  result={}
  for row in values:
   plane=row['plane']
   if plane not in planes or plane in result:raise ValueError('unknown/duplicate CER plane')
   if row['bytes']!=686555136 or len(row['sha256'])!=64:raise ValueError('invalid CER identity row')
   result[plane]=row
  if set(result)!=set(planes):raise ValueError('all four CER planes required')
  return result
 actual,wanted=indexed(rows),indexed(expected)
 differences=[{'plane':plane,'expected':wanted[plane],'actual':actual[plane]}for plane in planes if actual[plane]['bytes']!=wanted[plane]['bytes']or actual[plane]['sha256']!=wanted[plane]['sha256']]
 return {'status':'FIRST_DIFFERENCE'if differences else'EXACT_HISTORICAL_SHA','stage':'cer','compared':4,'differences':differences,'before_fusion':True}

def clone_provider(original,after_definitions):
 """Inject only after the pinned defs exec. Original function code object reused."""
 namespace=dict(original.__globals__)
 expected=namespace['_SOURCES']['fusion']['filename']+':'+str(namespace['_SPANS']['fusion_defs']['lo'])
 seen=[]
 def scoped_exec(code,globals_,locals_=None):
  builtins.exec(code,globals_,locals_)
  if getattr(code,'co_filename',None)==expected:
   if seen:raise RuntimeError('duplicate original definitions boundary')
   seen.append(True);after_definitions(globals_)
 namespace['exec']=scoped_exec
 clone=types.FunctionType(original.__code__,namespace,original.__name__,original.__defaults__,original.__closure__)
 clone.__kwdefaults__=original.__kwdefaults__
 return clone,seen

def fuse_pack(banks,cameras,rgb,rgb_width,rgb_height,frame_ids,retained,fused_ply,original_jpeg_paths=None,*,resume_cer_lineage=None):
 import reconstruction_original_dense_raw_driver as original
 if sha(original.__file__)!=DRIVER_SHA:raise RuntimeError('pinned original raw-driver source changed')
 binding=json.loads((HERE/'BINDING.json').read_text())
 for name,expected in binding['python_files'].items():
  p=HERE/name
  if p.parent!=HERE or sha(p)!=expected:raise RuntimeError('Q helper source changed: '+name)
 if binding.get('fullchain_phone_admitted')is not True:raise RuntimeError('root phone admission not frozen')
 if binding.get('implementation')!='NEON_ROWS_WINDOW':raise RuntimeError('explicit selected Q implementation required')
 count=original.transport_frame_count(cameras)
 if count!=97 or original_jpeg_paths is None:
  if binding.get('raw_rows32',{}).get('enabled')is True and original_jpeg_paths is None and resume_cer_lineage is None:
   from .raw_rows32 import fuse_pack_raw
   return fuse_pack_raw(original,binding,clone_provider,sha,CER_DISPATCH,banks,cameras,rgb,rgb_width,rgb_height,frame_ids,retained,fused_ply)
  if binding.get('unsupported_input_policy')!='original_whole_call':raise ValueError('unsupported raw-input scope')
  return original.fuse_pack(banks,cameras,rgb,rgb_width,rgb_height,frame_ids,retained,fused_ply,original_jpeg_paths)
 np,cv2,_=original.dependencies(fusion=True)
                                                                                  
 if type(banks)is not tuple or len(banks)!=4:raise ValueError('four raw banks required')
 for bank in banks:original.checked_view(bank,97*1152*1536*4)
 target=Path(retained)
 if not target.is_absolute() or target.exists() or target.is_symlink():raise ValueError('fresh absolute retained required')
                                                                                 
                                                                          
 if target.name!='retained':raise ValueError('original request retained layout required')
 work=target.parent/'work'
 rows=[]
 for plane in ('depth','conf0','conf1','conf2'):
  file=work/(plane+'.f32')
  if file.resolve()!=file or not file.is_file() or file.stat().st_size!=686555136:raise ValueError('current run bank missing/shape')
  rows.append({'plane':plane,'bytes':686555136,'sha256':sha(file),'name':file.name})
 evidence=target.parent/'Q_FUSION.json'
 if evidence.exists():raise ValueError('Q fusion evidence already exists')
 report={'status':'PREPARING','implementation':'NEON_ROWS_WINDOW','scope':'actual full_pipeline regenerated bank inputs, same fuse_pack ABI','bank_identity':rows,'bank_manifest_sha256':hashlib.sha256(json.dumps(rows,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'binding_sha256':sha(HERE/'BINDING.json'),'original_driver_sha256':DRIVER_SHA,'original_function_code_reused':True,'cer_dispatch':dict(CER_DISPATCH),'frames':[],'files':{},'native_row_coverage':None,'counts':{},'hist':[0]*11,'started_ns':time.time_ns()}
 def save():evidence.write_text(json.dumps(report,indent=2)+'\n')
 comparison=compare_historical_cer(rows,binding['historical_cer_outputs'])
 report['historical_cer_comparison']=comparison
 if comparison['status']=='FIRST_DIFFERENCE':
  report.update(status='FIRST_DIFFERENCE',first_difference_stage='cer',fusion_started=False);save()
  raise RuntimeError('Q first historical difference at cer; Fusion not started')
 if binding.get('cer_owned')is True:
  from .cer_input_borrow import COUNTERS as input_counters
  from .cer_output_owned import OUTPUT_COUNTERS as output_counters
  report['cer_input_counters']=dict(input_counters);report['cer_output_counters']=dict(output_counters)
  if resume_cer_lineage is None:
   if CER_DISPATCH!={'owned_calls':97,'original_calls':0}:raise RuntimeError('full_pipeline requires exactly97 selected CER calls')
  else:
   from .resume_lineage import require_resume_cer_lineage
   require_resume_cer_lineage(resume_cer_lineage,rows,binding,CER_DISPATCH)
   report['current_CER_execution']='REUSED_PRIOR_CER'
   report['prior_CER_lineage']=resume_cer_lineage
 from .resume_historical_gate import HistoricalOutputGate,required_digest
 oracle_sha = required_digest('WORLDSWITHIN_DIAGNOSTIC_ORACLE_SHA256')
 historical_gate=HistoricalOutputGate(HERE.parents[2]/binding['historical_output_oracle_relative'],oracle_sha,binding['original_total_kept'],report,save)
 save();state=None;coverage=None;loads=None;runtime_guard=None
 def install(scope):
  nonlocal state,coverage,loads,runtime_guard
  from .window_io import WindowLoads,LazyDepth
  from . import native_hook
  from .native_row_coverage import NativeRowCoverage
  if Path(native_hook.__file__).resolve()!=HERE/'native_hook.py':raise RuntimeError('Q native hook module path mismatch')
  def checked(row):
   path=work/row['name']
   if path.resolve()!=path or path.stat().st_size!=row['bytes']:raise ValueError('current run bank changed')
   return path
  from .fusion_runtime_guard import FusionRuntimeGuard, NativeRuntime
  report['runtime_guard']={}
  runtime_guard=FusionRuntimeGuard(NativeRuntime(original,np,cv2,binding),report['runtime_guard'],save)
  runtime_guard.initialize()
  loads=WindowLoads(np,rows,checked,original._B,shape=(97,1152,1536),capacity=3)
  scope['np'].loads=loads;scope['D']=LazyDepth(loads)
  src=Path(original.__file__).parent
  state=native_hook.install(scope,(src/'selected_original_fuse_verbatim_sat_angle.py').read_bytes(),(src/'selected_original_filter.py').read_bytes(),pinned_runtime=True,mode='both')
  coverage=NativeRowCoverage(state,list(range(97)),'rows',binding['rows_source_sha256'],binding['signed_rows_library_sha256'],1152,1536)
  scope['check_geometric_consistency']=coverage.counted(scope['check_geometric_consistency'])
  report['native_row_coverage']=coverage.data
  one=scope['one']
  def observed_one(frame):
   coverage.begin(frame);begin=time.monotonic()
   try:
    value=one(frame)
    if value[0]!=frame:raise RuntimeError('original returned different frame')
    row=coverage.finish(frame)
    NativeRowCoverage.require_frame(row)
    historical_gate.check_frame(frame,target)
    files={path.name:sha(path)for path in sorted(target.glob(f'{frame:08d}.*'))}
    if len(files)!=4:raise RuntimeError('four actual retained outputs required')
    report['frames'].append({'frame':frame,'files':files,'seconds':time.monotonic()-begin,'counts':value[1],'hist':value[2].tolist()})
    report['files'].update(files);report['counts'][str(frame)]=value[1]
    report['hist']=(np.asarray(report['hist'],np.int64)+value[2]).tolist()
    report['native_stats']=dict(state.stats);save();loads.clear()
    return value
   except BaseException:
    if coverage.before is not None:coverage.finish(frame,completed=False)
    save();raise
  scope['one']=observed_one
 provider,seen=clone_provider(original.fuse_pack,install)
 if provider.__code__ is not original.fuse_pack.__code__:raise RuntimeError('original code was not reused')
 try:
  report['status']='RUNNING';save();begin=time.monotonic()
  kept=provider(banks,cameras,rgb,rgb_width,rgb_height,frame_ids,retained,fused_ply,original_jpeg_paths)
  historical_gate.check_order_and_total(target,kept)
  if len(seen)!=1 or state is None or coverage is None:raise RuntimeError('selected implementation was not installed')
  coverage.require_complete(report['frames'],report['counts'],report['frames'])
  if len(report['files'])!=388:raise RuntimeError('all97 retained outputs missing')
  report['files']['order.json']=sha(target/'order.json')
  stats=(ctypes.c_uint64*4)();state.lib.pw_neon_rows_stats(stats)
  report['neon_counters']=list(stats)
  if stats[1]!=0 or stats[0]!=state.stats['geometry_native'] or stats[3]!=2*stats[0]:raise RuntimeError('NEON row-task coverage mismatch')
  report.update(status='COMPLETED_CANDIDATE_NOT_PARITY_CLAIM',kept=int(kept),elapsed_seconds=time.monotonic()-begin,native_stats=dict(state.stats))
  return kept
 except BaseException as error:
  report.update(status='FIRST_DIFFERENCE'if report.get('status')=='FIRST_DIFFERENCE'else'FAILED',error=repr(error),traceback=traceback.format_exc());raise
 finally:
  try:
   if runtime_guard is not None:runtime_guard.restore()
  except BaseException as error:
   report.update(status='FAILED',restoration_error=repr(error),restoration_traceback=traceback.format_exc());raise
  finally:
   if loads is not None:loads.clear()
   report['ended_ns']=time.time_ns();save()


def merge_frame(low,high,outputs,frame_index):
 """Exact selected merge code with only the proven ArrayIO allocation facade."""
 import reconstruction_original_dense_raw_driver as original
 if sha(original.__file__)!=DRIVER_SHA:raise RuntimeError('pinned original raw-driver source changed')
 binding=json.loads((HERE/'BINDING.json').read_text())
 if binding.get('fullchain_phone_admitted')is not True:raise RuntimeError('root phone admission not frozen')
 for name,expected in binding['python_files'].items():
  if (HERE/name).parent!=HERE or sha(HERE/name)!=expected:raise RuntimeError('Q helper source changed: '+name)
 if binding.get('cer_owned')is not True:
  CER_DISPATCH['original_calls']+=1
  return original.merge_frame(low,high,outputs,frame_index)
 CER_DISPATCH['owned_calls']+=1
 return clone_merge_provider(original.merge_frame)(low,high,outputs,frame_index)

def clone_merge_provider(original):
 from .cer_output_owned import CerOutputArrayIO
 namespace=dict(original.__globals__);namespace['ArrayIO']=CerOutputArrayIO
 clone=types.FunctionType(original.__code__,namespace,original.__name__,original.__defaults__,original.__closure__)
 clone.__kwdefaults__=original.__kwdefaults__
 return clone
