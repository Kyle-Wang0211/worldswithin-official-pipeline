"""Synchronous original whole prefix. No launcher, suffix, retry or fallback.

run_json returns an integer to the existing C ABI embedding. Failed jobs keep
all native owners, callbacks, modules and uncertain FDs strongly reachable.
Package schema is documented in python_runtime/PROFILE_SCHEMA.md.
"""
import ctypes as C
import json
import math
import mmap
import struct
import os
import stat
from pathlib import Path
import platform
import sys
import time
import traceback
from . import production_feature_materials as features
from . import production_disk_guard as disk
from .prefix_disk_retirement import PrefixRetirement, feature_reference_consumers
from .python_runtime import owner_context, gpu_provider as gpu, c_upload_receipt
from .python_runtime.transport_io import mapping, verify, inside, digest
from .python_runtime.memory_policy import validated_policy, bind_native_policy
from reconstruction_distribution_identity import configured_bundle_id, native_verify

PROFILE='whole_mixed_precision_gpu_fusion'
_PENDING={}
_LAST_ERROR=''
PLANES=('depth','conf0','conf1','conf2')
                                                                            
                                                                                 
                                                                                                      
                                                                            
RETIREMENT_CONSUMER_SHAS={'0ac58ae11d61d91fd551140e0623e46549d9e4a5f6c31652ed917272404fd8ae',
    'b16fde385a054e69d09766502ffa93ccf930ab66d01bfa5beb22de8e4728ad56'}
Callback=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_int,C.c_int,C.c_int,C.c_int,C.c_double)
class Request(C.Structure):
    _fields_=[(k,C.c_uint32)for k in ('abi_version','struct_bytes','count','reserved')]+[(k,C.c_char_p)for k in ('feature_low','rest_low','feature_high','rest_high')]+[('cameras_low',C.POINTER(C.c_float)),('neighbors',C.POINTER(C.c_int32))]+[(k,C.c_char_p)for k in ('noise_low','noise_high','images_low','images_high')]+[('jpeg_paths',C.c_void_p),('output_root',C.c_char_p),('progress',Callback),('user',C.c_void_p)]
assert C.sizeof(Request)==128

def require(ok,message):
    if not ok:raise ValueError(message)
def write_json(path,value):
    with Path(path).open('x')as stream:json.dump(value,stream,allow_nan=False);stream.write('\n')
def row(path,root=None):
    path=Path(path)
    return dict(path=str(path.relative_to(root))if root else str(path),bytes=path.stat().st_size,sha256=digest(path))
def absolute(value):
    p=Path(value);require(p.is_absolute()and p.resolve()==p and str(p)!='/', 'canonical absolute path required');return p

def retirement_disk_capability(profile,bundle):
    """Eligibility is actual same-bundle code/native identity, not phone success."""
    prefix='Frameworks/RECONSTRUCTIONPythonExtensions/reconstruction_production_whole/'
    for leaf in ('production_whole_driver.py','production_disk_guard.py','prefix_disk_retirement.py'):
        rel=prefix+leaf;pin=profile.get('source_pins',{}).get(rel)
        if not isinstance(pin,str)or profile.get('python_bindings',{}).get(rel)!=pin:return False
        path=Path(bundle)/rel
        if not path.is_file()or path.is_symlink()or path.resolve()!=path or digest(path)!=pin:return False
    consumer=profile.get('bindings',{}).get('consumer',{})
    if (consumer.get('path')!='Frameworks/libReconstructionWholeDense.dylib'or
        consumer.get('sha256')not in RETIREMENT_CONSUMER_SHAS):return False
    path=Path(bundle)/consumer.get('path','')
    return path.is_file()and not path.is_symlink()and path.resolve()==path and digest(path)==consumer['sha256']

def prefix_admission_plan(profile,bundle,count,manifest_bytes):
    enabled=retirement_disk_capability(profile,bundle)
    plan=disk.prefix_peak_budget(count,manifest_bytes=manifest_bytes,feature_retirement_enabled=enabled)
    plan['feature_retirement_enabled']=enabled
    plan['suffix_policy']='LEGACY_PER_STAGE_HEADROOM_NOT_A_WHOLE_CHAIN_BOUND'
    if enabled:plan['activation']='QUALIFIED_LAST_USE_WITH_EXPLICIT_LEGACY_SUFFIX_ALLOWANCE'
    return plan

def generated_live_bytes(root):
    """Measured/reporting-only bytes already occupied; never charge them twice."""
    total=0
    for current,directories,files in os.walk(root,followlinks=False):
        for name in directories+files:
            path=Path(current)/name;info=path.lstat()
            require(not stat.S_ISLNK(info.st_mode),'generated disk accounting symlink')
            if stat.S_ISREG(info.st_mode):total+=info.st_size
    return total

def admit_fixed_stage(scratch,count,stage,state):
    plan=disk.require_stage_space(scratch,count,stage,
        existing_live_bytes=generated_live_bytes(scratch),
        failure_slot_allocated=state.get('failure_reserve')is not None)
    state.setdefault('stage_disk_admissions',[]).append(plan)
    with (Path(scratch)/'DISK_STAGE_ADMISSION.jsonl').open('a')as stream:
        json.dump(plan,stream,allow_nan=False);stream.write('\n')
    return plan

def validated_frame_bits(request):
    """Mirror Dart ByteData.setFloat64(..., Endian.big), without editing JSON.

    Dense transport contains numeric frame_id; ordered_material carries its
    typed binary64 identity. Integer inputs must be exactly representable,
    avoiding silent cross-language rounding of a newly supplied integer ID.
    """
    frames=request['dense_inputs']['frames'];features.require_count(len(frames))
    identities=[];previous=None
    for i,frame in enumerate(frames):
        require(isinstance(frame,dict)and 'frame_id'in frame,'actual numeric frame_id required before native work')
        value=frame['frame_id']
        require(type(value)in (int,float),'frame_id must be JSON number, not boolean/text')
        try:number=float(value)
        except (ValueError,OverflowError):raise ValueError('ambiguous/nonfinite binary64 frame_id')
        require(math.isfinite(number),'finite binary64 frame_id required')
        if type(value)is int:require(int(number)==value,'integer frame_id not exactly representable in binary64')
        require(previous is None or number>previous,'actual frame IDs must be strictly ascending and unique')
        previous=number;bits=struct.pack('>d',number).hex()
        if 'frame_id_f64_bits'in frame:require(frame['frame_id_f64_bits']==bits,'provided dense frame identity mismatch')
        identities.append(bits)
    tables=[(key,request[key],False)for key in ('ordered_material','frames')if key in request]
    if 'original_fusion_jpegs'in request['dense_inputs']:
        tables.append(('dense_inputs.original_fusion_jpegs',request['dense_inputs']['original_fusion_jpegs'],True))
    for key,rows,required_indices in tables:
        require(isinstance(rows,list)and len(rows)==len(frames),'frame identity count mismatch '+key)
        for i,frame in enumerate(rows):
            require(isinstance(frame,dict),'top-level identity row must be object')
            require(frame.get('frame_id_f64_bits')==identities[i],'typed frame identity mismatch '+key)
            for index in ('pack_index','original_index'):
                if required_indices or index in frame:require(type(frame.get(index))is int and frame[index]==i,'typed frame order mismatch '+key)
    return tuple(identities)

