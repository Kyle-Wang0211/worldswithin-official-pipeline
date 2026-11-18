"""Exact immutable Fusion bank transport; no numeric algorithm substitutions."""
from pathlib import Path
import hashlib,json,shutil,time

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda:stream.read(1<<20),b''):h.update(chunk)
    return h.hexdigest()

def exact_segments(row,shape):
    n,h,w=shape;unit=h*w*4
    assert len(shape)==3 and all(type(x) is int and x>0 for x in shape)
    assert 'frames' not in row,'Sparse reference-only input is forbidden for full97'
    if 'chunks' not in row:
        assert row['bytes']==n*unit
        return [(0,n,row)]
    assert 'device_relative' not in row and 'name' not in row,'ambiguous full/chunk row'
    chunks=row['chunks'];assert isinstance(chunks,list) and chunks
    result=[];expected=0
    for item in chunks:
        first=item['first_frame'];count=item['frame_count']
        assert type(first) is int and type(count) is int and count>0
        assert first==expected,'duplicate, gap, or reordered chunk'
        assert item['bytes']==count*unit and first+count<=n
        assert item['source_byte_offset']==first*unit
        assert item['source_bank_sha256']==row['sha256']
        result.append((first,count,item));expected+=count
    assert expected==n,'incomplete bank'
    if 'bytes' in row:assert row['bytes']==n*unit
    return result

def mapped_fusion_loads(np,rows,checked,base,*,shape=(97,1152,1536),refs=None):
    n,h,w=shape
    assert refs==list(range(n)),'reference order must remain original ascending order'
    assert len(rows)==4 and [r['plane'] for r in rows]==['depth','conf0','conf1','conf2']
                                                                  
    segments=[exact_segments(row,shape) for row in rows]
    owners=[];loads={}
    for row,pieces in zip(rows,segments):
        full=hashlib.sha256()
        for first,count,item in pieces:
            path=checked(item)
                                                                                     
            assert Path(path).stat().st_size==count*h*w*4
            bank=np.memmap(path,dtype='<f4',mode='r',shape=(count,h,w));owners.append(bank)
            for local in range(count):
                frame=first+local;key=f"{base}/{row['plane']}/{frame:04d}.npy"
                assert key not in loads
                plane=np.asarray(bank[local]);loads[key]=plane
                if 'chunks' in row:full.update(memoryview(plane).cast('B'))
                                                                              
        if 'chunks' in row:assert full.hexdigest()==row['sha256'],'ordered full-bank SHA mismatch'
    return loads,owners

def resolve_input(row,source,container):
    if 'device_relative' in row:
        relative=Path(row['device_relative'])
        assert not relative.is_absolute() and relative.parts and relative.parts[0]=='Documents' and '..' not in relative.parts
        path=(container/relative).resolve();assert path.is_relative_to(container)
    else:
        relative=Path(row['name']);assert not relative.is_absolute() and '..' not in relative.parts
        path=(source/relative).resolve();assert path.is_relative_to(source.resolve())
    return path

def verify_only(control):
    """Read existing task-owned input identities. Never numerical PASS."""
    source=Path(control['normal_directory']);output=Path(control['output_directory'])/'fusion'
    output.mkdir(parents=True,exist_ok=False)
    container=Path(control['output_directory']).parents[3].resolve()
    report={'status':'PREPARING','mode':'verify_only','numerical_execution':False,'rows':[],'started_ns':time.time_ns()}
    try:
        manifest=source/'INPUTS.json';assert sha(manifest)==control['fusion_manifest_sha256']
        binding=json.loads(manifest.read_text());assert binding['shape']==[97,1152,1536] and binding['refs']==list(range(97))
        rows=control['fusion_verify_rows'];assert isinstance(rows,list) and rows
        for row in rows:
            path=resolve_input(row,source,container)
            observation={'path':str(path),'expected_bytes':row['bytes'],'expected_sha256':row['sha256']}
            if path.is_file():
                observation.update(bytes=path.stat().st_size,sha256=sha(path))
                observation['valid']=observation['bytes']==row['bytes'] and observation['sha256']==row['sha256']
            else:observation.update(valid=False,missing=True)
            report['rows'].append(observation)
        usage=shutil.disk_usage(source)
        report['disk_usage']={'total_bytes':usage.total,'used_bytes':usage.used,'free_bytes':usage.free}
        report['status']='PREPARED' if all(r['valid'] for r in report['rows']) else 'INPUTS_INCOMPLETE'
    except BaseException as error:
        report.update(status='FAILED',error=repr(error));raise
    finally:
        report['ended_ns']=time.time_ns()
        (output/'RESULT.json').write_text(json.dumps(report,indent=2)+'\n')
    return report

class StageLimitError(RuntimeError):pass

def input_identity(binding,package_binding,runtime,original_sources):
    """Semantic immutable identity independent of full-vs-chunk transport paths."""
    def item(row):return {'bytes':row['bytes'],'sha256':row['sha256']}
    return {'shape':binding['shape'],'refs':binding['refs'],'banks':[dict(plane=r['plane'],**item(r)) for r in binding['banks']],
            'cameras':item(binding['cameras']),'frame_ids':item(binding['frame_ids']),'jpegs':[item(r) for r in binding['jpegs']],
            'numeric_config':{'geo_pixel':.125,'mask_thres':[2],'sat_tol':1e-5,'min_tri_angle':3.0},
            'runtime':runtime,'original_sources':original_sources,'package_binding':package_binding}

def validate_oracle(oracle,identity,n=97):
    assert oracle['status']=='PASS' and oracle['arm']=='reference_pipeline'
    assert oracle['refs']==list(range(n)) and oracle['source_count']==n
    assert oracle['input_identity']==identity,'oracle immutable input/runtime/source identity mismatch'
    validate_completed(oracle,n)

def validate_completed(report,n=97):
    frames=report['frames'];assert len(frames)==n
    assert [row['frame'] for row in frames]==list(range(n)),'missing/duplicate/reordered completed reference'
    assert set(report['counts'])==set(map(str,range(n)))
    assert len(report['files'])==n*4,'expected four retained files per reference'
    merged={}
    for row in frames:
        assert len(row['files'])==4
        assert all(name.startswith(f"{row['frame']:08d}.") for name in row['files'])
        assert not set(merged).intersection(row['files'])
        merged.update(row['files'])
    assert merged==report['files']

def stage_bounds(control,retained,start,arm):
    maximum=3600 if arm=='reference_pipeline' else 1200
    limit=control['fusion_wall_limit_seconds']
    assert type(limit) in (int,float) and 0<limit<=maximum,'explicit bounded wall limit required'
    elapsed=time.monotonic()-start
    usage=shutil.disk_usage(retained)
    retained_bytes=sum(p.stat().st_size for p in retained.rglob('*') if p.is_file())
    snapshot={'stage_elapsed_seconds':elapsed,'wall_limit_seconds':limit,'free_bytes':usage.free,'retained_bytes':retained_bytes}
    if elapsed>limit or usage.free<1073741824 or retained_bytes>400000000:
        raise StageLimitError(json.dumps(snapshot,sort_keys=True))
    return snapshot
