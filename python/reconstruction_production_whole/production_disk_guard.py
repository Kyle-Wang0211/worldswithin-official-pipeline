"""Observational filesystem budgets; generation has no free-space threshold.

Legacy cold fallback assumes all cold outputs coexist; existing input/model bytes are already charged to
statvfs free bytes and must not be subtracted again. Suffix allowance is an
explicit conservative policy allowance, not a proven upper bound for arbitrary
mesh/texture outputs. This module never deletes files or changes GPU ownership; opt-in stage plans
require actual last-consumer retirement in the caller.
"""
import errno
import json
import os
from pathlib import Path

RESERVE_BYTES=1024**3
FAILURE_SLOT_BYTES=16384

def cold_budget(count, *, manifest_bytes):
    if type(count)is not int or count<=0:raise ValueError('actual frame count 10..97 required')
    if type(manifest_bytes)is not int or manifest_bytes<0:raise ValueError('validated manifest bytes required')
    low=768*576;high=1536*1152
    components={
        'high_gray_and_rgb':count*high*5,
        'feature_f16':count*29859840,
        'mvs_f32':count*(low+high)*16,
        'cer_f32':count*high*16,
        'retained_depth_rgb_camera':count*(high*5+200),
                                                                             
                                                                            
        'suffix_floor':1024**3,
        'journals_metadata':16*1024**2+count*65536,
        'filesystem_reserve':RESERVE_BYTES,
    }
    return dict(schema='RECONSTRUCTION_COLD_DISK_ADMISSION_V1',frames=count,
                manifest_bytes=manifest_bytes,components=components,
                required_free_bytes=sum(components.values()),
                suffix_bound='UNKNOWN_STAGE_ADMISSION_REQUIRED',
                scope='exact cold prefix outputs plus suffix floor; not output upper bound')

def prefix_peak_budget(count, *, manifest_bytes, feature_retirement_enabled=False):
    """Opt-in only after real per-ref last-use unlink is integrated and tested.

    Until then reuse cold_budget unchanged. Once enabled, features are all gone
    before CER; worst case every feature survives to the end of its MVS pass.
    No credit is taken for earlier per-ref retirements or prefix-end deletion.
    Unknown suffix output is NOT included: strict JIT suffix gates are required.
    """
    if type(feature_retirement_enabled) is not bool:
        raise ValueError('explicit feature retirement capability required')
    plan = cold_budget(count, manifest_bytes=manifest_bytes)
    if not feature_retirement_enabled:
        return plan
    c = plan['components']
    phases = dict(
        mvs=c['high_gray_and_rgb']+c['feature_f16']+c['mvs_f32'],
        fusion=c['high_gray_and_rgb']+c['mvs_f32']+c['cer_f32']+c['retained_depth_rgb_camera'])
    peak = max(phases.values())
    return dict(schema='RECONSTRUCTION_STAGED_PREFIX_DISK_V1', frames=count,
        manifest_bytes=manifest_bytes, already_occupied_input_bytes=manifest_bytes,
        phase_live_bytes=phases, peak_live_generated_bytes=peak,
        required_free_bytes=peak+c['journals_metadata']+RESERVE_BYTES+FAILURE_SLOT_BYTES,
        failure_slot_bytes=FAILURE_SLOT_BYTES,
        suffix_bound='UNKNOWN_STAGE_ADMISSION_REQUIRED',
        activation='REQUIRES_REAL_FEATURE_RETIREMENT_AND_SUFFIX_STAGE_GATES')


