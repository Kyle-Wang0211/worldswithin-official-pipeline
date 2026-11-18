                                                                                                 
#include "core/providers/webgpu/math/gemm_utils.h"
#include "pw_gru_selected.inc"
#include "pw_affine_silu.inc"
#include "pw_film_affine_silu.inc"
#include "pw_s3_selected.inc"
namespace onnxruntime { namespace webgpu { namespace low_profile {
#include "pw_fixed_loader_specs.inc"
static const PwFixedLoaderSpec& PwLowS1Spec(){
 static const PwFixedLoaderSpec v={{TensorShape{1,4,48,72,96},TensorShape{8,12,3,3}},TensorShape{48,8,72,96},1,false,48,12,72,96,8,72,96,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{let sourceDepth=i0+i1/4-1;if(sourceDepth<0||sourceDepth>=48){return 0.0;}return readL0(((i1%4)*48+sourceDepth)*6912+i2*96+i3);}
)PWLF","readT0"};return v;
}
static const PwFixedLoaderSpec& PwLowS1Lookup(size_t id){return id==374?PwLowS1Spec():PwFixedLoaderSpecs().at(id);}

class PwFixedLoaderProgram final : public Program<PwFixedLoaderProgram> {
 public:explicit PwFixedLoaderProgram(uint32_t id):Program{"low_profile::PwFixedConvLoaderORTTile32"},id_{id}{}
 WEBGPU_PROGRAM_DEFINE_UNIFORM_VARIABLES({"dim_a_outer",ProgramUniformVariableDataType::Uint32},{"dim_b_outer",ProgramUniformVariableDataType::Uint32},{"dim_inner",ProgramUniformVariableDataType::Uint32},{"logical_dispatch_x",ProgramUniformVariableDataType::Uint32},{"logical_dispatch_y",ProgramUniformVariableDataType::Uint32},{"logical_dispatch_z",ProgramUniformVariableDataType::Uint32});
 Status GenerateShaderCode(ShaderHelper& shader) const override {
  if(id_==211){
   const auto& low=shader.AddInput("low",ShaderUsage::UseValueTypeAlias);const auto& high=shader.AddInput("high",ShaderUsage::UseValueTypeAlias);const auto& y=shader.AddOutput("y",ShaderUsage::UseValueTypeAlias);
   shader.MainFunctionBody()<<"let i=global_idx;if(i<28311552u){let c=i/442368u;let yy=i/768u%576u;let xx=i%768u;let a="<<low.GetByOffset("c*110592u+(yy/2u)*384u+xx/2u")<<";let b="<<high.GetByOffset("i")<<";let value=a+b;"<<y.SetByOffset("i","value")<<"}\n";return Status::OK();
  }
  if(id_==213){
   const auto& low=shader.AddInput("low",ShaderUsage::UseValueTypeAlias);const auto& high=shader.AddInput("high",ShaderUsage::UseValueTypeAlias);const auto& y=shader.AddOutput("y",ShaderUsage::UseValueTypeAlias);
   shader.MainFunctionBody()<<"let i=global_idx;if(i<7077888u){let c=i/110592u;let yy=i/384u%288u;let xx=i%384u;let a="<<low.GetByOffset("c*27648u+(yy/2u)*192u+xx/2u")<<";let b="<<high.GetByOffset("i")<<";let value=a+b;"<<y.SetByOffset("i","value")<<"}\n";return Status::OK();
  }
  const auto& v=PwLowS1Lookup(id_);auto& code=shader.AdditionalImplementation();
  for(size_t j=0;j<v.data_inputs;j++){
   const auto& x=shader.AddInput("leaf"+std::to_string(j),ShaderUsage::UseValueTypeAlias);
   code<<"fn readL"<<j<<"(index:i32)->f32{return "<<x.GetByOffset("u32(index)")<<";}\n";
  }
  const auto& weight=shader.AddInput("weight",ShaderUsage::UseValueTypeAlias);
  const ShaderVariableHelper* bias=nullptr;
  if(v.has_bias)bias=&shader.AddInput("bias",ShaderUsage::UseValueTypeAlias);
  const auto& out=shader.AddOutput("out",ShaderUsage::UseValueTypeAlias);
  code<<"const C="<<v.c<<";const IH="<<v.ih<<";const IW="<<v.iw<<";const O="<<v.o<<";const OH="<<v.oh<<";const OW="<<v.ow<<";const KY="<<v.ky<<";const KX="<<v.kx<<";const SY="<<v.sy<<";const SX="<<v.sx<<";const DY="<<v.dy<<";const DX="<<v.dx<<";const PY="<<v.py<<";const PX="<<v.px<<";const P=OH*OW;\n"<<v.reader<<"\n";
  code<<"fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let col=colIn*4;\n";
  if(!(v.fit_a&&v.fit_inner))code<<"if(row<i32(uniforms.dim_a_outer) && col<i32(uniforms.dim_inner)){\n";
  code<<R"WGSL(
 let inChannels=C;
 let outWidth=OW;
 let outRow=row/outWidth;
 let outCol=row%outWidth;
 let WRow=col/(KX*inChannels);
 let WCol=col/inChannels%KX;
 let xRow=outRow*SY+DY*WRow-PY;
 let xCol=outCol*SX+DX*WCol-PX;
 let xCh=col%inChannels;
 var resData=vec4<f32>(0.0);
 if(xRow>=0 && xRow<IH && xCol>=0 && xCol<IW){
)WGSL";
  code<<"resData=vec4<f32>(";
  for(int k=0;k<4;k++){if(k)code<<",";code<<v.root<<"(batch,xCh+"<<k<<",xRow,xCol)";}
  code<<");}\nreturn resData;\n";
  if(!(v.fit_a&&v.fit_inner))code<<"}\nreturn vec4<f32>(0.0);\n";
  code<<"}\nfn loadWeight(row:i32,o:i32)->f32{let ch=row%C;let k=row/C;let y=k/KX;let x=k%KX;return "<<weight.GetByOffset("u32(((o*C+ch)*KY+y)*KX+x)")<<";}\nfn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{let col=colIn*4;\n";
  if(!(v.fit_b&&v.fit_inner))code<<"if(row<i32(uniforms.dim_inner) && col<i32(uniforms.dim_b_outer)){\n";
  code<<"return vec4<f32>(loadWeight(row,col),loadWeight(row,col+1),loadWeight(row,col+2),loadWeight(row,col+3));\n";
  if(!(v.fit_b&&v.fit_inner))code<<"}\nreturn vec4<f32>(0.0);\n";
  code<<"}\nfn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){let col=colIn*4;if(row<i32(uniforms.dim_a_outer) && col<i32(uniforms.dim_b_outer)){var value=valueIn;\n";
  if(v.has_bias){code<<"value=value+vec4<f32>(";for(int k=0;k<4;k++){if(k)code<<",";code<<bias->GetByOffset("u32(col+"+std::to_string(k)+")");}code<<");\n";}
  for(int k=0;k<4;k++)code<<out.SetByOffset("u32((batch*O+col+"+std::to_string(k)+")*P+row)","value["+std::to_string(k)+"]")<<"\n";
  code<<"}}\n";
  if ((id_==0 || id_==3 || id_==4) && v.n==1 && v.c==32 && v.ih==576 && v.iw==768 && v.o==8 && v.oh==576 && v.ow==768 && v.ky==7 && v.kx==7 && v.sy==1 && v.sx==1 && v.dy==1 && v.dx==1 && v.py==3 && v.px==3 && v.data_inputs==3 && v.has_bias && v.fit_a && !v.fit_b && v.fit_inner) {
    std::cerr<<"PW_FAMILY034_GENERATE spec="<<id_<<"\n";
    code<<R"PWDC(var<workgroup> halo:array<vec4<f32>,1568>;
var<workgroup> direct_B:array<array<vec4<f32>,2>,32>;
)PWDC";
    shader.MainFunctionBody()<<R"PWDC(  let logical_workgroup_id_z = workgroup_idx / (uniforms.logical_dispatch_x * uniforms.logical_dispatch_y);
  let logical_workgroup_id_y = (workgroup_idx % (uniforms.logical_dispatch_x * uniforms.logical_dispatch_y)) / uniforms.logical_dispatch_x;
  let logical_workgroup_id_x = (workgroup_idx % (uniforms.logical_dispatch_x * uniforms.logical_dispatch_y)) % uniforms.logical_dispatch_x;
  let logical_workgroup_id = vec3u(logical_workgroup_id_x, logical_workgroup_id_y, logical_workgroup_id_z);
  const workgroupSize = vec3u(workgroup_size_x, workgroup_size_y, workgroup_size_z);
  let logical_global_id = logical_workgroup_id * workgroupSize + local_id;
let lid=local_id;let tid=local_id.y*8u+local_id.x;let wid=logical_workgroup_id;

 let batch=i32(wid.z);let lx=i32(lid.x);let ly=i32(lid.y);let baseY=i32(wid.y)*8;let baseX=i32(wid.x)*8;
 for(var i=i32(tid);i<1568;i=i+64){let cv=i%8;let hx=i/8%14;let hy=i/(8*14);let y=baseY+hy-3;let x=baseX+hx-3;var v=vec4<f32>(0.0);if(y>=0&&y<IH&&x>=0&&x<IW){let ch=cv*4;v=vec4<f32>(readT0(batch,ch,y,x),readT0(batch,ch+1,y,x),readT0(batch,ch+2,y,x),readT0(batch,ch+3,y,x));}halo[i]=v;}
 workgroupBarrier();var acc0=vec4<f32>(0.0);var acc1=vec4<f32>(0.0);
 for(var t=0;t<49;t=t+1){let wr=i32(tid)/2;let wc=i32(tid)%2;direct_B[wr][wc]=mm_readB(batch,t*32+wr,wc);workgroupBarrier();let ky=t/7;let kx=t%7;
  for(var k=0;k<8;k=k+1){let ACached=halo[((ly+ky)*14+lx+kx)*8+k];
   let B00=direct_B[k*4][0];let B01=direct_B[k*4+1][0];let B02=direct_B[k*4+2][0];let B03=direct_B[k*4+3][0];
   acc0=B00*ACached.x+acc0;acc0=B01*ACached.y+acc0;acc0=B02*ACached.z+acc0;acc0=B03*ACached.w+acc0;
   let B10=direct_B[k*4][1];let B11=direct_B[k*4+1][1];let B12=direct_B[k*4+2][1];let B13=direct_B[k*4+3][1];
   acc1=B10*ACached.x+acc1;acc1=B11*ACached.y+acc1;acc1=B12*ACached.z+acc1;acc1=B13*ACached.w+acc1;
  }workgroupBarrier();
 }
 let row=(baseY+ly)*OW+baseX+lx;mm_write(batch,row,0,acc0);mm_write(batch,row,1,acc1);
)PWDC";
    return Status::OK();
  }
  if(id_==7){
   shader.AdditionalImplementation()<<R"PW7(const K=144;
var<workgroup> halo:array<vec4<f32>,400>;
var<workgroup> direct_B:array<array<vec4<f32>,9>,32>;
)PW7";
   shader.MainFunctionBody()<<R"PW7(
let batch=i32(workgroup_id.z);let lx=i32(local_id.x);let ly=i32(local_id.y);let baseY=i32(workgroup_id.y)*8;let baseX=i32(workgroup_id.x)*8;
for(var i=i32(local_idx);i<400;i=i+64){let cv=i%4;let hx=i/4%10;let hy=i/(4*10);let y=baseY+hy-1;let x=baseX+hx-1;var v=vec4<f32>(0.0);if(y>=0&&y<IH&&x>=0&&x<IW){let ch=cv*4;v=vec4<f32>(readT0(batch,ch,y,x),readT0(batch,ch+1,y,x),readT0(batch,ch+2,y,x),readT0(batch,ch+3,y,x));}halo[i]=v;}
workgroupBarrier();var acc:array<vec4<f32>,9>;
for(var t=0;t<5;t=t+1){for(var i=i32(local_idx);i<32*9;i=i+64){let wr=i/9;let wc=i%9;direct_B[wr][wc]=mm_readB(batch,t*32+wr,wc);}workgroupBarrier();
for(var k=0;k<8;k=k+1){let col=t*32+k*4;var ACached=vec4<f32>(0.0);if(col<K){let ky=col/(KX*C);let kx=col/C%KX;let cv=col%C/4;ACached=halo[((ly+ky)*10+lx+kx)*4+cv];}for(var g=0;g<9;g=g+1){let B0=direct_B[k*4][g];let B1=direct_B[k*4+1][g];let B2=direct_B[k*4+2][g];let B3=direct_B[k*4+3][g];acc[g]=B0*ACached.x+acc[g];acc[g]=B1*ACached.y+acc[g];acc[g]=B2*ACached.z+acc[g];acc[g]=B3*ACached.w+acc[g];}}workgroupBarrier();}
let row=(baseY+ly)*OW+baseX+lx;for(var g=0;g<9;g=g+1){mm_write(batch,row,g,acc[g]);})PW7";
   return Status::OK();
  }
  return MakeMatMulPackedVec4Source(shader,InlinedVector<int64_t>{4,4,1},8,8,"f32",nullptr,false,false,1.0f,true,4,32);
 }
 private:uint32_t id_;
};
                                                                                       