PASSIVE_RUNTIME_IMAGE_SHA256={
    'Frameworks/PWOnnxRuntime.framework/PWOnnxRuntime':'1a6626919bdc446c0d13b0af8bac0c4636438db3be9a8dfdf820fa94e88c684c',
}

def resolve_passive_runtime_images(profile,bundle):
    """Resolve this installed bundle; never trust external absolute passive maps."""
    require('passive_runtime_images'not in profile,'absolute passive runtime map forbidden')
    rows=profile['passive_runtime_image_rows']
    require(isinstance(rows,list),'bundle-relative passive runtime image rows required')
    result={}
    for item in rows:
        require(isinstance(item,dict),'passive image file row required')
        relative=item['path']
        require(relative in PASSIVE_RUNTIME_IMAGE_SHA256,'foreign passive ORT/Dawn image not allowed')
        require(item['sha256']==PASSIVE_RUNTIME_IMAGE_SHA256[relative],'selected actual passive image SHA required')
        path=native_verify(bundle,item);key=str(path)
        require(key not in result,'duplicate passive runtime image path')
        result[key]=item['sha256']
    return result

def verified_independent_product_scope(envelope):
    """Revalidate the explicit Dart independent identity against actual package files."""
    scope=envelope.get('identity_scope',{})
    if scope.get('profile') not in ('independent_product_entry_test_v1','independent_product_copy_v1'):return False
    import plistlib
    require(set(scope)-{'build'}=={'schema','profile','bundle_id','bundle_root','documents_directory','info_plist_sha256','package_sha256'},'Bench identity scope fields')
    bundle=absolute(envelope['bundle_root'])
    setting = ('WORLDSWITHIN_PRODUCT_COPY_BUNDLE_ID' if scope['profile'] == 'independent_product_copy_v1'
               else 'WORLDSWITHIN_BENCH_BUNDLE_ID')
    expected = configured_bundle_id(setting)
    require(expected is not None and scope['schema']=='RECONSTRUCTION_EXECUTION_IDENTITY_SCOPE_V1'and scope['bundle_id']==expected and scope['bundle_root']==str(bundle),'Independent identity scope mismatch')
    documents=absolute(scope['documents_directory']);require(documents.is_dir(),'Bench Documents identity')
    info_path=inside(bundle,'Info.plist');package_path=inside(bundle,'reconstruction_production/PACKAGE.json')
    require(digest(info_path)==scope['info_plist_sha256']and digest(package_path)==scope['package_sha256'],'Bench actual identity hashes changed')
    info=plistlib.loads(info_path.read_bytes());package=json.loads(package_path.read_text())
    require(info['CFBundleIdentifier']==scope['bundle_id'],'Bench actual bundle identity')
    require(package['schema']=='RECONSTRUCTION_PRODUCTION_PACKAGE_V1'and package['bundle_id']==scope['bundle_id']and package['info_plist_sha256']==scope['info_plist_sha256']and package['platform']=='ios','Bench actual package identity')
    if scope['profile']=='independent_product_copy_v1':
        require(package.get('execution_identity_profile')==scope['profile'],'Bench explicit product-copy package profile')
    return True

def verified_independent_bench(envelope):
    scope=envelope.get('identity_scope',{})
    return (scope.get('profile')=='independent_product_entry_test_v1' and
            verified_independent_product_scope(envelope))

def selected_reference_production(envelope):
    """Authorize the selected product recipe under its actual execution identity."""
    scope=envelope.get('identity_scope',{})
    if scope.get('profile') == 'independent_product_copy_v1':
        require(verified_independent_product_scope(envelope),'verified product-copy identity required')
        identity=scope
    else:
        bundle_id = configured_bundle_id('WORLDSWITHIN_PRODUCTION_BUNDLE_ID')
        identity = dict(schema='RECONSTRUCTION_EXECUTION_IDENTITY_SCOPE_V1', profile='production', bundle_id=bundle_id)
        if bundle_id is None or {key: value for key, value in scope.items() if key != 'build'} != identity:
            return False
    import plistlib
    bundle=absolute(envelope['bundle_root'])
    info_path=inside(bundle,'Info.plist');package_path=inside(bundle,'reconstruction_production/PACKAGE.json')
    info=plistlib.loads(info_path.read_bytes());package=json.loads(package_path.read_text())
    require(info['CFBundleIdentifier']==identity['bundle_id'] and package['bundle_id']==identity['bundle_id'],'actual selected product bundle identity')
    require(package['platform']=='ios' and package['info_plist_sha256']==digest(info_path),'actual production package identity')
    require(package['schema']=='RECONSTRUCTION_PRODUCTION_PACKAGE_V1','actual production package schema')
    profile_path=inside(bundle,'reconstruction_production/WHOLE_PROFILE.json')
    require(package['whole_profile_sha256']==digest(profile_path),'actual production selected profile hash')
    stored=json.loads(profile_path.read_text());profile=envelope['whole_profile']
    recipe=stored.get('production_reference_closure_current',{})
    require(recipe.get('schema')=='RECONSTRUCTION_PRODUCTION_REFERENCE_CLOSURE_V1' and recipe.get('selected_source_revision')=='db7b72a45ec3161cd3cb1d96e1dc7813ceeb20f4','selected production REFERENCE source')
    require(recipe.get('model_graph_values')==[4119,3555] and recipe.get('fnet_graph_values')==65,'selected production REFERENCE graph identity')
    require(profile.get('production_reference_closure_current')==recipe and profile['models']==stored['models'],'production request recipe binding')
    require({k:v['sha256'] for k,v in stored['models'].items()}==features._MIXED_FNET_FAMILY,'exact selected production REFERENCE models')
    validated_policy(profile,identity)
    return True

