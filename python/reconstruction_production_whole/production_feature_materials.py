"""Actual per-capture transport for the existing portable S6 FNET producer.

No model execution fallback: the caller must supply a hash-bound native library
exporting feature_material_portable_v2/feature_producer.h. CPU validation is not
an iOS/GPU execution claim. The embedding app owns runtime admission and sole
Dawn initialization. No benchmark photo, camera or SEEDED identity is embedded here.
"""
import ctypes as C
import importlib.util
import hashlib
import json
import os
from pathlib import Path
import struct

SCHEMA = 'RECONSTRUCTION_PRODUCTION_FEATURE_REQUEST_V1'
PREPROCESS_SHA256 = '453f0d9a7f62c9a693aaac58e57cf2e01b77cb5820d54f678a35aa5cabb33be6'
MODEL_SHA256 = {
    'feature_low': 'ac6bc8f7822c735191e50038e6b0ac9c4b57cbce82faf1f778e217d361027593',
    'feature_high': '376e6c36db0064adae367db87f35b3ef9c6a187e0e36c18a818e3ab7942a0683',
    'rest_low': 'cf39dfa7d93de9fa397a2c14c00ae60ab6c88a5fef1293bbd8633d6f469d54d5',
    'rest_high': '358c581f62fedb71209eb248b03fd6131d1e6a05bf73527921fdaae493b150d0',
}

def require(condition, message):
    if not condition: raise ValueError(message)

def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False, allow_nan=False).encode()

def sha256(data): return hashlib.sha256(data).hexdigest()

def require_sha(value):
    require(isinstance(value,str) and len(value)==64 and all(c in '0123456789abcdef' for c in value), 'SHA256 required')
    return value

def require_count(count):
    require(type(count) is int and count > 0, 'actual positive ordered frame count required')

def file_sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(65536), b''): h.update(block)
    return h.hexdigest()

def bound_file(row, expected_bytes=None):
    path = Path(row['path'])
    require(path.is_absolute() and path.resolve()==path and path.is_file(), 'canonical actual file required')
    require(path.stat().st_size == row['bytes'], 'bound file size changed')
    if expected_bytes is not None: require(row['bytes']==expected_bytes, 'actual frame/table dimensions')
    require(file_sha(path)==require_sha(row['sha256']), 'bound file SHA changed')
    return path

def material_cache_key(photo_sha256, model_sha256, preprocess_sha256, prepared_gray_sha256):
    identity = dict(photo_sha256=require_sha(photo_sha256), model_sha256=require_sha(model_sha256),
                    preprocess_sha256=require_sha(preprocess_sha256), prepared_gray_sha256=require_sha(prepared_gray_sha256))
    return sha256(canonical(identity))

def require_feature_disk_space(root, bytes_to_write):
    """Observe free bytes without rejecting generation against a reserve.

    Native file writes retain ENOSPC handling.
    """
    fs=os.statvfs(root)
    require(fs.f_bavail>=0 and fs.f_frsize>0,'valid actual filesystem availability')
    return fs.f_bavail*fs.f_frsize

