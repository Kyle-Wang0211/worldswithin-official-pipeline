"""Lightweight per-reference evidence; changes neither arithmetic nor scheduling."""
class NativeRowCoverage:
    KEYS=('geometry_native','geometry_fallback','sees_native','sees_fallback')
    def __init__(self,state,expected,variant,source_sha,library_sha,height,width):
        if expected not in ([0,48,96],list(range(97))):raise ValueError('unsupported reference scope')
        if variant not in ('baseline','rows') or height<=1:raise ValueError('unsupported candidate/dimensions')
        self.state=state;self.expected=list(expected);self.attempts=0;self.before=None
        self.data={'candidate_source_sha256':source_sha,'signed_or_loaded_library_sha256':library_sha,
                   'variant':variant,'height':height,'width':width,'max_geometry_threads':2 if variant=='rows' else 1,
                   'frames':[],'status':'RUNNING','eligibility_evidence':'Every counted call must return through selected native wrapper with no fallback; that wrapper validates ordinary finite input domain. Independent attempt count must equal successful native calls. No extra array scan added.'}
    def counted(self,original):
        def call(*args,**kwargs):
            self.attempts+=1
            return original(*args,**kwargs)
        return call
    def begin(self,frame):
        i=len(self.data['frames'])
        if self.before is not None or i>=len(self.expected) or self.expected[i]!=frame:raise ValueError('frame order/duplicate/open context')
        self.before=(frame,self.attempts,dict(self.state.stats))
    def finish(self,frame,completed=True):
        if self.before is None or self.before[0]!=frame:raise ValueError('missing matching frame start')
        _,calls,before=self.before;self.before=None
        row={'frame':frame,'geometry_attempts':self.attempts-calls,'completed':completed}
        for key in self.KEYS:
            now=self.state.stats[key];old=before[key]
            if type(now) is not int or type(old) is not int or now<old:raise ValueError('invalid/descending native counters')
            row[key+'_delta']=now-old
        row['eligible_geometry_calls']=row['geometry_native_delta']
        row['valid']=(completed and row['geometry_attempts']>0
                      and row['geometry_native_delta']==row['geometry_attempts']
                      and row['geometry_fallback_delta']==0
                      and row['sees_native_delta']>0 and row['sees_fallback_delta']==0)
        self.data['frames'].append(row)
        if not row['valid']:self.data['status']='INVALID_NATIVE_ROW_COVERAGE'
        return row
    @staticmethod
    def require_frame(row):
        if not row['valid']:raise ValueError('INVALID_NATIVE_ROW_COVERAGE '+repr(row))
    def require_complete(self,output_frames,counts,timing):
        if self.before is not None:raise ValueError('unfinished frame')
        if [r['frame'] for r in self.data['frames']]!=self.expected:raise ValueError('missing/extra coverage frame')
        if [r['frame'] for r in output_frames]!=self.expected:raise ValueError('missing/extra output frame')
        if list(map(int,counts))!=self.expected:raise ValueError('missing/extra count frame')
        if [r['frame'] for r in timing]!=self.expected:raise ValueError('missing/extra timing frame')
        for row in self.data['frames']:self.require_frame(row)
        for key in self.KEYS:
            if sum(r[key+'_delta'] for r in self.data['frames'])!=self.state.stats[key]:raise ValueError('aggregate native counter mismatch')
        if sum(r['geometry_attempts'] for r in self.data['frames'])!=self.attempts:raise ValueError('aggregate attempt mismatch')
        self.data['status']='PASS_REQUIRED_NATIVE_COVERAGE'
