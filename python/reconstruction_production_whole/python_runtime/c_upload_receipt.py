"""Bounded read-only native C route capture; persists raw evidence before rejection."""
import ctypes as C
import json
import os
from pathlib import Path
EXPECTED_BYTES=[2654208,7077888,14155776]*10+[21233664,1280,1280,1280,1536,442368,1769472]
def _bytes(transport):
    _require(transport in ('sealed','gpu_fixed_slots_v1'),'unknown feature transport receipt')
    return EXPECTED_BYTES[30:] if transport=='gpu_fixed_slots_v1' else EXPECTED_BYTES

C_SOURCE='actual_wgpuDeviceHasFeature_after_initialization'

def _require(value,message):
    if not value:raise ValueError(message)

def _success_retired(label,count,terminal):
    _require(label=='MVS_FINALLY' and isinstance(terminal,dict),'released C2 inputs require actual success terminal evidence')
    _require(terminal.get('schema')=='RECONSTRUCTION_C_UPLOAD_TERMINAL_V1' and terminal.get('owner_state')=='MVS_COMPLETED_ENV_DEVICE_RETAINED','C2 terminal owner state not successful')
    events=terminal.get('events');_require(isinstance(events,list),'C2 actual owner events required')
    returns=[(i,e) for i,e in enumerate(events) if e.get('stage')=='WHOLE_DAWN_ORIGINAL_MVS_RETURN']
    _require(len(returns)==1,'C2 unique actual native return required')
    index,event=returns[0]
    _require(type(event.get('rc')) is int and event['rc']==0 and event.get('scene_count')==count and event.get('error')=='' and event.get('device_held_through_CER') is True,'C2 actual native return not successful')
    _require(type(event.get('actual_pid')) is int and event['actual_pid']==os.getpid(),'C2 stale native return PID')
    progress=[(i,e) for i,e in enumerate(events) if e.get('stage')=='ORIGINAL_MVS_PROGRESS' and e.get('phase') in (2,3)]
    expected=[(pass_,phase,ref) for pass_ in (0,1) for phase,ref in [(2,r) for r in range(count)]+[(3,-1)]]
    _require([(e.get('pass_index'),e.get('phase'),e.get('ref')) for _,e in progress]==expected and all(all(type(e.get(k)) is int for k in ('count','pass_index','phase','ref')) and e['count']==count and i<index for i,e in progress),'C2 complete ordered two-pass callback evidence absent')
    manifest=terminal.get('manifest')
    _require(isinstance(manifest,dict) and all(type(manifest.get(k)) is int for k in ('schema','completed_reference_runs','memory_limits_bytes')) and manifest.get('schema')==1 and manifest.get('scope')=='MVS_ONLY_LOW_THEN_HIGH' and manifest.get('status')=='MVS_ONLY_COMPLETED' and manifest.get('completed_reference_runs')==2*count and manifest.get('error')=='' and manifest.get('memory_limits_bytes')==2000000000,'C2 committed successful MVS manifest absent')
    refs=terminal.get('refs')
    _require(isinstance(refs,list) and [(r.get('pass'),r.get('reference')) for r in refs]==[(p,r) for p in (0,1) for r in range(count)] and all(type(r.get('pass')) is int and type(r.get('reference')) is int and r.get('finite') is True for r in refs),'C2 complete finite native reference receipts absent')
    return True

