namespace onnxruntime { namespace webgpu { namespace high_profile {
class PwFilmAffineSiluProgram final: public Program<PwFilmAffineSiluProgram> {
public: PwFilmAffineSiluProgram(uint32_t spatial,uint32_t stage):Program{"high_profile::PwFilmAffineSiluExactSixBarrier"},spatial_{spatial},stage_{stage}{}
Status GenerateShaderCode(ShaderHelper& shader) const override {
 const auto& x=shader.AddInput("x",ShaderUsage::UseValueTypeAlias);const auto& scale=shader.AddInput("scale",ShaderUsage::UseValueTypeAlias);const auto& bias=shader.AddInput("bias",ShaderUsage::UseValueTypeAlias);const auto& condition=shader.AddInput("condition",ShaderUsage::UseValueTypeAlias);const auto& shift=shader.AddInput("shift",ShaderUsage::UseValueTypeAlias);const auto& y=shader.AddOutput("y",ShaderUsage::UseValueTypeAlias);
 auto& body=shader.MainFunctionBody();body<<"let channel=global_idx*4u/"<<spatial_<<"u;\n";
 const char* names[]={"norm_mul","norm_add","condition_mul","shift_add","sigmoid","self_mul"};
 for(uint32_t j=0;j<stage_;j++)shader.AdditionalImplementation()<<"var<workgroup> "<<names[j]<<"_scratch:array<vec4<f32>,64>;\n";
 body<<"let a_mul="<<x.GetByOffset("global_idx")<<";let b_mul=vec4<f32>("<<scale.GetByOffset("channel")<<");\nnorm_mul_scratch[local_idx]=a_mul*b_mul;\nworkgroupBarrier();\n";
 if(stage_>=2)body<<"let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>("<<bias.GetByOffset("channel")<<");\nnorm_add_scratch[local_idx]=a_add+b_add;\nworkgroupBarrier();\n";
 if(stage_>=3)body<<"let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>("<<condition.GetByOffset("channel")<<");\ncondition_mul_scratch[local_idx]=a_condition*b_condition;\nworkgroupBarrier();\n";
 if(stage_>=4)body<<"let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>("<<shift.GetByOffset("channel")<<");\nshift_add_scratch[local_idx]=a_shift+b_shift;\nworkgroupBarrier();\n";
 if(stage_>=5)body<<"let a=shift_add_scratch[local_idx];\nsigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));\nworkgroupBarrier();\n";
 if(stage_>=6)body<<"let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];\nself_mul_scratch[local_idx]=final_a*final_b;\nworkgroupBarrier();\n";
 body<<y.SetByOffset("global_idx",std::string(names[stage_-1])+"_scratch[local_idx]")<<"\n";return Status::OK();
}
private:uint32_t spatial_,stage_;
};
static Status PwSelectedFilmAffineSiluCompute(ComputeContext& context,uint32_t stage){
 ORT_RETURN_IF_NOT(context.InputCount()==5&&stage>=1&&stage<=6,"film fixed5bindings/stage");const auto* x=context.Input<Tensor>(0);const auto& s=x->Shape();
 ORT_RETURN_IF_NOT(s==TensorShape({1,8,288,384})||s==TensorShape({1,8,576,768})||s==TensorShape({1,16,144,192})||s==TensorShape({1,16,288,384})||s==TensorShape({1,32,144,192}),"film frozen5NCHWshape guard");
 ORT_RETURN_IF_NOT(x->IsDataType<float>(),"film f32");for(int j=1;j<5;j++){const auto* t=context.Input<Tensor>(j);ORT_RETURN_IF_NOT(t->IsDataType<float>()&&t->Shape()==(j<=2?TensorShape({s[1],1,1}):TensorShape({1,s[1],1,1})),"film originalchannelbroadcast guard");}
 const int64_t spatial=s[2]*s[3],n=s.Size();ORT_RETURN_IF_NOT(spatial%4==0&&n%256==0&&n/256<=65535,"film completeworkgroups");auto* y=context.Output(0,s);PwFilmAffineSiluProgram p(narrow<uint32_t>(spatial),stage);
 for(int j=0;j<5;j++)p.AddInput({context.Input<Tensor>(j),ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,j==0?4:1});
 p.AddOutput({y,ProgramTensorMetadataDependency::TypeAndRank,ProgramOutput::Flatten,4}).SetWorkgroupSize(64).SetDispatchGroupSize(narrow<uint32_t>(n/256)).CacheHint(spatial,stage);
 std::cerr<<"PW_FILM_AFFINE_SILU_EXACT_CALL stage="<<stage<<" n="<<n<<" c="<<s[1]<<" spatial="<<spatial<<"\n";return context.RunProgram(p);
}
}}}
