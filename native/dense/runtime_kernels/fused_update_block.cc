                                                                                                  
                                                                                                
                                                                             
#include "core/providers/webgpu/webgpu_kernel.h"
#include "core/providers/webgpu/shader_helper.h"
#include "core/providers/webgpu/math/gemm_utils.h"
#include "onnx/defs/schema.h"
#include "onnx/defs/shape_inference.h"
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
namespace onnxruntime { namespace webgpu {
struct FusedUpdateBuffer {int64_t elements; int role; int index; std::vector<int64_t> shape;int pool;};
struct FusedUpdateBinding {int buffer; bool output; int components; std::string name;};
struct FusedUpdateStep {std::string key;std::vector<FusedUpdateBinding> bindings;std::array<uint32_t,3> wg,dispatch;std::string implementation,body;int kind;std::array<uint32_t,3> mm;};
struct FusedUpdateConfig {int input_count;std::vector<FusedUpdateBuffer> buffers;std::vector<FusedUpdateStep> steps;std::vector<std::vector<int64_t>> outputs;std::vector<int64_t> pool_capacities;};
#include "fused_update_configs.inc"
class FusedUpdateProgram final:public Program<FusedUpdateProgram>{
 public:explicit FusedUpdateProgram(const FusedUpdateStep&s):Program("FusedUpdatePackedProgram"),step_(s){}
 Status GenerateShaderCode(ShaderHelper&shader)const override{
  for(const auto&b:step_.bindings)if(!b.output)shader.AddInput(b.name,ShaderUsage::None);
  for(const auto&b:step_.bindings)if(b.output)shader.AddOutput(b.name,ShaderUsage::None);
  shader.AdditionalImplementation()<<step_.implementation;
  shader.MainFunctionBody()<<step_.body;
  return Status::OK();
 }
 private:const FusedUpdateStep&step_;
};
                                                                                                         
class FusedUpdateTile32Program final:public Program<FusedUpdateTile32Program>{
 public:explicit FusedUpdateTile32Program(const FusedUpdateStep&s):Program("FusedUpdateTile32"),step_(s){}
 WEBGPU_PROGRAM_DEFINE_UNIFORM_VARIABLES({"dim_a_outer",ProgramUniformVariableDataType::Uint32},{"dim_b_outer",ProgramUniformVariableDataType::Uint32},{"dim_inner",ProgramUniformVariableDataType::Uint32},{"logical_dispatch_x",ProgramUniformVariableDataType::Uint32},{"logical_dispatch_y",ProgramUniformVariableDataType::Uint32},{"logical_dispatch_z",ProgramUniformVariableDataType::Uint32});
 Status GenerateShaderCode(ShaderHelper&shader)const override{
  for(const auto&b:step_.bindings)if(!b.output)shader.AddInput(b.name,ShaderUsage::None);
  for(const auto&b:step_.bindings)if(b.output)shader.AddOutput(b.name,ShaderUsage::None);
  shader.AdditionalImplementation()<<step_.implementation;
  return MakeMatMulPackedVec4Source(shader,InlinedVector<int64_t>{4,4,1},8,8,"f32",nullptr,false,false,1.0f,true,4,32);
 }
 private:const FusedUpdateStep&step_;
};
constexpr const char* kFusedUpdateDomain="reconstruction.fused_update";
class ReconstructionFusedUpdateBlock final:public WebGpuKernel{
 public:explicit ReconstructionFusedUpdateBlock(const OpKernelInfo&info):WebGpuKernel(info),config_(info.GetAttrOrDefault<int64_t>("config",-1)){}
 Status ComputeInternal(ComputeContext&context)const override{
  ORT_RETURN_IF_NOT(config_>=0&&size_t(config_)<fused_update_configs.size(),"Fused update unknown frozen config");
  const auto&c=fused_update_configs[size_t(config_)];
  ORT_RETURN_IF_NOT(context.InputCount()==c.input_count&&(context.OutputCount()==3||context.OutputCount()==3+int(c.pool_capacities.size())),"Fused update exact arity");
  ORT_RETURN_IF_NOT(context.DeviceLimits().maxComputeWorkgroupStorageSize>=8192,"Fused update originalTile32/GN requires8KiB workgroup storage; frozen max<=16KiB");
  std::vector<const Tensor*> tensors(c.buffers.size(),nullptr);std::vector<Tensor*> writable(c.buffers.size(),nullptr);
                                                                                                                       
                                                                                                                
  std::vector<std::unique_ptr<Tensor>> private_pool;
  std::vector<Tensor*> pool;
  const bool whole_scratch=context.OutputCount()==3+int(c.pool_capacities.size());
  for(size_t i=0;i<c.pool_capacities.size();++i){auto capacity=c.pool_capacities[i];ORT_RETURN_IF_NOT(capacity>0&&capacity%4==0,"Fused update pool F32vec4 capacity");
   if(whole_scratch){auto*t=context.Output(int(3+i),TensorShape{capacity});ORT_RETURN_IF_NOT(t&&t->IsDataType<float>(),"Fused update actual whole scratch output");pool.push_back(t);}
   else{private_pool.emplace_back(std::make_unique<Tensor>(context.CreateGPUTensor(DataTypeImpl::GetType<float>(),TensorShape{capacity})));pool.push_back(private_pool.back().get());}
  }
  std::fprintf(stderr,"RECONSTRUCTION_FUSED_UPDATE_SCRATCH_BIND config=%lld wholegraph=%d slots=%zu bytes=",(long long)config_,int(whole_scratch),pool.size());size_t scratch_bytes=0;for(auto capacity:c.pool_capacities)scratch_bytes+=size_t(capacity)*4;std::fprintf(stderr,"%zu\n",scratch_bytes);
  for(size_t i=0;i<c.buffers.size();++i){const auto&b=c.buffers[i];
   if(b.role==0){const auto*t=context.Input<Tensor>(b.index);ORT_RETURN_IF_NOT(t&&t->IsDataType<float>()&&t->Shape()==TensorShape(b.shape),"Fused update exact input shape/type: ",i);ORT_RETURN_IF_NOT(t->Shape().Size()==b.elements,"Fused update exact input elements");tensors[i]=t;}
   else if(b.role==1){auto*t=context.Output(b.index,TensorShape(c.outputs[size_t(b.index)]));tensors[i]=t;writable[i]=t;}
   else if(b.pool>=0){ORT_RETURN_IF_NOT(size_t(b.pool)<pool.size()&&pool[size_t(b.pool)]->Shape().Size()>=b.elements,"Fused update frozen pool binding");tensors[i]=pool[size_t(b.pool)];writable[i]=pool[size_t(b.pool)];}
                                                                                                                       
  }
  for(const auto&s:c.steps){
   uint64_t lanes=uint64_t(s.wg[0])*s.wg[1]*s.wg[2];ORT_RETURN_IF_NOT(lanes<=context.DeviceLimits().maxComputeInvocationsPerWorkgroup&&s.wg[0]<=context.DeviceLimits().maxComputeWorkgroupSizeX&&s.wg[1]<=context.DeviceLimits().maxComputeWorkgroupSizeY,"Fused update workgroup limit");
   auto submit=[&](auto&program)->Status{
    program.CacheHint(s.key,"officialORTTile32_sourcefixed_liveness_v1").SetWorkgroupSize(s.wg[0],s.wg[1],s.wg[2]).SetDispatchGroupSize(s.dispatch[0],s.dispatch[1],s.dispatch[2]);
    for(const auto&b:s.bindings)if(!b.output){ORT_RETURN_IF_NOT(tensors[size_t(b.buffer)],"Fused update missing live input");program.AddInput({tensors[size_t(b.buffer)],ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,b.components});}
    for(const auto&b:s.bindings)if(b.output){ORT_RETURN_IF_NOT(writable[size_t(b.buffer)],"Fused update missing live output");program.AddOutput({writable[size_t(b.buffer)],ProgramTensorMetadataDependency::TypeAndRank,ProgramOutput::Flatten,b.components});}
    return context.RunProgram(program);
   };
   if(s.kind==1){FusedUpdateTile32Program program(s);program.AddUniformVariables({{s.mm[0]},{s.mm[1]},{s.mm[2]},{s.dispatch[0]},{s.dispatch[1]},{s.dispatch[2]}});ORT_RETURN_IF_ERROR(submit(program));}
   else{FusedUpdateProgram program(s);ORT_RETURN_IF_ERROR(submit(program));}
  }
  return Status::OK();
 }
 private:int64_t config_;
};
ONNX_OPERATOR_KERNEL_EX(ReconstructionFusedUpdateBlock,kFusedUpdateDomain,1,kWebGpuExecutionProvider,(*KernelDefBuilder::Create()).TypeConstraint("T",DataTypeImpl::GetTensorType<float>()),ReconstructionFusedUpdateBlock);
KernelCreateInfo CreateReconstructionFusedUpdateBlockKernelInfo(){return BuildKernelCreateInfo<ONNX_OPERATOR_KERNEL_CLASS_NAME(kWebGpuExecutionProvider,kFusedUpdateDomain,1,ReconstructionFusedUpdateBlock)>();}
}}
extern "C" int ReconstructionRegisterFusedUpdateSchema()noexcept{try{static std::once_flag once;std::call_once(once,[]{using namespace ONNX_NAMESPACE;
 auto&v=OpSchemaRegistry::DomainToVersionRange::Instance();if(v.Map().find("reconstruction.fused_update")==v.Map().end())v.AddDomainToVersion("reconstruction.fused_update",1,1);
 auto s=OpSchema().SetName("ReconstructionFusedUpdateBlock").SetDomain("reconstruction.fused_update").SinceVersion(1).SetDoc("Source-faithful originalORT Tile32 updateblock architecture; original equations/learnedweights and externalABI unchanged")
 .Input(0,"X","original external block tensors followed by frozen packed weights","T",OpSchema::Variadic)
 .Output(0,"H","hidden","T").Output(1,"D","depth residual","T").Output(2,"C","confidence","T")
 .Output(3,"Scratch","existing frozen scratch slots, actual wholegraph allocation owners","T",OpSchema::Variadic,true,0)
 .Attr("config","frozen four-config identity",AttributeProto::INT).TypeConstraint("T",{"tensor(float)"},"f32 only")
 .TypeAndShapeInferenceFunction([](InferenceContext&ctx){const auto*a=ctx.getAttribute("config");if(!a||a->i()<0||size_t(a->i())>=onnxruntime::webgpu::fused_update_configs.size())fail_shape_inference("Fused update config");const auto&c=onnxruntime::webgpu::fused_update_configs[size_t(a->i())];for(int i=0;i<3;++i){propagateElemTypeFromInputToOutput(ctx,0,i);auto*y=ctx.getOutputType(i)->mutable_tensor_type()->mutable_shape();for(auto d:c.outputs[size_t(i)])y->add_dim()->set_dim_value(d);}if(ctx.getNumOutputs()>3){if(ctx.getNumOutputs()!=3+c.pool_capacities.size())fail_shape_inference("Fused update whole scratcharity");for(size_t i=0;i<c.pool_capacities.size();++i){propagateElemTypeFromInputToOutput(ctx,0,3+i);ctx.getOutputType(3+i)->mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(c.pool_capacities[i]);}}});
 RegisterSchema(std::move(s),0,true,true);});return 0;}catch(...){return 1;}}