def validate_envelope(envelope):
    independent_bench=verified_independent_bench(envelope)
    require(envelope['schema']=='RECONSTRUCTION_PRODUCTION_WHOLE_REQUEST_V1','whole request schema')
    require(envelope['dense_producer_profile']==PROFILE,'explicit whole profile required')
    request=envelope['request'];bits=validated_frame_bits(request)
    profile=envelope['whole_profile'];roots=envelope['io_roots']
    require(request['io_roots']==roots,'unmodified request IO roots required')
    for key in ('durable_input_root','downstream_output_root','texture_scene_root'):absolute(roots[key])
    bundle=absolute(envelope['bundle_root']);passive_images=resolve_passive_runtime_images(profile,bundle)
    dense=request['dense_inputs'];n=len(dense['frames'])
    features.require_count(n)
    require(profile['schema']=='RECONSTRUCTION_PRODUCTION_WHOLE_PROFILE_V1','whole configuration schema')
    require(profile['algorithm_route']=='LOCAL_DENSE_RECONSTRUCTION','selected route required')
    validated_policy(profile,envelope.get('identity_scope'))
    require(isinstance(profile['m0_run_id'],str)and profile['m0_run_id'],'explicit actual M0 observer run_id')
    for key in ('runtime','consumer','producer','M0','thermal'):
        verify(bundle,profile['bindings'][key])
    for key,value in profile['source_pins'].items():
        require(digest(inside(bundle,key))==value,'package source pin changed '+key)
    require(profile['source_pins'],'actual source pin table required')
    for key,value in profile['python_bindings'].items():require(digest(inside(bundle,key))==value,'Python binding changed')
                                                                            
                                                                                  
    for key in ('runtime','consumer'):
        b=profile['bindings'][key];receipt=json.loads(verify(bundle,b['receipt']).read_text())
        require(receipt['accepted_source_closure']is True and receipt['binary_sha256']==b['sha256'],'signed source receipt')
        require(receipt['algorithm_route']==profile['algorithm_route'],'route receipt mismatch')
        expected='8fdce22dbfe29605026039aab2d2d12bd3aacdb1481c4345f06a497f75bc3ac3'if key=='runtime'else'361715d71b944d1b1f848bdfddbf27286b9f8f52ef82dc8092da569cd8d9f167'
        require(receipt['base_unsigned_sha256']==expected,'selected pixel1/CO361 base identity')
        if selected_reference_production(envelope):
            adaptation=receipt.get('project_adaptation',{})
            require(adaptation.get('numerical_kernel_sources_changed') is False and adaptation.get('memory_policy')=='production_observe_only_v1' and adaptation.get('frame_count_policy')=='actual_positive_count','declared actual positive production adaptation required')
            evidence_row=receipt['source_evidence_bundle_row'];verify(bundle,evidence_row)
            require(evidence_row['sha256']==receipt['source_evidence_sha256'],'actual current150 source evidence identity')
            proof_row=receipt['source_proof_bundle_row'];verify(bundle,proof_row)
            acceptance_row=receipt['root_acceptance_binding']
            acceptance=json.loads(verify(bundle,acceptance_row).read_text())
            require(acceptance_row['sha256']==receipt['root_source_acceptance']['sha256'] and acceptance['decision']=='accept' and acceptance['acceptance_type']=='SOURCE_CLOSURE_ONLY' and acceptance['source_proof_sha256']==proof_row['sha256'],'root source acceptance binding')
            require(acceptance['native_libraries'].get(b['path'])==b['sha256'] and acceptance['source_evidence_sha256']==evidence_row['sha256'],'root accepted current150 component identity')
        elif key!='runtime' or not independent_bench:
            require(receipt['numeric_ast_unchanged']is True,'transport specialization numeric closure missing')
    require(profile['native_binding']==json.loads(verify(bundle,profile['qualifier_binding']).read_text())['raw_rows32'],'native qualifier binding mismatch')
    return request,profile,bundle,n,bits,passive_images

def prepare_high(feature_request,root):
    """Original dense_pipeline.cc:460–465 Pillow chain, one real JPEG at a time.

    HIGH L first then resize; fusion RGB independently resize RGB. float32/255
    precedes F16 exactly as original gray_to_f16. LOW is NEVER upsampled.
    """
    import numpy as np
    from PIL import Image
    import PIL
    require(np.__version__=='1.26.4'and PIL.__version__=='12.3.0','selected preprocessing runtime')
    high=root/'images_high.f16';rgb=root/'fusion_rgb.u8'
    with high.open('xb')as hs,rgb.open('xb')as rs:
        for frame in feature_request['frames']:
            path=features.bound_file(frame['photo'])
            with Image.open(path)as image:
                original=image.convert('RGB')
                gray=original.convert('L').resize((1536,1152),Image.Resampling.BILINEAR)
                normalized=np.asarray(gray).astype(np.float32)/np.float32(255.0)
                hs.write(normalized.astype('<f2').tobytes())
                rs.write(original.resize((1536,1152),Image.Resampling.BILINEAR).tobytes())
    return row(high),row(rgb)

def request_native(feature_request,mvsroot,progress):
    n=len(feature_request['frames'])
    cameras=(C.c_float*(n*36)).from_buffer_copy(features.bound_file(feature_request['cameras']).read_bytes())
    neighbors=(C.c_int32*(n*9)).from_buffer_copy(features.bound_file(feature_request['neighbors']).read_bytes())
    models=[features.bound_file(feature_request['models'][k])for k in ('feature_low','rest_low','feature_high','rest_high')]
    paths=[features.bound_file(feature_request[k])for k in ('low_noise','high_noise')]+[features.bound_file(feature_request['gray_banks'][str(w)])for w in (768,1536)]
    r=Request(1,C.sizeof(Request),n,0,*[str(p).encode()for p in models],cameras,neighbors,*[str(p).encode()for p in paths],None,str(mvsroot).encode(),progress,None)
    return r,[cameras,neighbors,progress,models,paths]

FIXED_RUNTIME_POLICY={'RECONSTRUCTION_MATCHED_MVS_ROUTE':'whole','RECONSTRUCTION_MATCHED_FIFO_MODE':'4','RECONSTRUCTION_BENCH_MAX_INFLIGHT_SUBMITS':'4','RECONSTRUCTION_BENCH_SEGMENT_SESSION_CACHE':'0','RECONSTRUCTION_PRIVATE_REST_GRAPH_CAPTURE':'0','RECONSTRUCTION_S1_STORAGE_BUFFER_CACHE_MODE':'disabled'}

PRODUCTION_REFERENCE_RUNTIME_POLICY={'RECONSTRUCTION_BENCH_HOST_MATERIAL_MMAP': '1', 'RECONSTRUCTION_BENCH_UMA_INPUT_UPLOAD': '1', 'RECONSTRUCTION_SUBMIT_DRAIN': '1', 'RECONSTRUCTION_MATCHED_MVS_ROUTE': 'whole', 'RECONSTRUCTION_MATCHED_FIFO_MODE': '4', 'RECONSTRUCTION_BENCH_MAX_INFLIGHT_SUBMITS': '4', 'RECONSTRUCTION_BENCH_SEGMENT_SESSION_CACHE': '0', 'RECONSTRUCTION_PRIVATE_REST_GRAPH_CAPTURE': '1', 'RECONSTRUCTION_S1_STORAGE_BUFFER_CACHE_MODE': 'disabled', 'RECONSTRUCTION_BENCH_UMA_INPUT_REWRITE_MAP': '1', 'RECONSTRUCTION_VALIDATION_TARGETED_TARGETED_REUSE': '1', 'RECONSTRUCTION_ETA_DIAGNOSTIC_ONLY': '0', 'RECONSTRUCTION_OFFICIAL_OBJECT_POOL': '1', 'RECONSTRUCTION_BENCH_OVERLAP_CER_N1': '0', 'PW_GPU_FEATURE_TRANSFER': '1', 'PW_DIRECT_FEATURE_CAPACITY': '10', 'RECONSTRUCTION_PHASE_BOUNDARY_DRAIN': '1', 'RECONSTRUCTION_CAPTURE_SHARED_WORKSPACE': '1', 'PW_FNET_MIXED_CORRIDOR': '1', 'OFFICIAL_AETHER_PYR_PERSIST': '0'}

_SELECTED_CAPTURE='0'