def _validate_c2(caps,native,label,initial,terminal=None,transport='sealed'):
    slot_count=len(_bytes(transport))
    rewrite=native.get('rewrite')
    _require(isinstance(rewrite,dict) and rewrite.get('schema')=='RECONSTRUCTION_C2_REWRITE_ACTUAL_V1','exact C2 rewrite schema required')
    _require(type(rewrite.get('opt_in')) is int and rewrite['opt_in']==1,'actual C2 opt-in required')
    keys=('registered_generations','map_async_attempted','write_unmap_success','wait_calls','timeouts','map_failures','wait_failures','direct_bytes','unique_rewritten_generations')
    _require(all(type(rewrite.get(k)) is int and 0<=rewrite[k]<=0xffffffffffffffff for k in keys),'invalid C2 counters')
    _require(all(rewrite[k]==0 for k in ('timeouts','map_failures','wait_failures')),'C2 map/wait failure or timeout')
    expected_count=0 if label=='FEATURE_CAPS' else slot_count
    _require(rewrite['registered_generations']==expected_count,'C2 generation registry changed')
    released=[row.get('release_requested') for row in native['rows']]
    successful_terminal=isinstance(terminal,dict) and terminal.get('owner_state')=='MVS_COMPLETED_ENV_DEVICE_RETAINED'
    retired=bool(any(released)) or successful_terminal
    if retired:
        _success_retired(label,36,terminal)
        _require(len(released)==slot_count and all(type(value) is int and value==1 for value in released),'C2 success must retire all37 fixed input generations')
    repeats=total=unique=0
    for row in native['rows']:
        writes=row.get('writes');direct=row.get('direct_writes')
        _require(type(writes) is int and type(direct) is int and writes>=0 and writes==direct,'C2 write counters differ')
        _require(row.get('staging_submissions')==0,'C2 tracked input used staging')
        _require(type(row.get('release_requested')) is int and row['release_requested']==int(retired),'C2 unexpected in-run/failure generation release')
        _require(row.get('post_write_unmapped')==int(writes>0),'C2 final unmap not observed')
        _require(row.get('first_write_kind')==int(writes>0) and row.get('first_write_bytes')==(row['request_bytes'] if writes else 0),'C2 first input length/route changed')
        repeated=max(direct-1,0);repeats+=repeated;total+=repeated*row['request_bytes'];unique+=int(repeated>0)
                                                                            
                                                                         
                                                                              
                                                                                 
    _require(rewrite['map_async_attempted']==rewrite['write_unmap_success']==repeats,'C2 successful MapAsync/writeUnmap counts differ')
    _require(rewrite['wait_calls']>=repeats and (repeats>0 or rewrite['wait_calls']==0),'C2 poll wait count below attempts or without attempts')
    _require(rewrite['direct_bytes']==total and rewrite['unique_rewritten_generations']==unique,'C2 repeated byte/generation coverage differs')
    if repeats or retired:
        _require(label=='MVS_FINALLY','C2 repeats before first reference completed')
        _require(isinstance(initial,dict),'C2 same-run HIGH_INIT identity snapshot required')
        _require(initial.get('label')=='HIGH_INIT' and initial.get('count')==36 and initial.get('return_codes')=={'caps':0,'native':0},'C2 initial native snapshot invalid')
        _require('validation_error' not in initial,'C2 initial evidence was rejected')
        initial_native=initial.get('native')
        _require(isinstance(initial_native,dict),'C2 initial native object required')
        initial_rewrite=initial_native.get('rewrite')
        _require(isinstance(initial_rewrite,dict),'C2 initial rewrite metadata required; C1 fallback forbidden')
        _require(initial_rewrite.get('schema')==rewrite['schema']=='RECONSTRUCTION_C2_REWRITE_ACTUAL_V1','C2 initial rewrite schema differs')
        _require(type(initial_rewrite.get('opt_in')) is int and initial_rewrite['opt_in']==rewrite['opt_in']==1,'C2 initial opt-in differs')
        _require(type(initial_rewrite.get('registered_generations')) is int and initial_rewrite['registered_generations']==rewrite['registered_generations']==slot_count,'C2 initial generation registry differs')
        _require(initial.get('feature_transport','sealed')==transport,'C2 initial feature transport differs')
        validate(initial['caps'],initial_native,36,'HIGH_INIT',transport=transport)
        _require(initial['caps']['device_handle']==caps['device_handle'],'C2 initialized device changed')
        identity=('id','buffer','device','expected_bytes','request_bytes','descriptor_bytes','allocation_status')
        _require(all(tuple(row[k] for k in identity)==tuple(old[k] for k in identity) for row,old in zip(native['rows'],initial['native']['rows'])),'C2 fixed input identity/generation changed')
    return dict(schema=rewrite['schema'],successful_rewrites=repeats,repeated_direct_bytes=total,
        unique_rewritten_generations=unique,generation_evidence='native_token_enforcement_plus_unchanged_registration_and_HIGH_INIT_identity',registered_slot_count=slot_count,feature_transport=transport,
        individual_generation_tokens_exported=False,terminal_release_verified=retired,poll_wait_calls=rewrite['wait_calls'],
        poll_count_upper_bound='NOT_EXPORTED; native 30s deadline is wall-clock based',physical_budget_verdict='NOT_ASSESSED_BY_UPLOAD_VALIDATOR')

