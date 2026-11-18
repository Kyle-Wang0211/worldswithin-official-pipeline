                                                                           
                                                                                           
#include "pw_grid_group_mean.h"
#include "core/providers/webgpu/program.h"
#include "core/providers/webgpu/webgpu_kernel.h"
#include "core/providers/webgpu/shader_helper.h"
#include "onnx/defs/schema.h"
#include "onnx/defs/shape_inference.h"
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>
namespace onnxruntime { namespace webgpu { namespace low_profile {
class PwGridGroupMeanProgram final : public Program<PwGridGroupMeanProgram> {
 public:
 PwGridGroupMeanProgram(uint32_t c,uint32_t h,uint32_t w,uint32_t g,uint32_t d,uint32_t oh=0,uint32_t ow=0):Program{"low_profile::PwGridSampleGroupMean"},c_{c},h_{h},w_{w},g_{g},d_{d},oh_{oh?oh:h},ow_{ow?ow:w}{}
 WEBGPU_PROGRAM_DEFINE_UNIFORM_VARIABLES({"channels",ProgramUniformVariableDataType::Uint32});
 Status GenerateShaderCode(ShaderHelper& shader) const override {
  const auto& x=shader.AddInput("x",ShaderUsage::UseUniform|ShaderUsage::UseValueTypeAlias);
  const auto& grid=shader.AddInput("grid",ShaderUsage::UseUniform);
  const auto& ref=shader.AddInput("reference",ShaderUsage::UseUniform);
  const auto& y=shader.AddOutput("y",ShaderUsage::UseUniform);
  auto& extra=shader.AdditionalImplementation();
  extra<<"const C="<<c_<<"u; const H="<<h_<<"u; const W="<<w_<<"u; const G="<<g_<<"u; const D="<<d_<<"u;\n";
  extra<<"fn gs_pixel(img_base: u32, r: i32, col: i32) -> f32 {\n"
       <<" if(r < 0 || r >= i32(H) || col < 0 || col >= i32(W)){return 0.0;}\n"
       <<" return f32("<<x.GetByOffset("img_base+u32(r)*W+u32(col)")<<");\n}\n"
       <<"fn gs_denormalize(n:f32,length:u32)->f32 { return (n + 1.0) * 0.5 * f32(length - 1u); }\n"
       <<"fn gs_sample(c:u32,dhw:u32)->f32 {\n"
       <<" let grid_base=dhw*2u;\n"
       <<" let gx=f32("<<grid.GetByOffset("grid_base")<<");\n"
       <<" let gy=f32("<<grid.GetByOffset("grid_base+1u")<<");\n"
       <<" let px=gs_denormalize(gx,W); let py=gs_denormalize(gy,H);\n"
       <<" let img_base=c*H*W;\n"
       <<" let x1=i32(floor(px)); let y1=i32(floor(py)); let x2=x1+1; let y2=y1+1;\n"
       <<" let dx1=px-f32(x1); let dx2=1.0-dx1; let dy1=py-f32(y1); let dy2=1.0-dy1;\n"
       <<" let p11=gs_pixel(img_base,y1,x1); let p12=gs_pixel(img_base,y1,x2);\n"
       <<" let p21=gs_pixel(img_base,y2,x1); let p22=gs_pixel(img_base,y2,x2);\n"
       <<" let result = dy2 * (dx2 * p11 + dx1 * p12) + dy1 * (dx2 * p21 + dx1 * p22);\n"
       <<" return result;\n}\n"
       <<"var<workgroup> sampled_scratch:array<f32,"<<(c_/g_)*64<<">;\n"
       <<"var<workgroup> product_scratch:array<f32,"<<(c_/g_)*64<<">;\n";
  auto& body=shader.MainFunctionBody();
  body<<"let group=global_idx/"<<d_*oh_*ow_<<"u; let dhw=global_idx%"<<d_*oh_*ow_<<"u; let hw=dhw%"<<oh_*ow_<<"u;\n"
      <<"for(var q=0u;q<uniforms.channels;q++){ let c=group*"<<c_/g_<<"u+q; sampled_scratch[q*64u+local_idx]=gs_sample(c,dhw); }\n"
      <<"workgroupBarrier();\n"
      <<"for(var q=0u;q<uniforms.channels;q++){ let c=group*"<<c_/g_<<"u+q; let sampled=sampled_scratch[q*64u+local_idx];\n"
      <<"let current_element=sampled*"<<"f32("<<ref.GetByOffset("c*"+std::to_string(oh_*ow_)+"u+hw")<<")"<<"; product_scratch[q*64u+local_idx]=current_element; }\n"
      <<"workgroupBarrier();\n"
      <<"var sum=f32(0);\nfor(var q=0u;q<uniforms.channels;q++){sum += f32(product_scratch[q*64u+local_idx]);}\n"
      << y.SetByOffset("global_idx","sum / f32(uniforms.channels)")<<"\n";
  return Status::OK();
 }
 private:uint32_t c_,h_,w_,g_,d_,oh_,ow_;
};
class PwGridSampleGroupMean final : public WebGpuKernel {
 public:
 explicit PwGridSampleGroupMean(const OpKernelInfo& info):WebGpuKernel(info),groups_{info.GetAttrOrDefault<int64_t>("groups",0)},depth_{info.GetAttrOrDefault<int64_t>("depth",0)}{}
 Status ComputeInternal(ComputeContext& context) const override {
  const auto* x=context.Input<Tensor>(0);const auto* grid=context.Input<Tensor>(1);const auto* ref=context.Input<Tensor>(2);
  ORT_RETURN_IF_NOT((x->IsDataType<float>()||x->IsDataType<MLFloat16>())&&(ref->IsDataType<float>()||ref->IsDataType<MLFloat16>())&&grid->IsDataType<float>(),"GSGM feature/reference must be F32/F16; grid coordinates must be FP32");
  const auto& xs=x->Shape();const auto& gs=grid->Shape();const auto& rs=ref->Shape();
  ORT_RETURN_IF_NOT(xs.NumDimensions()==4&&gs.NumDimensions()==4&&rs.NumDimensions()==6,"experimental rank mismatch");
  ORT_RETURN_IF_NOT(xs[0]==1&&groups_>0&&depth_>0&&xs[1]%groups_==0&&xs[2]>0&&xs[3]>0,"experimental shape mismatch");
  const int64_t channels=xs[1]/groups_;ORT_RETURN_IF_NOT(channels==12||channels==4||(channels==8&&groups_==4&&depth_==1&&xs==TensorShape({1,32,144,192})),"unsupported frozen channel family");
  const int64_t oh=rs[4],ow=rs[5];
  ORT_RETURN_IF_NOT(oh>0&&ow>0,"positive output spatial shape required");
  ORT_RETURN_IF_NOT(gs[0]==1&&gs[1]==depth_*oh&&gs[2]==ow&&gs[3]==2,"grid shape mismatch");
  ORT_RETURN_IF_NOT(rs==TensorShape({1,groups_,channels,1,oh,ow}),"reference group order mismatch");
  TensorShape ys{1,groups_,depth_,oh,ow};const int64_t count=ys.Size();
  const bool grouped_island=depth_==48||depth_==4;
  ORT_RETURN_IF_NOT(count>0&&count%64==0,"unsupported exact workgroup shape");
  uint32_t dispatch_x=narrow<uint32_t>(count/64),dispatch_y=1;
  if(grouped_island){
    ORT_RETURN_IF_NOT(groups_==4&&((depth_==48&&xs[1]==48&&((xs[2]==144&&xs[3]==192)||(xs[2]==72&&xs[3]==96)))||(depth_==4&&xs[1]==16&&((xs[2]==576&&xs[3]==768)||(xs[2]==288&&xs[3]==384)))),"unsupported frozen grouped depth island");
    dispatch_y=depth_==48?8u:4u;ORT_RETURN_IF_NOT(dispatch_x%dispatch_y==0,"grouped exact dispatch division");dispatch_x/=dispatch_y;
  }
  ORT_RETURN_IF_NOT(dispatch_x<=65535&&dispatch_y<=65535,"unsupported exact dispatch limit");
  auto* out=context.Output(0,ys);const auto c=narrow<uint32_t>(xs[1]),h=narrow<uint32_t>(xs[2]),w=narrow<uint32_t>(xs[3]),g=narrow<uint32_t>(groups_),d=narrow<uint32_t>(depth_);
  PwGridGroupMeanProgram program(c,h,w,g,d,narrow<uint32_t>(oh),narrow<uint32_t>(ow));
  program.AddInputs({{x,ProgramTensorMetadataDependency::TypeAndRank},{grid,ProgramTensorMetadataDependency::TypeAndRank},{ref,ProgramTensorMetadataDependency::TypeAndRank}})
      .AddOutput({out,ProgramTensorMetadataDependency::TypeAndRank}).SetWorkgroupSize(64)
      .SetDispatchGroupSize(dispatch_x,dispatch_y).CacheHint(c,h,w,g,d).AddUniformVariable({narrow<uint32_t>(channels)});
  std::cerr<<(grouped_island?"PW_GROUPED_DEPTH_ISLAND_CALL groups=":"PW_NATIVE_FUSED_CALL groups=")<<groups_<<" depth="<<depth_<<" count="<<count<<"\n";
  if(oh!=h||ow!=w)program.CacheHint(c,h,w,g,d,narrow<uint32_t>(oh),narrow<uint32_t>(ow));
   return context.RunProgram(program);
 }
 private:int64_t groups_,depth_;
};
}}}
#include "pw_weighted_accumulate.inc"
#include "pw_fixed_loader.inc"
