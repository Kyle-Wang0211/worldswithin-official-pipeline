                                                                     
                                                                    
namespace onnxruntime { namespace webgpu { namespace low_profile {
class PwWeightedAccumulateProgram final : public Program<PwWeightedAccumulateProgram> {
 public:
 PwWeightedAccumulateProgram(uint32_t spatial,bool first,bool alias,bool packed):Program{"low_profile::PwWeightedAccumulate"},first_{first},alias_{alias},packed_{packed}{}
 WEBGPU_PROGRAM_DEFINE_UNIFORM_VARIABLES({"spatial_vecs",ProgramUniformVariableDataType::Uint32});
 Status GenerateShaderCode(ShaderHelper& shader) const override {
  std::vector<std::string> group_read;
  for(int g=0;g<(packed_?1:4);++g){const auto& v=shader.AddInput("mean"+std::to_string(g),ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);group_read.push_back(v.GetByOffset(packed_?"global_idx":"j"));}
  const auto& weight=shader.AddInput("weight",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);
  std::string previous;
  if(!alias_){const auto& prev=shader.AddInput("previous",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);previous=prev.GetByOffset(first_?"0u":"global_idx");if(first_)previous="vec4<f32>("+previous+".x)";}
  const auto& y=shader.AddOutput("accumulator",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);
  if(alias_) previous=y.GetByOffset("global_idx");
  shader.AdditionalImplementation()<<"var<workgroup> product_scratch:array<vec4<f32>,64>;\n";
  auto& body=shader.MainFunctionBody();
  body<<"let S=uniforms.spatial_vecs; let p=global_idx%S; let depth=global_idx/(4u*S); let group=(global_idx/S)%4u; let j=depth*S+p;\n";
  if(packed_)body<<"let mean="<<group_read[0]<<";\n";
  else body<<"var mean:vec4<f32>; switch(group){case 0u:{mean="<<group_read[0]<<";}case 1u:{mean="<<group_read[1]<<";}case 2u:{mean="<<group_read[2]<<";}default:{mean="<<group_read[3]<<";}}\n";
  body<<"product_scratch[local_idx]="<<weight.GetByOffset("p")<<"*mean;\nworkgroupBarrier();\nlet t=product_scratch[local_idx];\nworkgroupBarrier();\n";
  body<<y.SetByOffset("global_idx",first_?"t + "+previous:previous+" + t")<<"\n";
  return Status::OK();
 }
 private:bool first_,alias_,packed_;
};
class PwWeightedAccumulate final : public WebGpuKernel {
 public:
 explicit PwWeightedAccumulate(const OpKernelInfo& info):WebGpuKernel(info),first_{info.GetAttrOrDefault<int64_t>("first",-1)}{}
 Status ComputeInternal(ComputeContext& context) const override {
  ORT_RETURN_IF_NOT(first_==0||first_==1,"first attribute required");
  const auto* previous=context.Input<Tensor>(0);const auto* a=context.Input<Tensor>(1);const auto& ms=a->Shape();
  const bool packed=ms.NumDimensions()==5&&ms[2]==4;
  ORT_RETURN_IF_NOT(ms.NumDimensions()==5&&ms[0]==1&&ms[1]==4&&(ms[2]==1||packed)&&ms[3]>0&&ms[4]>0,"frozen four-group mean shape required");
  for(int i=2;i<=4;++i) ORT_RETURN_IF_NOT(context.Input<Tensor>(i)->Shape()==ms,"mean shapes differ");
  if(packed)for(int i=2;i<=4;i++)ORT_RETURN_IF_NOT(context.Input<Tensor>(i)->DataRaw()==a->DataRaw(),"packed duplicateviews mustbeidentical tensor");
  const auto* weight=context.Input<Tensor>(5);const auto& ws=weight->Shape();
  ORT_RETURN_IF_NOT(ws.NumDimensions()==5&&ws[0]==1&&ws[1]==1&&ws[2]==1&&ws[3]==ms[3]&&ws[4]==ms[4],"frozen weight broadcast required");
  TensorShape shape{1,4,4,ms[3],ms[4]};
  ORT_RETURN_IF_NOT(first_?previous->Shape().IsScalar():previous->Shape()==shape,"previous shape/initial scalar mismatch");
  const int64_t spatial=ms[3]*ms[4];ORT_RETURN_IF_NOT(spatial%256==0&&shape.Size()/256<=65535,"exact no-partial-workgroup shape required");
  auto* out=context.Output(0,shape);const bool alias=previous->DataRaw()==out->MutableDataRaw();
  ORT_RETURN_IF_NOT(!first_||!alias,"never mutate initial scalar");
  PwWeightedAccumulateProgram program(narrow<uint32_t>(spatial),first_!=0,alias,packed);
  for(int i=1;i<=5;++i)if(!packed||i==1||i==5)program.AddInput({context.Input<Tensor>(i),ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,4});
                                                                             
                                                                                
  if(!alias)program.AddInput({previous,ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,4});
  program.AddOutput({out,ProgramTensorMetadataDependency::TypeAndRank,ProgramOutput::Flatten,4})
      .SetWorkgroupSize(64).SetDispatchGroupSize(narrow<uint32_t>(shape.Size()/256))
      .CacheHint(narrow<uint32_t>(spatial),first_,alias,packed).AddUniformVariable({narrow<uint32_t>(spatial/4)});
  std::cerr<<"PW_WEIGHTED_ACCUMULATE_CALL packed="<<packed<<" first="<<first_<<" alias="<<alias<<"\n";return context.RunProgram(program);
 }
 private:int64_t first_;
};
}}}