def bind_fixed_runtime_policy(envelope=None):
    global _SELECTED_CAPTURE
    if envelope is not None:
        production=selected_reference_production(envelope)
        _SELECTED_CAPTURE='1' if production else (os.environ.get('RECONSTRUCTION_PRIVATE_REST_GRAPH_CAPTURE','0') if verified_independent_bench(envelope) else '0')
        if production:
            os.environ.update(PRODUCTION_REFERENCE_RUNTIME_POLICY)
            bundle=str(absolute(envelope['bundle_root']))
            os.environ['ALICEVISION_ROOT']=bundle
            os.environ['ALICEVISION_OCIO']=bundle+'/share/aliceVision/config.ocio'
            bind_native_policy(envelope['whole_profile'],os.environ)
        require(_SELECTED_CAPTURE in ('0','1'),'Bench capture requires actual process env 0/1')
                                                                              
                                                                               
    policy=dict(FIXED_RUNTIME_POLICY);policy['RECONSTRUCTION_PRIVATE_REST_GRAPH_CAPTURE']=_SELECTED_CAPTURE
    os.environ.update(policy)
    os.environ['RECONSTRUCTION_BENCH_UMA_INPUT_UPLOAD']='1'
    os.environ.pop('RECONSTRUCTION_SEGMENT_PLAN',None);os.environ.pop('RECONSTRUCTION_SEGMENTED_TRIAL',None)

def require_fixed_runtime_policy():
    policy=dict(FIXED_RUNTIME_POLICY);policy['RECONSTRUCTION_PRIVATE_REST_GRAPH_CAPTURE']=_SELECTED_CAPTURE
    require(all(os.environ.get(k)==v for k,v in policy.items()),'fixed native policy changed after startup')
    require(os.environ.get('RECONSTRUCTION_BENCH_UMA_INPUT_UPLOAD')=='1','C upload opt-in changed after startup')
    require(os.environ.get('RECONSTRUCTION_SUBMIT_DRAIN','1')=='1','accepted DE7 early release disabled')

def capture_c_upload(rt,scratch,label,count,profile,state,*,phase=None,pass_index=None,ref=None,strict=True):
    """Bind native upload evidence and actual terminal owner events to this job."""
    try:
        policy_error=None
        try:require_fixed_runtime_policy()
        except BaseException as error:policy_error=repr(error)
        terminal=None
        if label=='MVS_FINALLY' and state.get('session') is not None:
            session=state['session']
            terminal=dict(schema='RECONSTRUCTION_C_UPLOAD_TERMINAL_V1',owner_state=session.owner.state,
                events=[dict(event) for event in session.events if event.get('stage') in ('WHOLE_DAWN_ORIGINAL_MVS_RETURN','ORIGINAL_MVS_PROGRESS')])
        value=c_upload_receipt.capture(rt,scratch,label,count,strict=False,terminal=terminal,transport=state.get('feature_transport','sealed'))
        context=dict(schema='RECONSTRUCTION_C_UPLOAD_CONTEXT_V1',label=label,count=count,
            phase=phase,pass_index=pass_index,ref=ref,
            tier='HIGH' if pass_index==1 else None,
            native={key:profile['bindings'][key]['sha256'] for key in ('runtime','consumer')},
            models={key:item['sha256'] for key,item in profile['models'].items()},
            source_inputs_sha256=state['source_inputs_sha256'],
            fixed_runtime_policy={key:os.environ.get(key) for key in FIXED_RUNTIME_POLICY},
            raw_receipt=row(Path(scratch)/('C_UPLOAD_'+label+'.json')),
            assessment=value.get('assessment'),policy_error=policy_error,
            validation_error=value.get('validation_error') or value.get('persistence_error') or policy_error)
        write_json(Path(scratch)/('C_UPLOAD_'+label+'_CONTEXT.json'),context)
        state.setdefault('C_upload_receipts',{})[label]=context
        if strict:require(context['validation_error'] is None,'persisted C evidence rejected: '+str(context['validation_error']))
        return value
    except BaseException as error:
        if strict:raise
        return dict(label=label,validation_error=repr(error),status='FAILED_EVIDENCE_PRESERVE_ORIGINAL_ERROR')


def bind_environment(profile,bundle,banks):
    bind_fixed_runtime_policy()
    for tier,w in (('LOW',768),('HIGH',1536)):
        os.environ['RECONSTRUCTION_S6_SEGMENT_PLAN_'+tier]=str(verify(bundle,profile['segment_plans'][tier]))
        prefix='RECONSTRUCTION_S6_MATERIAL_'+tier+'_'
        if os.environ.get('PW_GPU_FEATURE_TRANSFER','0')=='1':
            for key in ('INDEX','ROOT','INDEX_SHA','MANIFEST_SHA','PREPROCESS_SHA'):os.environ.pop(prefix+key,None)
            continue
        bank=banks[w]
        for key,value in (('INDEX',bank['index']),('ROOT',bank['root']),('INDEX_SHA',bank['index_sha256']),('MANIFEST_SHA',bank['manifest_sha256']),('PREPROCESS_SHA',bank['preprocess_sha256'])):os.environ[prefix+key]=value

def load_native(profile,bundle,state,passive_images):
    b=profile['bindings'];rtpath=verify(bundle,b['runtime']);copath=verify(bundle,b['consumer'])
    gpu.reject_other_runtime_images(gpu.image_paths(),rtpath,passive_images)
    rt=C.CDLL(str(rtpath),mode=os.RTLD_LOCAL|os.RTLD_NOW);state['runtime']=rt
    co=C.CDLL(str(copath),mode=os.RTLD_LOCAL|os.RTLD_NOW);state['consumer']=co
    gpu.reject_other_runtime_images(gpu.image_paths(),rtpath,passive_images)
    for name in ('pw_upload_fixed_input_begin','pw_upload_fixed_input_end','pw_upload_receipt_json_v1','reconstruction_s1_device_caps_json'):
        require(getattr(rt,name,None) is not None,'required actual C upload hook absent: '+name)
    require(C.cast(rt.OrtGetApiBase,C.c_void_p).value==C.cast(co.OrtGetApiBase,C.c_void_p).value,'sole actual ORT identity')
    transfer=os.environ.get('PW_GPU_FEATURE_TRANSFER','0')
    require(transfer in ('0','1'),'feature transfer flag must be0/1')
    if transfer=='1':
        capability=getattr(co,'reconstruction_feature_transport_abi_v1',None)
        require(capability is not None,'loaded consumer lacks direct feature transport ABI')
        capability.argtypes=[];capability.restype=C.c_uint32
        require(capability()==1,'loaded consumer direct feature transport ABI mismatch')
    state['feature_transport']='gpu_fixed_slots_v1' if transfer=='1' else 'sealed'
    co.reconstruction_continuous_mvs_abi.restype=C.c_uint32
    require(co.reconstruction_continuous_mvs_abi()==1,'original continuous ABI1')
    return rt,co

def verified_import(name,profile,bundle):
    import importlib
    module=importlib.import_module(name)
    path=absolute(module.__file__)
    require(path.is_relative_to(bundle),'numerical module loaded outside selected package '+name)
    relative=str(path.relative_to(bundle))
    require(relative in profile['source_pins']and digest(path)==profile['source_pins'][relative],'loaded numerical module source pin missing/changed '+name)
    return module

