#include "fusion_existing_owner.h"
#include "core/providers/webgpu/webgpu_context.h"
#include <dawn/native/DawnNative.h>
#include "dawn/native/Device.h"
#include "dawn/native/Instance.h"
using onnxruntime::webgpu::WebGpuContextFactory;
int pw_owner_backend_open_existing(int32_t id,PwOwnerSnapshot*out)noexcept{
 bool factory_ref=false;
 try{
                                                                             
                                                                                 
  auto&original=WebGpuContextFactory::GetContext(id);if(id!=0||!original.HasDevice())return 2;
  auto device=original.Device().Get();auto instance=dawn::native::ToAPI(dawn::native::FromAPI(device)->GetInstance());if(!device||!instance)return 2;
  onnxruntime::webgpu::WebGpuContextConfig c{};c.context_id=id;
  auto&same=WebGpuContextFactory::CreateContext(c);factory_ref=true;
  if(same.Device().Get()!=device){WebGpuContextFactory::ReleaseContext(id);return 2;}
  const auto&p=dawn::native::GetProcs();p.deviceAddRef(device);p.instanceAddRef(instance);
  *out={instance,device,&p,sizeof(p)};return 0;
 }catch(...){if(factory_ref){try{WebGpuContextFactory::ReleaseContext(id);}catch(...){}}return 2;}
}
void pw_owner_backend_close_existing(int32_t id,PwOwnerSnapshot*s)noexcept{
 const auto&p=dawn::native::GetProcs();if(s->device)p.deviceRelease(static_cast<WGPUDevice>(s->device));if(s->instance)p.instanceRelease(static_cast<WGPUInstance>(s->instance));
 try{WebGpuContextFactory::ReleaseContext(id);}catch(...){                                                                       }*s={};
}
