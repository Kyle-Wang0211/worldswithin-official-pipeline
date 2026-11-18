namespace onnxruntime { namespace webgpu { namespace high_profile {
class PwAffineSiluProgram final: public Program<PwAffineSiluProgram> {
public: PwAffineSiluProgram(uint32_t spatial):Program{"high_profile::PwAffineSiluExactThreeBarrier"},spatial_{spatial}{}
Status GenerateShaderCode(ShaderHelper& shader) const override {
 const auto& x=shader.AddInput("x",ShaderUsage::UseValueTypeAlias);const auto& scale=shader.AddInput("scale",ShaderUsage::UseValueTypeAlias);const auto& bias=shader.AddInput("bias",ShaderUsage::UseValueTypeAlias);const auto& y=shader.AddOutput("y",ShaderUsage::UseValueTypeAlias);
 shader.AdditionalImplementation()<<"var<workgroup> mul_scratch:array<vec4<f32>,64>;\nvar<workgroup> add_scratch:array<vec4<f32>,64>;\nvar<workgroup> sigmoid_scratch:array<vec4<f32>,64>;\n";
 auto& body=shader.MainFunctionBody();body<<"let channel=global_idx*4u/"<<spatial_<<"u;\nlet a_mul="<<x.GetByOffset("global_idx")<<";let b_mul=vec4<f32>("<<scale.GetByOffset("channel")<<");\nmul_scratch[local_idx]=a_mul*b_mul;\nworkgroupBarrier();\nlet a_add=mul_scratch[local_idx];let b_add=vec4<f32>("<<bias.GetByOffset("channel")<<");\nadd_scratch[local_idx]=a_add+b_add;\nworkgroupBarrier();\nlet a=add_scratch[local_idx];\nsigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));\nworkgroupBarrier();\nlet final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];\n"<<y.SetByOffset("global_idx","final_a*final_b")<<"\n";return Status::OK();
}
private:uint32_t spatial_;
};
static Status PwSelectedAffineSiluCompute(ComputeContext& context){
 const auto* x=context.Input<Tensor>(0);const auto* scale=context.Input<Tensor>(1);const auto* bias=context.Input<Tensor>(2);const auto& s=x->Shape();
 ORT_RETURN_IF_NOT(s.NumDimensions()==4&&s[0]==1&&(s[1]==8||s[1]==16||s[1]==32),"frozen affine-silu NCHW shape");
 ORT_RETURN_IF_NOT(scale->Shape()==TensorShape({s[1],1,1})&&bias->Shape()==scale->Shape(),"original channel affine broadcast");
 const int64_t spatial=s[2]*s[3],n=s.Size();ORT_RETURN_IF_NOT(spatial>0&&spatial%4==0&&n%256==0&&n/256<=65535,"exact complete-workgroup affine-silu");
 auto* y=context.Output(0,s);PwAffineSiluProgram p(narrow<uint32_t>(spatial));
 p.AddInputs({{x,ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,4},{scale,ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,1},{bias,ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,1}}).AddOutput({y,ProgramTensorMetadataDependency::TypeAndRank,ProgramOutput::Flatten,4}).SetWorkgroupSize(64).SetDispatchGroupSize(narrow<uint32_t>(n/256)).CacheHint(spatial);
 std::cerr<<"PW_AFFINE_SILU_EXACT_CALL n="<<n<<" c="<<s[1]<<" spatial="<<spatial<<"\n";return context.RunProgram(p);
}
}}}