def cer(control,docs,work,profile,bundle):
    q=verified_import("reconstruction_fusion.q_fullchain_fusion_driver",profile,bundle)
    import contextlib,mmap
    count=control['frame_count']
    with contextlib.ExitStack()as stack:
        maps={tier:{k:stack.enter_context(mapping(verify(docs,control['banks'][tier][k])))for k in PLANES}for tier in ('low','high')}
        merged={k:stack.enter_context(mapping(work/(k+'.f32'),True,count*1536*1152*4))for k in PLANES}
        for i in range(count):
            views=[]
            try:
                low=[];high=[];outputs=[]
                for k in PLANES:
                    lv=maps['low'][k][i*768*576*4:(i+1)*768*576*4];hv=maps['high'][k][i*1536*1152*4:(i+1)*1536*1152*4];ov=merged[k][i*1536*1152*4:(i+1)*1536*1152*4]
                    views.extend((lv,hv,ov));low.append((lv,768,576));high.append((hv,1536,1152));outputs.append(ov)
                q.merge_frame(tuple(low),tuple(high),tuple(outputs),i)
            finally:
                for v in views:v.release()
            for k in PLANES:
                off=i*1536*1152*4;size=1536*1152*4
                mm=merged[k].obj;mm.flush(off,size)
                require(hasattr(mm,'madvise'),'bounded CER requires mmap.madvise')
                mm.madvise(mmap.MADV_DONTNEED,off,size)
                maps['high'][k].obj.madvise(mmap.MADV_DONTNEED,off,size)
                maps['low'][k].obj.madvise(mmap.MADV_DONTNEED,i*768*576*4,768*576*4)

def _cer_original_one(i, maps, merged, q):
    views = []
    try:
        low = []
        high = []
        outputs = []
        for k in PLANES:
            lv = maps['low'][k][i * 768 * 576 * 4:(i + 1) * 768 * 576 * 4]
            hv = maps['high'][k][i * 1536 * 1152 * 4:(i + 1) * 1536 * 1152 * 4]
            ov = merged[k][i * 1536 * 1152 * 4:(i + 1) * 1536 * 1152 * 4]
            views.extend((lv, hv, ov))
            low.append((lv, 768, 576))
            high.append((hv, 1536, 1152))
            outputs.append(ov)
        q.merge_frame(tuple(low), tuple(high), tuple(outputs), i)
    finally:
        for v in views:
            v.release()
    for k in PLANES:
        off = i * 1536 * 1152 * 4
        size = 1536 * 1152 * 4
        mm = merged[k].obj
        mm.flush(off, size)
        require(hasattr(mm, 'madvise'), 'bounded CER requires mmap.madvise')
        mm.madvise(mmap.MADV_DONTNEED, off, size)
        maps['high'][k].obj.madvise(mmap.MADV_DONTNEED, off, size)
        maps['low'][k].obj.madvise(mmap.MADV_DONTNEED, i * 768 * 576 * 4, 768 * 576 * 4)
class _ProductionCerN1:
    """Existing ordered N1 execution; original CER row AST is unchanged."""
    def __init__(self,root,work,count,q):
        from concurrent.futures import ThreadPoolExecutor
        self.root=Path(root);self.work=Path(work);self.count=count;self.q=q
        self.pool=ThreadPoolExecutor(max_workers=1,thread_name_prefix='originalCER')
        self.future=None;self.pending=None;self.acks=[];self.next=0
        self.receipts={};self.receipt_file=None;self.stack=None;self.maps=None;self.merged=None
        self.worker_ms=0.;self.phase4_polls=0;self.closed=False
    def _commit(self,p,r):
        import hashlib
        if self.receipt_file is None:self.receipt_file=(self.root/'refs.jsonl').open('rb')
        raw=self.receipt_file.readline(8193)
        require(raw.endswith(b'\n')and len(raw)<=8192,'actual committed four-plane receipt unavailable')
        v=json.loads(raw);require(v['pass']==p and v['reference']==r and v['finite']is True and len(v['sha256'])==4,'actual CER callback receipt mismatch')
        self.receipts[p,r]=v
    def _open(self):
        import contextlib
        self.stack=contextlib.ExitStack()
        try:
            self.maps={}
            for tier,w,h in [('low',768,576),('high',1536,1152)]:
                self.maps[tier]={}
                for k in PLANES:
                    p=self.root/tier/(k+'.f32')
                    require(p.resolve()==p and p.is_file()and p.stat().st_size==self.count*w*h*4,'actual open CER bank extent')
                    self.maps[tier][k]=self.stack.enter_context(mapping(p))
            self.merged={k:self.stack.enter_context(mapping(self.work/(k+'.f32'),True,self.count*1536*1152*4))for k in PLANES}
        except BaseException:
            self._close();raise
    def _work(self,r):
        import hashlib
        if self.stack is None:self._open()
        t=time.perf_counter()
        for p,tier,w,h in [(0,'low',768,576),(1,'high',1536,1152)]:
            receipt=self.receipts[p,r]
            for j,k in enumerate(PLANES):
                view=self.maps[tier][k][r*w*h*4:(r+1)*w*h*4]
                try:require(hashlib.sha256(view).hexdigest()==receipt['sha256'][j],'actual committed CER row hash changed')
                finally:view.release()
        _cer_original_one(r,self.maps,self.merged,self.q)
        self.worker_ms+=(time.perf_counter()-t)*1000
        self.acks.append(r)
    def progress(self,phase,p,r):
        require(not self.closed,'CER worker already closed')
        if phase==2:
            self._commit(p,r)
            if p==1:
                require(self.pending is None,'CER pending ref replayed')
                self.pending=r
        if phase==4:self.phase4_polls+=1
        if self.future is not None:
            if not self.future.done():return 2 if self.pending is not None else 0
            self.future.result();self.future=None
        if self.pending is not None:
            ref=self.pending;require(ref==self.next and (0,ref)in self.receipts,'ordered original CER frontier')
            self.future=self.pool.submit(self._work,ref);self.pending=None;self.next+=1
        return 0
    def _close(self):
        if self.stack is not None:self.stack.close();self.stack=None
    def finish(self):
        try:
            require(self.pending is None,'unadmitted CER tail')
            if self.future is not None:self.future.result();self.future=None
            require(self.acks==list(range(self.count)),'CER original ordered outputs incomplete')
            self.pool.submit(self._close).result()
            return dict(count=self.count,acks=self.acks,inflight_limit=1,worker_elapsed_ms=self.worker_ms,phase4_polls=self.phase4_polls,all_views_released=True)
        finally:self.shutdown()
    def shutdown(self):
        if self.closed:return
        try:
            if self.future is not None:
                try:self.future.result()
                except BaseException:pass
            self.pool.submit(self._close).result()
        finally:
            self.pool.shutdown(wait=True);self.closed=True
            if self.receipt_file is not None:self.receipt_file.close();self.receipt_file=None

def _require_cer_retirement_barrier(n1,count):
    if n1 is not None:
        require(n1.closed and n1.future is None and n1.stack is None and
                n1.acks==list(range(count)),
                'N1 CPU consumer must join and close all mappings before CER retirement')