def validate(caps,native,count,label,initial=None,terminal=None,transport='sealed'):
    expected_bytes=_bytes(transport);slot_count=len(expected_bytes)
    _require(count==36,'C profile is whole36 only')
    _require(isinstance(caps,dict)and caps.get('source')=='actual_enabled_device_features_after_initialization','actual initialized-device caps required')
    _require(caps.get('C_upload_capability_source')==C_SOURCE,'actual DeviceHasFeature C field missing')
    for key in ('BufferMapExtendedUsages','C_upload_actual_integrated','C_upload_eligible_first_write'):
        _require(type(caps.get(key))is int and caps[key]in (0,1),'invalid C capability '+key)
    eligible=bool(caps['BufferMapExtendedUsages'])and caps.get('adapter_type')==2
    _require(caps['C_upload_actual_integrated']==int(caps.get('adapter_type')==2)and caps['C_upload_eligible_first_write']==int(eligible),'contradictory actual capability')
    _require(isinstance(native,dict)and native.get('schema')=='RECONSTRUCTION_C_UPLOAD_ACTUAL_V1'and native.get('source')=='actual_scoped_Create_and_Upload_buffer_properties','actual native allocation receipt required')
    _require(native.get('lost')==0,'native C receipt overflow/error')
    rows=native.get('rows');_require(isinstance(rows,list),'native C rows')
    if label=='FEATURE_CAPS':
        _require(native.get('count')==native.get('attempted')==len(rows)==0,'C input scope unexpectedly used before HIGH')
        if 'rewrite' in native:_validate_c2(caps,native,label,initial,terminal=terminal,transport=transport)
        return dict(status='CAPABILITY_ONLY',eligible=eligible,observed_slots=0)
    _require(native.get('count')==native.get('attempted')==len(rows)==slot_count,'actual HIGH37 allocations incomplete')
    device=int(caps['device_handle'],0);_require(device>0 and len({r.get('buffer')for r in rows})==slot_count,'actual unique fixed-input buffers')
    _require(sum(r.get('request_bytes',0)for r in rows)==sum(expected_bytes),'actual fixed-input total bytes')
    for i,(r,expected)in enumerate(zip(rows,expected_bytes)):
        _require(r.get('id')==i+1 and r.get('expected_bytes')==r.get('request_bytes')==r.get('descriptor_bytes')==expected,'actual slot bytes/order '+str(i))
        _require(r.get('device')==device and r.get('buffer',0)>0,'actual device/buffer '+str(i))
        _require(r.get('allocation_status')==1 and r.get('standard_usage')==r.get('size_match')==1,'actual allocation completion '+str(i))
        _require(r.get('actual_device_feature')==caps['BufferMapExtendedUsages']and r.get('adapter_type')==caps['adapter_type'],'actual allocation feature identity '+str(i))
        _require(r.get('selected_map')==r.get('has_mapwrite')==r.get('creation_mapped')==int(eligible),'actual mapped/fallback route '+str(i))
        if label=='HIGH_INIT':_require(r.get('writes')==0,'initialization already rewrote fixed input')
        if label=='HIGH_REF0':
            _require(r.get('writes')==1 and r.get('first_write_kind')==(1 if eligible else 2)and r.get('first_write_bytes')==expected,'actual first upload route '+str(i))
            _require(r.get('direct_writes')==int(eligible)and r.get('staging_submissions')==int(not eligible)and r.get('post_write_unmapped')==1,'actual unmap/staging '+str(i))
        if 'rewrite' not in native:_require(r.get('direct_writes',0)<=1,'input was remapped/recreated on later ref')
    c2=None
    if 'rewrite' in native:
        _require(eligible,'C2 requires actual integrated mapped device')
        c2=_validate_c2(caps,native,label,initial,terminal=terminal,transport=transport)
    return dict(status=('MAPPED_FIRST_UPLOAD_VERIFIED'if eligible else'STAGING_FIRST_UPLOAD_VERIFIED')if label=='HIGH_REF0'else('ACTUAL_MAPPED_ALLOCATIONS'if eligible else'ACTUAL_FALLBACK_ALLOCATIONS'),eligible=eligible,observed_slots=slot_count,observed_input_bytes=sum(expected_bytes),first_upload_checked=label=='HIGH_REF0',c2=c2)

def capture(rt,root,label,count,strict=True,terminal=None,transport='sealed'):
    _require(label in ('FEATURE_CAPS','HIGH_INIT','HIGH_REF0','MVS_FINALLY'),'bounded receipt label')
    value=dict(schema='RECONSTRUCTION_C_UPLOAD_PERSISTED_V1',label=label,count=count,feature_transport=transport,return_codes={},raw={})
    try:
        for name,key in (('reconstruction_s1_device_caps_json','caps'),('pw_upload_receipt_json_v1','native')):
            query=getattr(rt,name);query.argtypes=[C.c_char_p,C.c_size_t];query.restype=C.c_int
            buf=C.create_string_buffer(65536);rc=query(buf,len(buf));value['return_codes'][key]=rc;value['raw'][key]=buf.value.decode(errors='replace')
            _require(rc==0,'actual query failed '+name+' rc='+str(rc));value[key]=json.loads(buf.value)
        initial=None
        if label=='MVS_FINALLY' and 'rewrite' in value['native']:
            initial=json.loads((Path(root)/'C_UPLOAD_HIGH_INIT.json').read_text())
        if terminal is not None:
            terminal=dict(terminal)
            if terminal.get('owner_state')=='MVS_COMPLETED_ENV_DEVICE_RETAINED':
                terminal['manifest']=json.loads((Path(root)/'mvs/manifest.json').read_text())
                terminal['refs']=[json.loads(line) for line in (Path(root)/'mvs/refs.jsonl').read_text().splitlines()]
            value['terminal']=terminal
        value['assessment']=validate(value['caps'],value['native'],count,label,initial=initial,terminal=terminal,transport=transport)
    except BaseException as error:value['validation_error']=repr(error)
    path=Path(root)/('C_UPLOAD_'+label+'.json')
    try:
        with path.open('x')as stream:json.dump(value,stream,allow_nan=False);stream.write('\n');stream.flush();os.fsync(stream.fileno())
    except BaseException as error:
        if strict:raise
        return dict(persistence_error=repr(error),label=label,validation_error=value.get('validation_error'))
    if strict and 'validation_error'in value:raise ValueError('persisted C evidence rejected: '+value['validation_error'])
    return value
