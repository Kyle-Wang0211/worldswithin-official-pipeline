"""Read this invocation's actual external M0 observations, never historical peak."""
import json,os

def measured_peak(path,*,expected_run_id,enter_label,return_label,enter_window_ns,return_window_ns,physical_limit_bytes=2000000000):
    if type(physical_limit_bytes)is not int or physical_limit_bytes not in (2000000000,(1<<63)-1):raise ValueError('invalid bound memory cap')
    if not isinstance(expected_run_id,str)or not expected_run_id:raise ValueError('explicit actual M0 run_id required')
    if not enter_label or not return_label or enter_label==return_label:raise ValueError('distinct invocation boundaries required')
    windows=(*enter_window_ns,*return_window_ns)
    if len(windows)!=4 or any(type(v)is not int or v<=0 for v in windows)or not windows[0]<=windows[1]<=windows[2]<=windows[3]:raise ValueError('actual invocation time windows required')
    peak=0;entered=False;returned=False;last=0;pid=os.getpid();last_sequence=None
    with path.open('rb')as stream:
        for raw in stream:
            if not raw.endswith(b'\n'):break
            row=json.loads(raw);stamp=row['mono_ns']
            if type(stamp)is not int:raise ValueError('M0 timestamp missing')
            if stamp<windows[0]or stamp>windows[3]:continue
            if row['schema']!='reconstruction.m0.memory/1'or row['pid']!=pid or row['run_id']!=expected_run_id:raise ValueError('M0 actual App/run identity')
            if row['event']=='boundary'and row['stage']==enter_label:
                if entered or not windows[0]<=stamp<=windows[1]:raise ValueError('ambiguous/current ENTER timestamp')
                entered=True
            if not entered:continue
            if stamp<last or (last_sequence is not None and row['seq']!=last_sequence+1):raise ValueError('M0 observation order/gap')
            if row['cap_B']!=physical_limit_bytes or row['receipt_rc']!=0 or row['cap_exceeded_latched']:raise ValueError('M0 failure/cap')
            last=stamp;last_sequence=row['seq'];value=row['phys_footprint_B']
            if type(value)is not int or value<=0:raise ValueError('missing measured physical footprint')
            peak=max(peak,value)
            if row['event']=='boundary'and row['stage']==return_label:
                if not windows[2]<=stamp<=windows[3]:raise ValueError('current RETURN timestamp')
                returned=True;break
    if not entered or not returned or not 0<peak<=physical_limit_bytes:raise ValueError('continuous current prefix observation incomplete')
    return peak
