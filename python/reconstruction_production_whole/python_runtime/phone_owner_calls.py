"""Thin callable table, not an autorun or new phone test launcher.
The unique phoneowner supplies content-verified loaded unified RT+CO handles,
original ABI1 Request, accepted Config/current-CER Input and real M0 boundary.
"""
import ctypes as C
import os
from pathlib import Path

class RetainedWholeOwner:
    def __init__(self,rt,co,config,Request,Input,boundary,record,owners,*,production=False):
        if type(production) is not bool:raise ValueError('verified production scope boolean required')
        self.production=production
        if C.cast(rt.OrtGetApiBase,C.c_void_p).value!=C.cast(co.OrtGetApiBase,C.c_void_p).value:raise RuntimeError('CO and Fusion do not resolve one actualORT')
        self.rt=rt;self.co=co;self.config=config;self.Request=Request;self.Input=Input;self.boundary=boundary;self.record=record
        self.owners=[rt,co,config,*owners];self.owner=C.c_void_p();self.state='UNCREATED';self.scene_count=None
        self.RunOriginal=C.CFUNCTYPE(C.c_int,C.POINTER(Request),C.c_void_p,C.c_size_t)
        self.native_mvs=self.RunOriginal(C.cast(co.reconstruction_continuous_mvs_run,C.c_void_p).value)
        self.owners.append(self.native_mvs)
        rt.reconstruction_whole_dawn_create_v19.argtypes=[C.POINTER(type(config)),C.POINTER(C.c_void_p)]
        rt.reconstruction_whole_dawn_mvs_v19.argtypes=[C.c_void_p,self.RunOriginal,C.POINTER(Request),C.c_void_p,C.c_size_t]
        rt.reconstruction_whole_dawn_fusion_v19.argtypes=[C.c_void_p,C.POINTER(Input)]
        rt.reconstruction_whole_dawn_destroy_v19.argtypes=[C.c_void_p]
        for name in ('create','mvs','fusion','destroy'):getattr(rt,'reconstruction_whole_dawn_'+name+'_v19').restype=C.c_int
    def create(self):
        if self.state!='UNCREATED':raise RuntimeError('one wholeowner only')
        self.boundary('WHOLE_DAWN_BEFORE_CREATE')
        rc=self.rt.reconstruction_whole_dawn_create_v19(C.byref(self.config),C.byref(self.owner))
        self.state='CREATED'if rc==0 else'FAILED_RETAIN_PENDING_OWNER'
        self.record({'stage':'WHOLE_DAWN_CREATE','rc':rc,'actual_pid':os.getpid(),'owner_nonnull':bool(self.owner)})
        if rc:raise RuntimeError('wholeownerCreate refused '+str(rc))
    def mvs(self,request,input_owners):
        if self.state!='CREATED'or request.count<=0 or request.struct_bytes!=C.sizeof(self.Request)or C.sizeof(self.Request)!=128:raise RuntimeError('original128byte actual positive production count or legacy10..97 request required')
        self.scene_count=request.count;self.owners.extend([request,*input_owners]);error=C.create_string_buffer(4096)
        self.boundary('WHOLE_DAWN_BEFORE_ORIGINAL_MVS')
        rc=self.rt.reconstruction_whole_dawn_mvs_v19(self.owner,self.native_mvs,C.byref(request),error,len(error))
        self.state='MVS_COMPLETED_ENV_DEVICE_RETAINED'if rc==0 else'FAILED_RETAIN_PENDING_OWNER'
        self.record({'stage':'WHOLE_DAWN_ORIGINAL_MVS_RETURN','rc':rc,'actual_pid':os.getpid(),'scene_count':self.scene_count,'error':error.value.decode(errors='replace'),'device_held_through_CER':rc==0})
        if rc:raise RuntimeError('originalMVS incomplete; owner/ticket retained '+error.value.decode(errors='replace'))
        self.boundary('WHOLE_DAWN_ORIGINAL_MVS_RETURNED')
    def fusion(self,input_,input_owners):
        if self.state!='MVS_COMPLETED_ENV_DEVICE_RETAINED'or input_.frames!=self.scene_count or input_.refs_count!=self.scene_count or input_.allCER!=1:raise RuntimeError('only current same-scene complete CER may enterFusion')
        self.owners.extend([input_,*input_owners]);self.boundary('WHOLE_DAWN_BEFORE_ADOPTED_GPU_FUSION')
        rc=self.rt.reconstruction_whole_dawn_fusion_v19(self.owner,C.byref(input_))
        self.state='FUSION_RETURNED_PENDING_WHOLE_DESTROY'if rc==0 else'FAILED_RETAIN_PENDING_OWNER'
        self.record({'stage':'WHOLE_DAWN_GPU_FUSION_RETURN','rc':rc,'actual_pid':os.getpid(),'scene_count':self.scene_count,'device_identity_checked_natively':rc==0,'physical_peak_B':None})
        if rc:raise RuntimeError('GPUFusion failed/pending; owner preserved '+str(rc))
        self.boundary('WHOLE_DAWN_GPU_FUSION_RETURNED')
    def destroy(self):
        if not self.owner:raise RuntimeError('no owner')
        rc=self.rt.reconstruction_whole_dawn_destroy_v19(self.owner)
        if rc:
            self.state='FAILED_RETAIN_PENDING_OWNER'
            self.record({'stage':'WHOLE_DAWN_DESTROY','rc':rc,'actual_pid':os.getpid(),'physical_peak_B':None})
            raise RuntimeError('live native work/lease refusesDestroy '+str(rc))
                                                                            
                                                                             
        self.owner=None;self.state='DESTROYED_SUFFIX_READY'
                                                                                 
                                                                                 
                                                                            
        self.owners=[self.rt,self.co];self.config=None
        self.record({'stage':'WHOLE_DAWN_DESTROY','rc':rc,'actual_pid':os.getpid(),'physical_peak_B':None})
        self.boundary('WHOLE_DAWN_DESTROY0_BEFORE_ORIGINAL_SUFFIX')
                                                                               
                                                                                
