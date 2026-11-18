#include "core/providers/webgpu/shader_helper.h"
namespace onnxruntime { namespace webgpu {
void MatMulReadFnSource(ShaderHelper&, std::string_view, const ShaderVariableHelper&, const std::string&, const ShaderIndicesHelper*, std::string_view, std::string_view, bool);
}}
#include "pw_grid_group_mean.h"
#include "low/pw_grid_group_mean.cc"
#include "high/pw_grid_group_mean.cc"
namespace onnxruntime { namespace webgpu {
class PwGridSampleGroupMean final : public WebGpuKernel {
 std::unique_ptr<low_profile::PwGridSampleGroupMean> low_; std::unique_ptr<high_profile::PwGridSampleGroupMean> high_;
 public:
 explicit PwGridSampleGroupMean(const OpKernelInfo& info):WebGpuKernel(info){auto p=info.GetAttrOrDefault<int64_t>("pw_profile",-1);ORT_ENFORCE(p==0||p==1,"Explicit pw_profile0LOW/1HIGH required");if(p==0)low_=std::make_unique<low_profile::PwGridSampleGroupMean>(info);else high_=std::make_unique<high_profile::PwGridSampleGroupMean>(info);}
 Status ComputeInternal(ComputeContext& context) const override{return low_?low_->ComputeInternal(context):high_->ComputeInternal(context);}
};
ONNX_OPERATOR_KERNEL_EX(PwGridSampleGroupMean,kPwGridDomain,1,kWebGpuExecutionProvider,(*KernelDefBuilder::Create()).TypeConstraint("TX",{DataTypeImpl::GetTensorType<float>(),DataTypeImpl::GetTensorType<MLFloat16>()}).TypeConstraint("TR",{DataTypeImpl::GetTensorType<float>(),DataTypeImpl::GetTensorType<MLFloat16>()}).TypeConstraint("TF",DataTypeImpl::GetTensorType<float>()),PwGridSampleGroupMean);
KernelCreateInfo CreatePwGridSampleGroupMeanKernelInfo(){return BuildKernelCreateInfo<ONNX_OPERATOR_KERNEL_CLASS_NAME(kWebGpuExecutionProvider,kPwGridDomain,1,PwGridSampleGroupMean)>();}
class PwWeightedAccumulate final : public WebGpuKernel {
 std::unique_ptr<low_profile::PwWeightedAccumulate> low_; std::unique_ptr<high_profile::PwWeightedAccumulate> high_;
 public:
 explicit PwWeightedAccumulate(const OpKernelInfo& info):WebGpuKernel(info){auto p=info.GetAttrOrDefault<int64_t>("pw_profile",-1);ORT_ENFORCE(p==0||p==1,"Explicit pw_profile0LOW/1HIGH required");if(p==0)low_=std::make_unique<low_profile::PwWeightedAccumulate>(info);else high_=std::make_unique<high_profile::PwWeightedAccumulate>(info);}
 Status ComputeInternal(ComputeContext& context) const override{return low_?low_->ComputeInternal(context):high_->ComputeInternal(context);}
};
ONNX_OPERATOR_KERNEL_EX(PwWeightedAccumulate,kPwGridDomain,1,kWebGpuExecutionProvider,(*KernelDefBuilder::Create()).TypeConstraint("T",DataTypeImpl::GetTensorType<float>()).MayInplace(0,0),PwWeightedAccumulate);
KernelCreateInfo CreatePwWeightedAccumulateKernelInfo(){return BuildKernelCreateInfo<ONNX_OPERATOR_KERNEL_CLASS_NAME(kWebGpuExecutionProvider,kPwGridDomain,1,PwWeightedAccumulate)>();}
class PwConcatConv7 final : public WebGpuKernel {
 std::unique_ptr<low_profile::PwConcatConv7> low_; std::unique_ptr<high_profile::PwConcatConv7> high_;
 public:
 explicit PwConcatConv7(const OpKernelInfo& info):WebGpuKernel(info){auto p=info.GetAttrOrDefault<int64_t>("pw_profile",-1);ORT_ENFORCE(p==0||p==1,"Explicit pw_profile0LOW/1HIGH required");if(p==0)low_=std::make_unique<low_profile::PwConcatConv7>(info);else high_=std::make_unique<high_profile::PwConcatConv7>(info);}
 Status ComputeInternal(ComputeContext& context) const override{return low_?low_->ComputeInternal(context):high_->ComputeInternal(context);}
};
ONNX_OPERATOR_KERNEL_EX(PwConcatConv7,kPwGridDomain,1,kWebGpuExecutionProvider,(*KernelDefBuilder::Create()).TypeConstraint("T",DataTypeImpl::GetTensorType<float>()),PwConcatConv7);
KernelCreateInfo CreatePwConcatConv7KernelInfo(){return BuildKernelCreateInfo<ONNX_OPERATOR_KERNEL_CLASS_NAME(kWebGpuExecutionProvider,kPwGridDomain,1,PwConcatConv7)>();}
}}
extern "C" int ReconstructionRegisterFusedUpdateSchema() noexcept;
#include "pw_cost_accumulate3.inc"
extern "C" __attribute__((visibility("default"))) int ReconstructionRegisterExperimentalSchema() noexcept {
 try {static std::once_flag once;std::call_once(once,[]{
  using namespace ONNX_NAMESPACE;
  OpSchemaRegistry::DomainToVersionRange::Instance().AddDomainToVersion(onnxruntime::webgpu::kPwGridDomain,1,1);
  onnxruntime::webgpu::RegisterPwCostAccumulate3Schema();
  auto schema=OpSchema().SetName("PwGridSampleGroupMean").SetDomain(onnxruntime::webgpu::kPwGridDomain).SinceVersion(1)
   .SetDoc("Exact selected experimental GridSample(bilinear,zeros,align_corners1), Reshape, Mul, ascending ReduceMean composition")
   .Input(0,"X","NCHWsource feature storage","TX").Input(1,"Grid","NDHW2grid FP32 coordinates","TF").Input(2,"Reference","grouped reference feature storage","TR")
   .Output(0,"Y","NGDHWgroupmean FP32 accumulation","TF").Attr("groups","groupcount",AttributeProto::INT).Attr("depth","depthcount",AttributeProto::INT)
   .Attr("pw_profile","fixed original provider recipe:0LOW,1HIGH",AttributeProto::INT).TypeConstraint("TX",{"tensor(float)","tensor(float16)"},"feature storage").TypeConstraint("TR",{"tensor(float)","tensor(float16)"},"reference feature storage").TypeConstraint("TF",{"tensor(float)"},"FP32 coordinates and cost")
   .TypeAndShapeInferenceFunction([](InferenceContext& ctx){ctx.getOutputType(0)->mutable_tensor_type()->set_elem_type(TensorProto::FLOAT);if(!hasInputShape(ctx,0)||!hasInputShape(ctx,2))return;const auto& xs=getInputShape(ctx,0);const auto& rs=getInputShape(ctx,2);if(xs.dim_size()!=4)fail_shape_inference("Xrank4required");if(rs.dim_size()!=6)fail_shape_inference("Reference rank6required");auto* ys=ctx.getOutputType(0)->mutable_tensor_type()->mutable_shape();*ys->add_dim()=xs.dim(0);ys->add_dim()->set_dim_value(getAttribute(ctx,"groups",0));ys->add_dim()->set_dim_value(getAttribute(ctx,"depth",0));*ys->add_dim()=rs.dim(4);*ys->add_dim()=rs.dim(5);});
  RegisterSchema(std::move(schema),0,true,true);
  auto weighted=OpSchema().SetName("PwWeightedAccumulate").SetDomain(onnxruntime::webgpu::kPwGridDomain).SinceVersion(1)
   .SetDoc("Selected f32 fourgroup Concat(axis2), weightfirst Mul, ordered Add; optional lastuse accumulator reuse")
   .Input(0,"Previous","initialpositivezero scalar or orderedprioraccumulator","T")
   .Input(1,"Mean0","first groupmean","T").Input(2,"Mean1","second groupmean","T")
   .Input(3,"Mean2","third groupmean","T").Input(4,"Mean3","fourth groupmean","T")
   .Input(5,"Weight","broadcast N11HW weight","T").Output(0,"Y","N44HW accumulator","T")
   .Attr("first","one for product+scalarzero; zero for previous+product",AttributeProto::INT)
   .Attr("pw_profile","fixed original provider recipe:0LOW,1HIGH",AttributeProto::INT).TypeConstraint("T",{"tensor(float)"},"f32only")
   .TypeAndShapeInferenceFunction([](InferenceContext& ctx){propagateElemTypeFromInputToOutput(ctx,1,0);if(!hasInputShape(ctx,1))return;const auto& s=getInputShape(ctx,1);if(s.dim_size()!=5)fail_shape_inference("rank5mean required");auto* out=ctx.getOutputType(0)->mutable_tensor_type()->mutable_shape();*out->add_dim()=s.dim(0);*out->add_dim()=s.dim(1);out->add_dim()->set_dim_value(4);*out->add_dim()=s.dim(3);*out->add_dim()=s.dim(4);});
  RegisterSchema(std::move(weighted),0,true,true);
  auto fused=OpSchema().SetName("PwConcatConv7").SetDomain(onnxruntime::webgpu::kPwGridDomain).SinceVersion(1).SetDoc("Fixed originalORTConv2dMM packedvec4tile32 index-loader specs; source/model/runtime jointlyfrozen").Input(0,"Inputs","fixedspec leaf tensors then originalweight and optionalbias","T",OpSchema::Variadic,true,1).Output(0,"Y","originalNCHWoutput","T").Attr("spec","fixedspecid",AttributeProto::INT).Attr("shape","fixedNCHWshape",AttributeProto::INTS).Attr("pw_profile","fixed original provider recipe:0LOW,1HIGH",AttributeProto::INT).TypeConstraint("T",{"tensor(float)"},"f32only").TypeAndShapeInferenceFunction([](InferenceContext& ctx){propagateElemTypeFromInputToOutput(ctx,0,0);const auto* a=ctx.getAttribute("shape");if(!a)fail_shape_inference("fixedshape required");auto* ys=ctx.getOutputType(0)->mutable_tensor_type()->mutable_shape();ys->clear_dim();for(int64_t v:a->ints()){if(v<=0)fail_shape_inference("positivefixedshape required");ys->add_dim()->set_dim_value(v);}});
  RegisterSchema(std::move(fused),0,true,true);

 });return ReconstructionRegisterFusedUpdateSchema();}catch(const std::exception& e){std::cerr<<"PW_SCHEMA_REGISTER_FAILED "<<e.what()<<"\n";return 1;}catch(...){return 2;}
}