def make_control(profile,request,feature_request,mvsroot,rgb,docs,out,work,retained,validated_bits,passive_images,identity_scope=None):
    import array
    n=len(feature_request['frames']);m=json.loads((mvsroot/'manifest.json').read_text());refs=[json.loads(s)for s in (mvsroot/'refs.jsonl').read_text().splitlines()if s]
    require(m['status']=='MVS_ONLY_COMPLETED'and m['completed_reference_runs']==2*n and m['inputs']['count']==n,'actual complete LOW/HIGH barrier')
    require([r['sha256']for r in m['inputs']['models']]==[feature_request['models'][k]['sha256']for k in ('feature_low','rest_low','feature_high','rest_high')],'actual MVS model identity')
    require(len(refs)==2*n,'actual 2N refs required')
    banks={}
    for p,(tier,w,h)in enumerate((('low',768,576),('high',1536,1152))):
        current=refs[p*n:(p+1)*n]
        require(all(r['pass']==p and r['reference']==i and r['finite']is True and r['elements_per_plane']==w*h for i,r in enumerate(current)),'actual reference order/shape/finite')
        banks[tier]={k:dict(path=str((mvsroot/tier/(k+'.f32')).relative_to(docs)),bytes=n*w*h*4,frame_bytes=w*h*4,frame_sha256=[r['sha256'][j]for r in current])for j,k in enumerate(PLANES)}
        for r in banks[tier].values():verify(docs,r)
    cams=array.array('f');cams.frombytes(features.bound_file(feature_request['cameras']).read_bytes())
    require(sys.byteorder=='little'and len(cams)==n*36,'original camera table')
    for i in range(n):
        for k in (0,2,4,5):cams[i*36+k]*=2.0
        cams[i*36+26]=1536.0;cams[i*36+27]=1152.0
    high=out/'cameras_high.f32'
    with high.open('xb')as f:f.write(cams.tobytes())
    bits=list(validated_bits)
    require(len(set(bits))==n and all(len(x)==16 and all(c in '0123456789abcdef'for c in x)for x in bits),'original frame bits required')
    return dict(frame_count=n,ref_order=list(range(n)),frame_id_f64_bits=bits,physical_limit_bytes=profile['physical_limit_bytes'],memory_policy=profile.get('memory_policy'),identity_scope=identity_scope,mvs_receipt=row(mvsroot/'manifest.json',docs),banks=banks,model_sha256=[r['sha256']for r in m['inputs']['models']],fusion_rgb={**rgb,'path':str(Path(rgb['path']).relative_to(docs))},cameras_high=row(high,docs),output_relative=str(out.relative_to(docs)),work_absolute=str(work),retained_absolute=str(retained),python_bindings=profile['python_bindings'],passive_runtime_images=dict(passive_images),original_fusion=profile['original_fusion'],qualification_source=profile['qualification_source'])

