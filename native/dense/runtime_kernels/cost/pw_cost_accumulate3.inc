                                                                                         
                                                                                                  
#include <array>
namespace onnxruntime { namespace webgpu {
class PwCostAccumulate3Program final : public Program<PwCostAccumulate3Program> {
 uint32_t c_,h_,w_,g_,d_,oh_,ow_;bool first_,alias_,uniform_grid_;
 public:
 PwCostAccumulate3Program(uint32_t c,uint32_t h,uint32_t w,uint32_t oh,uint32_t ow,bool first,bool alias,bool ug):Program{"PwCostAccumulate3"},c_(c),h_(h),w_(w),g_(4),d_(4),oh_(oh),ow_(ow),first_(first),alias_(alias),uniform_grid_(ug){}
 WEBGPU_PROGRAM_DEFINE_UNIFORM_VARIABLES({"channels",ProgramUniformVariableDataType::Uint32},{"height",ProgramUniformVariableDataType::Uint32},{"width",ProgramUniformVariableDataType::Uint32});
 Status GenerateShaderCode(ShaderHelper& shader)const override{
  const auto& x0=shader.AddInput("feature0",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);
  const auto& grid=shader.AddInput("packed_grid",ShaderUsage::UseUniform);
  const auto& ref=shader.AddInput("reference",ShaderUsage::UseUniform);
  const auto& x1=shader.AddInput("feature1",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);
  const auto& x2=shader.AddInput("feature2",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);
  const auto& weight=shader.AddInput("packed_weight",ShaderUsage::UseUniform);
  std::string previous;
  if(!alias_){const auto& p=shader.AddInput("previous",ShaderUsage::UseUniform);previous="f32("+p.GetByOffset(first_?"0u":"global_idx")+")";}
  const auto& y=shader.AddOutput("accumulator",ShaderUsage::UseUniform);
  if(alias_)previous="f32("+y.GetByOffset("global_idx")+")";
  auto& ex=shader.AdditionalImplementation();auto& body=shader.MainFunctionBody();
  ex<<"const C="<<c_<<"u;const H="<<h_<<"u;const W="<<w_<<"u;const G=4u;const D=4u;const OH="<<oh_<<"u;const OW="<<ow_<<"u;\n";
  ex<<"fn gs_denormalize(n:f32,length:u32)->f32{return (n+1.0)*0.5*f32(length-1u);}\n";
  std::array<const ShaderVariableHelper*,3> xs{&x0,&x1,&x2};
  for(int v=0;v<3;++v){const std::string n=std::to_string(v);
   ex<<"fn gs_pixel"<<n<<"(img_base:u32,r:i32,col:i32)->f32{if(r<0||r>=i32(H)||col<0||col>=i32(W)){return 0.0;}return f32("<<xs[v]->GetByOffset("img_base+u32(r)*W+u32(col)")<<");}\n";
   ex<<"fn gs_sample"<<n<<"(c:u32,dhw:u32)->f32{let grid_base=("<<v<<"u*D*OH*OW+dhw)*2u;let gx=f32("<<grid.GetByOffset("grid_base")<<");let gy=f32("<<grid.GetByOffset("grid_base+1u")<<");\n"
     <<"let px=gs_denormalize(gx,"<<(uniform_grid_?"uniforms.width":"W")<<");let py=gs_denormalize(gy,"<<(uniform_grid_?"uniforms.height":"H")<<");let img_base=c*H*W;let x1=i32(floor(px));let y1=i32(floor(py));let x2=x1+1;let y2=y1+1;let dx1=px-f32(x1);let dx2=1.0-dx1;let dy1=py-f32(y1);let dy2=1.0-dy1;\n"
     <<"let p11=gs_pixel"<<n<<"(img_base,y1,x1);let p12=gs_pixel"<<n<<"(img_base,y1,x2);let p21=gs_pixel"<<n<<"(img_base,y2,x1);let p22=gs_pixel"<<n<<"(img_base,y2,x2);let result=dy2*(dx2*p11+dx1*p12)+dy1*(dx2*p21+dx1*p22);return result;}\n";
  }
  ex<<"var<workgroup> sampled_scratch:array<f32,"<<(c_/4)*64<<">;var<workgroup> product_scratch:array<f32,"<<(c_/4)*64<<">;var<workgroup> mean_scratch:array<f32,64>;var<workgroup> weighted_scratch:array<f32,64>;var<workgroup> accumulator_scratch:array<f32,64>;\n";
  body<<"let group=global_idx/(D*OH*OW);let dhw=global_idx%(D*OH*OW);let hw=dhw%(OH*OW);var accumulated="<<previous<<";\n";
  for(int v=0;v<3;v++){
   body<<"for(var q=0u;q<uniforms.channels;q++){let c=group*"<<c_/4<<"u+q;sampled_scratch[q*64u+local_idx]=gs_sample"<<v<<"(c,dhw);}\nworkgroupBarrier();\n"
       <<"for(var q=0u;q<uniforms.channels;q++){let c=group*"<<c_/4<<"u+q;let sampled=sampled_scratch[q*64u+local_idx];let current_element=sampled*f32("<<ref.GetByOffset("c*OH*OW+hw")<<");product_scratch[q*64u+local_idx]=current_element;}\nworkgroupBarrier();\n"
       <<"var sum"<<v<<"=f32(0);for(var q=0u;q<uniforms.channels;q++){sum"<<v<<"+=f32(product_scratch[q*64u+local_idx]);}mean_scratch[local_idx]=sum"<<v<<"/f32(uniforms.channels);\nworkgroupBarrier();\n"
       <<"weighted_scratch[local_idx]=f32("<<weight.GetByOffset(std::to_string(v)+"u*OH*OW+hw")<<")*mean_scratch[local_idx];\nworkgroupBarrier();\nlet t"<<v<<"=weighted_scratch[local_idx];\nworkgroupBarrier();\n"
       <<"accumulator_scratch[local_idx]="<<(first_&&v==0?"t0+accumulated":"accumulated+t"+std::to_string(v))<<";\nworkgroupBarrier();\naccumulated=accumulator_scratch[local_idx];\nworkgroupBarrier();\n";
  }
  body<<y.SetByOffset("global_idx","accumulated")<<"\n";return Status::OK();
 }
};
class PwCostAccumulate3 final : public WebGpuKernel {
 int64_t first_,profile_,groups_,depth_;
 public:
 explicit PwCostAccumulate3(const OpKernelInfo& i):WebGpuKernel(i),first_(i.GetAttrOrDefault<int64_t>("first",-1)),profile_(i.GetAttrOrDefault<int64_t>("pw_profile",-1)),groups_(i.GetAttrOrDefault<int64_t>("groups",-1)),depth_(i.GetAttrOrDefault<int64_t>("depth",-1)){}
 Status ComputeInternal(ComputeContext& context)const override{
  const auto*x=context.Input<Tensor>(0);const auto*grid=context.Input<Tensor>(1);const auto*ref=context.Input<Tensor>(2);const auto*x1=context.Input<Tensor>(3);const auto*x2=context.Input<Tensor>(4);const auto*weight=context.Input<Tensor>(5);const auto*prev=context.Input<Tensor>(6);
  ORT_RETURN_IF_NOT((first_==0||first_==1)&&(profile_==0||profile_==1)&&groups_==4&&depth_==4,"frozen Cost3 attributes");
  ORT_RETURN_IF_NOT(x&&x1&&x2&&grid&&ref&&weight&&prev,"Cost3 seven exact inputs required");
  ORT_RETURN_IF_NOT((x->IsDataType<float>()||x->IsDataType<MLFloat16>())&&x1->DataType()==x->DataType()&&x2->DataType()==x->DataType()&&(ref->IsDataType<float>()||ref->IsDataType<MLFloat16>())&&grid->IsDataType<float>()&&weight->IsDataType<float>()&&prev->IsDataType<float>(),"Cost3 feature storage F16/F32 and geometry/weight/accumulator F32");
  const auto&xs=x->Shape();const auto&rs=ref->Shape();
  ORT_RETURN_IF_NOT(xs.NumDimensions()==4&&xs[0]==1&&x1->Shape()==xs&&x2->Shape()==xs&&rs.NumDimensions()==6&&rs[0]==1&&rs[1]==4&&rs[2]==xs[1]/4&&rs[3]==1,"Cost3 reference/feature ranks/order");
  ORT_RETURN_IF_NOT(profile_==0?(xs==TensorShape({1,32,144,192})||xs==TensorShape({1,16,288,384})):(xs==TensorShape({1,32,288,384})||xs==TensorShape({1,16,576,768})),"Cost3 unsupported frozen exactshape");
  const auto oh=rs[4],ow=rs[5];ORT_RETURN_IF_NOT(oh==xs[2]&&ow==xs[3]&&grid->Shape()==TensorShape({3,4*oh,ow,2})&&weight->Shape()==TensorShape({3,1,1,oh,ow}),"Cost3 packedgrid/weight index bounds");
  TensorShape ys{1,4,4,oh,ow};ORT_RETURN_IF_NOT(first_?prev->Shape().IsScalar():prev->Shape()==ys,"Cost3 previous/firstscalar shape");
  auto*out=context.Output(0,ys);const bool alias=prev->DataRaw()==out->MutableDataRaw();ORT_RETURN_IF_NOT(!first_||!alias,"Cost3 never aliases initialscalar");const uint32_t count=narrow<uint32_t>(ys.Size());ORT_RETURN_IF_NOT(count%256==0&&count/256<=65535,"Cost3 exacttwo-dimensional workgroups");
  PwCostAccumulate3Program p(narrow<uint32_t>(xs[1]),narrow<uint32_t>(xs[2]),narrow<uint32_t>(xs[3]),narrow<uint32_t>(oh),narrow<uint32_t>(ow),first_!=0,alias,profile_==1&&xs[1]==32);
  for(int i=0;i<6;i++){p.AddInput({context.Input<Tensor>(i),ProgramTensorMetadataDependency::TypeAndRank});}if(!alias)p.AddInput({prev,ProgramTensorMetadataDependency::TypeAndRank});
  p.AddOutput({out,ProgramTensorMetadataDependency::TypeAndRank}).SetWorkgroupSize(64).SetDispatchGroupSize(count/256,4).CacheHint(xs[1],xs[2],xs[3],oh,ow,first_,alias,profile_).AddUniformVariables({{narrow<uint32_t>(xs[1]/4)},{narrow<uint32_t>(xs[2])},{narrow<uint32_t>(xs[3])}});
  if(std::getenv("PW_COST_STORAGE_TRACE"))std::cerr<<"PW_COST3_CALL profile="<<profile_<<" first="<<first_<<" alias="<<alias<<" bindings="<<(alias?7:8)<<" count="<<count<<" packed_grid_B="<<grid->SizeInBytes()<<" packed_weight_B="<<weight->SizeInBytes()<<" output_B="<<out->SizeInBytes()<<" means_global_B=0 F32acc=1\n";
  return context.RunProgram(p);
 }
};
ONNX_OPERATOR_KERNEL_EX(PwCostAccumulate3,kPwGridDomain,1,kWebGpuExecutionProvider,(*KernelDefBuilder::Create()).TypeConstraint("TX",{DataTypeImpl::GetTensorType<float>(),DataTypeImpl::GetTensorType<MLFloat16>()}).TypeConstraint("TR",{DataTypeImpl::GetTensorType<float>(),DataTypeImpl::GetTensorType<MLFloat16>()}).TypeConstraint("TF",DataTypeImpl::GetTensorType<float>()).MayInplace(6,0),PwCostAccumulate3);
KernelCreateInfo CreatePwCostAccumulate3KernelInfo(){return BuildKernelCreateInfo<ONNX_OPERATOR_KERNEL_CLASS_NAME(kWebGpuExecutionProvider,kPwGridDomain,1,PwCostAccumulate3)>();}
void RegisterPwCostAccumulate3Schema(){using namespace ONNX_NAMESPACE;auto s=OpSchema().SetName("PwCostAccumulate3").SetDomain(kPwGridDomain).SinceVersion(1).SetDoc("Pinned officialGetCost three consecutive sourceviews; existingbilinear groupmean/weightfirst Mul/ordered Add; F32packconcat boundary; not a new costalgorithm")
 .Input(0,"X0","firstsourcefeature","TX").Input(1,"GridPack","three originalD4 F32grids concatenated axis0","TF").Input(2,"Reference","originalreferenceview reshapealias","TR").Input(3,"X1","secondsourcefeature","TX").Input(4,"X2","thirdsourcefeature","TX").Input(5,"WeightPack","three original N11HW F32weights concatenated axis0","TF").Input(6,"Previous","orderedprioraccumulator or originalpositivezero scalar","TF").Output(0,"Y","originalthird Add output F32","TF")
 .Attr("groups","fixed4",AttributeProto::INT).Attr("depth","fixed4",AttributeProto::INT).Attr("first","first originalsourcegroup",AttributeProto::INT).Attr("pw_profile","0LOW1HIGH",AttributeProto::INT).TypeConstraint("TX",{"tensor(float)","tensor(float16)"},"unchangedfeaturestorage").TypeConstraint("TR",{"tensor(float)","tensor(float16)"},"unchangedreferencestorage").TypeConstraint("TF",{"tensor(float)"},"F32geometryweightsmeansaccumulator")
 .TypeAndShapeInferenceFunction([](InferenceContext&c){c.getOutputType(0)->mutable_tensor_type()->set_elem_type(TensorProto::FLOAT);if(!hasInputShape(c,2))return;const auto&r=getInputShape(c,2);if(r.dim_size()!=6)fail_shape_inference("Cost3reference rank6");auto*s=c.getOutputType(0)->mutable_tensor_type()->mutable_shape();*s->add_dim()=r.dim(0);s->add_dim()->set_dim_value(4);s->add_dim()->set_dim_value(4);*s->add_dim()=r.dim(4);*s->add_dim()=r.dim(5);});RegisterSchema(std::move(s),0,true,true);}
}}