def stage_budget(count, stage, *, existing_live_bytes, remaining_write_bytes=None,
                 failure_slot_allocated=False):
    """Only remaining NEW writes are charged against freshly queried free space.

    existing_live_bytes is measured/reporting-only: those bytes already occupy
    the filesystem. Unknown mesh/codec writes need a positive caller-established
    byte bound; neither input length nor a reserve is an output-size bound.
    """
    if type(existing_live_bytes) is not int or existing_live_bytes < 0:
        raise ValueError('measured nonnegative existing live bytes required')
    if type(failure_slot_allocated) is not bool:
        raise ValueError('actual failure slot allocation state required')
    if not isinstance(stage,str) or not stage:
        raise ValueError('explicit stage required')
    components=cold_budget(count,manifest_bytes=0)['components']
    known=('high_gray_and_rgb','feature_f16','mvs_f32','cer_f32','retained_depth_rgb_camera')
    if stage in known:
        if remaining_write_bytes is not None:
            raise ValueError('known stage uses fixed complete remaining-write bound')
        remaining_write_bytes=components[stage]
    if type(remaining_write_bytes) is not int or remaining_write_bytes <= 0:
        raise ValueError('unknown suffix requires positive measured remaining-write bound')
    failure=0 if failure_slot_allocated else FAILURE_SLOT_BYTES
    return dict(schema='RECONSTRUCTION_STAGE_DISK_V1',stage=stage,
        existing_live_bytes=existing_live_bytes,remaining_write_bytes=remaining_write_bytes,
        failure_slot_bytes=failure,filesystem_reserve=RESERVE_BYTES,
        required_free_bytes=remaining_write_bytes+RESERVE_BYTES+failure)


def require_stage_space(path,count,stage,*,retired_bytes=0,retirement_evidence=None,**kwargs):
    if type(retired_bytes)is not int or retired_bytes<0:
        raise ValueError('actual nonnegative retired bytes required')
    if retired_bytes and (not isinstance(retirement_evidence,str) or not retirement_evidence):
        raise ValueError('positive retired bytes require actual receipt source')
    plan=stage_budget(count,stage,**kwargs)
                                                                      
    plan.update(retired_bytes=retired_bytes,retirement_evidence=retirement_evidence,
                remaining_stage=stage,required_additional_bytes=plan['required_free_bytes'])
    plan['available_free_bytes']=require_space(path,plan['required_free_bytes'],stage)
    plan['admission_enforced']=False
    return plan

def available_bytes(path):
    fs=os.statvfs(path)
    if (type(fs.f_bavail)is not int or type(fs.f_frsize)is not int or
        fs.f_bavail<0 or fs.f_frsize<=0 or fs.f_frsize>2**63-1 or
        fs.f_bavail>(2**63-1)//fs.f_frsize):
        raise ValueError('invalid or overflowing filesystem availability')
    return fs.f_bavail*fs.f_frsize

def require_space(path,required,phase):
    if type(required)is not int or required<=0:raise ValueError('positive byte requirement')
    free=available_bytes(path)
    if type(free)is not int or free<0:raise ValueError('invalid filesystem free bytes')
                                                                             
                                                                                  
    return free

def reserve_failure_slot(path):
    path=Path(path)
    content=b'{"schema":"RECONSTRUCTION_FAILURE_RESERVE_V1","status":"RESERVED_NOT_A_RESULT"}'
    with path.open('xb')as stream:
        stream.write(content+b' '*(FAILURE_SLOT_BYTES-len(content)))
        stream.flush();os.fsync(stream.fileno())
    return path

def write_result(path,value):
    with Path(path).open('x')as stream:
        json.dump(value,stream,allow_nan=False);stream.write('\n')

def persist_failure(path,error,*,reserve=None,owner_state=None):
    evidence=dict(schema='RECONSTRUCTION_PRODUCTION_WHOLE_FAILURE_V1',status=1,
        error=repr(error)[:4096],owner_state=owner_state,pending_retained=True)
    try:write_result(path,evidence)
    except BaseException as persistence_error:
        evidence['result_persistence_error']=repr(persistence_error)[:1024]
        if reserve is not None:
            try:
                payload=json.dumps(evidence,ensure_ascii=True).encode()
                if len(payload)>FAILURE_SLOT_BYTES:raise ValueError('failure evidence slot capacity')
                                                                             
                with Path(reserve).open('r+b')as stream:
                    stream.write(payload+b' '*(FAILURE_SLOT_BYTES-len(payload)))
                    stream.flush();os.fsync(stream.fileno())
            except BaseException as reserve_error:evidence['reserve_persistence_error']=repr(reserve_error)[:1024]
    return evidence
