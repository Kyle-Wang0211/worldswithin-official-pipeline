#include "fusion_post_owner.h"
#include "core/providers/webgpu/webgpu_context.h"
#include "onnxruntime_c_api.h"
#include <new>
using onnxruntime::webgpu::WebGpuContextFactory;
                                                                            
                                                                                 
bool WebGpuContextFactory::HasAnyContextForPostOwner(){std::lock_guard<std::mutex>lock(mutex_);return contexts_&&!contexts_->empty();}
namespace{struct NativePostOwner{OrtEnv*env=nullptr;bool context=false;};}
int pw_post_backend_create_official(void**out)noexcept{if(!out)return 1;*out=nullptr;auto*p=new(std::nothrow)NativePostOwner;if(!p)return 3;
 try{
                                                                           
                                                                            
  if(WebGpuContextFactory::HasAnyContextForPostOwner()){delete p;return 7;}
  const auto*a=OrtGetApiBase()->GetApi(ORT_API_VERSION);auto*s=a->CreateEnv(ORT_LOGGING_LEVEL_WARNING,"reconstruction-fusion-post-owner",&p->env);if(s){a->ReleaseStatus(s);delete p;return 3;}
  auto&ctx=WebGpuContextFactory::DefaultContext();p->context=true;if(!ctx.HasDevice()){WebGpuContextFactory::ReleaseContext(0);a->ReleaseEnv(p->env);delete p;return 3;}
  *out=p;return 0;
 }catch(...){if(p->context){try{WebGpuContextFactory::ReleaseContext(0);}catch(...){}}if(p->env)OrtGetApiBase()->GetApi(ORT_API_VERSION)->ReleaseEnv(p->env);delete p;return 3;}
}
int pw_post_backend_release_official(void*opaque)noexcept{auto*p=static_cast<NativePostOwner*>(opaque);if(!p)return 1;
 try{if(p->context){WebGpuContextFactory::ReleaseContext(0);p->context=false;}if(p->env){OrtGetApiBase()->GetApi(ORT_API_VERSION)->ReleaseEnv(p->env);p->env=nullptr;}delete p;return 0;}catch(...){return 3;}
}
