namespace onnxruntime { namespace webgpu { namespace high_profile {
class PwGruTailProgram final : public Program<PwGruTailProgram> {
 public: PwGruTailProgram():Program{"high_profile::PwGruTailExactTwoBarrier"}{}
 Status GenerateShaderCode(ShaderHelper& shader) const override {
  const auto& z=shader.AddInput("z",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);
  const auto& h=shader.AddInput("h",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);
  const auto& q=shader.AddInput("q",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);
  const auto& y=shader.AddOutput("y",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);
  shader.AdditionalImplementation()<<"var<workgroup> sub_scratch:array<vec4<f32>,64>;\nvar<workgroup> hm_scratch:array<vec4<f32>,64>;\nvar<workgroup> zq_scratch:array<vec4<f32>,64>;\n";
  auto& body=shader.MainFunctionBody();
  body<<"let av = vec4<f32>(1.0);\nlet bv = "<<z.GetByOffset("global_idx")<<";\nsub_scratch[local_idx] = av - bv;\nworkgroupBarrier();\nlet a_hm = sub_scratch[local_idx];\nlet b_hm = "<<h.GetByOffset("global_idx")<<";\nhm_scratch[local_idx] = a_hm * b_hm;\nlet a_zq = "<<z.GetByOffset("global_idx")<<";\nlet b_zq = "<<q.GetByOffset("global_idx")<<";\nzq_scratch[local_idx] = a_zq * b_zq;\nworkgroupBarrier();\nlet a = hm_scratch[local_idx];\nlet b = zq_scratch[local_idx];\n"<<y.SetByOffset("global_idx","a + b")<<"\n";
  return Status::OK();
 }
};

static Status PwSelectedGruTailCompute(ComputeContext& context){
  const auto* z=context.Input<Tensor>(0);const auto* h=context.Input<Tensor>(1);const auto* q=context.Input<Tensor>(2);const auto& s=z->Shape();
  ORT_RETURN_IF_NOT(s.NumDimensions()==4&&s[0]==1&&(s[1]==32||s[1]==20)&&s==h->Shape()&&s==q->Shape(),"frozen exact GRU tail shape required");
  const int64_t n=s.Size();ORT_RETURN_IF_NOT(n>0&&n%256==0&&n/256<=65535,"exact no-partial-workgroup GRU shape required");
  auto* y=context.Output(0,s);PwGruTailProgram program;
  program.AddInputs({{z,ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,4},{h,ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,4},{q,ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,4}})
   .AddOutput({y,ProgramTensorMetadataDependency::TypeAndRank,ProgramOutput::Flatten,4}).SetWorkgroupSize(64).SetDispatchGroupSize(narrow<uint32_t>(n/256));
  std::cerr<<"PW_GRU_TAIL_EXACT_CALL n="<<n<<"\n";return context.RunProgram(program);
}
}}}