def run(envelope,state):
    bind_fixed_runtime_policy(envelope)
    request,profile,bundle,n,validated_bits,passive_images=validate_envelope(envelope)
    production=selected_reference_production(envelope);state['production_reference']=production
    require(sys.platform in ('ios','darwin')and sys.version_info[:3]==(3,10,21)and (platform.machine()in ('arm64','aarch64')or platform.machine().startswith('iPhone')),'existing pinned arm64 iOS worker required')
                                                                                 
                                                                            
    artifacts=request['dense_inputs']['artifacts']
    require(all(type(r.get('bytes'))is int and r['bytes']>=0 for r in artifacts),'actual manifest byte sizes required')
    plan=prefix_admission_plan(profile,bundle,n,sum(r['bytes']for r in artifacts))
                                                                            
                                                                            
                                                                        
                                                                                
                                                                               
    plan['cold_coexistence_required_free_bytes']=plan['required_free_bytes']
    remaining=disk.stage_budget(n,'high_gray_and_rgb',existing_live_bytes=0,
        failure_slot_allocated=False)
    journals=disk.cold_budget(n,manifest_bytes=0)['components']['journals_metadata']
    plan['required_free_bytes']=remaining['required_free_bytes']+journals
    plan['admission_basis']='NEXT_STAGE_REMAINING_WRITES_WITH_EXISTING_LATER_STAGE_GATES'
    plan['next_stage_remaining_writes']=remaining
    plan['journals_metadata_bytes']=journals
    output=absolute(envelope['io_roots']['downstream_output_root'])
    probe=output
    while not probe.exists()and probe!=probe.parent:probe=probe.parent
    plan['available_free_bytes']=disk.require_space(probe,plan['required_free_bytes'],'whole_cold')
    state['disk_admission']=plan
    C.CDLL('/System/Library/Frameworks/UIKit.framework/UIKit')
    require(owner_context.CURRENT is None or (owner_context.CURRENT.owner.state=='DESTROYED_SUFFIX_READY'and not owner_context.CURRENT.owner.owner),'whole worker already occupied/pending; no retry')
    roots=envelope['io_roots'];out=absolute(roots['downstream_output_root']);docs=Path(os.path.commonpath([str(absolute(roots['durable_input_root'])),str(out)]))
    require(out!=docs and out.is_relative_to(docs),'private output root')
    scratch=out/('whole_prefix_'+envelope['run_id']);require(scratch.parent==out,'run_id must be simple component');scratch.mkdir(mode=0o700)
    state['scratch']=scratch
    retirement=PrefixRetirement(scratch,envelope['run_id']);state['prefix_retirement']=retirement
    state['failure_reserve']=disk.reserve_failure_slot(scratch/'FAILURE_COMPACT.json')
    b=profile['bindings'];m0=C.CDLL(str(verify(bundle,b['M0'])));state['M0']=m0
    m0.reconstruction_m0_observer_boundary.argtypes=[C.c_char_p];m0.reconstruction_m0_observer_boundary.restype=C.c_int
    require(isinstance(envelope['run_id'],str)and envelope['run_id']and len(envelope['run_id'])<=64,'bounded actual run ID')
    enter_label='RECONSTRUCTION_ENTER:'+envelope['run_id'];return_label='RECONSTRUCTION_RETURN:'+envelope['run_id']
    enter_before_ns=time.clock_gettime_ns(time.CLOCK_MONOTONIC)
    require(m0.reconstruction_m0_observer_boundary(enter_label.encode())==0,'continuous external M0 must already be active')
    enter_after_ns=time.clock_gettime_ns(time.CLOCK_MONOTONIC)
    thermal_lib=C.CDLL(str(verify(bundle,b['thermal'])));state['thermal']=thermal_lib
    thermal=thermal_lib.reconstruction_thermal_snapshot_v1;thermal.argtypes=[C.POINTER(C.c_int),C.POINTER(C.c_double),C.POINTER(C.c_double),C.POINTER(C.c_ulonglong)];thermal.restype=C.c_int
    def before_native_request(phase,pack_index):
        require_fixed_runtime_policy()
        require(phase in ('feature_create','feature_request'),'actual feature gate phase')
        require((phase=='feature_create'and pack_index is None)or (phase=='feature_request'and type(pack_index)is int and 0<=pack_index<n),'actual feature gate frame')
        gpu.wait_thermal(thermal,phase+'_'+str(pack_index),scratch/'THERMAL_PAUSE.jsonl')
        require(m0.reconstruction_m0_observer_boundary((phase+'_'+str(pack_index)).encode())==0,'continuous feature M0 boundary')
    rt,co=load_native(profile,bundle,state,passive_images)
    for library,name in ((rt,'reconstruction_whole_dawn_frame_count_supported_v1'),(co,'reconstruction_continuous_mvs_frame_count_supported_v1')):
        capability=getattr(library,name);capability.argtypes=[C.c_uint32];capability.restype=C.c_int
        require(capability(n)==1,'native generic-N capability absent')
    models={k:row(verify(bundle,r))for k,r in profile['models'].items()}
                                                                                     
                                                                        
    dense=request['dense_inputs'];root=absolute(dense['input_root']);art={r['path']:{**r,'path':str(root/r['path'])}for r in dense['artifacts']}
    partial={'frames':[dict(photo=art[f['jpeg_path']])for f in dense['frames']]}
    admit_fixed_stage(scratch,n,'high_gray_and_rgb',state)
    high,rgb=prepare_high(partial,scratch)
    feature_request=features.prepare_feature_request(request,gray_banks={'768':profile['low_gray'],'1536':high},model_paths=models,original_fixture_receipt=profile['original_fixture_receipt'],independent_bench=verified_independent_bench(envelope),production_reference=selected_reference_production(envelope),model_graph_values=profile.get('production_reference_closure_current',profile.get('validation_fused_graph',{})).get('model_graph_values'),fnet_graph_values=profile.get('production_reference_closure_current',profile.get('validation_fused_graph',{})).get('fnet_graph_values'))
    state['feature_request']=feature_request
    state['source_inputs_sha256']=features.sha256(features.canonical(request))
    producer=features.NativeFeatureProducer(str(verify(bundle,b['producer'])),b['producer']['sha256'],runtime_path=str(verify(bundle,b['runtime'])),runtime_sha256=b['runtime']['sha256']);state['producer']=producer
    materials={}
    neighbor_bytes=features.bound_file(feature_request['neighbors']).read_bytes()
    consumers=feature_reference_consumers(list(struct.iter_unpack('<9i',neighbor_bytes)))
    admit_fixed_stage(scratch,n,'feature_f16',state)
    for width in (() if state['feature_transport']=='gpu_fixed_slots_v1' else (768,1536)):
        cache=scratch/('materials_'+str(width));cache.mkdir()
        rows=producer.produce(feature_request,width,cache,cache_budget_bytes=profile['cache_budget_bytes'][str(width)],before_native_request=before_native_request)
        materials[width]=features.publish_material_index(feature_request,width,cache,rows)
                                                                           
                                                                                 
        pass_index=0 if width==768 else 1
        for produced in rows:
            retirement.register(cache/produced['file'],consumers=tuple(
                f'feature:{pass_index}:{ref}' for ref in consumers[produced['pack_index']]),
                producer_sha256=produced['sha256'],
                producer_manifest_sha256=materials[width]['manifest_sha256'])
    if production:
        state['C_upload_diagnostic']=dict(status='DISABLED_NOT_RUN',scope='benchmark_fixed_input_allocation_receipts',frame_count=n)
    if not production and state['feature_transport']=='sealed':capture_c_upload(rt,scratch,'FEATURE_CAPS',n,profile,state)
    bind_environment(profile,bundle,materials)
    ctx=owner_context.Session(rt,co,gpu,m0,thermal,scratch,Request,production=production);state['session']=ctx;owner_context.CURRENT=ctx
    times={};pass_start={}
    overlap=(os.environ.get('RECONSTRUCTION_BENCH_OVERLAP_CER_N1')=='1' and
             verified_independent_bench(envelope))
    n1=None
    try:
        if overlap:
            fusion=request['stage_paths']['fusionPly'];work=inside(out,fusion['work_dir']);retained=inside(out,fusion['fused_depth_dir']);work.mkdir(parents=True,exist_ok=True)
            admit_fixed_stage(scratch,n,'cer_f32',state)
            original_cer=verified_import('reconstruction_original_dense_raw_driver',profile,bundle)
            original_cer.validate_dependencies()
            n1=_ProductionCerN1(scratch/'mvs',work,n,verified_import('reconstruction_fusion.q_fullchain_fusion_driver',profile,bundle))
            state['cer_n1']=n1
        @Callback
        def progress(_,phase,pass_,ref,count,elapsed):
            try:
                require_fixed_runtime_policy()
                require(os.environ.get('PW_GPU_FEATURE_TRANSFER','0')==('1' if state['feature_transport']=='gpu_fixed_slots_v1' else '0'),'feature transport changed after binding')
                if not production and phase==5 and pass_==0 and state['feature_transport']=='gpu_fixed_slots_v1':capture_c_upload(rt,scratch,'FEATURE_CAPS',n,profile,state,phase=phase,pass_index=pass_,ref=ref)
                require(count==n and pass_ in (0,1),'actual scene count/pass')
                disk.require_space(scratch,disk.RESERVE_BYTES,'mvs_progress')
                ctx.boundary(f'MVS_{pass_}_{phase}_{ref}')
                if not production and phase==5 and pass_==1:capture_c_upload(rt,scratch,'HIGH_INIT',n,profile,state,phase=phase,pass_index=pass_,ref=ref)
                if not production and phase==2 and pass_==1 and ref==0:capture_c_upload(rt,scratch,'HIGH_REF0',n,profile,state,phase=phase,pass_index=pass_,ref=ref)
                if phase==5:pass_start[pass_]=time.perf_counter()-elapsed/1000
                if phase==3:times[pass_]=(time.perf_counter()-pass_start[pass_])*1000;ctx.snapshot(f'PASS{pass_}_RETURN')
                ctx.record(dict(stage='ORIGINAL_MVS_PROGRESS',phase=phase,pass_index=pass_,ref=ref,count=count,elapsed_ms=elapsed))
                if phase==2:
                    retirement.retire_features_after_reference(pass_,ref,producer_pending=producer.pending is not None)
                return n1.progress(phase,pass_,ref)if n1 is not None else 0
            except BaseException as e:ctx.record_noexcept(dict(stage='MVS_CALLBACK_FAILED',error=repr(e)));return 1
        admit_fixed_stage(scratch,n,'mvs_f32',state)
        ctx.owner.create();mvsroot=scratch/'mvs';native,args=request_native(feature_request,mvsroot,progress)
        try:ctx.owner.mvs(native,args)
        finally:
            if not production:
                state['C_upload_final_receipt']=capture_c_upload(rt,scratch,'MVS_FINALLY',n,profile,state,strict=False)
            if n1 is not None and sys.exc_info()[0]is not None:n1.shutdown()
        require(set(times)=={0,1},'actual two pass return timings absent')
        if not production:require(all(label in state.get('C_upload_receipts',{}) and state['C_upload_receipts'][label]['validation_error'] is None for label in ('FEATURE_CAPS','HIGH_INIT','HIGH_REF0','MVS_FINALLY')),'actual ordered C upload receipts incomplete')
                                                                                
                                                                                   
                                                                              
        features.validate_request(feature_request)
                                                                                    
                                                                                   
                                                                                    
        state['feature_banks_retirement_receipt']=retirement.retire_remaining_features_after_mvs(
            owner_state=ctx.owner.state,producer_pending=producer.pending is not None)
        retirement.register(Path(high['path']),consumers=('mvs_input_validation',))
        retirement.complete('mvs_input_validation')
        state['mvs_input_retirement_receipt']=retirement.retire_mvs_inputs(
            owner_state=ctx.owner.state,producer_pending=producer.pending is not None)
        fusion=request['stage_paths']['fusionPly'];work=inside(out,fusion['work_dir']);retained=inside(out,fusion['fused_depth_dir']);work.mkdir(parents=True,exist_ok=True)
        control=make_control(profile,request,feature_request,mvsroot,rgb,docs,out,work,retained,validated_bits,passive_images,envelope.get('identity_scope'))
        for tier in ('low','high'):
            for plane in PLANES[1:]:
                retirement.register(mvsroot/tier/(plane+'.f32'),consumers=('cer',))
        stages=[dict(stage=name,return_code=0,elapsed_ms=times[i])for i,name in enumerate(('mvs768','mvs1536'))]
        admit_fixed_stage(scratch,n,'cer_f32',state)
        t=time.perf_counter();ctx.boundary('PRODUCTION_CER_ENTER')
        if n1 is None:cer(control,docs,work,profile,bundle)
        else:state['cer_overlap_receipt']=n1.finish()
        ctx.boundary('PRODUCTION_CER_RETURN');stages.append(dict(stage='cer',return_code=0,elapsed_ms=(time.perf_counter()-t)*1000))
        H=dict(count=n,width=1536,height=1152,frame_id_f64_bits=control['frame_id_f64_bits'],camera=control['cameras_high'],CER_stage_returned=True,CER_banks=[row(work/(k+'.f32'),docs)for k in PLANES])
        _require_cer_retirement_barrier(n1,n)
        retirement.retire_cer_sources(owner_state=ctx.owner.state,producer_pending=producer.pending is not None)
        for name in ('reconstruction_row_runtime.native_hook','reconstruction_original_dense_raw_driver','reconstruction_fusion.fusion_runtime_guard'):
            verified_import(name,profile,bundle)
        admit_fixed_stage(scratch,n,'retained_depth_rgb_camera',state)
        t=time.perf_counter();gpu.run(Path(__file__).parent,docs,bundle,out,bound_inputs=dict(handoff=H,control=control,bindings=b));stages.append(dict(stage='fusionPly',return_code=0,elapsed_ms=(time.perf_counter()-t)*1000))
        require(ctx.owner.state=='DESTROYED_SUFFIX_READY','actual destroy0 barrier')
                                                                                  
                                                                                  
                                                                               
        retirement.register(Path(rgb['path']),consumers=('fusion_rgb_emit',))
        retirement.complete('fusion_rgb_emit')
        state['fusion_rgb_retirement_receipt']=retirement.retire_fusion_rgb(
            owner_state=ctx.owner.state,producer_pending=producer.pending is not None)
                                                                                 
                                                                                    
        features.validate_fixture_content_binding(feature_request)
        for model_row in feature_request['models'].values():features.bound_file(model_row)
        require(features.sha256(features.canonical(request))==state['source_inputs_sha256'],'original request mutated')
        return_before_ns=time.clock_gettime_ns(time.CLOCK_MONOTONIC)
        ctx.boundary(return_label)
        return_after_ns=time.clock_gettime_ns(time.CLOCK_MONOTONIC)
        order=json.loads((retained/'order.json').read_text());points=order['kept_total'];require(order['order']==list(range(n))and type(points)is int and 0<points<=n*1536*1152,'actual retained order/points')
                                                                                  
                                                                                   
        from .python_runtime.peak_trace import measured_peak
        peak=measured_peak(absolute(profile['m0_trace_path']),expected_run_id=profile['m0_run_id'],enter_label=enter_label,return_label=return_label,enter_window_ns=(enter_before_ns,enter_after_ns),return_window_ns=(return_before_ns,return_after_ns),physical_limit_bytes=profile['physical_limit_bytes'])
        rows=[]
        for i in range(n):
            stem=retained/f'{i:08d}';meta=json.loads(stem.with_suffix('.json').read_text())
            require(meta['pack_index']==i and meta['original_index']==i and meta['frame_id_f64_bits']==control['frame_id_f64_bits'][i],'retained original identity')
            for suffix,size in (('.d16',3538944),('.camera.f64le',200),('.col.u8',5308416)):require(stem.with_suffix(suffix).stat().st_size==size,'retained actual layout')
            rows.append(dict(pack_index=i,original_index=i,frame_id_f64_bits=meta['frame_id_f64_bits'],d16=str(stem.with_suffix('.d16')),camera_f64=str(stem.with_suffix('.camera.f64le'))))
        require(fusion['out_ply']=='','retained-only transport requires empty out_ply')
                                                                               
                                                                                    
                                                                                  
                                                                                   
        retirement.add_generated_root(work)
                                                                              
                                                                                  
                                                                                   
                                                                         
        fixture_root=out.parent/'original_fixture'
        require(Path(feature_request['gray_banks']['768']['path'])==fixture_root/'images.f16',
                'actual same-run original fixture gray required')
        require(root==out.parent/'inputs','actual same-run generated input root required')
        retirement.add_generated_root(fixture_root)
        retirement.add_generated_root(root)
        final_payloads=[Path(feature_request['gray_banks']['768']['path']),
                        Path(feature_request['low_noise']['path']),
                        Path(feature_request['high_noise']['path'])]
                                                                                
                                                                                       
        for payload in final_payloads:
            retirement.register(payload,consumers=('fusion','source_validation'))
        retirement.complete('fusion');retirement.complete('source_validation')
        state['prefix_retirement_receipt']=retirement.retire(owner_state=ctx.owner.state,producer_pending=producer.pending is not None)
        return dict(schema='RECONSTRUCTION_PRODUCTION_WHOLE_RESULT_V1',dense_producer_profile=PROFILE,native_entry='reconstruction_production_whole_run_v1',status=0,frames=n,fused_frames=n,low_completed=n,high_completed=n,inferred=2*n,points=points,kept_total=points,physical_peak_bytes=peak,work_dir=str(work),out_ply='',fused_depth_dir=str(retained),order_json=str(retained/'order.json'),stages=stages,retained_frames=rows,disk_admission=state['disk_admission'],stage_disk_admissions=state['stage_disk_admissions'],feature_banks_retirement=state['feature_banks_retirement_receipt'])
    finally:
        if n1 is not None:
            if sys.exc_info()[0] is not None:
                try:n1.shutdown()
                except BaseException as close_error:
                    ctx.record_noexcept(dict(stage='N1_FAILURE_CLOSE_ERROR',error=repr(close_error)))
            else:n1.shutdown()

def run_json(input_path,result_path):
    global _LAST_ERROR
    state={'input_path':str(input_path),'result_path':str(result_path),'started_ns':time.monotonic_ns()}
    key=str(input_path)
    try:
        key=str(absolute(input_path));require(key not in _PENDING,'same job automatic retry forbidden');_PENDING[key]=state
        result=run(json.loads(Path(input_path).read_text()),state)
        write_json(absolute(result_path),result);state['status']='PREFIX_RETURNED0_SUFFIX_NOT_CALLED';return 0
    except BaseException as error:
        _LAST_ERROR=repr(error);state.update(status='FAILED_PRESERVE',error=_LAST_ERROR,traceback=traceback.format_exc())
                                                                               
        owner=state['session'].owner.state if state.get('session')else None
                                                                                
        state['failure_evidence']=disk.persist_failure(result_path,error,
            reserve=state.get('failure_reserve'),owner_state=owner)
        return 1

def last_error():return _LAST_ERROR