def layer_specs(width):
    require(width in (768,1536), 'selected width required')
    result=[]; offset=0
    for i,(channels,divisor) in enumerate(((48,8),(32,4),(16,2))):
        shape=[1,channels,width*3//4//divisor,width//divisor]
        size=2*shape[1]*shape[2]*shape[3]
        result.append(dict(level=i,name=f'/core/feature/out{i+1}/Conv_output_0',shape=shape,dtype=10,bytes=size,offset=offset))
        offset+=size
    return result

class _IndependentBenchModelRequest(dict):
    """In-memory model-selection marker; JSON cannot create this request type.

    This does not mint OriginalFixtureExecutionProof. Its file/order/noise
    checks remain required, and the caller must verify the actual Bench scope.
    """
    __slots__ = ('_model_family',)
    def __init__(self, value, family):
        super().__init__(value)
        self._model_family = family

_BENCH_REST_FAMILIES = (
    ('w6_r2', '36d89ea8ec7d713518712dfba6a500ead7724bf0212d8f8aeedee50ef9559461', 3557),
    ('w6_cost3_finaldiv', 'f8b1ad2a4f2b7912e7cb093d4edf02b936d1f9bc825ebf3da051ca5a27a08cd5', 3556),
    ('w6_cost3_layout_e', 'c6ef1789c0897264112ad28bf71e0e1f1c47166ed98559faa43b33b00ca1d680', 3555),
)

_MIXED_FNET_FAMILY = {
    'feature_low': '6a917de8928c891274383001e48945c60dd898e6c223c90e26d512e953fed60d',
    'feature_high': '5f0fb021b12e0ca092b1259bf9828c1588e1b1b0f1f166e1b067be1fae47f2a7',
    'rest_low': '4bfb139a9fd8f726e6a296826ef41ff5f3d77e0a0e9db8220c7df4ef8c67cdf1',
    'rest_high': 'c6ef1789c0897264112ad28bf71e0e1f1c47166ed98559faa43b33b00ca1d680',
}

def _bench_models(family):
    if family == 'mixed_precision_feature_graph':
        return _MIXED_FNET_FAMILY
    for name, high, _ in _BENCH_REST_FAMILIES:
        if family == name:
            return {**MODEL_SHA256,
                'rest_low': '4bfb139a9fd8f726e6a296826ef41ff5f3d77e0a0e9db8220c7df4ef8c67cdf1',
                'rest_high': high}
    raise ValueError('unknown in-memory Bench model family')

class _ProductionR26ModelRequest(_IndependentBenchModelRequest):
    """Exact selected REFERENCE recipe, authorized by real production package scope."""


def _model_identities_for_request(request):
    if type(request) in (_IndependentBenchModelRequest, _ProductionR26ModelRequest):
        return _bench_models(request._model_family)
    return MODEL_SHA256

def _bind_known_model_request(request, *, independent_bench=False, production_reference=False, model_graph_values=None, fnet_graph_values=None):
    require(type(independent_bench) is bool, 'verified Bench scope boolean required')
                                                                            
                                                                                
    request = json.loads(canonical(request))
    models = request.get('models')
    require(isinstance(models, dict) and set(models) == set(MODEL_SHA256), 'four selected models required')
    actual = {key: row.get('sha256') if isinstance(row, dict) else None for key, row in models.items()}
    require(type(production_reference) is bool, 'verified production REFERENCE authority required')
    if production_reference:
        require(actual == _MIXED_FNET_FAMILY, 'exact production REFERENCE four models required')
        require(model_graph_values == [4119, 3555] and type(fnet_graph_values) is int and fnet_graph_values == 65, 'exact production REFERENCE graph counts')
        require(os.environ.get('PW_FNET_MIXED_CORRIDOR') == '1', 'production REFERENCE mixed FNET flag')
        return _ProductionR26ModelRequest(request, 'mixed_precision_feature_graph')
    if actual == MODEL_SHA256:
        return request
    require(independent_bench, 'selected model identity')
    for family, _, high_count in _BENCH_REST_FAMILIES:
        if actual != _bench_models(family):
            continue
        require(type(model_graph_values) is list and len(model_graph_values) == 2
                and all(type(n) is int for n in model_graph_values)
                and model_graph_values == [4119, high_count], 'known Bench model graph counts')
        return _IndependentBenchModelRequest(request, family)
    if actual == _MIXED_FNET_FAMILY:
        require(type(model_graph_values) is list and len(model_graph_values) == 2
                and all(type(n) is int for n in model_graph_values)
                and model_graph_values == [4119, 3555]
                and type(fnet_graph_values) is int and fnet_graph_values == 65,
                'mixed FNET exact graph counts')
        require(os.environ.get('PW_FNET_MIXED_CORRIDOR') == '1', 'mixed FNET launch flag')
        return _IndependentBenchModelRequest(request, 'mixed_precision_feature_graph')
    raise ValueError('exact complete known Bench model family required')

def _validate_selected_models(request):
    selected = _model_identities_for_request(request)
    require(set(request['models']) == set(selected), 'four selected models required')
    for key, digest in selected.items():
        require(request['models'][key]['sha256'] == digest, 'selected model identity')
        bound_file(request['models'][key])

def validate_request(request):
    """Validate explicit input transport. All file rows use absolute path/bytes/sha256.

    Required: schema, frames[{pack_index,frame_id,photo:{file row}}], models
    {feature_low,rest_low,feature_high,rest_high}, cameras, neighbors,
    low_noise,high_noise, gray_banks:{'768':row,'1536':row}, preprocess_sha256,
    original_fixture_receipt:{file row}. Gray banks must be actual original
    fixture images.f16 in the original ordered frame sequence; this module
    neither resizes photos nor synthesizes SEEDED.
    """
    require(request['schema']==SCHEMA, 'feature request schema')
    n=len(request['frames']); require_count(n)
    require(request['preprocess_sha256']==PREPROCESS_SHA256, 'selected preprocessing identity')
    validate_fixture_content_binding(request)
    _validate_selected_models(request)
    ids=[]
    for i,frame in enumerate(request['frames']):
        require(frame['pack_index']==i,'actual frame order')
        ids.append(frame['frame_id']); bound_file(frame['photo'])
    require(len(set(ids))==n,'unique actual frame IDs')
    bound_file(request['cameras'],n*36*4)
    neighbors=bound_file(request['neighbors'],n*9*4)
    with neighbors.open('rb') as f:
        for ref in range(n):
            values=struct.unpack('<9i',f.read(36))
            require(all(0<=x<n for x in values) and len(set((ref,)+values))==10,'original ref+9 ordered unique neighbors')
    for width,key in ((768,'low_noise'),(1536,'high_noise')):
        pixels=width*(width*3//4)
        bound_file(request['gray_banks'][str(width)],n*pixels*2)
        bound_file(request[key],n*(pixels//16+pixels//4)*4)
    return n

def validate_fixture_content_binding(request):
    """Content linkage only. Dart retains the in-worker execution capability.

    Read the existing receipt -> contract -> input-manifest chain and the pinned
    original wrapper's actual source inspection. These JSON files never mint
    OriginalFixtureExecutionProof or authorize an external call/device action.
    """
    receipt_path=bound_file(request['original_fixture_receipt'])
    receipt=json.loads(receipt_path.read_bytes())
    require(receipt['status']=='returned' and receipt['source_and_inputs_unchanged'] is True and receipt.get('error_type') is None and 'verification_error_type' not in receipt,'actual fixture successful receipt')
    contract_path=Path(str(receipt_path)+'.contract.json')
    require(contract_path.resolve()==contract_path and file_sha(contract_path)==require_sha(receipt['contract_sha256']),'actual fixture contract SHA')
    contract=json.loads(contract_path.read_bytes()); n=len(request['frames']);require_count(n)
    production=type(request) is _ProductionR26ModelRequest or (type(request) is _IndependentBenchModelRequest and request._model_family == "mixed_precision_feature_graph")
    from .production_capacity import (PRODUCTION_FIXTURE_WRAPPER_SHA256,
        PRODUCTION_NOISE_BANK_SHA256, PRODUCTION_NOISE_PRODUCER_JSON_SHA256)
    v2=receipt['schema']=='RECONSTRUCTION_INTACT_FIXTURE_EXECUTION_RECEIPT_V2'
    require(receipt['schema']==('RECONSTRUCTION_INTACT_FIXTURE_EXECUTION_RECEIPT_V2' if v2 else 'RECONSTRUCTION_INTACT_FIXTURE_EXECUTION_RECEIPT_V1') and contract['schema']==('RECONSTRUCTION_INTACT_FIXTURE_EXECUTION_CONTRACT_V2' if v2 else 'RECONSTRUCTION_INTACT_FIXTURE_EXECUTION_CONTRACT_V1'),'original receipt/contract schema')
    if v2:
        require(receipt['actual_frames']==n and contract['actual_frames']==n and receipt['producer_profile']==contract['producer_profile']=='original_numpy_opencv_retained_v2','actual original frame count/profile')
    else: require(n==97 and not production,'original V1 remains legacy97 only')
    if production:
        require(v2 and contract.get('frame_count_policy')=='actual_positive_count','production actual positive frame-count policy required')
    elif v2:
        require(contract['noise_bank_capacity']==97,'legacy noise bank identity required')
    source=Path(contract['argv'][0]); capture=Path(contract['argv'][1]); output=Path(contract['argv'][2])
    source_sha='e18aa44182c1916364f9a7c0a9fc116d57d92cfce902abbbd7862019b23535b3'
    wrapper_sha='ec43166afdb8bd413d9fd069d9511abe66ab7c92d25f58aee6530423cd474d1e' if v2 else 'ef5c8791bdfc1ac6ab055f0495d797974b84764b2c4626ab0c6d852fbe2c5127'
    if production:wrapper_sha=PRODUCTION_FIXTURE_WRAPPER_SHA256
    wrapper=source.with_name('reconstruction_original_fixture_bridge.py')
    require(source.is_absolute() and source.resolve()==source and contract['source_sha256']==source_sha and file_sha(source)==source_sha,'pinned actual original producer source')
    require(contract['wrapper_sha256']==wrapper_sha and wrapper.resolve()==wrapper and file_sha(wrapper)==wrapper_sha,'pinned actual original fixture wrapper')
    binding=contract['input_manifest_binding'];manifest_path=Path(binding['path'])
    require(manifest_path.is_absolute() and manifest_path.resolve()==manifest_path and file_sha(manifest_path)==require_sha(binding['sha256']),'actual original source manifest SHA')
    manifest=json.loads(manifest_path.read_bytes())
    require(manifest==contract['inputs'] and manifest['root']==str(capture) and manifest['selected_frames']==n,'original manifest/contract input identity')
    spec=importlib.util.spec_from_file_location('_reconstruction_verified_fixture_io',wrapper)
    bridge=importlib.util.module_from_spec(spec);spec.loader.exec_module(bridge)
    require(contract['classification']=='actual_execution_no_parity_claim' and contract['environment']==bridge.ENV,'original fixture classification/environment')
    require(all(contract['runtime'][k]==value for k,value in bridge.RUNTIME.items()),'original fixture provider versions')
    require(bridge.capture_manifest(capture,expected_frames=n)==manifest,'actual original capture sources changed')
    source_rows={row['path']:row for row in manifest['artifacts']}
    poses={p['frame_id']:p for p in json.loads((capture/'official_sfm_sparse_meta.json').read_bytes())['poses'] if p.get('registered')}
    for i,frame in enumerate(request['frames']):
        name=manifest['ordered_source_names'][i]; row=source_rows['photos_jpg/'+name]
        require(all(row[k]==frame['photo'][k] for k in ('bytes','sha256')),'source JPEG differs from original ordered capture')
        require(frame['pack_index']==i and frame['frame_id']==manifest['frame_ids'][i],'original ordered frame identity')
        actual=frame['original_frame_transport']; pose=poses[frame['frame_id']]
        side=json.loads((capture/'sidecars'/name.replace('.jpg','.json')).read_bytes())
        require(actual['frame_id']==frame['frame_id'] and actual['q_wxyz']==pose['quat_wxyz'] and actual['t']==pose['t'],'original refined pose/frame identity')
        require([actual[k] for k in ('fx','fy','cx','cy')]==side['intrinsics_fxfycxcy'] and [actual[k] for k in ('image_w','image_h')]==[side['image_w'],side['image_h']],'original intrinsics/dimensions identity')
    outputs={row['path']:row for row in receipt['outputs']}
    expected={'cams.f32','neighbors.i32','images.f16','frames.json'}|{f'rgb/{i:08d}.jpg' for i in range(n)}
    require(len(outputs)==len(receipt['outputs']) and set(outputs)==expected,'exact original output layout')
    require(output.is_absolute() and output.resolve()==output,'actual original output root')
    for name,row in outputs.items(): bound_file({**row,'path':str(output/name)})
    for name,row in (('images.f16',request['gray_banks']['768']),('cams.f32',request['cameras']),('neighbors.i32',request['neighbors'])):
        require(all(outputs[name][k]==row[k] for k in ('bytes','sha256')),'substituted original fixture output')
    metadata=json.loads((output/'frames.json').read_bytes())
    require(metadata['count']==n and metadata['width']==768 and metadata['height']==576 and metadata['num_src']==9 and metadata['frame_ids']==manifest['frame_ids'] and metadata['names']==manifest['ordered_source_names'],'original produced frame table identity')
    noise=request['noise_identity']
    expected_producer = (PRODUCTION_NOISE_PRODUCER_JSON_SHA256 if production else
        'aa09d2db3c79d940d0f741fae4dcd0523bf02d8ff69145cf98d5ed47aff4703f')
    require(noise['kind']=='seeded_cpu_gaussian_prefix' and noise['ordered_frames']==n and noise['source_producer_sha256']==expected_producer,'original SEEDED producer/order')
    for label,frame_bytes,bank_sha in (('low',552960,'be0a6856547538a5a10537e9c64390e8db7e9eac7e0a39ca563b01ff7ce27984'),('high',2211840,'77b9df4766b323048b02a8dfecd1fd46d375a5be8c4492d8d8c6aed5c4c1b6b1')):
        if production:
            bank_sha=PRODUCTION_NOISE_BANK_SHA256[label]
            require(isinstance(bank_sha,str) and len(bank_sha)==64,'verified production150 noise banks not yet bound')
        row=noise['prefix_streams'][label]; actual=request[label+'_noise']
        checks=dict(source_bank_frames=(150 if production else 97),ordered_frames=n,frame_bytes=frame_bytes,bytes=n*frame_bytes,source_offset=0,seed_base=0,seed_rule='torch.manual_seed(0 + frame_index)',draw_order='stage2_then_stage3_randn_like',copy_kind=('original_seed_bank_with_generated_tail' if production and n>150 else 'exact_first_N_frame_blocks'),source_bank_sha256=bank_sha,sha256=actual['sha256'])
        if production and n>150:require(row.get('generator_sha256')=='d8f756b5f482c0c5bd1b77a4176cda3928f160b0eacc879f4b7840b4a70458e1','original per-frame generator identity')
        require(all(row[k]==value for k,value in checks.items()) and row['sha256']==noise[label+'_sha256'],'original SEEDED exact prefix/table binding')
        bound_file(actual,n*frame_bytes)
                                                                                
    require(file_sha(contract_path)==receipt['contract_sha256'] and file_sha(manifest_path)==binding['sha256'],'original binding changed during inspection')
    bound_file(request['original_fixture_receipt'])


def prepare_feature_request(dense_request, *, gray_banks, model_paths, original_fixture_receipt, independent_bench=False, production_reference=False, model_graph_values=None, fnet_graph_values=None):
    """Adapt the existing dense_inputs transport without replacing its ordering.

    gray_banks is explicit: LOW comes from original_fixture images.f16; HIGH
    must come from the already-selected original high-resolution preprocessing.
    The caller retains OriginalFixtureExecutionProof authority in the worker.
    JSON receipt validation cannot mint that in-memory execution capability.
    """
    dense=dense_request['dense_inputs']; root=Path(dense['input_root'])
    require(root.is_absolute() and root.resolve()==root,'actual dense input root')
    artifacts={}
    for row in dense['artifacts']:
        require(row['path'] not in artifacts,'duplicate dense artifact')
        path=root/row['path']
        require(not Path(row['path']).is_absolute() and path.resolve().is_relative_to(root),'dense artifact confinement')
        artifacts[row['path']]={**row,'path':str(path)}
    request=dict(schema=SCHEMA,preprocess_sha256=PREPROCESS_SHA256,
        dense_request_sha256=sha256(canonical(dense_request)),noise_identity=dense_request.get('noise_identity'),
        frames=[dict(pack_index=i,frame_id=frame['frame_id'],photo=artifacts[frame['jpeg_path']],
                     original_frame_transport=frame) for i,frame in enumerate(dense['frames'])],
        models=model_paths,gray_banks=gray_banks,original_fixture_receipt=original_fixture_receipt,
        cameras=artifacts[dense['cameras_f32']],neighbors=artifacts[dense['neighbors_i32']],
        low_noise=artifacts[dense['low_noise_f32']],high_noise=artifacts[dense['high_noise_f32']])
    request = _bind_known_model_request(request, independent_bench=independent_bench, production_reference=production_reference, model_graph_values=model_graph_values, fnet_graph_values=fnet_graph_values)
    validate_fixture_content_binding(request)
    validate_request(request)
    return request

class LayerSpec(C.Structure):
    _fields_=[('name',C.c_char_p),('shape',C.c_int64*4),('bytes',C.c_size_t)]
class Config(C.Structure):
    _fields_=[(x,C.c_char_p) for x in ('model_path','model_sha256','preprocess_sha256','cache_root')]+[('width',C.c_int),('height',C.c_int),('layers',LayerSpec*3),('max_frame_payload_B',C.c_size_t),('max_cache_payload_B',C.c_size_t)]
class Identity(C.Structure):
    _fields_=[('pack_index',C.c_int),('photo_sha256',C.c_char_p),('prepared_gray_sha256',C.c_char_p)]
class NativeRequest(C.Structure):
    _fields_=[('pack_index',C.c_int)]+[(x,C.c_char_p) for x in ('photo_sha256','model_sha256','preprocess_sha256')]+[('gray',C.POINTER(C.c_uint16)),('gray_elements',C.c_size_t),('interrupt_after_complete_partial',C.c_int)]

                                                                              
                                                                             
                                                                               
_PENDING_FEATURE_OWNERS = {}

class PendingFeatureOwner:
    def __init__(self, library, handle, config, request, bank, model_file):
        require(handle, 'non-null real native producer owner required')
        self.library=library
        self.handle=C.c_void_p(handle)
        self.config=config
        self.request=request
        self.bank=bank
        self.model_file=model_file
        self.phase='CREATED'
        self.current=None
        self.error=None


class NativeFeatureProducer:
    """Require hash-bound producer and sole runtime, then actual native bind.

    The native bind verifies linked OrtGetApiBase identity and configures the
    existing S1 resource gates. No producer may be created before bind returns0.
    """
    def __init__(self, library_path, library_sha256, *, runtime_path, runtime_sha256):
        path=Path(library_path)
        require(path.is_absolute() and path.resolve()==path, 'canonical native library')
        require(file_sha(path)==require_sha(library_sha256), 'native producer library SHA')
        runtime=Path(runtime_path)
        require(runtime.is_absolute() and runtime.resolve()==runtime and runtime.is_file(),'canonical actual sole runtime')
        require(file_sha(runtime)==require_sha(runtime_sha256),'sole runtime SHA')
        self.runtime_bound=False
        self.runtime_binding=dict(path=str(runtime),sha256=runtime_sha256)
        self.lib=C.CDLL(str(path))
        self.lib.pw_feature_runtime_bind_v1.argtypes=[C.c_char_p,C.c_char_p,C.c_size_t]
        self.lib.pw_feature_runtime_bind_v1.restype=C.c_int
        self.lib.pw_feature_producer_create.argtypes=[C.POINTER(Config),C.c_char_p,C.c_size_t]
        self.lib.pw_feature_producer_create.restype=C.c_void_p
        self.lib.pw_feature_producer_register.argtypes=[C.c_void_p,C.POINTER(Identity)]
        self.lib.pw_feature_producer_register.restype=C.c_int
        self.lib.pw_feature_producer_request.argtypes=[C.c_void_p,C.POINTER(NativeRequest),C.c_char_p,C.c_size_t,C.c_char_p,C.c_size_t]
        self.lib.pw_feature_producer_request.restype=C.c_int
        self.lib.pw_feature_producer_release_for_whole_v1.argtypes=[C.POINTER(C.c_void_p),C.c_char_p,C.c_size_t]
        self.lib.pw_feature_producer_release_for_whole_v1.restype=C.c_int
        self.pending=None
        error=C.create_string_buffer(4096)
        rc=self.lib.pw_feature_runtime_bind_v1(str(runtime).encode(),error,len(error))
        require(rc==0,'native sole runtime binding: '+error.value.decode(errors='replace'))
        self.runtime_bound=True

    def produce(self, request, width, cache_root, *, cache_budget_bytes, before_native_request):
        """Run/reuse each actual frame serially; return original native receipts.

        Construction must complete the native sole-runtime/S1 binding. The
        caller owns remaining thermal/admission gates before this call.
        Cache admission includes previously stored objects;
        a budget cannot be inferred from this capture's N alone.
        """
        require(callable(before_native_request),'actual Critical-only thermal gate callback required')
        require(self.runtime_bound,'verified sole runtime must bind before native producer creation')
        require(self.pending is None,'unresolved native feature owner; automatic retry forbidden')
        n=validate_request(request); specs=layer_specs(width)
        root=Path(cache_root)
        require(root.is_absolute() and root.resolve()==root and not any(c.isspace() for c in str(root)), 'canonical whitespace-free cache root')
        model=request['models']['feature_low' if width==768 else 'feature_high']
        total=sum(x['bytes'] for x in specs)
        require(type(cache_budget_bytes) is int and cache_budget_bytes>=n*total, 'explicit disk cache budget')
        config=Config(str(model['path']).encode(),model['sha256'].encode(),PREPROCESS_SHA256.encode(),str(root).encode(),width,width*3//4,
            (LayerSpec*3)(*(LayerSpec(x['name'].encode(),(C.c_int64*4)(*x['shape']),x['bytes']) for x in specs)),total,cache_budget_bytes)
        require_feature_disk_space(root,n*total)
        before_native_request('feature_create',None)
        require_feature_disk_space(root,n*total)                                    
        error=C.create_string_buffer(4096)
        bank=Path(request['gray_banks'][str(width)]['path']).open('rb')
        try:
            model_file=Path(model['path']).open('rb')
        except BaseException:
            bank.close()                                        
            raise
        handle=self.lib.pw_feature_producer_create(C.byref(config),error,len(error))
        if not handle:
            bank.close(); model_file.close()                                    
            raise ValueError('native FNET create: '+error.value.decode(errors='replace'))
        owner=PendingFeatureOwner(self.lib,handle,config,request,bank,model_file)
        owner.runtime_binding=self.runtime_binding
        self.pending=owner
        _PENDING_FEATURE_OWNERS[id(owner)]=owner
        rows=[]; pixels=width*(width*3//4)
        try:
            for i,frame in enumerate(request['frames']):
                gray=bank.read(pixels*2); require(len(gray)==pixels*2,'short actual gray plane')
                photo=frame['photo']['sha256']; gray_sha=sha256(gray)
                identity=Identity(i,photo.encode(),gray_sha.encode())
                owner.current=dict(gray=gray,identity=identity,error=error)
                require(self.lib.pw_feature_producer_register(owner.handle,C.byref(identity))==0,'native frame registration')
                data=(C.c_uint16*pixels).from_buffer_copy(gray)
                call=NativeRequest(i,photo.encode(),model['sha256'].encode(),PREPROCESS_SHA256.encode(),data,pixels,0)
                receipt=C.create_string_buffer(16384)
                owner.current.update(data=data,call=call,receipt=receipt)
                before_native_request('feature_request',i)
                require_feature_disk_space(root,total)
                owner.phase='NATIVE_REQUEST_STARTED'
                rc=self.lib.pw_feature_producer_request(owner.handle,C.byref(call),receipt,len(receipt),error,len(error))
                owner.phase='NATIVE_REQUEST_RETURNED'
                require(rc==0,'native FNET request: '+error.value.decode(errors='replace'))
                row=json.loads(receipt.value)
                require(row['pack_index']==i and row['prepared_gray_sha256']==gray_sha and row['photo_sha256']==photo,'native receipt frame binding')
                rows.append(row)
                                                                          
                                                                             
                                                                  
            validate_request(request)
            owner.phase='RELEASE_STARTED'
            rc=self.lib.pw_feature_producer_release_for_whole_v1(C.byref(owner.handle),error,len(error))
            owner.phase='RELEASE_RETURNED' if rc==0 else 'RELEASE_REJECTED'
            require(rc==0 and owner.handle.value is None,'native feature whole-phase release: '+error.value.decode(errors='replace'))
        except BaseException as exc:
            owner.error=repr(exc)
                                                                             
                                                                             
            raise
        owner.phase='RELEASED_FOR_WHOLE'
        owner.bank.close(); owner.model_file.close(); owner.current=None
        del _PENDING_FEATURE_OWNERS[id(owner)]
        self.pending=None
        return rows

def index_row(row, path):
    fields=[str(row['pack_index']),row['photo_sha256']]
    total=sum(x['bytes'] for x in row['layers'])
    for layer in row['layers']:
        fields.extend([str(path),layer['sha256'],str(layer['offset']),str(layer['bytes']),str(total)])
    fields.extend([row['cache_key'],row['prepared_gray_sha256']])
    return '\t'.join(fields)

def publish_material_index(request, width, root, produced_rows):
    """Verify actual native F16 objects, then seal the exact BANKV3 transport.

    Returned MANIFEST_SHA binds full capture order/cameras/SEEDED/models, whereas
    each reusable object key binds only original photo/model/preprocess/gray.
    """
    n=validate_request(request); specs=layer_specs(width); root=Path(root)
    require(root.is_absolute() and root.resolve()==root,'canonical bank root')
    require(len(produced_rows)==n,'actual N feature receipts')
    model=_model_identities_for_request(request)['feature_low' if width==768 else 'feature_high']
    text=[]; keys=set()
    gray_bank=Path(request['gray_banks'][str(width)]['path'])
    for i,row in enumerate(produced_rows):
        with gray_bank.open('rb') as gray:
            gray.seek(i*width*(width*3//4)*2)
            require(sha256(gray.read(width*(width*3//4)*2))==row['prepared_gray_sha256'],'receipt actual original gray plane')
        require(row['pack_index']==i and row['photo_sha256']==request['frames'][i]['photo']['sha256'],'feature photo/frame sequence')
        key=material_cache_key(row['photo_sha256'],model,PREPROCESS_SHA256,row['prepared_gray_sha256'])
        require(key not in keys,'consumer requires unique per-frame material keys')
        keys.add(key)
        require(row['cache_key']==key and row['file']==f'objects/{key}/features.f16','feature cache identity/path')
        path=bound_file(dict(path=str(root/row['file']),bytes=row['bytes'],sha256=row['sha256']),sum(x['bytes'] for x in specs))
        require(not(path.stat().st_mode&0o222),'native object must be sealed readonly')
        require(len(row['layers'])==3,'three original F16 scales')
        with path.open('rb') as f:
            for actual,expected in zip(row['layers'],specs):
                require(all(actual[k]==v for k,v in expected.items()),'original feature layer name/shape/dtype/offset')
                digest=hashlib.sha256(); remaining=expected['bytes']
                while remaining:
                    data=f.read(min(65536,remaining)); require(data and len(data)%2==0,'F16 layer truncated')
                    require(all(v&0x7c00!=0x7c00 for (v,) in struct.iter_unpack('<H',data)),'nonfinite F16 feature')
                    digest.update(data); remaining-=len(data)
                require(digest.hexdigest()==actual['sha256'],'actual F16 layer SHA')
        text.append(index_row(row,path))
    validate_request(request)
    manifest=canonical(dict(schema=SCHEMA,request=request,width=width,rows=produced_rows))
    manifest_sha=sha256(manifest)
    header='\t'.join(('BANKV3',str(width),model,PREPROCESS_SHA256,manifest_sha,str(n),'objects_key_v2'))
    index=(header+'\n'+'\n'.join(text)+'\n').encode()
                                                                               
    for name,data in ((f'{manifest_sha}.manifest.json',manifest),(f'{manifest_sha}.tsv',index)):
        dest=root/name
        if dest.exists(): require(dest.read_bytes()==data,'immutable material binding conflict')
        else:
            with dest.open('xb') as f: f.write(data)
            dest.chmod(0o444)
    return dict(root=str(root),index=str(root/f'{manifest_sha}.tsv'),index_sha256=sha256(index),manifest_sha256=manifest_sha,preprocess_sha256=PREPROCESS_SHA256,width=width,count=n)