class PwConcatConv7 final : public WebGpuKernel {
 public:explicit PwConcatConv7(const OpKernelInfo& info):WebGpuKernel(info),id_{info.GetAttrOrDefault<int64_t>("spec",-1)},film_stage_{6}{if(id_==378){const auto& name=info.node().Name();const std::string tag="/_diag_stage_";auto k=name.rfind(tag);if(k!=std::string::npos){ORT_ENFORCE(k+tag.size()+1==name.size()&&name.back()>='1'&&name.back()<='5',"LOW film diagnosticstage guard");film_stage_=static_cast<uint32_t>(name.back()-'0');}}if(id_==375){ORT_ENFORCE(info.node().Name()=="/core/update_block_depth3/unet/downs.0.1/downs.0.1.1/Conv/PwS3DirectRead","LOW S3 exact node guard");}}
 Status ComputeInternal(ComputeContext& context) const override {
  if(id_==377){ORT_RETURN_IF_NOT(context.InputCount()==3,"LOW Affine3bindings");const auto& s=context.Input<Tensor>(0)->Shape();ORT_RETURN_IF_NOT(s==TensorShape({1,8,144,192})||s==TensorShape({1,8,288,384})||s==TensorShape({1,16,72,96})||s==TensorShape({1,16,144,192})||s==TensorShape({1,32,72,96}),"LOW actualAffine5shape guard");return PwSelectedAffineSiluCompute(context);}
  if(id_==378){return PwSelectedFilmAffineSiluCompute(context,film_stage_);}

  if(id_==375){ORT_RETURN_IF_NOT(context.InputCount()==3,"selected LOW S3 requires3original inputs");return PwSelectedS3DirectReadCompute(context);}
  if(id_==372){ORT_RETURN_IF_NOT(context.InputCount()==3,"selected GRU372 requires3f32 inputs");return PwSelectedGruTailCompute(context);}
  ORT_RETURN_IF_NOT(id_==374||(id_>=0&&static_cast<size_t>(id_)<PwFixedLoaderSpecs().size()),"unknown fixed Conv loader spec");const auto& v=PwLowS1Lookup(static_cast<size_t>(id_));
  ORT_RETURN_IF_NOT(context.InputCount()==static_cast<int>(v.inputs.size())&&v.inputs.size()<=7,"fixed loader binding count");
  PwFixedLoaderProgram p(narrow<uint32_t>(id_));
  for(int j=0;j<context.InputCount();j++){const auto* x=context.Input<Tensor>(j);ORT_RETURN_IF_NOT(x->Shape()==v.inputs[j],"fixed loader input shape id=",id_," binding=",j);p.AddInput({x,ProgramTensorMetadataDependency::TypeAndRank,ProgramInput::Flatten,1});}
  ORT_RETURN_IF_NOT(v.c%4==0&&v.o%4==0&&v.oh*v.ow>8,"fixed vec4 helper branch");auto* y=context.Output(0,v.output);uint32_t gx=narrow<uint32_t>((v.o+31)/32),gy=narrow<uint32_t>((v.oh*v.ow+31)/32),gz=narrow<uint32_t>(v.n);
  if ((id_==0 || id_==3 || id_==4) && v.n==1 && v.c==32 && v.ih==576 && v.iw==768 && v.o==8 && v.oh==576 && v.ow==768 && v.ky==7 && v.kx==7 && v.sy==1 && v.sx==1 && v.dy==1 && v.dx==1 && v.py==3 && v.px==3 && v.data_inputs==3 && v.has_bias && v.fit_a && !v.fit_b && v.fit_inner) {gx=narrow<uint32_t>(v.ow/8);gy=narrow<uint32_t>(v.oh/8);std::cerr<<"PW_FAMILY034_DISPATCH spec="<<id_<<"\n";}
  if(id_==7){
   ORT_RETURN_IF_NOT(v.n==1&&v.c==16&&v.o==36&&v.ih==576&&v.iw==768&&v.oh==576&&v.ow==768&&v.ky==3&&v.kx==3&&v.sy==1&&v.sx==1&&v.dy==1&&v.dx==1&&v.py==1&&v.px==1&&v.has_bias,"spec7 immutable shape contract");
   gx=96;gy=72;gz=1;p.CacheHint("PW_SPEC7_SPATIAL_HALO_V1");
   std::cerr<<"PW_SPEC7_DIRECT_HALO_CALL spec=7 dispatch=96,72,1 K=144/160\n";
  }
  if(id_==211){gx=442368;gy=1;gz=1;}
  if(id_==213){gx=110592;gy=1;gz=1;}
  if(id_==374)std::cerr<<"PW_LOW_S1_DIRECT_READ_CALL spec=374\n";
  if(id_==212)std::cerr<<"PW_FEATURE_LOW_RESIZE_ADD_DIRECT_CALL spec=212\n";
  if(id_==213)std::cerr<<"PW_FEATURE_LOW_RESIZE_ADD_LOAD_CALL spec=213\n";
  if(id_==210)std::cerr<<"PW_FEATURE_RESIZE_ADD_DIRECT_CALL spec=210\n";
  if(id_==211)std::cerr<<"PW_FEATURE_RESIZE_ADD_LOAD_CALL spec=211\n";
  p.AddOutput({y,ProgramTensorMetadataDependency::TypeAndRank,ProgramOutput::Flatten,1}).SetWorkgroupSize(8,8,1).SetDispatchGroupSize(gx,gy,gz).CacheHint(id_).AddUniformVariables({{narrow<uint32_t>(v.oh*v.ow)},{narrow<uint32_t>(v.o)},{narrow<uint32_t>(v.ky*v.kx*v.c)},{gx},{gy},{gz}});
  std::cerr<<"PW_FIXED_CONV_LOADER_CALL spec="<<id_<<"\n";return context.RunProgram(p);
 }
 private:int64_t id_;uint32_t film_stage_;
};
}}}
