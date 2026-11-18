"""Actual-N raw RGB8 fuse_pack through the selected 32-row native geometry owner.

The original fuse_pack code object runs unchanged; only check_geometric_consistency
rows go native, exactly as in the full97 combination. Raw captures have no
historical oracle, so per-frame retained hashes are recorded for an external
comparison against an original-whole-call run of the same capture.
"""
from pathlib import Path
import ctypes,hashlib,json,time,traceback
HERE=Path(__file__).resolve().parent
PLANE_FRAME_BYTES=1152*1536*4
ROWS32='Frameworks/libreconstructionFusionRows32.dylib'

def fuse_pack_raw(original,binding,clone_provider,sha,cer_dispatch,banks,cameras,rgb,rgb_width,rgb_height,frame_ids,retained,fused_ply):
 raw=binding['raw_rows32']
 if raw.get('worker_count')!=32 or raw.get('rows_library_relative')!=ROWS32 or len(raw.get('signed_rows_library_sha256',''))!=64:raise ValueError('explicit raw selected32 owner required')
 z_root=HERE.parent/'reconstruction_row_runtime'
 for name,digest in raw['z_python_files'].items():
  p=z_root/name
  if p.is_symlink()or not p.is_file()or sha(p)!=digest:raise ValueError('selected native32 helper source changed: '+name)
 np,cv2,_=original.dependencies(fusion=True)
 count=original.transport_frame_count(cameras)
 if not 10<=count<=97:raise ValueError('raw rows32 requires 10..97 frames')
 bank_bytes=count*PLANE_FRAME_BYTES
 if type(banks)is not tuple or len(banks)!=4:raise ValueError('four raw banks required')
 for bank in banks:original.checked_view(bank,bank_bytes)
 target=Path(retained)
 if not target.is_absolute() or target.exists() or target.is_symlink():raise ValueError('fresh absolute retained required')
 work=target.parent/'work'
 started=time.monotonic()
 rows=[]
 for plane in ('depth','conf0','conf1','conf2'):
  file=work/(plane+'.f32')
  if file.resolve()!=file or not file.is_file() or file.stat().st_size!=bank_bytes:raise ValueError('current run bank missing/shape')
  rows.append({'plane':plane,'bytes':bank_bytes,'sha256':sha(file),'name':file.name})
 evidence=target.parent/'Q_FUSION.json'
 if evidence.exists():raise ValueError('Q fusion evidence already exists')
 report={'status':'PREPARING','implementation':'NEON_ROWS_WINDOW_RAW_N','worker_count':32,'frame_count':count,
  'scope':'raw actual-N bank inputs, same fuse_pack ABI; candidate, compare retained hashes to original whole call',
  'bank_identity':rows,'bank_hash_seconds':time.monotonic()-started,'binding_sha256':sha(HERE/'BINDING.json'),
  'original_function_code_reused':True,'cer_dispatch':dict(cer_dispatch),'frames':[],'files':{},'native_row_coverage':None,
  'counts':{},'hist':[0]*11,'started_ns':time.time_ns()}
 def save():evidence.write_text(json.dumps(report,indent=2)+'\n')
 save();state=None;coverage=None;loads=None;runtime_guard=None
 native_binding={'rows_library_relative':ROWS32,'signed_rows_library_sha256':raw['signed_rows_library_sha256'],'worker_count':32}
 def install(scope):
  nonlocal state,coverage,loads,runtime_guard
  from .window_io import WindowLoads,LazyDepth
  from reconstruction_row_runtime import native_hook
  native_hook.configure(native_binding)
  if Path(native_hook.__file__).resolve()!=z_root/'native_hook.py':raise RuntimeError('selected32 native hook module path mismatch')
  from .native_row_coverage import NativeRowCoverage
  def checked(row):
   path=work/row['name']
   if path.resolve()!=path or path.stat().st_size!=row['bytes']:raise ValueError('current run bank changed')
   return path
  from .fusion_runtime_guard import FusionRuntimeGuard,NativeRuntime
  report['runtime_guard']={}
  runtime_guard=FusionRuntimeGuard(NativeRuntime(original,np,cv2,native_binding),report['runtime_guard'],save)
  runtime_guard.initialize()
  loads=WindowLoads(np,rows,checked,original._B,shape=(count,1152,1536),capacity=3)
  scope['np'].loads=loads;scope['D']=LazyDepth(loads)
  src=Path(original.__file__).parent
  state=native_hook.install(scope,(src/'selected_original_fuse_verbatim_sat_angle.py').read_bytes(),(src/'selected_original_filter.py').read_bytes(),pinned_runtime=True,mode='both')
  coverage=NativeRowCoverage(state,list(range(97)),'rows',raw['rows_source_sha256'],raw['signed_rows_library_sha256'],1152,1536)
  coverage.expected=list(range(count))
  coverage.data['max_geometry_threads']=32
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
  kept=provider(banks,cameras,rgb,rgb_width,rgb_height,frame_ids,retained,fused_ply,None)
  if len(seen)!=1 or state is None or coverage is None:raise RuntimeError('selected implementation was not installed')
  coverage.require_complete(report['frames'],report['counts'],report['frames'])
  if len(report['files'])!=4*count:raise RuntimeError('all retained outputs missing')
  report['files']['order.json']=sha(target/'order.json')
  stats=(ctypes.c_uint64*4)();state.lib.pw_neon_rows_stats(stats)
  report['neon_counters']=list(stats)
  if stats[1]!=0 or stats[0]!=state.stats['geometry_native'] or stats[3]!=32*stats[0]:raise RuntimeError('selected32 row-task coverage mismatch')
  report.update(status='COMPLETED_CANDIDATE_NOT_PARITY_CLAIM',kept=int(kept),elapsed_seconds=time.monotonic()-begin,native_stats=dict(state.stats))
  return kept
 except BaseException as error:
  report.update(status='FAILED',error=repr(error),traceback=traceback.format_exc());raise
 finally:
  try:
   if runtime_guard is not None:runtime_guard.restore()
  except BaseException as error:
   report.update(status='FAILED',restoration_error=repr(error),restoration_traceback=traceback.format_exc());raise
  finally:
   if loads is not None:loads.clear()
   report['ended_ns']=time.time_ns();save()
