                                                                                      
static const std::vector<FusedUpdateConfig> fused_update_configs = {
{16,{
{442368,0,0,{1,16,144,192},-1},
{884736,2,-1,{32,144,192},0},
{884736,2,-1,{32,144,192},1},
{110592,0,1,{1,4,144,192},-1},
{884736,2,-1,{32,144,192},0},
{884736,2,-1,{32,144,192},2},
{857088,2,-1,{31,144,192},0},
{884736,0,2,{1,32,144,192},-1},
{27648,0,3,{1,1,144,192},-1},
{442368,2,-1,{16,144,192},1},
{442368,2,-1,{16,144,192},0},
{16,0,4,{1,16,1,1},-1},
{16,0,5,{1,16,1,1},-1},
{442368,2,-1,{16,144,192},3},
{110592,2,-1,{16,72,96},0},
{110592,2,-1,{16,72,96},4},
{16,0,6,{1,16,1,1},-1},
{16,0,7,{1,16,1,1},-1},
{110592,2,-1,{16,72,96},5},
{221184,2,-1,{32,72,96},4},
{221184,0,8,{1,32,72,96},-1},
{221184,2,-1,{32,72,96},6},
{221184,2,-1,{32,72,96},7},
{221184,2,-1,{32,72,96},8},
{221184,2,-1,{32,72,96},6},
{221184,2,-1,{32,72,96},7},
{221184,1,0,{32,72,96},-1},
{221184,2,-1,{32,72,96},4},
{221184,2,-1,{32,72,96},6},
{221184,2,-1,{32,72,96},4},
{32,0,9,{1,32,1,1},-1},
{32,0,10,{1,32,1,1},-1},
{221184,2,-1,{32,72,96},7},
{221184,2,-1,{32,72,96},4},
{442368,2,-1,{16,144,192},0},
{442368,2,-1,{16,144,192},4},
{16,0,11,{1,16,1,1},-1},
{16,0,12,{1,16,1,1},-1},
{442368,2,-1,{16,144,192},6},
{442368,2,-1,{16,144,192},4},
{442368,2,-1,{16,144,192},3},
{442368,2,-1,{16,144,192},4},
{16,0,13,{1,16,1,1},-1},
{16,0,14,{1,16,1,1},-1},
{442368,2,-1,{16,144,192},6},
{442368,2,-1,{16,144,192},4},
{27648,1,1,{1,144,192},-1},
{27648,1,2,{1,144,192},-1},
{1024,2,-1,{1024},2},
{25920,2,-1,{25920},-1},
{236992,0,15,{236992},-1},
},{
{"2edccb5092a5c39f9ee11f1724926b6f4c4d007d97c01800512b0e41d324b186",{{50,false,4,"wts"},{1,true,1,"b0"},{0,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/encoder/convc1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=0u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[1152u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=max(value.x, 0.0);
b0[1u*27648u+pix]=max(value.y, 0.0);
b0[2u*27648u+pix]=max(value.z, 0.0);
b0[3u*27648u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*27648u+pix]=max(value.x, 0.0);
b0[5u*27648u+pix]=max(value.y, 0.0);
b0[6u*27648u+pix]=max(value.z, 0.0);
b0[7u*27648u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*27648u+pix]=max(value.x, 0.0);
b0[9u*27648u+pix]=max(value.y, 0.0);
b0[10u*27648u+pix]=max(value.z, 0.0);
b0[11u*27648u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*27648u+pix]=max(value.x, 0.0);
b0[13u*27648u+pix]=max(value.y, 0.0);
b0[14u*27648u+pix]=max(value.z, 0.0);
b0[15u*27648u+pix]=max(value.w, 0.0);
}
case 4u:{
b0[16u*27648u+pix]=max(value.x, 0.0);
b0[17u*27648u+pix]=max(value.y, 0.0);
b0[18u*27648u+pix]=max(value.z, 0.0);
b0[19u*27648u+pix]=max(value.w, 0.0);
}
case 5u:{
b0[20u*27648u+pix]=max(value.x, 0.0);
b0[21u*27648u+pix]=max(value.y, 0.0);
b0[22u*27648u+pix]=max(value.z, 0.0);
b0[23u*27648u+pix]=max(value.w, 0.0);
}
case 6u:{
b0[24u*27648u+pix]=max(value.x, 0.0);
b0[25u*27648u+pix]=max(value.y, 0.0);
b0[26u*27648u+pix]=max(value.z, 0.0);
b0[27u*27648u+pix]=max(value.w, 0.0);
}
case 7u:{
b0[28u*27648u+pix]=max(value.x, 0.0);
b0[29u*27648u+pix]=max(value.y, 0.0);
b0[30u*27648u+pix]=max(value.z, 0.0);
b0[31u*27648u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,144}},
{"51d134fd986797da22c5bb77e419619cad64414e212eb33ae3dcef3e2b13efa9",{{50,false,4,"wts"},{2,true,1,"b0"},{1,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/encoder/convc2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=1168u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[3472u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=max(value.x, 0.0);
b0[1u*27648u+pix]=max(value.y, 0.0);
b0[2u*27648u+pix]=max(value.z, 0.0);
b0[3u*27648u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*27648u+pix]=max(value.x, 0.0);
b0[5u*27648u+pix]=max(value.y, 0.0);
b0[6u*27648u+pix]=max(value.z, 0.0);
b0[7u*27648u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*27648u+pix]=max(value.x, 0.0);
b0[9u*27648u+pix]=max(value.y, 0.0);
b0[10u*27648u+pix]=max(value.z, 0.0);
b0[11u*27648u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*27648u+pix]=max(value.x, 0.0);
b0[13u*27648u+pix]=max(value.y, 0.0);
b0[14u*27648u+pix]=max(value.z, 0.0);
b0[15u*27648u+pix]=max(value.w, 0.0);
}
case 4u:{
b0[16u*27648u+pix]=max(value.x, 0.0);
b0[17u*27648u+pix]=max(value.y, 0.0);
b0[18u*27648u+pix]=max(value.z, 0.0);
b0[19u*27648u+pix]=max(value.w, 0.0);
}
case 5u:{
b0[20u*27648u+pix]=max(value.x, 0.0);
b0[21u*27648u+pix]=max(value.y, 0.0);
b0[22u*27648u+pix]=max(value.z, 0.0);
b0[23u*27648u+pix]=max(value.w, 0.0);
}
case 6u:{
b0[24u*27648u+pix]=max(value.x, 0.0);
b0[25u*27648u+pix]=max(value.y, 0.0);
b0[26u*27648u+pix]=max(value.z, 0.0);
b0[27u*27648u+pix]=max(value.w, 0.0);
}
case 7u:{
b0[28u*27648u+pix]=max(value.x, 0.0);
b0[29u*27648u+pix]=max(value.y, 0.0);
b0[30u*27648u+pix]=max(value.z, 0.0);
b0[31u*27648u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,288}},
{"5e8c89794ab4a996573430de914448c7914a7a189af36536902edb872499e86d",{{50,false,4,"wts"},{4,true,1,"b0"},{3,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/encoder/convd1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 4u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=36){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/12;let kx=(q/4)%3;let ch=q%4;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=36 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%4);let ky=u32(row/12);let kx=u32((row/4)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=3488u+(((grp*8u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[4064u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=max(value.x, 0.0);
b0[1u*27648u+pix]=max(value.y, 0.0);
b0[2u*27648u+pix]=max(value.z, 0.0);
b0[3u*27648u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*27648u+pix]=max(value.x, 0.0);
b0[5u*27648u+pix]=max(value.y, 0.0);
b0[6u*27648u+pix]=max(value.z, 0.0);
b0[7u*27648u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*27648u+pix]=max(value.x, 0.0);
b0[9u*27648u+pix]=max(value.y, 0.0);
b0[10u*27648u+pix]=max(value.z, 0.0);
b0[11u*27648u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*27648u+pix]=max(value.x, 0.0);
b0[13u*27648u+pix]=max(value.y, 0.0);
b0[14u*27648u+pix]=max(value.z, 0.0);
b0[15u*27648u+pix]=max(value.w, 0.0);
}
case 4u:{
b0[16u*27648u+pix]=max(value.x, 0.0);
b0[17u*27648u+pix]=max(value.y, 0.0);
b0[18u*27648u+pix]=max(value.z, 0.0);
b0[19u*27648u+pix]=max(value.w, 0.0);
}
case 5u:{
b0[20u*27648u+pix]=max(value.x, 0.0);
b0[21u*27648u+pix]=max(value.y, 0.0);
b0[22u*27648u+pix]=max(value.z, 0.0);
b0[23u*27648u+pix]=max(value.w, 0.0);
}
case 6u:{
b0[24u*27648u+pix]=max(value.x, 0.0);
b0[25u*27648u+pix]=max(value.y, 0.0);
b0[26u*27648u+pix]=max(value.z, 0.0);
b0[27u*27648u+pix]=max(value.w, 0.0);
}
case 7u:{
b0[28u*27648u+pix]=max(value.x, 0.0);
b0[29u*27648u+pix]=max(value.y, 0.0);
b0[30u*27648u+pix]=max(value.z, 0.0);
b0[31u*27648u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,36}},
{"c4bc1174be784b54a751eb3843ed4f0b5e669d0d051faacc0b77eeb037d10ffa",{{50,false,4,"wts"},{5,true,1,"b0"},{4,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/encoder/convd2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=4080u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[6384u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=max(value.x, 0.0);
b0[1u*27648u+pix]=max(value.y, 0.0);
b0[2u*27648u+pix]=max(value.z, 0.0);
b0[3u*27648u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*27648u+pix]=max(value.x, 0.0);
b0[5u*27648u+pix]=max(value.y, 0.0);
b0[6u*27648u+pix]=max(value.z, 0.0);
b0[7u*27648u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*27648u+pix]=max(value.x, 0.0);
b0[9u*27648u+pix]=max(value.y, 0.0);
b0[10u*27648u+pix]=max(value.z, 0.0);
b0[11u*27648u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*27648u+pix]=max(value.x, 0.0);
b0[13u*27648u+pix]=max(value.y, 0.0);
b0[14u*27648u+pix]=max(value.z, 0.0);
b0[15u*27648u+pix]=max(value.w, 0.0);
}
case 4u:{
b0[16u*27648u+pix]=max(value.x, 0.0);
b0[17u*27648u+pix]=max(value.y, 0.0);
b0[18u*27648u+pix]=max(value.z, 0.0);
b0[19u*27648u+pix]=max(value.w, 0.0);
}
case 5u:{
b0[20u*27648u+pix]=max(value.x, 0.0);
b0[21u*27648u+pix]=max(value.y, 0.0);
b0[22u*27648u+pix]=max(value.z, 0.0);
b0[23u*27648u+pix]=max(value.w, 0.0);
}
case 6u:{
b0[24u*27648u+pix]=max(value.x, 0.0);
b0[25u*27648u+pix]=max(value.y, 0.0);
b0[26u*27648u+pix]=max(value.z, 0.0);
b0[27u*27648u+pix]=max(value.w, 0.0);
}
case 7u:{
b0[28u*27648u+pix]=max(value.x, 0.0);
b0[29u*27648u+pix]=max(value.y, 0.0);
b0[30u*27648u+pix]=max(value.z, 0.0);
b0[31u*27648u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,288}},
{"15f2fbdc8d7207d88515bb12dc675ae1b4f522e9d86051fe9c83b7607b0847e4",{{50,false,4,"wts"},{6,true,1,"b0"},{2,false,1,"b1"},{5,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/encoder/output_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 64u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=576){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/192;let kx=(q/64)%3;let ch=q%64;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=576 || colIn*4>=31){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/192);let kx=u32((row/64)%3);let col=u32(colIn*4);let grp=col/32u;let mv=(col%32u)/4u;
 let ix=6400u+(((grp*64u+ch)*3u+ky)*3u+kx)*8u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=31){return;}
 let pix=u32(row);let value=valueIn+wts[11008u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=max(value.x, 0.0);
b0[1u*27648u+pix]=max(value.y, 0.0);
b0[2u*27648u+pix]=max(value.z, 0.0);
b0[3u*27648u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*27648u+pix]=max(value.x, 0.0);
b0[5u*27648u+pix]=max(value.y, 0.0);
b0[6u*27648u+pix]=max(value.z, 0.0);
b0[7u*27648u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*27648u+pix]=max(value.x, 0.0);
b0[9u*27648u+pix]=max(value.y, 0.0);
b0[10u*27648u+pix]=max(value.z, 0.0);
b0[11u*27648u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*27648u+pix]=max(value.x, 0.0);
b0[13u*27648u+pix]=max(value.y, 0.0);
b0[14u*27648u+pix]=max(value.z, 0.0);
b0[15u*27648u+pix]=max(value.w, 0.0);
}
case 4u:{
b0[16u*27648u+pix]=max(value.x, 0.0);
b0[17u*27648u+pix]=max(value.y, 0.0);
b0[18u*27648u+pix]=max(value.z, 0.0);
b0[19u*27648u+pix]=max(value.w, 0.0);
}
case 5u:{
b0[20u*27648u+pix]=max(value.x, 0.0);
b0[21u*27648u+pix]=max(value.y, 0.0);
b0[22u*27648u+pix]=max(value.z, 0.0);
b0[23u*27648u+pix]=max(value.w, 0.0);
}
case 6u:{
b0[24u*27648u+pix]=max(value.x, 0.0);
b0[25u*27648u+pix]=max(value.y, 0.0);
b0[26u*27648u+pix]=max(value.z, 0.0);
b0[27u*27648u+pix]=max(value.w, 0.0);
}
case 7u:{
b0[28u*27648u+pix]=max(value.x, 0.0);
b0[29u*27648u+pix]=max(value.y, 0.0);
b0[30u*27648u+pix]=max(value.z, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,31,576}},
{"9438147895eacebebcb5c5a979798c76388f4ba161ca504e621272ed042e4648",{{50,false,4,"wts"},{9,true,1,"b0"},{7,false,1,"b1"},{6,false,1,"b2"},{8,false,1,"b3"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/init_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  return v;
}
fn S2(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 63u) { return S1(c - 32u, y, x); }
  if (c < 64u) { return S2(c - 63u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=3136){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/448;let kx=(q/64)%7;let ch=q%64;let iy=oy+ky-3;let ix=ox+kx-3;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=3136 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/448);let kx=u32((row/64)%7);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=11024u+(((grp*64u+ch)*7u+ky)*7u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[23568u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,3136}},
{"508b12738800cb59a970acdfe5136826dd65d9e261648dfd04cfa96e27388212",{{50,false,4,"wts"},{10,true,1,"b0"},{9,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.0.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=23584u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[24160u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,144}},
{"8cc6d243b265f2f2570c65f5a14086acecfbb9b6aa14a6c505c72c287fb230f8",{{10,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[0u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"dc859311cf811104ab9813fcd009d1648cdbe6b088f74b9b478e406292db4e47",{{48,false,4,"coef"},{10,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[0u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"4d1728817ae628f27d3d12dd8428ba45afe293c3a3ee8410b1a04e59f0bd17e8",{{11,false,1,"t3"},{12,false,1,"t4"},{10,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.1986364126205444, 0.5594167113304138, 1.0380840301513672, 1.1380891799926758, 0.8133781552314758, 1.0343083143234253, 1.2137706279754639, 0.030133645981550217, 0.9016430974006653, 0.5148126482963562, 0.7006478309631348, 1.2527954578399658, 0.9384449124336243, 0.8976232409477234, 1.084325909614563, 1.2279353141784668);
    let P1 = array<f32, 16>(-0.13004527986049652, -0.15402673184871674, -0.24893417954444885, -0.39097949862480164, 0.04706350341439247, -0.0242949016392231, -0.6387122869491577, -0.5896123647689819, -0.1273721307516098, -0.053643472492694855, 0.1896776258945465, -0.3317961096763611, 0.11920168995857239, -0.03751483932137489, -0.26678842306137085, -0.16058798134326935);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"c04e00a2e600ad5f7e3fa3a91df7dd5afab9bfa0a3b8514ef97fcecf2a8cdf81",{{50,false,4,"wts"},{13,true,1,"b0"},{10,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.0.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=24176u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[24752u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,144}},
{"d53e06c928f5f01788b5b19f4a3095e8f0554757c52eb316791387780d9878cf",{{13,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[16u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"41187dd6fd5dba5aadeb00342b6ee15c7428cdd272521614f4050c842e4a36b2",{{48,false,4,"coef"},{13,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[16u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"f0887045add37b42310295f6ba880074bcbf41ee25b19667a44ccba2c1d1a500",{{13,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.1047250032424927, 0.685846209526062, 0.9526513814926147, 0.5178439021110535, 0.8143125176429749, 0.6715943813323975, 0.6923924684524536, 0.581524670124054, 0.44882750511169434, 0.4984413981437683, 0.2861297130584717, 1.3021514415740967, 0.31866008043289185, 0.8268529176712036, 0.5300660729408264, 0.5082259178161621);
    let P1 = array<f32, 16>(0.08633971959352493, -0.5057839155197144, -0.27021822333335876, -0.3934597373008728, -0.3684293031692505, -0.09857098758220673, -0.24548020958900452, -0.485678106546402, -0.035703208297491074, -0.452858567237854, -0.1595015972852707, 0.32351166009902954, -0.19372627139091492, -0.1065969318151474, 0.4007500410079956, -0.2689298391342163);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"b2033e8be9e78d333e1ac363f6fb764bfb52ba980955eb7dd47cace2b212462f",{{50,false,4,"wts"},{14,true,1,"b0"},{13,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.0.1/downs.0.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c / 4u; let sy = 2u * u32(y) + ((c % 4u) / 2u); let sx = 2u * u32(x) + (c % 2u);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 64u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=64){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/64;let kx=(q/64)%1;let ch=q%64;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=64 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/64);let kx=u32((row/64)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=24768u+(((grp*64u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[25024u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,16,64}},
{"f1f64e722e50a841439a0b92e66c5e3bd3e9006ef20dbbb232e8a74f3e5961ab",{{50,false,4,"wts"},{15,true,1,"b0"},{14,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.1.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=144){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=25040u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[25616u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,16,144}},
{"24588f43709c92988ab0a0c331b9cd963d961f5fb2a947dc143b6a2986883916",{{15,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=6912u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 6912u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[32u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"7002eaec61f0d7b16a0fed3f0cb101d8386b5a67f563aa0e888f45afeea5ac3c",{{48,false,4,"coef"},{15,true,4,"raw"}},{64,1,1},{432,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/6912u;
let channel_scale_shift=coef[32u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"5a5add640dd4030bf67fd5a436bce7762b9ff8fcecdca9d45293ee5898929968",{{16,false,1,"t3"},{17,false,1,"t4"},{15,true,4,"raw"}},{64,1,1},{432,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.1656359434127808, 1.0220509767532349, 0.9135286808013916, 1.562838077545166, 1.1679651737213135, 0.8115286231040955, 0.9273597598075867, 1.4646879434585571, 1.1482347249984741, 1.3680312633514404, 1.2225645780563354, 1.2056280374526978, 0.839081346988678, 1.1358751058578491, 1.2441197633743286, 1.3879389762878418);
    let P1 = array<f32, 16>(0.18913982808589935, 0.39439207315444946, -0.08656201511621475, -0.007233678828924894, 0.00128381687682122, -0.10223761945962906, 0.04372747987508774, 0.11366121470928192, 0.07911962270736694, 0.23726889491081238, 0.03492538258433342, -0.12713566422462463, -0.19741547107696533, -0.10769984126091003, -0.0664978101849556, 0.2987874448299408);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"336b393ad8a450a4eec09ce938f4e60fffeac4b96f50a9e14c7420cdc2eee20f",{{50,false,4,"wts"},{18,true,1,"b0"},{15,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.1.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=144){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=25632u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[26208u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,16,144}},
{"ca0f4da8b79fd20d08b02f3a6fe9397399f6045dfb2e442e96829efa1aef151c",{{18,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=6912u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 6912u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[48u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"47d70641d1d38d498cc5f1b378389fda72ff3051448c3c5e5496875941bcdca2",{{48,false,4,"coef"},{18,true,4,"raw"}},{64,1,1},{432,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/6912u;
let channel_scale_shift=coef[48u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"dd7a121bc4f937bd673f4e0dd040d4886de9b801f73eb13691f50eda34c0f29f",{{18,true,4,"raw"}},{64,1,1},{432,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(0.4264046549797058, 0.2521263659000397, 0.5886488556861877, 1.008545994758606, 0.6010521054267883, 0.743996798992157, 0.5754590630531311, 0.48691561818122864, 0.4907357394695282, 0.601002037525177, 0.6257888078689575, 1.000486969947815, 0.9903489351272583, 0.8512178659439087, 0.6736283898353577, 0.5382153987884521);
    let P1 = array<f32, 16>(-0.05529317259788513, -0.3090578019618988, 0.026272732764482498, 0.07905453443527222, 0.014642244204878807, -0.11696235835552216, -0.08243121206760406, -0.3349977731704712, 0.05361523851752281, -0.10277555137872696, -0.19824987649917603, 0.19482800364494324, -0.017356008291244507, 0.32419145107269287, -0.027341431006789207, 0.0236190352588892);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"59c0bd9d44c095768a4896989909680c1aa6cf550fc65d5dd297bd052dfbd17f",{{50,false,4,"wts"},{19,true,1,"b0"},{18,false,1,"b1"},{14,false,1,"b2"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v + b2[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=144){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=26224u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[27376u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,144}},
{"c0815fb3ed5010390133c99058c05360d5b4b6768a02d4590c8b5cf615e03b17",{{50,false,4,"wts"},{21,true,1,"b0"},{22,true,1,"b1"},{20,false,1,"b2"},{19,false,1,"b3"}},{8,8,1},{2,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/gru/convz1_1/Conv, /core/update_block_depth2/unet/gru/convr1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 64u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=320){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/320;let kx=(q/64)%5;let ch=q%64;let iy=oy+ky-0;let ix=ox+kx-2;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=320 || colIn*4>=64){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/320);let kx=u32((row/64)%5);let col=u32(colIn*4);let grp=col/32u;let mv=(col%32u)/4u;
 let ix=27392u+(((grp*64u+ch)*1u+ky)*5u+kx)*8u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=64){return;}
 let pix=u32(row);let value=valueIn+wts[32512u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[1u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[2u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[3u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 1u:{
b0[4u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[5u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[6u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[7u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 2u:{
b0[8u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[9u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[10u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[11u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 3u:{
b0[12u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[13u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[14u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[15u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 4u:{
b0[16u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[17u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[18u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[19u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 5u:{
b0[20u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[21u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[22u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[23u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 6u:{
b0[24u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[25u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[26u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[27u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 7u:{
b0[28u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[29u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[30u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[31u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 8u:{
b1[0u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[1u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[2u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[3u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 9u:{
b1[4u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[5u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[6u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[7u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 10u:{
b1[8u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[9u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[10u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[11u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 11u:{
b1[12u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[13u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[14u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[15u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 12u:{
b1[16u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[17u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[18u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[19u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 13u:{
b1[20u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[21u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[22u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[23u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 14u:{
b1[24u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[25u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[26u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[27u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 15u:{
b1[28u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[29u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[30u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[31u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,64,320}},
{"d7e77142f97b1e2ca4ce9666fb1079ac5af8be0a05097dd632f689bb057a43e1",{{50,false,4,"wts"},{23,true,1,"b0"},{22,false,1,"b1"},{20,false,1,"b2"},{19,false,1,"b3"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/gru/convq1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v * b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 64u) { return S1(c - 32u, y, x); }
  return 0.0;
}

fn original_tanh(a: vec4<f32>) -> vec4<f32> {
  let expr = exp(-2 * abs(a));
  return sign(a) * (1 - expr) / (1 + expr);
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=320){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/320;let kx=(q/64)%5;let ch=q%64;let iy=oy+ky-0;let ix=ox+kx-2;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=320 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/320);let kx=u32((row/64)%5);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=32528u+(((grp*64u+ch)*1u+ky)*5u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[35088u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=original_tanh(value).x;
b0[1u*6912u+pix]=original_tanh(value).y;
b0[2u*6912u+pix]=original_tanh(value).z;
b0[3u*6912u+pix]=original_tanh(value).w;
}
case 1u:{
b0[4u*6912u+pix]=original_tanh(value).x;
b0[5u*6912u+pix]=original_tanh(value).y;
b0[6u*6912u+pix]=original_tanh(value).z;
b0[7u*6912u+pix]=original_tanh(value).w;
}
case 2u:{
b0[8u*6912u+pix]=original_tanh(value).x;
b0[9u*6912u+pix]=original_tanh(value).y;
b0[10u*6912u+pix]=original_tanh(value).z;
b0[11u*6912u+pix]=original_tanh(value).w;
}
case 3u:{
b0[12u*6912u+pix]=original_tanh(value).x;
b0[13u*6912u+pix]=original_tanh(value).y;
b0[14u*6912u+pix]=original_tanh(value).z;
b0[15u*6912u+pix]=original_tanh(value).w;
}
case 4u:{
b0[16u*6912u+pix]=original_tanh(value).x;
b0[17u*6912u+pix]=original_tanh(value).y;
b0[18u*6912u+pix]=original_tanh(value).z;
b0[19u*6912u+pix]=original_tanh(value).w;
}
case 5u:{
b0[20u*6912u+pix]=original_tanh(value).x;
b0[21u*6912u+pix]=original_tanh(value).y;
b0[22u*6912u+pix]=original_tanh(value).z;
b0[23u*6912u+pix]=original_tanh(value).w;
}
case 6u:{
b0[24u*6912u+pix]=original_tanh(value).x;
b0[25u*6912u+pix]=original_tanh(value).y;
b0[26u*6912u+pix]=original_tanh(value).z;
b0[27u*6912u+pix]=original_tanh(value).w;
}
case 7u:{
b0[28u*6912u+pix]=original_tanh(value).x;
b0[29u*6912u+pix]=original_tanh(value).y;
b0[30u*6912u+pix]=original_tanh(value).z;
b0[31u*6912u+pix]=original_tanh(value).w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,320}},
{"3d32be0b9e87eea6d21b64518d608053ef3126950cb1058a827ed035e24adead",{{21,false,4,"z"},{20,false,4,"h"},{23,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> sub_scratch:array<vec4<f32>,64>;
var<workgroup> hm_scratch:array<vec4<f32>,64>;
var<workgroup> zq_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(let av = vec4<f32>(1.0);
let bv = z[global_idx];
sub_scratch[local_idx] = av - bv;
workgroupBarrier();
let a_hm = sub_scratch[local_idx];
let b_hm = h[global_idx];
hm_scratch[local_idx] = a_hm * b_hm;
let a_zq = z[global_idx];
let b_zq = raw[global_idx];
zq_scratch[local_idx] = a_zq * b_zq;
workgroupBarrier();
let a = hm_scratch[local_idx];
let b = zq_scratch[local_idx];
raw[global_idx]=a + b;
)WGSL_CODE",0,{0,0,0}},
{"a521696cbf5a5d84ebc36d5b310ea5e52d5f70efc6cd77aec154766d80126dab",{{50,false,4,"wts"},{24,true,1,"b0"},{25,true,1,"b1"},{23,false,1,"b2"},{19,false,1,"b3"}},{8,8,1},{2,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/gru/convz2_1/Conv, /core/update_block_depth2/unet/gru/convr2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 64u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=320){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/64;let kx=(q/64)%1;let ch=q%64;let iy=oy+ky-2;let ix=ox+kx-0;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=320 || colIn*4>=64){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/64);let kx=u32((row/64)%1);let col=u32(colIn*4);let grp=col/32u;let mv=(col%32u)/4u;
 let ix=35104u+(((grp*64u+ch)*5u+ky)*1u+kx)*8u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=64){return;}
 let pix=u32(row);let value=valueIn+wts[40224u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[1u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[2u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[3u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 1u:{
b0[4u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[5u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[6u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[7u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 2u:{
b0[8u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[9u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[10u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[11u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 3u:{
b0[12u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[13u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[14u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[15u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 4u:{
b0[16u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[17u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[18u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[19u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 5u:{
b0[20u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[21u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[22u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[23u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 6u:{
b0[24u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[25u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[26u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[27u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 7u:{
b0[28u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[29u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[30u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[31u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 8u:{
b1[0u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[1u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[2u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[3u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 9u:{
b1[4u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[5u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[6u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[7u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 10u:{
b1[8u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[9u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[10u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[11u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 11u:{
b1[12u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[13u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[14u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[15u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 12u:{
b1[16u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[17u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[18u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[19u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 13u:{
b1[20u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[21u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[22u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[23u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 14u:{
b1[24u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[25u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[26u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[27u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 15u:{
b1[28u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[29u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[30u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[31u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,64,320}},
{"77e9dd29ae5e86d00041b1d498ff56bc123506c07ddbf2b6950fda9fd5982227",{{50,false,4,"wts"},{26,true,1,"b0"},{25,false,1,"b1"},{23,false,1,"b2"},{19,false,1,"b3"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/gru/convq2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v * b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 64u) { return S1(c - 32u, y, x); }
  return 0.0;
}

fn original_tanh(a: vec4<f32>) -> vec4<f32> {
  let expr = exp(-2 * abs(a));
  return sign(a) * (1 - expr) / (1 + expr);
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=320){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/64;let kx=(q/64)%1;let ch=q%64;let iy=oy+ky-2;let ix=ox+kx-0;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=320 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/64);let kx=u32((row/64)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=40240u+(((grp*64u+ch)*5u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[42800u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=original_tanh(value).x;
b0[1u*6912u+pix]=original_tanh(value).y;
b0[2u*6912u+pix]=original_tanh(value).z;
b0[3u*6912u+pix]=original_tanh(value).w;
}
case 1u:{
b0[4u*6912u+pix]=original_tanh(value).x;
b0[5u*6912u+pix]=original_tanh(value).y;
b0[6u*6912u+pix]=original_tanh(value).z;
b0[7u*6912u+pix]=original_tanh(value).w;
}
case 2u:{
b0[8u*6912u+pix]=original_tanh(value).x;
b0[9u*6912u+pix]=original_tanh(value).y;
b0[10u*6912u+pix]=original_tanh(value).z;
b0[11u*6912u+pix]=original_tanh(value).w;
}
case 3u:{
b0[12u*6912u+pix]=original_tanh(value).x;
b0[13u*6912u+pix]=original_tanh(value).y;
b0[14u*6912u+pix]=original_tanh(value).z;
b0[15u*6912u+pix]=original_tanh(value).w;
}
case 4u:{
b0[16u*6912u+pix]=original_tanh(value).x;
b0[17u*6912u+pix]=original_tanh(value).y;
b0[18u*6912u+pix]=original_tanh(value).z;
b0[19u*6912u+pix]=original_tanh(value).w;
}
case 5u:{
b0[20u*6912u+pix]=original_tanh(value).x;
b0[21u*6912u+pix]=original_tanh(value).y;
b0[22u*6912u+pix]=original_tanh(value).z;
b0[23u*6912u+pix]=original_tanh(value).w;
}
case 6u:{
b0[24u*6912u+pix]=original_tanh(value).x;
b0[25u*6912u+pix]=original_tanh(value).y;
b0[26u*6912u+pix]=original_tanh(value).z;
b0[27u*6912u+pix]=original_tanh(value).w;
}
case 7u:{
b0[28u*6912u+pix]=original_tanh(value).x;
b0[29u*6912u+pix]=original_tanh(value).y;
b0[30u*6912u+pix]=original_tanh(value).z;
b0[31u*6912u+pix]=original_tanh(value).w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,320}},
{"8961f129e1846ea64f51e2ce2cbeffd99413762c9a06984b7cfbcb600378fd9a",{{24,false,4,"z"},{23,false,4,"h"},{26,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> sub_scratch:array<vec4<f32>,64>;
var<workgroup> hm_scratch:array<vec4<f32>,64>;
var<workgroup> zq_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(let av = vec4<f32>(1.0);
let bv = z[global_idx];
sub_scratch[local_idx] = av - bv;
workgroupBarrier();
let a_hm = sub_scratch[local_idx];
let b_hm = h[global_idx];
hm_scratch[local_idx] = a_hm * b_hm;
let a_zq = z[global_idx];
let b_zq = raw[global_idx];
zq_scratch[local_idx] = a_zq * b_zq;
workgroupBarrier();
let a = hm_scratch[local_idx];
let b = zq_scratch[local_idx];
raw[global_idx]=a + b;
)WGSL_CODE",0,{0,0,0}},
{"6cf11ebf0df679f365613b9fd04867b797ad8d1e93cc6c15d68b15c416535bbd",{{50,false,4,"wts"},{27,true,1,"b0"},{26,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/mid/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=288){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=42816u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[45120u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,288}},
{"a16818608db4dcadc6ceb0d87db4f137ee38782139bec839b3067e43c2ef1713",{{27,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=13824u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 13824u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[64u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"cef69f97aa2a2175d205431c0163b5f12670718db1a161f8d3774836eb179c5a",{{48,false,4,"coef"},{27,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/13824u;
let channel_scale_shift=coef[64u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"20090a44574b20897fff8260b8c2533ee859ffedee3351d58ef128d3d750b96b",{{27,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.8687565326690674, 0.4855436384677887, 0.8288847804069519, 0.8124305009841919, 0.9690547585487366, 0.5397576093673706, 0.7347555160522461, 0.9895047545433044, 1.1399421691894531, 1.3236395120620728, 1.089347243309021, 0.2637065351009369, 0.9843631386756897, 0.7972566485404968, 0.939339280128479, 0.8749436140060425, 0.9894788265228271, 1.011549949645996, 0.24913384020328522, 0.9676110148429871, 0.9080445170402527, 0.7713121771812439, 2.188708543777466, 0.8992301821708679, 1.13258957862854, 1.0258735418319702, 1.0922471284866333, 0.9132866263389587, 1.4758555889129639, 1.9582046270370483, 0.7480019927024841, 0.8854714035987854);
    let P1 = array<f32, 32>(-0.6861222386360168, -0.023685097694396973, -0.08839099109172821, 0.03158329427242279, -0.18790461122989655, -0.14482815563678741, -0.044585470110177994, -0.1856883317232132, -0.14907203614711761, -0.18737438321113586, -0.20185130834579468, 0.03758436068892479, -0.22042186558246613, -0.40164294838905334, -0.1596652865409851, -0.12346552312374115, 0.03737301006913185, -0.3710264265537262, 0.04715000092983246, -0.11977212876081467, -0.2783011198043823, 0.03899167478084564, -1.0749720335006714, 0.11593834310770035, -0.3519933223724365, -0.2205507606267929, -0.25777894258499146, -0.481412410736084, -0.6253181099891663, -0.7272321581840515, -0.2917121946811676, -0.36585569381713867);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"f86817666d7a046754a683ec1c2d814def1edc450236b846a1418323096f04f3",{{50,false,4,"wts"},{28,true,1,"b0"},{27,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/mid/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=288){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=45136u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[47440u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,288}},
{"469a4e8ff81cfd2a4d35d8c9b554c0043341a61989cf8563b90de7812577329c",{{28,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=13824u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 13824u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[96u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"e07d7299f22a84fb48150bad083d02d391ad297afcddcfa98e5e9ee49cbd7374",{{48,false,4,"coef"},{28,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/13824u;
let channel_scale_shift=coef[96u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"f596d7bbfccb76a43064f552594e7d79f2192bbe4e8496be2a07494f597f43d0",{{28,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(0.5387988686561584, 0.5636143088340759, 0.680594265460968, 0.8938270211219788, 1.0003727674484253, 1.091995120048523, 0.9758369326591492, 1.3972996473312378, 1.0967028141021729, 0.6966249942779541, 1.133482575416565, 0.8077158331871033, 0.6302931904792786, 1.2267273664474487, 0.4888198971748352, 0.7880700826644897, 1.1416901350021362, 1.530874252319336, 1.1224730014801025, 0.8909685015678406, 1.3605130910873413, 1.45710289478302, 0.5946311354637146, 0.5151774883270264, 1.3208578824996948, 1.2722190618515015, 0.775576651096344, 1.0340111255645752, 0.871986448764801, 1.038068175315857, 0.7329596281051636, 1.2660458087921143);
    let P1 = array<f32, 32>(0.32512205839157104, -0.15672053396701813, -0.28089359402656555, 0.0486648865044117, 0.11971976608037949, 0.23062127828598022, 0.11328332126140594, 0.1437414437532425, 0.04265275225043297, 0.03263635188341141, -0.19049358367919922, -0.09746228158473969, -0.23391585052013397, -0.19655875861644745, 0.12476499378681183, 0.2380467802286148, 0.09270584583282471, 0.0012361172121018171, -0.0680839791893959, 0.005642448551952839, -0.40102604031562805, -0.4318714737892151, -0.26272356510162354, 0.057964473962783813, 0.03087073378264904, 0.011764796450734138, -0.04525846987962723, -0.041928570717573166, -0.13623659312725067, -0.47830596566200256, 0.06770452111959457, -0.0067858584225177765);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"068f983ebff5b0e3303dcf2a89ba89e0dadf56d0adf9e58de1fe814534fee3be",{{50,false,4,"wts"},{29,true,1,"b0"},{28,false,1,"b1"},{26,false,1,"b2"},{18,false,1,"b3"},{14,false,1,"b4"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.0.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v + b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  v = v + b4[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 48u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=432){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/144;let kx=(q/48)%3;let ch=q%48;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=432 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%48);let ky=u32(row/144);let kx=u32((row/48)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=47456u+(((grp*48u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[50912u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,432}},
{"9e22a4e9742cbfe31f959ca0d42a03e2de840f253eec50ebd4a1f1da231f8fdf",{{29,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=13824u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 13824u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[128u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"a9d8ee3518f957458a26d06f3eef0162362bb180d809532f0970523a02090a53",{{48,false,4,"coef"},{29,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/13824u;
let channel_scale_shift=coef[128u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"73f7f1a2a1cb4dfcb596dca93b4c200239dec12cf19b58779d249397a9dc97ff",{{30,false,1,"t3"},{31,false,1,"t4"},{29,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.0308760404586792, 0.9768494367599487, 1.0624679327011108, 1.1148196458816528, 1.3165335655212402, 1.0065652132034302, 0.849982738494873, 0.5452168583869934, 1.038499116897583, 0.9961291551589966, 1.0098718404769897, 1.149354100227356, 0.9134297370910645, 0.35010093450546265, 0.7904531359672546, 1.0531554222106934, 1.3158575296401978, 1.3141673803329468, 0.9110047221183777, 1.3051377534866333, 0.7811072468757629, 0.7838155031204224, 0.7789944410324097, 0.7397356629371643, 0.8324458003044128, 1.0806503295898438, 0.8571859002113342, 1.0524369478225708, 1.0622549057006836, 1.0270295143127441, 1.0744872093200684, 1.0360721349716187);
    let P1 = array<f32, 32>(-0.11898882687091827, -0.07399996370077133, -0.09363045543432236, 0.060254693031311035, -0.2898295223712921, -0.20711670815944672, -0.15681667625904083, -0.049781348556280136, -0.33909785747528076, -0.07590921223163605, -0.15767134726047516, -0.15208859741687775, -0.012827282771468163, -0.19780927896499634, -0.06026124209165573, -0.015910187736153603, -0.3834337294101715, -0.3974900245666504, 0.19380275905132294, -0.48559650778770447, -0.12340506166219711, 0.0010173931950703263, -0.17072758078575134, -0.039541155099868774, 0.034712281078100204, 0.009850078262388706, -0.23492296040058136, -0.024216393008828163, -0.14833246171474457, -0.08495593816041946, -0.30661019682884216, -0.0034423726610839367);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"44c5b38fc444e6f982f3f1fa4a29704d3d1df0521afbd4a7877678ecac2b7383",{{50,false,4,"wts"},{32,true,1,"b0"},{29,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.0.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=288){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=50928u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[53232u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,288}},
{"04358835a0c155994f8274be509f21d13e1cd92ded561cf18369a4fa1d5f519f",{{32,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=13824u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 13824u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[160u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"8088236491aa6ea9a326d403e5c6ecfd40fbfb108850e8c6236f4b8aabf44186",{{48,false,4,"coef"},{32,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/13824u;
let channel_scale_shift=coef[160u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"c41f0ced2746cbc05a6a81ecd0b43e88fba6f731e317b360fcd03f9110a30d92",{{32,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.44490647315979, 1.6919130086898804, 1.3472381830215454, 1.3623353242874146, 0.35969072580337524, 1.2056024074554443, 1.5124751329421997, 0.4818713068962097, 1.750336766242981, 1.1753854751586914, 0.6816576719284058, 1.5517477989196777, 0.4190564751625061, 1.7866673469543457, 1.4132189750671387, 0.957862138748169, 0.8487277030944824, 1.0132858753204346, 1.0177373886108398, 1.2792476415634155, 0.7283192873001099, 1.4185609817504883, 0.5185100436210632, 0.6604533791542053, 1.2438942193984985, 1.263862133026123, 0.7592844367027283, 0.8484734892845154, 1.4778989553451538, 0.683007538318634, 1.214734673500061, 0.9522296786308289);
    let P1 = array<f32, 32>(0.004071471747010946, -0.22744838893413544, -0.013897934928536415, 0.3516407310962677, -0.13022126257419586, -0.05033998563885689, -0.08578497171401978, -0.1444152146577835, -0.2647823691368103, 0.24466322362422943, -0.5058233141899109, -0.10308577865362167, -0.4688517153263092, -0.18714100122451782, 0.023832330480217934, -0.1518997848033905, 0.005278945434838533, -0.28324025869369507, -0.31157568097114563, -0.1834053248167038, 0.10060851275920868, -0.4304104745388031, 0.0943196639418602, -0.07065469771623611, 0.09681905806064606, -0.588946521282196, -0.5479710698127747, -0.4114896059036255, -0.17963950335979462, -0.4242757260799408, -0.4190806448459625, -0.20657925307750702);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"b7e027c3e0944b81aadad0b6ea2f6e19d0e97b365190d0819b94e8c6cdb4938c",{{50,false,4,"wts"},{33,true,1,"b0"},{28,false,1,"b1"},{26,false,1,"b2"},{18,false,1,"b3"},{14,false,1,"b4"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.0.0/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v + b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  v = v + b4[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 48u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=48){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/48;let kx=(q/48)%1;let ch=q%48;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=48 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%48);let ky=u32(row/48);let kx=u32((row/48)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=53248u+(((grp*48u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[53632u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,48}},
{"391d2525e15a3dfe9d12a253a3869f02adac7258c1d9d95fff8c28aebe04c114",{{50,false,4,"wts"},{34,true,1,"b0"},{32,false,1,"b1"},{33,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.0.1/ups.0.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y) >> 1u; let sx = u32(x) >> 1u;
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v + b2[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=53648u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[54800u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,288}},
{"806de907b8c5cb71460387d8a630b605fff5a4e8aa96e1e0b6d00ba78f2ed179",{{50,false,4,"wts"},{35,true,1,"b0"},{34,false,1,"b1"},{13,false,1,"b2"},{9,false,1,"b3"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.1.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  v = v + b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 32u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=54816u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[55968u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,288}},
{"e42a27179747124a450ac9e4f8e02eae957056d7ace3acc810ceaabda413a0ec",{{35,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[192u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"146392c16098db0e22da62f96ef326975fc866543a7e1562beff1bb783b9c41f",{{48,false,4,"coef"},{35,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[192u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"040b56a82b7a7d7054aa2a04719dd128d391d9edbe9c0e1ef8ce6780c6e25f67",{{36,false,1,"t3"},{37,false,1,"t4"},{35,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.4519963264465332, 1.0610110759735107, 0.9218735098838806, 0.6755930781364441, 0.8472209572792053, 1.103845238685608, 0.9080103635787964, 0.8837310671806335, 1.1101233959197998, 0.8308441042900085, 0.9686066508293152, 0.019005926325917244, 1.53010892868042, 1.3588731288909912, 0.8481330871582031, 0.39658570289611816);
    let P1 = array<f32, 16>(-0.35643166303634644, -0.038781922310590744, -0.2415296584367752, 0.07566027343273163, -0.03368420526385307, -0.08816328644752502, -0.012264067307114601, -0.08889932185411453, 0.15886850655078888, 0.23864559829235077, -0.20750151574611664, -0.47348421812057495, -0.3366808295249939, -0.4135027229785919, 0.012480298057198524, -0.06888872385025024);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"1147cc800ea8d3549b5d065cfb80c5ef686aa85b57fd9ac97bca83cb72578817",{{50,false,4,"wts"},{38,true,1,"b0"},{35,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.1.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=55984u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[56560u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,144}},
{"9cab7ad4bcfe433afca0858a9aeb75956c329a271732ccd2ab584b551a4eeba2",{{38,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[208u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"2a9dbee3167ec349c11ba7cc5c361496d0a20a86fba64f5418bd660b24173d9e",{{48,false,4,"coef"},{38,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[208u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"eea4597e8a79a2a5c10aa06924d9dae08b7a61169a6d7751c0877b6fb12d2a55",{{38,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(0.534520149230957, 0.8737297654151917, 0.5625705122947693, 1.0675586462020874, 0.4478868544101715, 0.9780964255332947, 1.8655935525894165, 0.7403324842453003, 2.3058743476867676, 1.077169418334961, 0.4175819456577301, 1.510103702545166, 1.26657235622406, 0.4426352083683014, 0.9839290380477905, 1.0774586200714111);
    let P1 = array<f32, 16>(0.3281431198120117, -0.34535565972328186, -0.1910775899887085, 0.05154091492295265, -0.09595222026109695, 0.1013239249587059, 0.057779811322689056, 0.07570431381464005, 0.19066999852657318, 0.10458994656801224, 0.05325464904308319, -0.03292659670114517, -0.016167787835001945, -0.21575531363487244, 0.4204941987991333, 0.024578193202614784);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"0f3769af8d557be925497058dec63828d3b19188efd0062bee1988a87f411025",{{50,false,4,"wts"},{39,true,1,"b0"},{34,false,1,"b1"},{13,false,1,"b2"},{9,false,1,"b3"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.1.0/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  v = v + b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 32u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=32){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/32;let kx=(q/32)%1;let ch=q%32;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=32 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/32);let kx=u32((row/32)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=56576u+(((grp*32u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[56704u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,32}},
{"1fd6c30656130ff5fbbd7201e3469cae45be21c0a55dba3f25326097f28c428e",{{50,false,4,"wts"},{40,true,1,"b0"},{38,false,1,"b1"},{39,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=56720u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[57296u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,144}},
{"61bc392eede5e7cfa0c8481423b966f9b27e265d0ca9eff6d6bc1a192daaf525",{{50,false,4,"wts"},{41,true,1,"b0"},{40,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/final_res_block/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 32u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=57312u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[58464u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,288}},
{"53f2f1a937d339ddbcd57ddbd37a68add18c9d7b2452cd2d2e25702bcb1f8042",{{41,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[224u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"dcc1163d330b5d7ee51246c18a4b97c54b0f61595c8c41d60771c670dc642ed4",{{48,false,4,"coef"},{41,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[224u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"beb1fc30af0db2eb59700af0009b65090c6ad679399c53f87cde4be8d85352d5",{{42,false,1,"t3"},{43,false,1,"t4"},{41,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(0.9875800013542175, 0.9973968267440796, 0.949870765209198, 1.751861333847046, 1.1787556409835815, 0.7179486751556396, 0.9730886220932007, 1.552246332168579, 0.6105612516403198, 1.0591378211975098, 1.2269864082336426, 1.1117626428604126, 1.8708029985427856, 1.240452527999878, 0.7325398325920105, 0.5766901969909668);
    let P1 = array<f32, 16>(-0.034808166325092316, 0.06577306985855103, -0.06579680740833282, -0.38482147455215454, -0.2830467224121094, -0.08609149605035782, -0.21742291748523712, 0.1288217306137085, -0.14161880314350128, -0.08213981986045837, 0.11768864840269089, -0.05320363864302635, -0.22585497796535492, -0.17181186378002167, -0.41826194524765015, 0.0582902692258358);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"da38d3dc497bb3a6b7c26d94a262921d27d5b33295fbecc286806d76d430c954",{{50,false,4,"wts"},{44,true,1,"b0"},{41,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/final_res_block/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=58480u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[59056u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,144}},
{"6a366ab8a50f87e4e3891149ee37fa65a98a31ad4927a7debd143f9deedcafe8",{{44,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[240u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"389e088d7f0f6729fba610cabc6022183d8ecc42fb6d184bf73234c35229a997",{{48,false,4,"coef"},{44,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[240u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"bc00e842eb5ee3f1b778c39998de7e7f365c62bd268820880fb33cb317db6ab0",{{44,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(0.695611298084259, 5.309262633090839e-05, 0.23800072073936462, 0.4856206476688385, -0.0027802600525319576, 0.0021251386497169733, -0.022010693326592445, 0.9605537056922913, 0.7428562045097351, 0.15878599882125854, 0.8231478333473206, 0.608710527420044, 0.1137300655245781, 0.13211171329021454, 0.13283611834049225, 1.6364182233810425);
    let P1 = array<f32, 16>(-0.1402619183063507, -1.278259038925171, -0.16009297966957092, -0.4925999939441681, -0.1277998685836792, -0.1433214694261551, -0.18093161284923553, -0.1027757078409195, -0.5494709610939026, -0.22653745114803314, -0.4480752944946289, -0.14242205023765564, -0.08907261490821838, -0.25149109959602356, -1.085011601448059, -0.27287963032722473);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"ca1c88825b5241142e87d9cff7390c25eb1a07d80e66b390e141465a1c6b4874",{{50,false,4,"wts"},{45,true,1,"b0"},{40,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/final_res_block/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 32u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=32){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/32;let kx=(q/32)%1;let ch=q%32;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=32 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/32);let kx=u32((row/32)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=59072u+(((grp*32u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[59200u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,32}},
{"52c2a02cb5c422da2ddb15023a6b9866cab7dbc05fdf7655c1db012d0d59b847",{{50,false,4,"wts"},{46,true,1,"b0"},{47,true,1,"b1"},{44,false,1,"b2"},{45,false,1,"b3"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/final_conv_1/Conv, /core/update_block_depth2/unet/conf_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  v = v + b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=16){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/16;let kx=(q/16)%1;let ch=q%16;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=16 || colIn*4>=2){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/16);let kx=u32((row/16)%1);let col=u32(colIn*4);let grp=col/4u;let mv=(col%4u)/4u;
 let ix=59216u+(((grp*16u+ch)*1u+ky)*1u+kx)*1u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=2){return;}
 let pix=u32(row);let value=valueIn+wts[59232u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b1[0u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,2,16}},
},{{1,32,72,96},{1,1,144,192},{1,1,144,192}},{884736,884736,884736,442368,442368,110592,442368,221184,221184}},
{20,{
{1769472,0,0,{1,16,288,384},-1},
{1769472,2,-1,{16,288,384},0},
{1769472,2,-1,{16,288,384},1},
{442368,0,1,{1,4,288,384},-1},
{1769472,2,-1,{16,288,384},0},
{1769472,2,-1,{16,288,384},2},
{1658880,2,-1,{15,288,384},0},
{1769472,0,2,{1,16,288,384},-1},
{110592,0,3,{1,1,288,384},-1},
{884736,2,-1,{8,288,384},1},
{884736,2,-1,{8,288,384},0},
{8,0,4,{1,8,1,1},-1},
{8,0,5,{1,8,1,1},-1},
{884736,2,-1,{8,288,384},3},
{221184,2,-1,{8,144,192},0},
{221184,2,-1,{8,144,192},4},
{8,0,6,{1,8,1,1},-1},
{8,0,7,{1,8,1,1},-1},
{221184,2,-1,{8,144,192},5},
{110592,2,-1,{16,72,96},4},
{110592,2,-1,{16,72,96},6},
{16,0,8,{1,16,1,1},-1},
{16,0,9,{1,16,1,1},-1},
{110592,2,-1,{16,72,96},7},
{221184,2,-1,{32,72,96},6},
{138240,0,10,{1,20,72,96},-1},
{138240,2,-1,{20,72,96},8},
{138240,2,-1,{20,72,96},9},
{138240,2,-1,{20,72,96},10},
{138240,2,-1,{20,72,96},8},
{138240,2,-1,{20,72,96},9},
{138240,1,0,{20,72,96},-1},
{221184,2,-1,{32,72,96},6},
{221184,2,-1,{32,72,96},8},
{221184,2,-1,{32,72,96},6},
{221184,2,-1,{32,72,96},9},
{32,0,11,{1,32,1,1},-1},
{32,0,12,{1,32,1,1},-1},
{221184,2,-1,{32,72,96},10},
{221184,2,-1,{32,72,96},9},
{442368,2,-1,{16,144,192},4},
{442368,2,-1,{16,144,192},6},
{16,0,13,{1,16,1,1},-1},
{16,0,14,{1,16,1,1},-1},
{442368,2,-1,{16,144,192},8},
{442368,2,-1,{16,144,192},6},
{884736,2,-1,{8,288,384},0},
{884736,2,-1,{8,288,384},4},
{8,0,15,{1,8,1,1},-1},
{8,0,16,{1,8,1,1},-1},
{884736,2,-1,{8,288,384},6},
{884736,2,-1,{8,288,384},4},
{884736,2,-1,{8,288,384},3},
{884736,2,-1,{8,288,384},4},
{8,0,17,{1,8,1,1},-1},
{8,0,18,{1,8,1,1},-1},
{884736,2,-1,{8,288,384},6},
{884736,2,-1,{8,288,384},4},
{110592,1,1,{1,288,384},-1},
{110592,1,2,{1,288,384},-1},
{1024,2,-1,{1024},2},
{60480,2,-1,{60480},-1},
{128832,0,19,{128832},-1},
},{
{"68a8612d18e4e31ee52011b5749f21c569cec1dd2320b2cde57a9af3d399d6af",{{62,false,4,"wts"},{1,true,1,"b0"},{0,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/encoder/convc1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=0u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[576u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=max(value.x, 0.0);
b0[1u*110592u+pix]=max(value.y, 0.0);
b0[2u*110592u+pix]=max(value.z, 0.0);
b0[3u*110592u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*110592u+pix]=max(value.x, 0.0);
b0[5u*110592u+pix]=max(value.y, 0.0);
b0[6u*110592u+pix]=max(value.z, 0.0);
b0[7u*110592u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*110592u+pix]=max(value.x, 0.0);
b0[9u*110592u+pix]=max(value.y, 0.0);
b0[10u*110592u+pix]=max(value.z, 0.0);
b0[11u*110592u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*110592u+pix]=max(value.x, 0.0);
b0[13u*110592u+pix]=max(value.y, 0.0);
b0[14u*110592u+pix]=max(value.z, 0.0);
b0[15u*110592u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,144}},
{"3c7b454383200243d5f4735b97db8d913ebabfd5d52549b9c02943fdec60505f",{{62,false,4,"wts"},{2,true,1,"b0"},{1,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/encoder/convc2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=592u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[1168u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=max(value.x, 0.0);
b0[1u*110592u+pix]=max(value.y, 0.0);
b0[2u*110592u+pix]=max(value.z, 0.0);
b0[3u*110592u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*110592u+pix]=max(value.x, 0.0);
b0[5u*110592u+pix]=max(value.y, 0.0);
b0[6u*110592u+pix]=max(value.z, 0.0);
b0[7u*110592u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*110592u+pix]=max(value.x, 0.0);
b0[9u*110592u+pix]=max(value.y, 0.0);
b0[10u*110592u+pix]=max(value.z, 0.0);
b0[11u*110592u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*110592u+pix]=max(value.x, 0.0);
b0[13u*110592u+pix]=max(value.y, 0.0);
b0[14u*110592u+pix]=max(value.z, 0.0);
b0[15u*110592u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,144}},
{"12a9b397676f88ac894d1edf80a0725862326d3b41d77696435620c208992966",{{62,false,4,"wts"},{4,true,1,"b0"},{3,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/encoder/convd1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 4u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=36){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/12;let kx=(q/4)%3;let ch=q%4;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=36 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%4);let ky=u32(row/12);let kx=u32((row/4)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=1184u+(((grp*8u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[1472u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=max(value.x, 0.0);
b0[1u*110592u+pix]=max(value.y, 0.0);
b0[2u*110592u+pix]=max(value.z, 0.0);
b0[3u*110592u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*110592u+pix]=max(value.x, 0.0);
b0[5u*110592u+pix]=max(value.y, 0.0);
b0[6u*110592u+pix]=max(value.z, 0.0);
b0[7u*110592u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*110592u+pix]=max(value.x, 0.0);
b0[9u*110592u+pix]=max(value.y, 0.0);
b0[10u*110592u+pix]=max(value.z, 0.0);
b0[11u*110592u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*110592u+pix]=max(value.x, 0.0);
b0[13u*110592u+pix]=max(value.y, 0.0);
b0[14u*110592u+pix]=max(value.z, 0.0);
b0[15u*110592u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,36}},
{"bfce20f4e6f347396934b37b58f5fcc7ec736e310973fe43ddd8f11b1125d442",{{62,false,4,"wts"},{5,true,1,"b0"},{4,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/encoder/convd2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=1488u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[2064u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=max(value.x, 0.0);
b0[1u*110592u+pix]=max(value.y, 0.0);
b0[2u*110592u+pix]=max(value.z, 0.0);
b0[3u*110592u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*110592u+pix]=max(value.x, 0.0);
b0[5u*110592u+pix]=max(value.y, 0.0);
b0[6u*110592u+pix]=max(value.z, 0.0);
b0[7u*110592u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*110592u+pix]=max(value.x, 0.0);
b0[9u*110592u+pix]=max(value.y, 0.0);
b0[10u*110592u+pix]=max(value.z, 0.0);
b0[11u*110592u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*110592u+pix]=max(value.x, 0.0);
b0[13u*110592u+pix]=max(value.y, 0.0);
b0[14u*110592u+pix]=max(value.z, 0.0);
b0[15u*110592u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,144}},
{"e9d11c342c5c1c2643759e56661b0645f24d4241e8b0ac863629373334c27710",{{62,false,4,"wts"},{6,true,1,"b0"},{2,false,1,"b1"},{5,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/encoder/output_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 32u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=288){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=15){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=2080u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=15){return;}
 let pix=u32(row);let value=valueIn+wts[3232u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=max(value.x, 0.0);
b0[1u*110592u+pix]=max(value.y, 0.0);
b0[2u*110592u+pix]=max(value.z, 0.0);
b0[3u*110592u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*110592u+pix]=max(value.x, 0.0);
b0[5u*110592u+pix]=max(value.y, 0.0);
b0[6u*110592u+pix]=max(value.z, 0.0);
b0[7u*110592u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*110592u+pix]=max(value.x, 0.0);
b0[9u*110592u+pix]=max(value.y, 0.0);
b0[10u*110592u+pix]=max(value.z, 0.0);
b0[11u*110592u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*110592u+pix]=max(value.x, 0.0);
b0[13u*110592u+pix]=max(value.y, 0.0);
b0[14u*110592u+pix]=max(value.z, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,15,288}},
{"e82bafede578ba2bfef205fbe2c2825622ccf7b02e473d6ba191d317e8ed8a4a",{{62,false,4,"wts"},{9,true,1,"b0"},{7,false,1,"b1"},{6,false,1,"b2"},{8,false,1,"b3"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/init_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  return v;
}
fn S2(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b3[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 31u) { return S1(c - 16u, y, x); }
  if (c < 32u) { return S2(c - 31u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=1568){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/224;let kx=(q/32)%7;let ch=q%32;let iy=oy+ky-3;let ix=ox+kx-3;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=1568 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/224);let kx=u32((row/32)%7);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=3248u+(((grp*32u+ch)*7u+ky)*7u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6384u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,1568}},
{"b8cb4b7cdb97789ffe45d6d0f111ff3c9f00c2ceb2e72da2970505fbec2e22bc",{{62,false,4,"wts"},{10,true,1,"b0"},{9,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.0.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=72){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=6400u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6544u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,72}},
{"03b32d80b2e5098a0395eb464e5982d05db0d16e57ea0ac4b73a02a9cafeff72",{{10,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[0u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"0f98a6ff87860a8c36574000b9921a385d80fc5373c8ccc4d721388e228f3c00",{{60,false,4,"coef"},{10,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[0u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"9c1ad1a6fb687afc8814c6de32256d86a0229976bf74dffbb68b1208cea09782",{{11,false,1,"t3"},{12,false,1,"t4"},{10,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.7367537617683411, 0.8846451640129089, 0.7256792783737183, 0.6895222067832947, 0.7935814261436462, 0.8293971419334412, 0.9833404421806335, 0.5809348225593567);
    let P1 = array<f32, 8>(0.04302620887756348, 0.07105983793735504, 0.17612402141094208, 0.17138101160526276, -0.2958891987800598, 0.17098644375801086, -0.1627522110939026, 0.09819024801254272);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"f95aa51e3f598d2d36977f45b6d0350c5c5ea6e597f6699e87fa9e93829e9869",{{62,false,4,"wts"},{13,true,1,"b0"},{10,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.0.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=72){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=6560u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6704u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,72}},
{"cae227bb8a172a697600b8a761fb70276085ede2b3d9e237ea1e007a7d634d12",{{13,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[8u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"a58fc0e40bf39f116df41b19181f4aa0eef7406317540353de3327c3e9ddbe37",{{60,false,4,"coef"},{13,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[8u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"ab54c7fbaaca1437d477358de15609e57207193ea841d13fc94acbbeeba57711",{{13,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.5202493071556091, 0.8691046237945557, 1.8021529912948608, 0.909307599067688, 0.6510735154151917, 0.6378940939903259, 0.8225512504577637, 0.4977917969226837);
    let P1 = array<f32, 8>(0.041183751076459885, 0.02951963059604168, -0.07823695242404938, -0.16703784465789795, 0.08494484424591064, 0.05579941347241402, 0.24901346862316132, 0.03699877858161926);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"190821519af2b4143a1ef03b553ff99c6a06f5f69b8b54944005835b0b7dc8ce",{{62,false,4,"wts"},{14,true,1,"b0"},{13,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.0.1/downs.0.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c / 4u; let sy = 2u * u32(y) + ((c % 4u) / 2u); let sx = 2u * u32(x) + (c % 2u);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  v = v + b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=32){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/32;let kx=(q/32)%1;let ch=q%32;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=32 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/32);let kx=u32((row/32)%1);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=6720u+(((grp*32u+ch)*1u+ky)*1u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6784u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,8,32}},
{"9312da104b941743172abc88f357ef7a0a8994f76b303f47adcfad72bf9ced76",{{62,false,4,"wts"},{15,true,1,"b0"},{14,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.1.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=72){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=6800u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6944u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,8,72}},
{"525307200150f6ed425154b099728b558452a88c8e8616ddc6b32cf20c5086db",{{15,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=13824u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 13824u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[16u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"b3eb7520db516043a80298b05750ee2e6d01e98cf64047fc763c06a014d5cf91",{{60,false,4,"coef"},{15,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/13824u;
let channel_scale_shift=coef[16u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"5807471a3ef0d85ef2a2af69a905f886936b60f6e0435959809625e7e128a716",{{16,false,1,"t3"},{17,false,1,"t4"},{15,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(1.0428279638290405, 0.014923558570444584, 1.1908085346221924, 0.9103469848632812, 1.2715349197387695, 0.9635223746299744, 1.3189589977264404, 0.9771658182144165);
    let P1 = array<f32, 8>(0.29251334071159363, 0.028385067358613014, 0.21297794580459595, 0.2029360681772232, 0.28699159622192383, 0.22290484607219696, 0.059083979576826096, -0.015475493855774403);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"5d4863c8b7a366583fe04eb22423ff907d6eca88acd8112cfc809ef9f05561a1",{{62,false,4,"wts"},{18,true,1,"b0"},{15,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.1.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=72){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=6960u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[7104u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,8,72}},
{"3a8c5ddc44760f66d0e78da04338c3fcd9dc900e5b98f9f40359cde8e306e525",{{18,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=13824u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 13824u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[24u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"00b648654adf62f7e5e2ee1763bd6795e5df2f83627bccbf056cca427f871240",{{60,false,4,"coef"},{18,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/13824u;
let channel_scale_shift=coef[24u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"5f4f163a6099271bdd8d6c9bb436dc399b0f2ae410444f05a99d5ef1a8340e32",{{18,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.15576818585395813, 1.2868891954421997, 0.573776125907898, 0.4996045231819153, 1.3239574432373047, 0.5896697640419006, 0.6019774675369263, 0.4583113193511963);
    let P1 = array<f32, 8>(0.0016845745267346501, -0.04538978263735771, 0.15842919051647186, 0.11213959008455276, -0.16300755739212036, -0.33581867814064026, -0.16958673298358917, -0.06238948181271553);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"bb535b4c577a56d6904f6c87140f0ce7718caec39462581e1a20b7e9e4937d58",{{62,false,4,"wts"},{19,true,1,"b0"},{18,false,1,"b1"},{14,false,1,"b2"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.1.1/downs.1.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c / 4u; let sy = 2u * u32(y) + ((c % 4u) / 2u); let sx = 2u * u32(x) + (c % 2u);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=32){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/32;let kx=(q/32)%1;let ch=q%32;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=32 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/32);let kx=u32((row/32)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=7120u+(((grp*32u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[7248u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,16,32}},
{"abe6b9f64f988fa01792ea1105a627bade3fc1c6745aab6e26ba8a5f7e9c9a23",{{62,false,4,"wts"},{20,true,1,"b0"},{19,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.2.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=144){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=7264u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[7840u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,16,144}},
{"3f6adf9eb2da2084519b48b8b8b2d330befe16d1b99b06b60f6e1167d0a5538a",{{20,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=6912u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 6912u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[32u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"a1a63e6ae8d34e1f1745a70fed4038eef23d98af04549e0d328d92d66af0c7bb",{{60,false,4,"coef"},{20,true,4,"raw"}},{64,1,1},{432,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/6912u;
let channel_scale_shift=coef[32u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"a0475891668462bebfeec10a85c6ef4fd372ad10cc4e8cf6ce9fd7650fa6d2eb",{{21,false,1,"t3"},{22,false,1,"t4"},{20,true,4,"raw"}},{64,1,1},{432,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.0209269523620605, 0.9248598217964172, 0.9582107067108154, 1.103807806968689, 1.0603458881378174, 1.067327857017517, 1.0920909643173218, 1.0016498565673828, 1.1540296077728271, 0.8908594846725464, 1.369123935699463, 1.130449652671814, 0.8257385492324829, 0.8000621199607849, 0.3224443793296814, 1.3338093757629395);
    let P1 = array<f32, 16>(0.117359459400177, 0.20187805593013763, 0.10750523954629898, 0.033330779522657394, 0.19661106169223785, 0.22223800420761108, 0.05804101377725601, 0.1299867033958435, -0.2936958074569702, 0.15021075308322906, 0.11419705301523209, 0.001880043651908636, 0.21458938717842102, 0.16501425206661224, 0.09037001430988312, 0.11944371461868286);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"c39952678293a159807898a7d0671c4401af5fdfd0297c2e4e7b93bf7c281ce3",{{62,false,4,"wts"},{23,true,1,"b0"},{20,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.2.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=144){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=7856u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[8432u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,16,144}},
{"843899450db926fad715c8a757487f273ac53f2203dce2d635418b0d35707646",{{23,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=6912u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 6912u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[48u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"70f625b00304cc779e473680a7699f9d85df68acac5a193a9b8bfdffa3b5a87d",{{60,false,4,"coef"},{23,true,4,"raw"}},{64,1,1},{432,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/6912u;
let channel_scale_shift=coef[48u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"f0f0f1e11bee1cfc34596621801c95d0f3c8ab38c33510069f8e0e89f3e06ea6",{{23,true,4,"raw"}},{64,1,1},{432,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(0.7623137831687927, 0.47251319885253906, 0.30448591709136963, 0.5249462127685547, 0.35084542632102966, 0.38846078515052795, 0.19160757958889008, 0.5472601652145386, 0.1673624962568283, 0.5664498805999756, 0.5249688029289246, 0.641364574432373, 0.4932307302951813, 0.3702468276023865, 0.5193073749542236, 0.4533976912498474);
    let P1 = array<f32, 16>(-0.14899942278862, -0.041682515293359756, 0.15352407097816467, -0.07781807333230972, -0.083533875644207, 0.011060367338359356, -0.2075349986553192, -0.02444523200392723, -0.06823026388883591, -0.3815551698207855, 0.05100448429584503, 0.0689125657081604, -0.025715690106153488, 0.17757229506969452, -0.045061904937028885, -0.11552629619836807);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"7293d63638ca9831b7fdafd16589bf61ae33c3ddec4a9782a099583f0f725c2e",{{62,false,4,"wts"},{24,true,1,"b0"},{23,false,1,"b1"},{19,false,1,"b2"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.2.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v + b2[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=144){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=8448u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[9600u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,144}},
{"804c1017132872033ea87e940403067a1cbc5e49fa9dfadda1c8bab44d3d7f31",{{62,false,4,"wts"},{26,true,1,"b0"},{27,true,1,"b1"},{25,false,1,"b2"},{24,false,1,"b3"}},{8,8,1},{2,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/gru/convz1_1/Conv, /core/update_block_depth3/unet/gru/convr1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  if (c < 52u) { return S1(c - 20u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=260){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/260;let kx=(q/52)%5;let ch=q%52;let iy=oy+ky-0;let ix=ox+kx-2;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=260 || colIn*4>=40){return vec4<f32>(0.0);}
 let ch=u32(row%52);let ky=u32(row/260);let kx=u32((row/52)%5);let col=u32(colIn*4);let grp=col/20u;let mv=(col%20u)/4u;
 let ix=9616u+(((grp*52u+ch)*1u+ky)*5u+kx)*5u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=40){return;}
 let pix=u32(row);let value=valueIn+wts[12216u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[1u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[2u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[3u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 1u:{
b0[4u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[5u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[6u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[7u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 2u:{
b0[8u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[9u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[10u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[11u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 3u:{
b0[12u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[13u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[14u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[15u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 4u:{
b0[16u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[17u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[18u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[19u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 5u:{
b1[0u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[1u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[2u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[3u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 6u:{
b1[4u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[5u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[6u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[7u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 7u:{
b1[8u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[9u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[10u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[11u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 8u:{
b1[12u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[13u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[14u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[15u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 9u:{
b1[16u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[17u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[18u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[19u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,40,260}},
{"071b5d5b957b013cfaf092369959f20e9c04a4250731c4aa7045412b09c52811",{{62,false,4,"wts"},{28,true,1,"b0"},{27,false,1,"b1"},{25,false,1,"b2"},{24,false,1,"b3"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/gru/convq1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v * b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  if (c < 52u) { return S1(c - 20u, y, x); }
  return 0.0;
}

fn original_tanh(a: vec4<f32>) -> vec4<f32> {
  let expr = exp(-2 * abs(a));
  return sign(a) * (1 - expr) / (1 + expr);
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=260){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/260;let kx=(q/52)%5;let ch=q%52;let iy=oy+ky-0;let ix=ox+kx-2;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=260 || colIn*4>=20){return vec4<f32>(0.0);}
 let ch=u32(row%52);let ky=u32(row/260);let kx=u32((row/52)%5);let col=u32(colIn*4);let grp=col/20u;let mv=(col%20u)/4u;
 let ix=12240u+(((grp*52u+ch)*1u+ky)*5u+kx)*5u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=20){return;}
 let pix=u32(row);let value=valueIn+wts[13540u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=original_tanh(value).x;
b0[1u*6912u+pix]=original_tanh(value).y;
b0[2u*6912u+pix]=original_tanh(value).z;
b0[3u*6912u+pix]=original_tanh(value).w;
}
case 1u:{
b0[4u*6912u+pix]=original_tanh(value).x;
b0[5u*6912u+pix]=original_tanh(value).y;
b0[6u*6912u+pix]=original_tanh(value).z;
b0[7u*6912u+pix]=original_tanh(value).w;
}
case 2u:{
b0[8u*6912u+pix]=original_tanh(value).x;
b0[9u*6912u+pix]=original_tanh(value).y;
b0[10u*6912u+pix]=original_tanh(value).z;
b0[11u*6912u+pix]=original_tanh(value).w;
}
case 3u:{
b0[12u*6912u+pix]=original_tanh(value).x;
b0[13u*6912u+pix]=original_tanh(value).y;
b0[14u*6912u+pix]=original_tanh(value).z;
b0[15u*6912u+pix]=original_tanh(value).w;
}
case 4u:{
b0[16u*6912u+pix]=original_tanh(value).x;
b0[17u*6912u+pix]=original_tanh(value).y;
b0[18u*6912u+pix]=original_tanh(value).z;
b0[19u*6912u+pix]=original_tanh(value).w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,20,260}},
{"55ee8b96245fef74cafaf62e34b50798d9d2affc3d3ab5e143aba318115cfcb7",{{26,false,4,"z"},{25,false,4,"h"},{28,true,4,"raw"}},{64,1,1},{540,1,1},R"WGSL_CODE(var<workgroup> sub_scratch:array<vec4<f32>,64>;
var<workgroup> hm_scratch:array<vec4<f32>,64>;
var<workgroup> zq_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(let av = vec4<f32>(1.0);
let bv = z[global_idx];
sub_scratch[local_idx] = av - bv;
workgroupBarrier();
let a_hm = sub_scratch[local_idx];
let b_hm = h[global_idx];
hm_scratch[local_idx] = a_hm * b_hm;
let a_zq = z[global_idx];
let b_zq = raw[global_idx];
zq_scratch[local_idx] = a_zq * b_zq;
workgroupBarrier();
let a = hm_scratch[local_idx];
let b = zq_scratch[local_idx];
raw[global_idx]=a + b;
)WGSL_CODE",0,{0,0,0}},
{"5918d81828cb93576f494131e1d3abc935ee66a5ee9adf16cb5790c640be0909",{{62,false,4,"wts"},{29,true,1,"b0"},{30,true,1,"b1"},{28,false,1,"b2"},{24,false,1,"b3"}},{8,8,1},{2,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/gru/convz2_1/Conv, /core/update_block_depth3/unet/gru/convr2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  if (c < 52u) { return S1(c - 20u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=260){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/52;let kx=(q/52)%1;let ch=q%52;let iy=oy+ky-2;let ix=ox+kx-0;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=260 || colIn*4>=40){return vec4<f32>(0.0);}
 let ch=u32(row%52);let ky=u32(row/52);let kx=u32((row/52)%1);let col=u32(colIn*4);let grp=col/20u;let mv=(col%20u)/4u;
 let ix=13552u+(((grp*52u+ch)*5u+ky)*1u+kx)*5u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=40){return;}
 let pix=u32(row);let value=valueIn+wts[16152u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[1u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[2u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[3u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 1u:{
b0[4u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[5u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[6u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[7u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 2u:{
b0[8u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[9u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[10u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[11u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 3u:{
b0[12u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[13u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[14u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[15u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 4u:{
b0[16u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b0[17u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b0[18u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b0[19u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 5u:{
b1[0u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[1u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[2u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[3u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 6u:{
b1[4u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[5u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[6u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[7u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 7u:{
b1[8u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[9u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[10u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[11u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 8u:{
b1[12u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[13u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[14u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[15u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 9u:{
b1[16u*6912u+pix]=1.0 / (1.0 + exp(-value.x));
b1[17u*6912u+pix]=1.0 / (1.0 + exp(-value.y));
b1[18u*6912u+pix]=1.0 / (1.0 + exp(-value.z));
b1[19u*6912u+pix]=1.0 / (1.0 + exp(-value.w));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,40,260}},
{"b0031b0f39c44290511fffa88a7a8834ccbc6e0bfa19017973a56b40b5974895",{{62,false,4,"wts"},{31,true,1,"b0"},{30,false,1,"b1"},{28,false,1,"b2"},{24,false,1,"b3"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/gru/convq2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v * b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  if (c < 52u) { return S1(c - 20u, y, x); }
  return 0.0;
}

fn original_tanh(a: vec4<f32>) -> vec4<f32> {
  let expr = exp(-2 * abs(a));
  return sign(a) * (1 - expr) / (1 + expr);
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=260){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/52;let kx=(q/52)%1;let ch=q%52;let iy=oy+ky-2;let ix=ox+kx-0;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=260 || colIn*4>=20){return vec4<f32>(0.0);}
 let ch=u32(row%52);let ky=u32(row/52);let kx=u32((row/52)%1);let col=u32(colIn*4);let grp=col/20u;let mv=(col%20u)/4u;
 let ix=16176u+(((grp*52u+ch)*5u+ky)*1u+kx)*5u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=20){return;}
 let pix=u32(row);let value=valueIn+wts[17476u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=original_tanh(value).x;
b0[1u*6912u+pix]=original_tanh(value).y;
b0[2u*6912u+pix]=original_tanh(value).z;
b0[3u*6912u+pix]=original_tanh(value).w;
}
case 1u:{
b0[4u*6912u+pix]=original_tanh(value).x;
b0[5u*6912u+pix]=original_tanh(value).y;
b0[6u*6912u+pix]=original_tanh(value).z;
b0[7u*6912u+pix]=original_tanh(value).w;
}
case 2u:{
b0[8u*6912u+pix]=original_tanh(value).x;
b0[9u*6912u+pix]=original_tanh(value).y;
b0[10u*6912u+pix]=original_tanh(value).z;
b0[11u*6912u+pix]=original_tanh(value).w;
}
case 3u:{
b0[12u*6912u+pix]=original_tanh(value).x;
b0[13u*6912u+pix]=original_tanh(value).y;
b0[14u*6912u+pix]=original_tanh(value).z;
b0[15u*6912u+pix]=original_tanh(value).w;
}
case 4u:{
b0[16u*6912u+pix]=original_tanh(value).x;
b0[17u*6912u+pix]=original_tanh(value).y;
b0[18u*6912u+pix]=original_tanh(value).z;
b0[19u*6912u+pix]=original_tanh(value).w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,20,260}},
{"c3e4d71bd1a2410b403e361977ba15e06bc625f66f0d508ea3e66f137bd963ce",{{29,false,4,"z"},{28,false,4,"h"},{31,true,4,"raw"}},{64,1,1},{540,1,1},R"WGSL_CODE(var<workgroup> sub_scratch:array<vec4<f32>,64>;
var<workgroup> hm_scratch:array<vec4<f32>,64>;
var<workgroup> zq_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(let av = vec4<f32>(1.0);
let bv = z[global_idx];
sub_scratch[local_idx] = av - bv;
workgroupBarrier();
let a_hm = sub_scratch[local_idx];
let b_hm = h[global_idx];
hm_scratch[local_idx] = a_hm * b_hm;
let a_zq = z[global_idx];
let b_zq = raw[global_idx];
zq_scratch[local_idx] = a_zq * b_zq;
workgroupBarrier();
let a = hm_scratch[local_idx];
let b = zq_scratch[local_idx];
raw[global_idx]=a + b;
)WGSL_CODE",0,{0,0,0}},
{"1c2236a07c03a74d7ffadf7bb22f39fa67b46f1519621a524e600cf19503ddf0",{{62,false,4,"wts"},{32,true,1,"b0"},{31,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/mid/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=180){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/60;let kx=(q/20)%3;let ch=q%20;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=180 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%20);let ky=u32(row/60);let kx=u32((row/20)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=17488u+(((grp*24u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[19216u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,180}},
{"6eb08db98e60110b28df7a0fef89eb5db5516717148fb9fe03c52a988acff333",{{32,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=13824u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 13824u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[64u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"5e89feca03ca919ec56d0dc6b73dc3783cd12a9701b5a8ab5a5cfece1729382e",{{60,false,4,"coef"},{32,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/13824u;
let channel_scale_shift=coef[64u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"2663f0573fbe8af983f2c8b930d6b49549cc6465b61d4bdb61658b0660b8f175",{{32,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.0066016912460327, 0.9786734580993652, 0.9980636835098267, 1.383861780166626, 0.5653182864189148, 0.8289008140563965, 1.2726531028747559, 1.3360278606414795, 0.9863739609718323, 0.79414302110672, 0.9148256182670593, 0.5539575219154358, 1.2147337198257446, 1.329681396484375, 0.9587805271148682, 0.9294324517250061, 1.0575658082962036, 1.381798267364502, 0.9386388659477234, 1.329537272453308, 0.515467643737793, 0.9772318601608276, 1.1901066303253174, 1.300421953201294, 1.6998525857925415, 0.549810528755188, 1.4652369022369385, 1.5634791851043701, 0.3340718448162079, 1.1365104913711548, 0.8679242134094238, 0.9671586751937866);
    let P1 = array<f32, 32>(-0.17669640481472015, -0.08895508199930191, -0.16099676489830017, -0.25679975748062134, -0.13155943155288696, -0.239667609333992, -0.3061389625072479, -0.6148698925971985, -0.12502118945121765, -0.13115721940994263, -0.22479499876499176, -0.089290551841259, -0.26162973046302795, -0.10780546814203262, -0.011291858740150928, -0.057862695306539536, -0.18267320096492767, -0.246023029088974, -0.1364057958126068, -0.2677566707134247, -0.22633549571037292, -0.24839939177036285, -0.3910634517669678, -0.17659923434257507, -0.4905271530151367, -0.11707214266061783, -0.2661204934120178, -0.5442976951599121, 0.09538312256336212, -0.16954822838306427, -0.1217859536409378, -0.2531115412712097);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"5b8d2591a32c2ee4a328e4e506319ae515bac43120cb77594b4f7b81b80eb72a",{{62,false,4,"wts"},{33,true,1,"b0"},{32,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/mid/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=288){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=19232u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[21536u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,288}},
{"1906b50bf43fe490cc8a0122ca6a362059ad01c76aad6bbb95ae61fe4d45cef4",{{33,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=13824u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 13824u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[96u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"f031d6661133ecbae7ed968d6ff0f8a217e0a3fe6d1570cb574dec6d587187d2",{{60,false,4,"coef"},{33,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/13824u;
let channel_scale_shift=coef[96u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"776c04b70f8ed7be790b87f3ab62df8989813639938d291bb66e3215c957dec2",{{33,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(0.5081897377967834, 0.7644691467285156, 0.8290867805480957, 0.8986796140670776, 0.762510359287262, 0.9900121092796326, 0.7823984026908875, 0.7295272350311279, 0.9870957732200623, 1.130997896194458, 0.875751256942749, 0.527197539806366, 0.796534538269043, 0.9716984033584595, 0.9013031125068665, 0.7536650896072388, 1.3932762145996094, 0.6555570363998413, 1.3508672714233398, 0.9287335872650146, 0.5928181409835815, 0.8442092537879944, 0.7362903952598572, 1.0761736631393433, 0.741619348526001, 1.1173399686813354, 0.3736945390701294, 0.9247899651527405, 0.9568728804588318, 0.8959638476371765, 0.5265315771102905, 0.26163533329963684);
    let P1 = array<f32, 32>(-0.09987767785787582, -0.13438884913921356, -0.05877642333507538, -0.23824474215507507, -0.13941921293735504, 0.12485743314027786, -0.13739053905010223, -0.0823148638010025, -0.1334325075149536, -0.35526829957962036, -0.37519562244415283, -0.05435401573777199, 0.016101744025945663, -0.4732634127140045, -0.1872863918542862, 0.06418627500534058, -0.6200655102729797, -0.21545864641666412, -0.7228708267211914, 0.03446994721889496, -0.2717108726501465, -0.05381655693054199, -0.09330607205629349, -0.281766802072525, -0.07468615472316742, 0.06546895205974579, -0.09822670370340347, -0.04003569483757019, 0.1357114315032959, -0.30019211769104004, -0.2645820081233978, -0.2570459246635437);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"c6ec89fc8a4ae53304c55a47071146da76bd0f76cca9cb7e06ca999e7a20cdbf",{{62,false,4,"wts"},{34,true,1,"b0"},{31,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/mid/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=20){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/20;let kx=(q/20)%1;let ch=q%20;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=20 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%20);let ky=u32(row/20);let kx=u32((row/20)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=21552u+(((grp*24u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[21744u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,20}},
{"595ad87a0b773be8fccaee5ad198ff2c41597861c3a4ef01ac77a2c1dd7959cd",{{62,false,4,"wts"},{35,true,1,"b0"},{33,false,1,"b1"},{34,false,1,"b2"},{23,false,1,"b3"},{19,false,1,"b4"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.0.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v + b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  v = v + b4[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 48u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=432){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/144;let kx=(q/48)%3;let ch=q%48;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=432 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%48);let ky=u32(row/144);let kx=u32((row/48)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=21760u+(((grp*48u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[25216u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,432}},
{"f6e182c5a2216cdcaca51dd672b10d42e3f934b0b93c74fdc63845dc0f400695",{{35,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=13824u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 13824u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[128u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"8e0c0096a434978b7a007013618dbd6b255ee9c624678182b7333b71dc8b9654",{{60,false,4,"coef"},{35,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/13824u;
let channel_scale_shift=coef[128u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"d32e0d523677b6edfbd7d009d6a2ca75cd2d3cd55b08c2bda29cccd20640fff7",{{36,false,1,"t3"},{37,false,1,"t4"},{35,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.3063278198242188, 1.2151318788528442, 1.1210111379623413, 1.3393210172653198, 0.6415340304374695, 1.1968353986740112, 1.2397879362106323, 0.8706600666046143, 1.1680362224578857, 1.0089877843856812, 1.1467055082321167, 1.1902170181274414, 1.1108332872390747, 1.1439334154129028, 1.2504383325576782, 0.506637692451477, 1.229719638824463, 1.1761924028396606, 0.8377634882926941, 1.1697442531585693, 0.9963582754135132, 1.1879781484603882, 1.1819031238555908, 1.084058403968811, 0.9828874468803406, 1.060011863708496, 0.729789674282074, 0.49181991815567017, 0.7541927099227905, 1.0839061737060547, 1.0446860790252686, 1.3604838848114014);
    let P1 = array<f32, 32>(-0.16197475790977478, -0.18183565139770508, 0.11513058096170425, -0.02175852097570896, -0.07427284121513367, 0.07947926968336105, -0.20542098581790924, -0.037493616342544556, -0.00292900949716568, -0.30592742562294006, -0.36552736163139343, -0.09798290580511093, -0.44761160016059875, -0.13710616528987885, -0.1656971126794815, -0.19841228425502777, 0.15257515013217926, -0.20376916229724884, -0.1494400054216385, -0.1402382254600525, 0.13688579201698303, 0.053514011204242706, -0.020509470254182816, -0.07318919152021408, -0.10586881637573242, -0.3925066888332367, 0.08316633850336075, -0.019759323447942734, 0.04890650138258934, -0.297044575214386, -0.017716679722070694, -0.2411116361618042);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"92b9fb1f02fb34ba627ac5f6ba4033c860fa569a2a8de772be88c932fae024a9",{{62,false,4,"wts"},{38,true,1,"b0"},{35,false,1,"b1"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.0.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=288){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=25232u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[27536u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,288}},
{"4f9e9db86cea637dce76e3d476172ce3e56bf561064861828ae8bf70be06b500",{{38,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=13824u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 13824u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[160u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"a30aa6eb529af43a2959076e024ae4844bd6957f53b3699b6ed1f1cb230e0766",{{60,false,4,"coef"},{38,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/13824u;
let channel_scale_shift=coef[160u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"4753c418430b42f39feb4fd66514bf69730013bfd5e9033d70548f9a50fe38de",{{38,true,4,"raw"}},{64,1,1},{864,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.2273502349853516, 1.9275903701782227, 1.3996951580047607, 0.51876300573349, 0.6036960482597351, 0.925854504108429, 2.1969854831695557, 0.9352912902832031, 1.4522594213485718, 1.1071077585220337, 1.263318657875061, 0.8660377264022827, 1.3773139715194702, 0.25398877263069153, 1.1053874492645264, 1.5405441522598267, 0.7302138805389404, 1.4792861938476562, 0.9397917985916138, 1.1901124715805054, 0.561262845993042, 0.8749394416809082, 0.7628927230834961, 0.5252271294593811, 1.0254178047180176, 0.5998544692993164, 0.6686235666275024, 1.03556489944458, 1.750168800354004, 1.4584732055664062, 2.0559194087982178, 0.5781452059745789);
    let P1 = array<f32, 32>(-0.25200989842414856, -1.1468274593353271, 0.09846160560846329, -0.3203883469104767, 0.04753804951906204, 0.04152490198612213, -0.2486402541399002, -0.5093510746955872, -0.2127590924501419, -0.6425765752792358, -0.26733407378196716, -0.023601679131388664, -0.20476870238780975, -0.1414472758769989, -0.36965397000312805, -0.7867390513420105, 0.13105562329292297, -0.8789019584655762, -0.34256643056869507, 0.22001755237579346, -0.09231618791818619, -0.16366301476955414, -0.103691466152668, 0.00915013812482357, -0.3460608422756195, -0.49706703424453735, -0.1851155161857605, 0.08504112809896469, 0.07026352733373642, -0.014422710053622723, -0.1667596697807312, 0.015447761863470078);
let channel=global_idx*4u/6912u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"bc48ff9b585495c3bda061316122ae569d83fa8620b1b5b9d531c9523d082eaf",{{62,false,4,"wts"},{39,true,1,"b0"},{33,false,1,"b1"},{34,false,1,"b2"},{23,false,1,"b3"},{19,false,1,"b4"}},{8,8,1},{1,216,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.0.0/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v + b2[(0u + sc) * 6912u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 96u + sx;
  var v = b3[(0u + sc) * 6912u + p];
  v = v + b4[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 48u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=6912 || q<0 || q>=48){return 0.0;}
 let oy=row/96;let ox=row%96;let ky=q/48;let kx=(q/48)%1;let ch=q%48;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=72 || ix<0 || ix>=96){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=48 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%48);let ky=u32(row/48);let kx=u32((row/48)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=27552u+(((grp*48u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=6912 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[27936u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*6912u+pix]=value.x;
b0[1u*6912u+pix]=value.y;
b0[2u*6912u+pix]=value.z;
b0[3u*6912u+pix]=value.w;
}
case 1u:{
b0[4u*6912u+pix]=value.x;
b0[5u*6912u+pix]=value.y;
b0[6u*6912u+pix]=value.z;
b0[7u*6912u+pix]=value.w;
}
case 2u:{
b0[8u*6912u+pix]=value.x;
b0[9u*6912u+pix]=value.y;
b0[10u*6912u+pix]=value.z;
b0[11u*6912u+pix]=value.w;
}
case 3u:{
b0[12u*6912u+pix]=value.x;
b0[13u*6912u+pix]=value.y;
b0[14u*6912u+pix]=value.z;
b0[15u*6912u+pix]=value.w;
}
case 4u:{
b0[16u*6912u+pix]=value.x;
b0[17u*6912u+pix]=value.y;
b0[18u*6912u+pix]=value.z;
b0[19u*6912u+pix]=value.w;
}
case 5u:{
b0[20u*6912u+pix]=value.x;
b0[21u*6912u+pix]=value.y;
b0[22u*6912u+pix]=value.z;
b0[23u*6912u+pix]=value.w;
}
case 6u:{
b0[24u*6912u+pix]=value.x;
b0[25u*6912u+pix]=value.y;
b0[26u*6912u+pix]=value.z;
b0[27u*6912u+pix]=value.w;
}
case 7u:{
b0[28u*6912u+pix]=value.x;
b0[29u*6912u+pix]=value.y;
b0[30u*6912u+pix]=value.z;
b0[31u*6912u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{6912,32,48}},
{"13d63e9bfd2e95ed668450624b4c3b70de361a52f33c539ea20bdbfb44e45120",{{62,false,4,"wts"},{40,true,1,"b0"},{38,false,1,"b1"},{39,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.0.1/ups.0.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y) >> 1u; let sx = u32(x) >> 1u;
  let p = sy * 96u + sx;
  var v = b1[(0u + sc) * 6912u + p];
  v = v + b2[(0u + sc) * 6912u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=27952u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[29104u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,288}},
{"87dede185afe1b15b730ff5dff1ee36b64246ffdbd0ccac4334247328f716c54",{{62,false,4,"wts"},{41,true,1,"b0"},{40,false,1,"b1"},{18,false,1,"b2"},{14,false,1,"b3"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.1.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  v = v + b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 24u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=216){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/72;let kx=(q/24)%3;let ch=q%24;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=216 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%24);let ky=u32(row/72);let kx=u32((row/24)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=29120u+(((grp*24u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[29984u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,216}},
{"dc038ba8f09fb05353fa59046a892dee6c8a9760f4c746da2175e6fe5d73856b",{{41,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[192u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"908a5c0ce63d63d6d5ff01051969794eef259e060dbb9ce42ea81d257f8e8450",{{60,false,4,"coef"},{41,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[192u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"db6d8356de1309b527a1015492c95c5e27d43091bbf80b2d30163efaaf4cfb3d",{{42,false,1,"t3"},{43,false,1,"t4"},{41,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.071933388710022, 0.6144050359725952, 0.8586381673812866, 1.3540096282958984, 1.3308886289596558, 1.5460915565490723, 0.7262976765632629, 0.9540891051292419, 0.9519808888435364, 1.113931655883789, 0.9017605185508728, 0.9464331865310669, 0.9993612170219421, 1.0123021602630615, 1.3898520469665527, 1.4444743394851685);
    let P1 = array<f32, 16>(-0.03616371005773544, -0.17774008214473724, -0.011179293505847454, -0.560824990272522, -0.21618051826953888, -0.29197508096694946, -0.10306720435619354, -0.01993977651000023, -0.12736448645591736, 0.07968734204769135, 0.0661754459142685, 0.06390447914600372, -0.21356584131717682, 0.05773831158876419, -0.4434376060962677, -0.5049852728843689);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"a54d4eeb9610f46ee30e721cf001259d5a4ce77662a5cc235ac98060d0b5188c",{{62,false,4,"wts"},{44,true,1,"b0"},{41,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.1.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=30000u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[30576u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,144}},
{"9c59ce9e2a9b000df0ce0d85a27bd92913a2e40dc4b017444d0040b4eb80051d",{{44,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[208u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"7685d836d6338b1b3b4be938b19c55cdd1c25bfbe6e9f51a3eda799edafad8ca",{{60,false,4,"coef"},{44,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[208u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"19ca3af3cb8b83171fb0776c241ee1addbdc034dc7803fc588d65fb234e9db12",{{44,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.384238600730896, 1.5721830129623413, 1.3906223773956299, 0.9189033508300781, 1.4390791654586792, 1.2698345184326172, 2.1414549350738525, 2.0327465534210205, 1.8643683195114136, 1.991784930229187, 2.0229380130767822, 1.2356288433074951, 0.9946795701980591, 1.0031911134719849, 1.0363576412200928, 1.3235195875167847);
    let P1 = array<f32, 16>(-0.0020258056465536356, -0.29081201553344727, -0.007741477806121111, 0.26457324624061584, -0.03336023911833763, 0.10371503233909607, -0.8393456339836121, -0.465729296207428, -0.21286973357200623, -0.5399280190467834, -0.4001143276691437, -0.04728317633271217, -0.22817765176296234, -0.6505697965621948, -0.05977911129593849, -0.05523374304175377);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"49f4a039e60f61e73cd02d49a05b411364bf91f64667c7b8432f25c90988528d",{{62,false,4,"wts"},{45,true,1,"b0"},{40,false,1,"b1"},{18,false,1,"b2"},{14,false,1,"b3"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.1.0/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  v = v + b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 24u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=24){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/24;let kx=(q/24)%1;let ch=q%24;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=24 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%24);let ky=u32(row/24);let kx=u32((row/24)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=30592u+(((grp*24u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[30688u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,24}},
{"59400754ab2c91ab894c8d1c07ee646a7c2c478641e57a41185f5d572d06bc2a",{{62,false,4,"wts"},{46,true,1,"b0"},{44,false,1,"b1"},{45,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.1.1/ups.1.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y) >> 1u; let sx = u32(x) >> 1u;
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=30704u+(((grp*16u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[30992u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,144}},
{"d6c46dea49d873755fe1e1b990db53c583340d2ece3217624f35027d5ef73d70",{{62,false,4,"wts"},{47,true,1,"b0"},{46,false,1,"b1"},{13,false,1,"b2"},{9,false,1,"b3"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.2.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  v = v + b3[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  if (c < 16u) { return S1(c - 8u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31008u+(((grp*16u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31296u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,144}},
{"c47737512aa268f6b2f34e2791b3f9a58b895409a06f3a032af440f4f0a942d4",{{47,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[224u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"bf9ac64e5d92b2a4c9a6359172c057a8e98adeb980c38fbfd13cf9a5e6a5b29e",{{60,false,4,"coef"},{47,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[224u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"1094fe3b090de73aa14e9662ec5e55dfa411d624359073edd8121482c7649e0c",{{48,false,1,"t3"},{49,false,1,"t4"},{47,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.8507720232009888, 0.6434476971626282, 1.2006244659423828, 0.642288863658905, 0.7959140539169312, 0.9013049006462097, 1.0159988403320312, 1.2116272449493408);
    let P1 = array<f32, 8>(-0.23002560436725616, -0.1665038764476776, -0.4210740029811859, -0.16059842705726624, -0.2862122058868408, -0.2962667942047119, -0.006381908897310495, -0.4407266676425934);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"b4c727d32f9f367115a3fe94ba38b9803756e880c07702260706cc9918a2a5c9",{{62,false,4,"wts"},{50,true,1,"b0"},{47,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.2.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=72){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31312u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31456u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,72}},
{"4ed59c0295c037ffbadb9afb9b482f615b6bafd20c58fa1b278c57bba3f6c5bd",{{50,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[232u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"4543aca9949e771d095ffd1a5d62fafd1eb7f3cea9a628873021906db6dd6c66",{{60,false,4,"coef"},{50,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[232u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"727cfe436a543d77c461df5a0b82e90517b3f88bd14a2de298715c75250e3728",{{50,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.8066206574440002, 0.7152596116065979, 0.7844045162200928, 0.9872947335243225, 0.7914931178092957, 1.1699351072311401, 1.1401056051254272, 0.8229765295982361);
    let P1 = array<f32, 8>(0.17504765093326569, 0.498104453086853, 0.4720593988895416, 0.008328877389431, 0.3995059132575989, 0.18382272124290466, -0.06084665283560753, 0.3168374300003052);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"27f1676dd360995285846f0309f60d952ed0a030d2d038004c175e1caf92e117",{{62,false,4,"wts"},{51,true,1,"b0"},{46,false,1,"b1"},{13,false,1,"b2"},{9,false,1,"b3"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.2.0/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  v = v + b3[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  if (c < 16u) { return S1(c - 8u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=16){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/16;let kx=(q/16)%1;let ch=q%16;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=16 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/16);let kx=u32((row/16)%1);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31472u+(((grp*16u+ch)*1u+ky)*1u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31504u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,16}},
{"3ab996f5610a6a03e05badf28f41f022f5546933ba54741e8aa76895335f7714",{{62,false,4,"wts"},{52,true,1,"b0"},{50,false,1,"b1"},{51,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.2.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  v = v + b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=72){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31520u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31664u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,72}},
{"cac4886ac53136ed6cd146b8ef2415f1f15a508d12541613455f90496dc180a2",{{62,false,4,"wts"},{53,true,1,"b0"},{52,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/final_res_block/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  if (c < 16u) { return S1(c - 8u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31680u+(((grp*16u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31968u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,144}},
{"60c9f593384f4d7fc02b9190c73ec5fa422e2e43c00a7530e9dfbaeb04ce38da",{{53,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[240u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"a2847b133b3061e1e96a345b7810d3b038a444ef8e892c6927839df7b1d6e452",{{60,false,4,"coef"},{53,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[240u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"46e3857ae47292b15752e29325c4370a5e833e00c872a5a334ef24a543eef977",{{54,false,1,"t3"},{55,false,1,"t4"},{53,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(1.0000128746032715, 1.419042944908142, 1.259379267692566, 1.2294976711273193, 1.2824100255966187, 0.8807469010353088, 1.0456726551055908, 0.506299614906311);
    let P1 = array<f32, 8>(-0.028392791748046875, -0.19860801100730896, 0.11362238973379135, 0.03532786667346954, -0.35307055711746216, 0.020872825756669044, -0.04130525514483452, -0.22246716916561127);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"e4aa904bee050da9048694e8a6d3ac336ae8b380dfc87ce9da809855b86ceea8",{{62,false,4,"wts"},{56,true,1,"b0"},{53,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/final_res_block/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=72){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31984u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[32128u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,72}},
{"ea62a4512325dfc803a2e03d50b4c7a025d2963c09715ce678eb7bac22780f1d",{{56,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[248u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"c5198fd6ce2130afb920248675e24849fbbaecfa7d6a73586a95d38bbc9bb766",{{60,false,4,"coef"},{56,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[248u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"fc273b5db6015df56d960a33b8ef164fbcd275f271a23e9bfd9782101513cefc",{{56,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.6700785756111145, -0.010936703532934189, 0.29364126920700073, 0.013072769157588482, 0.7185232043266296, -0.004493084270507097, 0.5402140021324158, 0.02248045802116394);
    let P1 = array<f32, 8>(-0.02259894087910652, -0.1038062646985054, -0.1199500635266304, -0.056562311947345734, 0.004977676551789045, 0.030465375632047653, -2.7194950234843418e-05, -0.14645349979400635);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"64e73fdea89d54830380aa29810fe198aac2114e8144cb4afaabd081d628f660",{{62,false,4,"wts"},{57,true,1,"b0"},{52,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/final_res_block/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  if (c < 16u) { return S1(c - 8u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=16){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/16;let kx=(q/16)%1;let ch=q%16;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=16 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/16);let kx=u32((row/16)%1);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=32144u+(((grp*16u+ch)*1u+ky)*1u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[32176u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,16}},
{"2824b9893ef6fc358785100c07da1a55df10cf557e77b0a7c70cf49adce155ae",{{62,false,4,"wts"},{58,true,1,"b0"},{59,true,1,"b1"},{56,false,1,"b2"},{57,false,1,"b3"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/final_conv_1/Conv, /core/update_block_depth3/unet/conf_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  v = v + b3[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=8){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/8;let kx=(q/8)%1;let ch=q%8;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=8 || colIn*4>=2){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/8);let kx=u32((row/8)%1);let col=u32(colIn*4);let grp=col/4u;let mv=(col%4u)/4u;
 let ix=32192u+(((grp*8u+ch)*1u+ky)*1u+kx)*1u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=2){return;}
 let pix=u32(row);let value=valueIn+wts[32200u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b1[0u*110592u+pix]=1.0 / (1.0 + exp(-value.y));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,2,8}},
},{{1,20,72,96},{1,1,288,384},{1,1,288,384}},{1769472,1769472,1769472,884736,884736,221184,884736,110592,442368,221184,221184}},
{16,{
{1769472,0,0,{1,4,4,288,384},-1},
{3538944,2,-1,{32,288,384},0},
{3538944,2,-1,{32,288,384},1},
{442368,0,1,{1,4,288,384},-1},
{3538944,2,-1,{32,288,384},0},
{3538944,2,-1,{32,288,384},2},
{3428352,2,-1,{31,288,384},0},
{3538944,0,2,{1,32,288,384},-1},
{110592,0,3,{1,1,288,384},-1},
{1769472,2,-1,{16,288,384},1},
{1769472,2,-1,{16,288,384},0},
{16,0,4,{1,16,1,1},-1},
{16,0,5,{1,16,1,1},-1},
{1769472,2,-1,{16,288,384},3},
{442368,2,-1,{16,144,192},0},
{442368,2,-1,{16,144,192},4},
{16,0,6,{1,16,1,1},-1},
{16,0,7,{1,16,1,1},-1},
{442368,2,-1,{16,144,192},5},
{884736,2,-1,{32,144,192},4},
{884736,0,8,{1,32,144,192},-1},
{884736,2,-1,{32,144,192},6},
{884736,2,-1,{32,144,192},7},
{884736,2,-1,{32,144,192},8},
{884736,2,-1,{32,144,192},6},
{884736,2,-1,{32,144,192},7},
{884736,1,0,{32,144,192},-1},
{884736,2,-1,{32,144,192},4},
{884736,2,-1,{32,144,192},6},
{884736,2,-1,{32,144,192},4},
{32,0,9,{1,32,1,1},-1},
{32,0,10,{1,32,1,1},-1},
{884736,2,-1,{32,144,192},7},
{884736,2,-1,{32,144,192},4},
{1769472,2,-1,{16,288,384},0},
{1769472,2,-1,{16,288,384},4},
{16,0,11,{1,16,1,1},-1},
{16,0,12,{1,16,1,1},-1},
{1769472,2,-1,{16,288,384},6},
{1769472,2,-1,{16,288,384},4},
{1769472,2,-1,{16,288,384},3},
{1769472,2,-1,{16,288,384},4},
{16,0,13,{1,16,1,1},-1},
{16,0,14,{1,16,1,1},-1},
{1769472,2,-1,{16,288,384},6},
{1769472,2,-1,{16,288,384},4},
{110592,1,1,{1,288,384},-1},
{110592,1,2,{1,288,384},-1},
{1024,2,-1,{1024},2},
{62208,2,-1,{62208},-1},
{235840,0,15,{235840},-1},
},{
{"ddf536f65d5da6d6851b8bcce28985e786d4a0f1dca0d46561b999829a836e03",{{50,false,4,"wts"},{1,true,1,"b0"},{0,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/encoder/convc1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=0u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[1152u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=max(value.x, 0.0);
b0[1u*110592u+pix]=max(value.y, 0.0);
b0[2u*110592u+pix]=max(value.z, 0.0);
b0[3u*110592u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*110592u+pix]=max(value.x, 0.0);
b0[5u*110592u+pix]=max(value.y, 0.0);
b0[6u*110592u+pix]=max(value.z, 0.0);
b0[7u*110592u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*110592u+pix]=max(value.x, 0.0);
b0[9u*110592u+pix]=max(value.y, 0.0);
b0[10u*110592u+pix]=max(value.z, 0.0);
b0[11u*110592u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*110592u+pix]=max(value.x, 0.0);
b0[13u*110592u+pix]=max(value.y, 0.0);
b0[14u*110592u+pix]=max(value.z, 0.0);
b0[15u*110592u+pix]=max(value.w, 0.0);
}
case 4u:{
b0[16u*110592u+pix]=max(value.x, 0.0);
b0[17u*110592u+pix]=max(value.y, 0.0);
b0[18u*110592u+pix]=max(value.z, 0.0);
b0[19u*110592u+pix]=max(value.w, 0.0);
}
case 5u:{
b0[20u*110592u+pix]=max(value.x, 0.0);
b0[21u*110592u+pix]=max(value.y, 0.0);
b0[22u*110592u+pix]=max(value.z, 0.0);
b0[23u*110592u+pix]=max(value.w, 0.0);
}
case 6u:{
b0[24u*110592u+pix]=max(value.x, 0.0);
b0[25u*110592u+pix]=max(value.y, 0.0);
b0[26u*110592u+pix]=max(value.z, 0.0);
b0[27u*110592u+pix]=max(value.w, 0.0);
}
case 7u:{
b0[28u*110592u+pix]=max(value.x, 0.0);
b0[29u*110592u+pix]=max(value.y, 0.0);
b0[30u*110592u+pix]=max(value.z, 0.0);
b0[31u*110592u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,32,144}},
{"e42847d40baaaf2efa4f0234055056b21fd6508687ddd2997246a3bb46357803",{{50,false,4,"wts"},{2,true,1,"b0"},{1,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/encoder/convc2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=288){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=1168u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[3472u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=max(value.x, 0.0);
b0[1u*110592u+pix]=max(value.y, 0.0);
b0[2u*110592u+pix]=max(value.z, 0.0);
b0[3u*110592u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*110592u+pix]=max(value.x, 0.0);
b0[5u*110592u+pix]=max(value.y, 0.0);
b0[6u*110592u+pix]=max(value.z, 0.0);
b0[7u*110592u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*110592u+pix]=max(value.x, 0.0);
b0[9u*110592u+pix]=max(value.y, 0.0);
b0[10u*110592u+pix]=max(value.z, 0.0);
b0[11u*110592u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*110592u+pix]=max(value.x, 0.0);
b0[13u*110592u+pix]=max(value.y, 0.0);
b0[14u*110592u+pix]=max(value.z, 0.0);
b0[15u*110592u+pix]=max(value.w, 0.0);
}
case 4u:{
b0[16u*110592u+pix]=max(value.x, 0.0);
b0[17u*110592u+pix]=max(value.y, 0.0);
b0[18u*110592u+pix]=max(value.z, 0.0);
b0[19u*110592u+pix]=max(value.w, 0.0);
}
case 5u:{
b0[20u*110592u+pix]=max(value.x, 0.0);
b0[21u*110592u+pix]=max(value.y, 0.0);
b0[22u*110592u+pix]=max(value.z, 0.0);
b0[23u*110592u+pix]=max(value.w, 0.0);
}
case 6u:{
b0[24u*110592u+pix]=max(value.x, 0.0);
b0[25u*110592u+pix]=max(value.y, 0.0);
b0[26u*110592u+pix]=max(value.z, 0.0);
b0[27u*110592u+pix]=max(value.w, 0.0);
}
case 7u:{
b0[28u*110592u+pix]=max(value.x, 0.0);
b0[29u*110592u+pix]=max(value.y, 0.0);
b0[30u*110592u+pix]=max(value.z, 0.0);
b0[31u*110592u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,32,288}},
{"e6c335bdce96da925685bb9d82f9ea3199abf2d02dfc1ddfaa9a5690ffd1e07e",{{50,false,4,"wts"},{4,true,1,"b0"},{3,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/encoder/convd1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 4u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=36){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/12;let kx=(q/4)%3;let ch=q%4;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=36 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%4);let ky=u32(row/12);let kx=u32((row/4)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=3488u+(((grp*4u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[3776u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=max(value.x, 0.0);
b0[1u*110592u+pix]=max(value.y, 0.0);
b0[2u*110592u+pix]=max(value.z, 0.0);
b0[3u*110592u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*110592u+pix]=max(value.x, 0.0);
b0[5u*110592u+pix]=max(value.y, 0.0);
b0[6u*110592u+pix]=max(value.z, 0.0);
b0[7u*110592u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*110592u+pix]=max(value.x, 0.0);
b0[9u*110592u+pix]=max(value.y, 0.0);
b0[10u*110592u+pix]=max(value.z, 0.0);
b0[11u*110592u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*110592u+pix]=max(value.x, 0.0);
b0[13u*110592u+pix]=max(value.y, 0.0);
b0[14u*110592u+pix]=max(value.z, 0.0);
b0[15u*110592u+pix]=max(value.w, 0.0);
}
case 4u:{
b0[16u*110592u+pix]=max(value.x, 0.0);
b0[17u*110592u+pix]=max(value.y, 0.0);
b0[18u*110592u+pix]=max(value.z, 0.0);
b0[19u*110592u+pix]=max(value.w, 0.0);
}
case 5u:{
b0[20u*110592u+pix]=max(value.x, 0.0);
b0[21u*110592u+pix]=max(value.y, 0.0);
b0[22u*110592u+pix]=max(value.z, 0.0);
b0[23u*110592u+pix]=max(value.w, 0.0);
}
case 6u:{
b0[24u*110592u+pix]=max(value.x, 0.0);
b0[25u*110592u+pix]=max(value.y, 0.0);
b0[26u*110592u+pix]=max(value.z, 0.0);
b0[27u*110592u+pix]=max(value.w, 0.0);
}
case 7u:{
b0[28u*110592u+pix]=max(value.x, 0.0);
b0[29u*110592u+pix]=max(value.y, 0.0);
b0[30u*110592u+pix]=max(value.z, 0.0);
b0[31u*110592u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,32,36}},
{"29c8d5582098baff8e699aaa5a5bdab7cd7ffcc4267f86ae681b87562e2fd616",{{50,false,4,"wts"},{5,true,1,"b0"},{4,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/encoder/convd2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=288){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=3792u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[6096u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=max(value.x, 0.0);
b0[1u*110592u+pix]=max(value.y, 0.0);
b0[2u*110592u+pix]=max(value.z, 0.0);
b0[3u*110592u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*110592u+pix]=max(value.x, 0.0);
b0[5u*110592u+pix]=max(value.y, 0.0);
b0[6u*110592u+pix]=max(value.z, 0.0);
b0[7u*110592u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*110592u+pix]=max(value.x, 0.0);
b0[9u*110592u+pix]=max(value.y, 0.0);
b0[10u*110592u+pix]=max(value.z, 0.0);
b0[11u*110592u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*110592u+pix]=max(value.x, 0.0);
b0[13u*110592u+pix]=max(value.y, 0.0);
b0[14u*110592u+pix]=max(value.z, 0.0);
b0[15u*110592u+pix]=max(value.w, 0.0);
}
case 4u:{
b0[16u*110592u+pix]=max(value.x, 0.0);
b0[17u*110592u+pix]=max(value.y, 0.0);
b0[18u*110592u+pix]=max(value.z, 0.0);
b0[19u*110592u+pix]=max(value.w, 0.0);
}
case 5u:{
b0[20u*110592u+pix]=max(value.x, 0.0);
b0[21u*110592u+pix]=max(value.y, 0.0);
b0[22u*110592u+pix]=max(value.z, 0.0);
b0[23u*110592u+pix]=max(value.w, 0.0);
}
case 6u:{
b0[24u*110592u+pix]=max(value.x, 0.0);
b0[25u*110592u+pix]=max(value.y, 0.0);
b0[26u*110592u+pix]=max(value.z, 0.0);
b0[27u*110592u+pix]=max(value.w, 0.0);
}
case 7u:{
b0[28u*110592u+pix]=max(value.x, 0.0);
b0[29u*110592u+pix]=max(value.y, 0.0);
b0[30u*110592u+pix]=max(value.z, 0.0);
b0[31u*110592u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,32,288}},
{"8f4cce69b1325b7f99d3c7c6da8b953bd5cca4c26ab1977b8047ef29c45b5161",{{50,false,4,"wts"},{6,true,1,"b0"},{2,false,1,"b1"},{5,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/encoder/output_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 64u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=576){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/192;let kx=(q/64)%3;let ch=q%64;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=576 || colIn*4>=31){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/192);let kx=u32((row/64)%3);let col=u32(colIn*4);let grp=col/32u;let mv=(col%32u)/4u;
 let ix=6112u+(((grp*64u+ch)*3u+ky)*3u+kx)*8u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=31){return;}
 let pix=u32(row);let value=valueIn+wts[10720u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=max(value.x, 0.0);
b0[1u*110592u+pix]=max(value.y, 0.0);
b0[2u*110592u+pix]=max(value.z, 0.0);
b0[3u*110592u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*110592u+pix]=max(value.x, 0.0);
b0[5u*110592u+pix]=max(value.y, 0.0);
b0[6u*110592u+pix]=max(value.z, 0.0);
b0[7u*110592u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*110592u+pix]=max(value.x, 0.0);
b0[9u*110592u+pix]=max(value.y, 0.0);
b0[10u*110592u+pix]=max(value.z, 0.0);
b0[11u*110592u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*110592u+pix]=max(value.x, 0.0);
b0[13u*110592u+pix]=max(value.y, 0.0);
b0[14u*110592u+pix]=max(value.z, 0.0);
b0[15u*110592u+pix]=max(value.w, 0.0);
}
case 4u:{
b0[16u*110592u+pix]=max(value.x, 0.0);
b0[17u*110592u+pix]=max(value.y, 0.0);
b0[18u*110592u+pix]=max(value.z, 0.0);
b0[19u*110592u+pix]=max(value.w, 0.0);
}
case 5u:{
b0[20u*110592u+pix]=max(value.x, 0.0);
b0[21u*110592u+pix]=max(value.y, 0.0);
b0[22u*110592u+pix]=max(value.z, 0.0);
b0[23u*110592u+pix]=max(value.w, 0.0);
}
case 6u:{
b0[24u*110592u+pix]=max(value.x, 0.0);
b0[25u*110592u+pix]=max(value.y, 0.0);
b0[26u*110592u+pix]=max(value.z, 0.0);
b0[27u*110592u+pix]=max(value.w, 0.0);
}
case 7u:{
b0[28u*110592u+pix]=max(value.x, 0.0);
b0[29u*110592u+pix]=max(value.y, 0.0);
b0[30u*110592u+pix]=max(value.z, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,31,576}},
{"989a0e1189c7f56fd158407eaf279adfd753cdd86ee884b0bb286c7d31dd53a3",{{50,false,4,"wts"},{9,true,1,"b0"},{7,false,1,"b1"},{6,false,1,"b2"},{8,false,1,"b3"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/init_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  return v;
}
fn S2(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b3[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 63u) { return S1(c - 32u, y, x); }
  if (c < 64u) { return S2(c - 63u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=3136){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/448;let kx=(q/64)%7;let ch=q%64;let iy=oy+ky-3;let ix=ox+kx-3;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=3136 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/448);let kx=u32((row/64)%7);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=10736u+(((grp*64u+ch)*7u+ky)*7u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[23280u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,3136}},
{"14428a8a99a9e391a23ecc784c786afbe7b8eed0ed5d0b6ba2a3458d75c58dab",{{50,false,4,"wts"},{10,true,1,"b0"},{9,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.0.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=23296u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[23872u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,144}},
{"5077c151cdb003d18c95a06fe14d37dca7f37a65f1066f9ac236a40ae2d8f7aa",{{10,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=110592u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 110592u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[0u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"f049479def6478d2ab40eede796c4edeab4a419594320d4a644b9ce8ec58a6cc",{{48,false,4,"coef"},{10,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/110592u;
let channel_scale_shift=coef[0u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"0f887a9a0836fc1902f22bfcb96226f66ec81620ba3ec3f70de88b07ca9822cb",{{11,false,1,"t3"},{12,false,1,"t4"},{10,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.1986364126205444, 0.5594167113304138, 1.0380840301513672, 1.1380891799926758, 0.8133781552314758, 1.0343083143234253, 1.2137706279754639, 0.030133645981550217, 0.9016430974006653, 0.5148126482963562, 0.7006478309631348, 1.2527954578399658, 0.9384449124336243, 0.8976232409477234, 1.084325909614563, 1.2279353141784668);
    let P1 = array<f32, 16>(-0.13004527986049652, -0.15402673184871674, -0.24893417954444885, -0.39097949862480164, 0.04706350341439247, -0.0242949016392231, -0.6387122869491577, -0.5896123647689819, -0.1273721307516098, -0.053643472492694855, 0.1896776258945465, -0.3317961096763611, 0.11920168995857239, -0.03751483932137489, -0.26678842306137085, -0.16058798134326935);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"585eebaf48732a86e8729fdac0f0f9c79db53ed71692e8a2124d2822ed62d1e6",{{50,false,4,"wts"},{13,true,1,"b0"},{10,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.0.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=23888u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[24464u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,144}},
{"26f196e6ea1e7c4f058df308e6e913af1ca678526c3ce9e9ab306469f8beca6b",{{13,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=110592u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 110592u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[16u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"8299a27aaba121780f16885a62bbfea1268b372366b8c718c6822f2108d4e322",{{48,false,4,"coef"},{13,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/110592u;
let channel_scale_shift=coef[16u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"6af9a2f2b968035a4a34830693d12f2fc2569c591b2ff8eb6ce6ef0f2c2aebed",{{13,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.1047250032424927, 0.685846209526062, 0.9526513814926147, 0.5178439021110535, 0.8143125176429749, 0.6715943813323975, 0.6923924684524536, 0.581524670124054, 0.44882750511169434, 0.4984413981437683, 0.2861297130584717, 1.3021514415740967, 0.31866008043289185, 0.8268529176712036, 0.5300660729408264, 0.5082259178161621);
    let P1 = array<f32, 16>(0.08633971959352493, -0.5057839155197144, -0.27021822333335876, -0.3934597373008728, -0.3684293031692505, -0.09857098758220673, -0.24548020958900452, -0.485678106546402, -0.035703208297491074, -0.452858567237854, -0.1595015972852707, 0.32351166009902954, -0.19372627139091492, -0.1065969318151474, 0.4007500410079956, -0.2689298391342163);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"2121a1dafbeade4a9b07a05449a72d5744c64c1520d34b01118bd3ec531622d7",{{50,false,4,"wts"},{14,true,1,"b0"},{13,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.0.1/downs.0.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c / 4u; let sy = 2u * u32(y) + ((c % 4u) / 2u); let sx = 2u * u32(x) + (c % 2u);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  v = v + b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 64u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=64){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/64;let kx=(q/64)%1;let ch=q%64;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=64 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/64);let kx=u32((row/64)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=24480u+(((grp*64u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[24736u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,64}},
{"e36b5cc77de5101d6dee77bdc0e91dff71b42512b3c8a145226fa1033a2ec444",{{50,false,4,"wts"},{15,true,1,"b0"},{14,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.1.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=24752u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[25328u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,144}},
{"f70aa14053b01da857bbbc1c31747f441f2b02f4c043dbb9f4a197a4fed47db4",{{15,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[32u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"50e74eadd365ecc33d8c45a74ee0477d0b98ca3d1a5df11325be60b3bfb3b8fe",{{48,false,4,"coef"},{15,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[32u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"b2f7e97393ae8568fd78ecd2c1411217697dee21cb5c352c88ccc8e63896c357",{{16,false,1,"t3"},{17,false,1,"t4"},{15,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.1656359434127808, 1.0220509767532349, 0.9135286808013916, 1.562838077545166, 1.1679651737213135, 0.8115286231040955, 0.9273597598075867, 1.4646879434585571, 1.1482347249984741, 1.3680312633514404, 1.2225645780563354, 1.2056280374526978, 0.839081346988678, 1.1358751058578491, 1.2441197633743286, 1.3879389762878418);
    let P1 = array<f32, 16>(0.18913982808589935, 0.39439207315444946, -0.08656201511621475, -0.007233678828924894, 0.00128381687682122, -0.10223761945962906, 0.04372747987508774, 0.11366121470928192, 0.07911962270736694, 0.23726889491081238, 0.03492538258433342, -0.12713566422462463, -0.19741547107696533, -0.10769984126091003, -0.0664978101849556, 0.2987874448299408);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"fc3411a2a140c9c332766fb6e7d0c6f3f542661098325a28444c63e187a8bafa",{{50,false,4,"wts"},{18,true,1,"b0"},{15,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.1.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=25344u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[25920u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,144}},
{"e6c56c663283735014ad2fe10d12a5cd70fded72ea245d82d2d2f34d60b35dd1",{{18,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[48u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"439fab6758d28bc8d5bfb6e7f642e57f249c06404f2c0bb94ab7ce39f3c80526",{{48,false,4,"coef"},{18,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[48u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"c39e01de4939edc02f11e7092f537200e6a20743e316f34febd60ccec1461c65",{{18,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(0.4264046549797058, 0.2521263659000397, 0.5886488556861877, 1.008545994758606, 0.6010521054267883, 0.743996798992157, 0.5754590630531311, 0.48691561818122864, 0.4907357394695282, 0.601002037525177, 0.6257888078689575, 1.000486969947815, 0.9903489351272583, 0.8512178659439087, 0.6736283898353577, 0.5382153987884521);
    let P1 = array<f32, 16>(-0.05529317259788513, -0.3090578019618988, 0.026272732764482498, 0.07905453443527222, 0.014642244204878807, -0.11696235835552216, -0.08243121206760406, -0.3349977731704712, 0.05361523851752281, -0.10277555137872696, -0.19824987649917603, 0.19482800364494324, -0.017356008291244507, 0.32419145107269287, -0.027341431006789207, 0.0236190352588892);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"6a454816e7ab339bcbee8c1f4fa0f8b2b6975dc0f0cc4784df7b6b0722fdb094",{{50,false,4,"wts"},{19,true,1,"b0"},{18,false,1,"b1"},{14,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/downs.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=25936u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[27088u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,144}},
{"55991eadec3b0a974eddb8cad5f2abe3da8e26bc96a957bb777e7b6b94ae345a",{{50,false,4,"wts"},{21,true,1,"b0"},{22,true,1,"b1"},{20,false,1,"b2"},{19,false,1,"b3"}},{8,8,1},{2,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/gru/convz1_1/Conv, /core/update_block_depth2/unet/gru/convr1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 64u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=320){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/320;let kx=(q/64)%5;let ch=q%64;let iy=oy+ky-0;let ix=ox+kx-2;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=320 || colIn*4>=64){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/320);let kx=u32((row/64)%5);let col=u32(colIn*4);let grp=col/32u;let mv=(col%32u)/4u;
 let ix=27104u+(((grp*64u+ch)*1u+ky)*5u+kx)*8u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=64){return;}
 let pix=u32(row);let value=valueIn+wts[32224u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[1u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[2u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[3u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 1u:{
b0[4u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[5u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[6u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[7u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 2u:{
b0[8u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[9u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[10u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[11u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 3u:{
b0[12u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[13u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[14u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[15u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 4u:{
b0[16u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[17u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[18u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[19u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 5u:{
b0[20u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[21u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[22u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[23u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 6u:{
b0[24u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[25u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[26u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[27u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 7u:{
b0[28u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[29u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[30u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[31u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 8u:{
b1[0u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[1u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[2u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[3u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 9u:{
b1[4u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[5u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[6u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[7u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 10u:{
b1[8u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[9u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[10u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[11u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 11u:{
b1[12u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[13u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[14u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[15u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 12u:{
b1[16u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[17u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[18u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[19u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 13u:{
b1[20u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[21u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[22u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[23u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 14u:{
b1[24u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[25u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[26u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[27u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 15u:{
b1[28u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[29u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[30u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[31u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,64,320}},
{"bb54f162a8fce7d561a928a6130af63e30f2c4886d12d10f3834eae753629df7",{{50,false,4,"wts"},{23,true,1,"b0"},{22,false,1,"b1"},{20,false,1,"b2"},{19,false,1,"b3"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/gru/convq1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v * b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 64u) { return S1(c - 32u, y, x); }
  return 0.0;
}

fn original_tanh(a: vec4<f32>) -> vec4<f32> {
  let expr = exp(-2 * abs(a));
  return sign(a) * (1 - expr) / (1 + expr);
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=320){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/320;let kx=(q/64)%5;let ch=q%64;let iy=oy+ky-0;let ix=ox+kx-2;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=320 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/320);let kx=u32((row/64)%5);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=32240u+(((grp*64u+ch)*1u+ky)*5u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[34800u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=original_tanh(value).x;
b0[1u*27648u+pix]=original_tanh(value).y;
b0[2u*27648u+pix]=original_tanh(value).z;
b0[3u*27648u+pix]=original_tanh(value).w;
}
case 1u:{
b0[4u*27648u+pix]=original_tanh(value).x;
b0[5u*27648u+pix]=original_tanh(value).y;
b0[6u*27648u+pix]=original_tanh(value).z;
b0[7u*27648u+pix]=original_tanh(value).w;
}
case 2u:{
b0[8u*27648u+pix]=original_tanh(value).x;
b0[9u*27648u+pix]=original_tanh(value).y;
b0[10u*27648u+pix]=original_tanh(value).z;
b0[11u*27648u+pix]=original_tanh(value).w;
}
case 3u:{
b0[12u*27648u+pix]=original_tanh(value).x;
b0[13u*27648u+pix]=original_tanh(value).y;
b0[14u*27648u+pix]=original_tanh(value).z;
b0[15u*27648u+pix]=original_tanh(value).w;
}
case 4u:{
b0[16u*27648u+pix]=original_tanh(value).x;
b0[17u*27648u+pix]=original_tanh(value).y;
b0[18u*27648u+pix]=original_tanh(value).z;
b0[19u*27648u+pix]=original_tanh(value).w;
}
case 5u:{
b0[20u*27648u+pix]=original_tanh(value).x;
b0[21u*27648u+pix]=original_tanh(value).y;
b0[22u*27648u+pix]=original_tanh(value).z;
b0[23u*27648u+pix]=original_tanh(value).w;
}
case 6u:{
b0[24u*27648u+pix]=original_tanh(value).x;
b0[25u*27648u+pix]=original_tanh(value).y;
b0[26u*27648u+pix]=original_tanh(value).z;
b0[27u*27648u+pix]=original_tanh(value).w;
}
case 7u:{
b0[28u*27648u+pix]=original_tanh(value).x;
b0[29u*27648u+pix]=original_tanh(value).y;
b0[30u*27648u+pix]=original_tanh(value).z;
b0[31u*27648u+pix]=original_tanh(value).w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,320}},
{"a3d9f2a1c40414ba1a607952a207dbf5d093620691ab8febba9ed4b877b757f7",{{21,false,4,"z"},{20,false,4,"h"},{23,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> sub_scratch:array<vec4<f32>,64>;
var<workgroup> hm_scratch:array<vec4<f32>,64>;
var<workgroup> zq_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(let av = vec4<f32>(1.0);
let bv = z[global_idx];
sub_scratch[local_idx] = av - bv;
workgroupBarrier();
let a_hm = sub_scratch[local_idx];
let b_hm = h[global_idx];
hm_scratch[local_idx] = a_hm * b_hm;
let a_zq = z[global_idx];
let b_zq = raw[global_idx];
zq_scratch[local_idx] = a_zq * b_zq;
workgroupBarrier();
let a = hm_scratch[local_idx];
let b = zq_scratch[local_idx];
raw[global_idx]=a + b;
)WGSL_CODE",0,{0,0,0}},
{"08906e942c772a856463e8553f084fdc736e5c163c2bcc59fc0c6e8024030bf2",{{50,false,4,"wts"},{24,true,1,"b0"},{25,true,1,"b1"},{23,false,1,"b2"},{19,false,1,"b3"}},{8,8,1},{2,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/gru/convz2_1/Conv, /core/update_block_depth2/unet/gru/convr2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 64u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=320){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/64;let kx=(q/64)%1;let ch=q%64;let iy=oy+ky-2;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=320 || colIn*4>=64){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/64);let kx=u32((row/64)%1);let col=u32(colIn*4);let grp=col/32u;let mv=(col%32u)/4u;
 let ix=34816u+(((grp*64u+ch)*5u+ky)*1u+kx)*8u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=64){return;}
 let pix=u32(row);let value=valueIn+wts[39936u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[1u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[2u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[3u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 1u:{
b0[4u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[5u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[6u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[7u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 2u:{
b0[8u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[9u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[10u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[11u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 3u:{
b0[12u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[13u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[14u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[15u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 4u:{
b0[16u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[17u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[18u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[19u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 5u:{
b0[20u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[21u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[22u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[23u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 6u:{
b0[24u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[25u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[26u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[27u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 7u:{
b0[28u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[29u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[30u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[31u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 8u:{
b1[0u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[1u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[2u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[3u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 9u:{
b1[4u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[5u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[6u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[7u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 10u:{
b1[8u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[9u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[10u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[11u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 11u:{
b1[12u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[13u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[14u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[15u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 12u:{
b1[16u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[17u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[18u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[19u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 13u:{
b1[20u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[21u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[22u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[23u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 14u:{
b1[24u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[25u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[26u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[27u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 15u:{
b1[28u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[29u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[30u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[31u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,64,320}},
{"1e57b222b4afccc9a3e277001eb3c0f89d3fad6540b3d505ef5a49ba94affba7",{{50,false,4,"wts"},{26,true,1,"b0"},{25,false,1,"b1"},{23,false,1,"b2"},{19,false,1,"b3"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/gru/convq2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v * b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 64u) { return S1(c - 32u, y, x); }
  return 0.0;
}

fn original_tanh(a: vec4<f32>) -> vec4<f32> {
  let expr = exp(-2 * abs(a));
  return sign(a) * (1 - expr) / (1 + expr);
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=320){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/64;let kx=(q/64)%1;let ch=q%64;let iy=oy+ky-2;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=320 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%64);let ky=u32(row/64);let kx=u32((row/64)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=39952u+(((grp*64u+ch)*5u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[42512u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=original_tanh(value).x;
b0[1u*27648u+pix]=original_tanh(value).y;
b0[2u*27648u+pix]=original_tanh(value).z;
b0[3u*27648u+pix]=original_tanh(value).w;
}
case 1u:{
b0[4u*27648u+pix]=original_tanh(value).x;
b0[5u*27648u+pix]=original_tanh(value).y;
b0[6u*27648u+pix]=original_tanh(value).z;
b0[7u*27648u+pix]=original_tanh(value).w;
}
case 2u:{
b0[8u*27648u+pix]=original_tanh(value).x;
b0[9u*27648u+pix]=original_tanh(value).y;
b0[10u*27648u+pix]=original_tanh(value).z;
b0[11u*27648u+pix]=original_tanh(value).w;
}
case 3u:{
b0[12u*27648u+pix]=original_tanh(value).x;
b0[13u*27648u+pix]=original_tanh(value).y;
b0[14u*27648u+pix]=original_tanh(value).z;
b0[15u*27648u+pix]=original_tanh(value).w;
}
case 4u:{
b0[16u*27648u+pix]=original_tanh(value).x;
b0[17u*27648u+pix]=original_tanh(value).y;
b0[18u*27648u+pix]=original_tanh(value).z;
b0[19u*27648u+pix]=original_tanh(value).w;
}
case 5u:{
b0[20u*27648u+pix]=original_tanh(value).x;
b0[21u*27648u+pix]=original_tanh(value).y;
b0[22u*27648u+pix]=original_tanh(value).z;
b0[23u*27648u+pix]=original_tanh(value).w;
}
case 6u:{
b0[24u*27648u+pix]=original_tanh(value).x;
b0[25u*27648u+pix]=original_tanh(value).y;
b0[26u*27648u+pix]=original_tanh(value).z;
b0[27u*27648u+pix]=original_tanh(value).w;
}
case 7u:{
b0[28u*27648u+pix]=original_tanh(value).x;
b0[29u*27648u+pix]=original_tanh(value).y;
b0[30u*27648u+pix]=original_tanh(value).z;
b0[31u*27648u+pix]=original_tanh(value).w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,320}},
{"296d5e9ff1fe37cabaa19b97576118eaa9ec507764dcf4ac06fc5afe950727f8",{{24,false,4,"z"},{23,false,4,"h"},{26,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> sub_scratch:array<vec4<f32>,64>;
var<workgroup> hm_scratch:array<vec4<f32>,64>;
var<workgroup> zq_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(let av = vec4<f32>(1.0);
let bv = z[global_idx];
sub_scratch[local_idx] = av - bv;
workgroupBarrier();
let a_hm = sub_scratch[local_idx];
let b_hm = h[global_idx];
hm_scratch[local_idx] = a_hm * b_hm;
let a_zq = z[global_idx];
let b_zq = raw[global_idx];
zq_scratch[local_idx] = a_zq * b_zq;
workgroupBarrier();
let a = hm_scratch[local_idx];
let b = zq_scratch[local_idx];
raw[global_idx]=a + b;
)WGSL_CODE",0,{0,0,0}},
{"fd5c719048e2ffd1443d357c5bca6386e8524a4e1baa227f2d359fcc33290ac0",{{50,false,4,"wts"},{27,true,1,"b0"},{26,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/mid/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=42528u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[44832u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,288}},
{"5ec3ed3909e9488d18e169b81b63a1282fcd5d8c95729c9fc402fa2a8bc53398",{{27,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[64u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"0a1a6a06978b826b2642382a1ef2bb7aa53b5bbbae734a07fccf488a8707d647",{{48,false,4,"coef"},{27,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[64u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"7535d6eba126605cf5d18b434a924b9671e6c8e938c77b6b5d87c73e3608c7e6",{{27,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.8687565326690674, 0.4855436384677887, 0.8288847804069519, 0.8124305009841919, 0.9690547585487366, 0.5397576093673706, 0.7347555160522461, 0.9895047545433044, 1.1399421691894531, 1.3236395120620728, 1.089347243309021, 0.2637065351009369, 0.9843631386756897, 0.7972566485404968, 0.939339280128479, 0.8749436140060425, 0.9894788265228271, 1.011549949645996, 0.24913384020328522, 0.9676110148429871, 0.9080445170402527, 0.7713121771812439, 2.188708543777466, 0.8992301821708679, 1.13258957862854, 1.0258735418319702, 1.0922471284866333, 0.9132866263389587, 1.4758555889129639, 1.9582046270370483, 0.7480019927024841, 0.8854714035987854);
    let P1 = array<f32, 32>(-0.6861222386360168, -0.023685097694396973, -0.08839099109172821, 0.03158329427242279, -0.18790461122989655, -0.14482815563678741, -0.044585470110177994, -0.1856883317232132, -0.14907203614711761, -0.18737438321113586, -0.20185130834579468, 0.03758436068892479, -0.22042186558246613, -0.40164294838905334, -0.1596652865409851, -0.12346552312374115, 0.03737301006913185, -0.3710264265537262, 0.04715000092983246, -0.11977212876081467, -0.2783011198043823, 0.03899167478084564, -1.0749720335006714, 0.11593834310770035, -0.3519933223724365, -0.2205507606267929, -0.25777894258499146, -0.481412410736084, -0.6253181099891663, -0.7272321581840515, -0.2917121946811676, -0.36585569381713867);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"07a3366a176fec9cb7cd2ed48c796ef4cf5969eee09bc62e2fc079204eb76f96",{{50,false,4,"wts"},{28,true,1,"b0"},{27,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/mid/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=44848u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[47152u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,288}},
{"7df10f28b4bce069e33640ec274cfa7674c633366df6c68f0c57a56c38aba3f6",{{28,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[96u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"158b591a0361885483b78463ffa37b6a7b1c02a08a475b1b4f10d4a2bc0c9c4f",{{48,false,4,"coef"},{28,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[96u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"42c279a98d3801095862ad3f38144d0219d61a6b3121889d61789288c8159e56",{{28,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(0.5387988686561584, 0.5636143088340759, 0.680594265460968, 0.8938270211219788, 1.0003727674484253, 1.091995120048523, 0.9758369326591492, 1.3972996473312378, 1.0967028141021729, 0.6966249942779541, 1.133482575416565, 0.8077158331871033, 0.6302931904792786, 1.2267273664474487, 0.4888198971748352, 0.7880700826644897, 1.1416901350021362, 1.530874252319336, 1.1224730014801025, 0.8909685015678406, 1.3605130910873413, 1.45710289478302, 0.5946311354637146, 0.5151774883270264, 1.3208578824996948, 1.2722190618515015, 0.775576651096344, 1.0340111255645752, 0.871986448764801, 1.038068175315857, 0.7329596281051636, 1.2660458087921143);
    let P1 = array<f32, 32>(0.32512205839157104, -0.15672053396701813, -0.28089359402656555, 0.0486648865044117, 0.11971976608037949, 0.23062127828598022, 0.11328332126140594, 0.1437414437532425, 0.04265275225043297, 0.03263635188341141, -0.19049358367919922, -0.09746228158473969, -0.23391585052013397, -0.19655875861644745, 0.12476499378681183, 0.2380467802286148, 0.09270584583282471, 0.0012361172121018171, -0.0680839791893959, 0.005642448551952839, -0.40102604031562805, -0.4318714737892151, -0.26272356510162354, 0.057964473962783813, 0.03087073378264904, 0.011764796450734138, -0.04525846987962723, -0.041928570717573166, -0.13623659312725067, -0.47830596566200256, 0.06770452111959457, -0.0067858584225177765);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"fb0cbb85246af10d1a778aa6563e6fd9c9e8002d198466bdc6ff5b11d74d5163",{{50,false,4,"wts"},{29,true,1,"b0"},{28,false,1,"b1"},{26,false,1,"b2"},{18,false,1,"b3"},{14,false,1,"b4"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.0.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  v = v + b4[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 48u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=432){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/144;let kx=(q/48)%3;let ch=q%48;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=432 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%48);let ky=u32(row/144);let kx=u32((row/48)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=47168u+(((grp*48u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[50624u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,432}},
{"c46394674b5d751441a1f40217ca962816052b153aebb7b75c30139d263b8621",{{29,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[128u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"9985dcf99b9a74a642cd178d99ca578e4db83f82035b2d0ee316bb5245b274da",{{48,false,4,"coef"},{29,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[128u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"be9baaa3767a64399f7fe699aecb2b91f06a529932bc63ab68de70b1714214ee",{{30,false,1,"t3"},{31,false,1,"t4"},{29,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.0308760404586792, 0.9768494367599487, 1.0624679327011108, 1.1148196458816528, 1.3165335655212402, 1.0065652132034302, 0.849982738494873, 0.5452168583869934, 1.038499116897583, 0.9961291551589966, 1.0098718404769897, 1.149354100227356, 0.9134297370910645, 0.35010093450546265, 0.7904531359672546, 1.0531554222106934, 1.3158575296401978, 1.3141673803329468, 0.9110047221183777, 1.3051377534866333, 0.7811072468757629, 0.7838155031204224, 0.7789944410324097, 0.7397356629371643, 0.8324458003044128, 1.0806503295898438, 0.8571859002113342, 1.0524369478225708, 1.0622549057006836, 1.0270295143127441, 1.0744872093200684, 1.0360721349716187);
    let P1 = array<f32, 32>(-0.11898882687091827, -0.07399996370077133, -0.09363045543432236, 0.060254693031311035, -0.2898295223712921, -0.20711670815944672, -0.15681667625904083, -0.049781348556280136, -0.33909785747528076, -0.07590921223163605, -0.15767134726047516, -0.15208859741687775, -0.012827282771468163, -0.19780927896499634, -0.06026124209165573, -0.015910187736153603, -0.3834337294101715, -0.3974900245666504, 0.19380275905132294, -0.48559650778770447, -0.12340506166219711, 0.0010173931950703263, -0.17072758078575134, -0.039541155099868774, 0.034712281078100204, 0.009850078262388706, -0.23492296040058136, -0.024216393008828163, -0.14833246171474457, -0.08495593816041946, -0.30661019682884216, -0.0034423726610839367);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"5c6d8a065502be6d2944589a9b5e861d164ed85db0eb68a9dd167a137c6eddcc",{{50,false,4,"wts"},{32,true,1,"b0"},{29,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.0.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=50640u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[52944u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,288}},
{"595b77ba39e12510e7371bed9f299d8d6870e75a55d2a3158012e1210ddf1ea4",{{32,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[160u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"6826f10a6bb0cf6a45974c006eea339f99ac9572a2c8d328781ac1e5edbe6207",{{48,false,4,"coef"},{32,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[160u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"c2ce99d4eb432417b640b58b7e978bdef725f463daaeb074b060329a18edc635",{{32,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.44490647315979, 1.6919130086898804, 1.3472381830215454, 1.3623353242874146, 0.35969072580337524, 1.2056024074554443, 1.5124751329421997, 0.4818713068962097, 1.750336766242981, 1.1753854751586914, 0.6816576719284058, 1.5517477989196777, 0.4190564751625061, 1.7866673469543457, 1.4132189750671387, 0.957862138748169, 0.8487277030944824, 1.0132858753204346, 1.0177373886108398, 1.2792476415634155, 0.7283192873001099, 1.4185609817504883, 0.5185100436210632, 0.6604533791542053, 1.2438942193984985, 1.263862133026123, 0.7592844367027283, 0.8484734892845154, 1.4778989553451538, 0.683007538318634, 1.214734673500061, 0.9522296786308289);
    let P1 = array<f32, 32>(0.004071471747010946, -0.22744838893413544, -0.013897934928536415, 0.3516407310962677, -0.13022126257419586, -0.05033998563885689, -0.08578497171401978, -0.1444152146577835, -0.2647823691368103, 0.24466322362422943, -0.5058233141899109, -0.10308577865362167, -0.4688517153263092, -0.18714100122451782, 0.023832330480217934, -0.1518997848033905, 0.005278945434838533, -0.28324025869369507, -0.31157568097114563, -0.1834053248167038, 0.10060851275920868, -0.4304104745388031, 0.0943196639418602, -0.07065469771623611, 0.09681905806064606, -0.588946521282196, -0.5479710698127747, -0.4114896059036255, -0.17963950335979462, -0.4242757260799408, -0.4190806448459625, -0.20657925307750702);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"f62de032f337a7373e33781e505189ecc9c9cfef34de82099d429d74521ee538",{{50,false,4,"wts"},{33,true,1,"b0"},{28,false,1,"b1"},{26,false,1,"b2"},{18,false,1,"b3"},{14,false,1,"b4"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.0.0/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  v = v + b4[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 48u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=48){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/48)%1;let ch=q%48;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=48 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%48);let ky=u32(row/48);let kx=u32((row/48)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=52960u+(((grp*48u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[53344u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,48}},
{"9b4beb1ab76b94d04d741783689187368d2e1d4845b0ae45fb075c8acc0b6592",{{50,false,4,"wts"},{34,true,1,"b0"},{32,false,1,"b1"},{33,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.0.1/ups.0.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y) >> 1u; let sx = u32(x) >> 1u;
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=288){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=53360u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[54512u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,288}},
{"d35d832762b9eb5e993f2c7f574d2615292a24e49fec942f6201de3cee93edc4",{{50,false,4,"wts"},{35,true,1,"b0"},{34,false,1,"b1"},{13,false,1,"b2"},{9,false,1,"b3"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.1.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  v = v + b3[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 32u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=288){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=54528u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[55680u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,288}},
{"b97b06c4aaaca4eac2afc5cec807af0941a7aa8afb702d6f8598e9bec9fb4394",{{35,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=110592u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 110592u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[192u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"c71693b1c1d8516656f421eb4e071a1f8abf7b1f26f82bab601bfcdbf0b757e7",{{48,false,4,"coef"},{35,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/110592u;
let channel_scale_shift=coef[192u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"53d5a9a782b1ebdad1820d4c09162bcc3a4d11860bbdfeb765d62892fe1d33ae",{{36,false,1,"t3"},{37,false,1,"t4"},{35,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.4519963264465332, 1.0610110759735107, 0.9218735098838806, 0.6755930781364441, 0.8472209572792053, 1.103845238685608, 0.9080103635787964, 0.8837310671806335, 1.1101233959197998, 0.8308441042900085, 0.9686066508293152, 0.019005926325917244, 1.53010892868042, 1.3588731288909912, 0.8481330871582031, 0.39658570289611816);
    let P1 = array<f32, 16>(-0.35643166303634644, -0.038781922310590744, -0.2415296584367752, 0.07566027343273163, -0.03368420526385307, -0.08816328644752502, -0.012264067307114601, -0.08889932185411453, 0.15886850655078888, 0.23864559829235077, -0.20750151574611664, -0.47348421812057495, -0.3366808295249939, -0.4135027229785919, 0.012480298057198524, -0.06888872385025024);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"8ab4a1a0d61b4cd74985b2d17e7aa8b122a7ab08a2cfa05f7738a50c89bd61bf",{{50,false,4,"wts"},{38,true,1,"b0"},{35,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.1.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=55696u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[56272u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,144}},
{"c7e2c84a0f719b506a29d8e72c3c0110c2d36c4d755b2ab5499159493e7ebbf5",{{38,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=110592u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 110592u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[208u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"1bda795ee5c9f9e61a5426af998922ef80c6d7c2aef4fed8ac022f4c944a96e8",{{48,false,4,"coef"},{38,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/110592u;
let channel_scale_shift=coef[208u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"59d0a6d26df3a815852253039fe54417fe9c0b59e506f12fa760d6eacc0863cd",{{38,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(0.534520149230957, 0.8737297654151917, 0.5625705122947693, 1.0675586462020874, 0.4478868544101715, 0.9780964255332947, 1.8655935525894165, 0.7403324842453003, 2.3058743476867676, 1.077169418334961, 0.4175819456577301, 1.510103702545166, 1.26657235622406, 0.4426352083683014, 0.9839290380477905, 1.0774586200714111);
    let P1 = array<f32, 16>(0.3281431198120117, -0.34535565972328186, -0.1910775899887085, 0.05154091492295265, -0.09595222026109695, 0.1013239249587059, 0.057779811322689056, 0.07570431381464005, 0.19066999852657318, 0.10458994656801224, 0.05325464904308319, -0.03292659670114517, -0.016167787835001945, -0.21575531363487244, 0.4204941987991333, 0.024578193202614784);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"643a40030c3b374fb7cab06f1bd8778dfd78a4692f59a9d725f936b390979482",{{50,false,4,"wts"},{39,true,1,"b0"},{34,false,1,"b1"},{13,false,1,"b2"},{9,false,1,"b3"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.1.0/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  v = v + b3[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 32u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=32){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/32;let kx=(q/32)%1;let ch=q%32;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=32 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/32);let kx=u32((row/32)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=56288u+(((grp*32u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[56416u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,32}},
{"1b4a7c9d5523d59f9ab828fd5de2e915e71bd31294e7397b514b17bd6b759ab1",{{50,false,4,"wts"},{40,true,1,"b0"},{38,false,1,"b1"},{39,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/ups.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  v = v + b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=56432u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[57008u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,144}},
{"a7d55523b17eb10d789320ace711c86a2673e2370855e61a575940bae839d71a",{{50,false,4,"wts"},{41,true,1,"b0"},{40,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/final_res_block/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 32u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=288){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=57024u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[58176u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,288}},
{"c47389624019bf7fad9fa357ec1e5e2c2fa19e736dfbf57cae33f4496ce7845e",{{41,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=110592u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 110592u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[224u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"dbcccaaa572faa13f8298a8b632d913c82e7d9249881f598dc4628a69c3c0217",{{48,false,4,"coef"},{41,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/110592u;
let channel_scale_shift=coef[224u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"66e2350b0591604d429107c8ba559b55185c460ff08d6706f8ead005623e40e5",{{42,false,1,"t3"},{43,false,1,"t4"},{41,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(0.9875800013542175, 0.9973968267440796, 0.949870765209198, 1.751861333847046, 1.1787556409835815, 0.7179486751556396, 0.9730886220932007, 1.552246332168579, 0.6105612516403198, 1.0591378211975098, 1.2269864082336426, 1.1117626428604126, 1.8708029985427856, 1.240452527999878, 0.7325398325920105, 0.5766901969909668);
    let P1 = array<f32, 16>(-0.034808166325092316, 0.06577306985855103, -0.06579680740833282, -0.38482147455215454, -0.2830467224121094, -0.08609149605035782, -0.21742291748523712, 0.1288217306137085, -0.14161880314350128, -0.08213981986045837, 0.11768864840269089, -0.05320363864302635, -0.22585497796535492, -0.17181186378002167, -0.41826194524765015, 0.0582902692258358);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"cbe34d79e9d89b89d231fa4a7adf97aaccaa32c5be1340ea3a9666b69737548e",{{50,false,4,"wts"},{44,true,1,"b0"},{41,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/final_res_block/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=58192u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[58768u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,144}},
{"413793303e4d37ad9b246f53135701325773432af60fd092ebe63251b9c29a17",{{44,false,4,"raw"},{48,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=110592u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 110592u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[240u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"466edc2ce17b82ad713d5d6a1b96210918a614eb955baa34e02ff6877576dabd",{{48,false,4,"coef"},{44,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/110592u;
let channel_scale_shift=coef[240u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"c4dbad55bdf3610e45b1acf376b4cdf7b163c8740f895381b611560d7aa37753",{{44,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(0.695611298084259, 5.309262633090839e-05, 0.23800072073936462, 0.4856206476688385, -0.0027802600525319576, 0.0021251386497169733, -0.022010693326592445, 0.9605537056922913, 0.7428562045097351, 0.15878599882125854, 0.8231478333473206, 0.608710527420044, 0.1137300655245781, 0.13211171329021454, 0.13283611834049225, 1.6364182233810425);
    let P1 = array<f32, 16>(-0.1402619183063507, -1.278259038925171, -0.16009297966957092, -0.4925999939441681, -0.1277998685836792, -0.1433214694261551, -0.18093161284923553, -0.1027757078409195, -0.5494709610939026, -0.22653745114803314, -0.4480752944946289, -0.14242205023765564, -0.08907261490821838, -0.25149109959602356, -1.085011601448059, -0.27287963032722473);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"b7c6e83785e63ea50227bf028b14262b44f5a425010be996f1ed6eb207642949",{{50,false,4,"wts"},{45,true,1,"b0"},{40,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/final_res_block/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 32u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=32){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/32;let kx=(q/32)%1;let ch=q%32;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=32 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/32);let kx=u32((row/32)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=58784u+(((grp*32u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[58912u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,32}},
{"408f2219ca9a4a7f054c629fd4fbeb4bea89c29f123b9ada25ed8c8215c8aa81",{{50,false,4,"wts"},{46,true,1,"b0"},{47,true,1,"b1"},{44,false,1,"b2"},{45,false,1,"b3"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth2/unet/final_conv_1/Conv, /core/update_block_depth2/unet/conf_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  v = v + b3[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=16){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/16;let kx=(q/16)%1;let ch=q%16;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=16 || colIn*4>=2){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/16);let kx=u32((row/16)%1);let col=u32(colIn*4);let grp=col/4u;let mv=(col%4u)/4u;
 let ix=58928u+(((grp*16u+ch)*1u+ky)*1u+kx)*1u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=2){return;}
 let pix=u32(row);let value=valueIn+wts[58944u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b1[0u*110592u+pix]=1.0 / (1.0 + exp(-value.y));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,2,16}},
},{{1,32,144,192},{1,1,288,384},{1,1,288,384}},{3538944,3538944,3538944,1769472,1769472,442368,1769472,884736,884736}},
{20,{
{7077888,0,0,{1,4,4,576,768},-1},
{7077888,2,-1,{16,576,768},0},
{7077888,2,-1,{16,576,768},1},
{1769472,0,1,{1,4,576,768},-1},
{7077888,2,-1,{16,576,768},0},
{7077888,2,-1,{16,576,768},2},
{6635520,2,-1,{15,576,768},0},
{7077888,0,2,{1,16,576,768},-1},
{442368,0,3,{1,1,576,768},-1},
{3538944,2,-1,{8,576,768},1},
{3538944,2,-1,{8,576,768},0},
{8,0,4,{1,8,1,1},-1},
{8,0,5,{1,8,1,1},-1},
{3538944,2,-1,{8,576,768},3},
{884736,2,-1,{8,288,384},0},
{884736,2,-1,{8,288,384},4},
{8,0,6,{1,8,1,1},-1},
{8,0,7,{1,8,1,1},-1},
{884736,2,-1,{8,288,384},5},
{442368,2,-1,{16,144,192},4},
{442368,2,-1,{16,144,192},6},
{16,0,8,{1,16,1,1},-1},
{16,0,9,{1,16,1,1},-1},
{442368,2,-1,{16,144,192},7},
{884736,2,-1,{32,144,192},6},
{552960,0,10,{1,20,144,192},-1},
{552960,2,-1,{20,144,192},8},
{552960,2,-1,{20,144,192},9},
{552960,2,-1,{20,144,192},10},
{552960,2,-1,{20,144,192},8},
{552960,2,-1,{20,144,192},9},
{552960,1,0,{20,144,192},-1},
{884736,2,-1,{32,144,192},6},
{884736,2,-1,{32,144,192},8},
{884736,2,-1,{32,144,192},6},
{884736,2,-1,{32,144,192},9},
{32,0,11,{1,32,1,1},-1},
{32,0,12,{1,32,1,1},-1},
{884736,2,-1,{32,144,192},10},
{884736,2,-1,{32,144,192},9},
{1769472,2,-1,{16,288,384},4},
{1769472,2,-1,{16,288,384},6},
{16,0,13,{1,16,1,1},-1},
{16,0,14,{1,16,1,1},-1},
{1769472,2,-1,{16,288,384},8},
{1769472,2,-1,{16,288,384},6},
{3538944,2,-1,{8,576,768},0},
{3538944,2,-1,{8,576,768},4},
{8,0,15,{1,8,1,1},-1},
{8,0,16,{1,8,1,1},-1},
{3538944,2,-1,{8,576,768},6},
{3538944,2,-1,{8,576,768},4},
{3538944,2,-1,{8,576,768},3},
{3538944,2,-1,{8,576,768},4},
{8,0,17,{1,8,1,1},-1},
{8,0,18,{1,8,1,1},-1},
{3538944,2,-1,{8,576,768},6},
{3538944,2,-1,{8,576,768},4},
{442368,1,1,{1,576,768},-1},
{442368,1,2,{1,576,768},-1},
{1024,2,-1,{1024},2},
{131328,2,-1,{131328},-1},
{128256,0,19,{128256},-1},
},{
{"8c131e707c9f54bd904cb349a93c2a1ac7a3ea57c596e41452f674fa0ddeba57",{{62,false,4,"wts"},{1,true,1,"b0"},{0,false,1,"b1"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/encoder/convc1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=144){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=0u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[576u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=max(value.x, 0.0);
b0[1u*442368u+pix]=max(value.y, 0.0);
b0[2u*442368u+pix]=max(value.z, 0.0);
b0[3u*442368u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*442368u+pix]=max(value.x, 0.0);
b0[5u*442368u+pix]=max(value.y, 0.0);
b0[6u*442368u+pix]=max(value.z, 0.0);
b0[7u*442368u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*442368u+pix]=max(value.x, 0.0);
b0[9u*442368u+pix]=max(value.y, 0.0);
b0[10u*442368u+pix]=max(value.z, 0.0);
b0[11u*442368u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*442368u+pix]=max(value.x, 0.0);
b0[13u*442368u+pix]=max(value.y, 0.0);
b0[14u*442368u+pix]=max(value.z, 0.0);
b0[15u*442368u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,16,144}},
{"451c0a1ba063be17f3904d4e7e5c51882a8e696777112691b95a420ccdd9ba8f",{{62,false,4,"wts"},{2,true,1,"b0"},{1,false,1,"b1"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/encoder/convc2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=144){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=592u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[1168u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=max(value.x, 0.0);
b0[1u*442368u+pix]=max(value.y, 0.0);
b0[2u*442368u+pix]=max(value.z, 0.0);
b0[3u*442368u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*442368u+pix]=max(value.x, 0.0);
b0[5u*442368u+pix]=max(value.y, 0.0);
b0[6u*442368u+pix]=max(value.z, 0.0);
b0[7u*442368u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*442368u+pix]=max(value.x, 0.0);
b0[9u*442368u+pix]=max(value.y, 0.0);
b0[10u*442368u+pix]=max(value.z, 0.0);
b0[11u*442368u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*442368u+pix]=max(value.x, 0.0);
b0[13u*442368u+pix]=max(value.y, 0.0);
b0[14u*442368u+pix]=max(value.z, 0.0);
b0[15u*442368u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,16,144}},
{"58acb58b00f8f5f420fbb97aabb20cae03ef61ec62ebfd576459e5e4c4d39205",{{62,false,4,"wts"},{4,true,1,"b0"},{3,false,1,"b1"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/encoder/convd1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 4u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=36){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/12;let kx=(q/4)%3;let ch=q%4;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=36 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%4);let ky=u32(row/12);let kx=u32((row/4)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=1184u+(((grp*4u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[1328u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=max(value.x, 0.0);
b0[1u*442368u+pix]=max(value.y, 0.0);
b0[2u*442368u+pix]=max(value.z, 0.0);
b0[3u*442368u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*442368u+pix]=max(value.x, 0.0);
b0[5u*442368u+pix]=max(value.y, 0.0);
b0[6u*442368u+pix]=max(value.z, 0.0);
b0[7u*442368u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*442368u+pix]=max(value.x, 0.0);
b0[9u*442368u+pix]=max(value.y, 0.0);
b0[10u*442368u+pix]=max(value.z, 0.0);
b0[11u*442368u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*442368u+pix]=max(value.x, 0.0);
b0[13u*442368u+pix]=max(value.y, 0.0);
b0[14u*442368u+pix]=max(value.z, 0.0);
b0[15u*442368u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,16,36}},
{"5a6f9b877a402ccb7f4ea6ecdc6124dc0b382644dd8720114646c0f661bd256c",{{62,false,4,"wts"},{5,true,1,"b0"},{4,false,1,"b1"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/encoder/convd2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=144){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=1344u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[1920u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=max(value.x, 0.0);
b0[1u*442368u+pix]=max(value.y, 0.0);
b0[2u*442368u+pix]=max(value.z, 0.0);
b0[3u*442368u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*442368u+pix]=max(value.x, 0.0);
b0[5u*442368u+pix]=max(value.y, 0.0);
b0[6u*442368u+pix]=max(value.z, 0.0);
b0[7u*442368u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*442368u+pix]=max(value.x, 0.0);
b0[9u*442368u+pix]=max(value.y, 0.0);
b0[10u*442368u+pix]=max(value.z, 0.0);
b0[11u*442368u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*442368u+pix]=max(value.x, 0.0);
b0[13u*442368u+pix]=max(value.y, 0.0);
b0[14u*442368u+pix]=max(value.z, 0.0);
b0[15u*442368u+pix]=max(value.w, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,16,144}},
{"a5a7b0ce2ed2c34fa70cda95e429098eed4eb4c495a410557665b87d40fd2aa6",{{62,false,4,"wts"},{6,true,1,"b0"},{2,false,1,"b1"},{5,false,1,"b2"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/encoder/output_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b2[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 32u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=288){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=15){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=1936u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=15){return;}
 let pix=u32(row);let value=valueIn+wts[3088u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=max(value.x, 0.0);
b0[1u*442368u+pix]=max(value.y, 0.0);
b0[2u*442368u+pix]=max(value.z, 0.0);
b0[3u*442368u+pix]=max(value.w, 0.0);
}
case 1u:{
b0[4u*442368u+pix]=max(value.x, 0.0);
b0[5u*442368u+pix]=max(value.y, 0.0);
b0[6u*442368u+pix]=max(value.z, 0.0);
b0[7u*442368u+pix]=max(value.w, 0.0);
}
case 2u:{
b0[8u*442368u+pix]=max(value.x, 0.0);
b0[9u*442368u+pix]=max(value.y, 0.0);
b0[10u*442368u+pix]=max(value.z, 0.0);
b0[11u*442368u+pix]=max(value.w, 0.0);
}
case 3u:{
b0[12u*442368u+pix]=max(value.x, 0.0);
b0[13u*442368u+pix]=max(value.y, 0.0);
b0[14u*442368u+pix]=max(value.z, 0.0);
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,15,288}},
{"71491e504cbad9440ea6eae68fcc2a2c4131404f13a16ef601d3392e80ed1e7a",{{62,false,4,"wts"},{9,true,1,"b0"},{7,false,1,"b1"},{6,false,1,"b2"},{8,false,1,"b3"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/init_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b2[(0u + sc) * 442368u + p];
  return v;
}
fn S2(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b3[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 31u) { return S1(c - 16u, y, x); }
  if (c < 32u) { return S2(c - 31u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=1568){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/224;let kx=(q/32)%7;let ch=q%32;let iy=oy+ky-3;let ix=ox+kx-3;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=1568 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/224);let kx=u32((row/32)%7);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=3104u+(((grp*32u+ch)*7u+ky)*7u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6240u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,1568}},
{"ffd86c7805f3767eaeb341cd7408b4be5dff98fd6f0f7cca8bfd4dab33cddcf4",{{62,false,4,"wts"},{10,true,1,"b0"},{9,false,1,"b1"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.0.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=72){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=6256u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6400u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,72}},
{"d3d7d7b6ec83651b9d2bdb3153f06b52b6bd8357909dd925a89271770fa530ed",{{10,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=221184u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 221184u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[0u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"c2f2ae271803bedcc64c1cf4c7935ba9ac8c70d72e7631e0fa5510f5fbf2a2c8",{{60,false,4,"coef"},{10,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/221184u;
let channel_scale_shift=coef[0u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"f5161403a1ed9612d50394f9f92b943094c73707696f5d2c17ca98132c3c2573",{{11,false,1,"t3"},{12,false,1,"t4"},{10,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.7367537617683411, 0.8846451640129089, 0.7256792783737183, 0.6895222067832947, 0.7935814261436462, 0.8293971419334412, 0.9833404421806335, 0.5809348225593567);
    let P1 = array<f32, 8>(0.04302620887756348, 0.07105983793735504, 0.17612402141094208, 0.17138101160526276, -0.2958891987800598, 0.17098644375801086, -0.1627522110939026, 0.09819024801254272);
let channel=global_idx*4u/442368u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"8bdcc45b1876876e9658ea95745e4e1da761743a6f46bf8fb80d9889edcf0247",{{62,false,4,"wts"},{13,true,1,"b0"},{10,false,1,"b1"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.0.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=72){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=6416u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6560u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,72}},
{"bd503675c34ed1e4e9bfaed4d432c597e527404cac3dbedd2d33fedb521244f1",{{13,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=221184u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 221184u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[8u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"de365a7a56fd1080a14f2cb2bde01c366537f7be56ea78f86bd4df1e9fb78a33",{{60,false,4,"coef"},{13,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/221184u;
let channel_scale_shift=coef[8u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"7be688424070c15285a5fc0fe3e8cea7ce89db4d042dff6bf72d432f5405f969",{{13,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.5202493071556091, 0.8691046237945557, 1.8021529912948608, 0.909307599067688, 0.6510735154151917, 0.6378940939903259, 0.8225512504577637, 0.4977917969226837);
    let P1 = array<f32, 8>(0.041183751076459885, 0.02951963059604168, -0.07823695242404938, -0.16703784465789795, 0.08494484424591064, 0.05579941347241402, 0.24901346862316132, 0.03699877858161926);
let channel=global_idx*4u/442368u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"0ce71b8da8da10c59476b90320029108ec90df08418c8515410968382cd4d3f9",{{62,false,4,"wts"},{14,true,1,"b0"},{13,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.0.1/downs.0.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c / 4u; let sy = 2u * u32(y) + ((c % 4u) / 2u); let sx = 2u * u32(x) + (c % 2u);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  v = v + b2[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=32){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/32;let kx=(q/32)%1;let ch=q%32;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=32 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/32);let kx=u32((row/32)%1);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=6576u+(((grp*32u+ch)*1u+ky)*1u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6640u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,32}},
{"0f93b65d4b7f0f7d2f93a77decc4c020b2d2d7efa6fc4eac68bba8e2d033418c",{{62,false,4,"wts"},{15,true,1,"b0"},{14,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.1.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=72){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=6656u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6800u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,72}},
{"b78eb6a75058cb9ff4d791b7d45bd9fcca8523361128a1c8fdf9ff5a2779a62e",{{15,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[16u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"2c64c9d29ee901a444fb7e7c6dc3a3eb32ade257861925af3095f1050a9025ae",{{60,false,4,"coef"},{15,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[16u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"dd09e6074385323f41ec5e50a38ecc03b9824b4e5533a75a91846b232f8c8334",{{16,false,1,"t3"},{17,false,1,"t4"},{15,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(1.0428279638290405, 0.014923558570444584, 1.1908085346221924, 0.9103469848632812, 1.2715349197387695, 0.9635223746299744, 1.3189589977264404, 0.9771658182144165);
    let P1 = array<f32, 8>(0.29251334071159363, 0.028385067358613014, 0.21297794580459595, 0.2029360681772232, 0.28699159622192383, 0.22290484607219696, 0.059083979576826096, -0.015475493855774403);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"adb5ae3c5aac290cc6ea32d8c052dce1e950388cb472019853cedad0bf0d0d49",{{62,false,4,"wts"},{18,true,1,"b0"},{15,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.1.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=72){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=6816u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[6960u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,8,72}},
{"11df6a5d679d937be2be81d5d4716d9ba77398e545eeee729e3e174d6078c199",{{18,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[24u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"ae88e86461477ad94a390a969521e747a9fa6a995f85110605ead8fed9d8dc0e",{{60,false,4,"coef"},{18,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[24u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"19d43857d48551730638132441f87dd4a914ed756b05937a06befddf81df2232",{{18,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.15576818585395813, 1.2868891954421997, 0.573776125907898, 0.4996045231819153, 1.3239574432373047, 0.5896697640419006, 0.6019774675369263, 0.4583113193511963);
    let P1 = array<f32, 8>(0.0016845745267346501, -0.04538978263735771, 0.15842919051647186, 0.11213959008455276, -0.16300755739212036, -0.33581867814064026, -0.16958673298358917, -0.06238948181271553);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"da23f5074ffd57e24ef0571394744ab473750a39bd094a0386d939de4a330774",{{62,false,4,"wts"},{19,true,1,"b0"},{18,false,1,"b1"},{14,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.1.1/downs.1.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c / 4u; let sy = 2u * u32(y) + ((c % 4u) / 2u); let sx = 2u * u32(x) + (c % 2u);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  v = v + b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=32){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/32;let kx=(q/32)%1;let ch=q%32;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=32 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/32);let kx=u32((row/32)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=6976u+(((grp*32u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[7104u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,32}},
{"200b63e93914c4fba40017185d47427047e7627d4bc5c9a6863742766abd721e",{{62,false,4,"wts"},{20,true,1,"b0"},{19,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.2.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=7120u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[7696u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,144}},
{"f486f678ce2181748d6a46a3835fe5a8856ed1ae5fec228fe19ad44563dd88e7",{{20,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[32u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"eaea5fcd0154abd3bd4109edeffe925c082b06e8487fcc45e1bd8fec037f0630",{{60,false,4,"coef"},{20,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[32u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"cabd39d3aac3ee43f83b1208f22b8e7a3d20de641464714fd65deca1c0887c24",{{21,false,1,"t3"},{22,false,1,"t4"},{20,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.0209269523620605, 0.9248598217964172, 0.9582107067108154, 1.103807806968689, 1.0603458881378174, 1.067327857017517, 1.0920909643173218, 1.0016498565673828, 1.1540296077728271, 0.8908594846725464, 1.369123935699463, 1.130449652671814, 0.8257385492324829, 0.8000621199607849, 0.3224443793296814, 1.3338093757629395);
    let P1 = array<f32, 16>(0.117359459400177, 0.20187805593013763, 0.10750523954629898, 0.033330779522657394, 0.19661106169223785, 0.22223800420761108, 0.05804101377725601, 0.1299867033958435, -0.2936958074569702, 0.15021075308322906, 0.11419705301523209, 0.001880043651908636, 0.21458938717842102, 0.16501425206661224, 0.09037001430988312, 0.11944371461868286);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"3e09f0973c8e5a5e963871eb3dccad9e8024ef3e59e19c40f1b1f137e6a26aa3",{{62,false,4,"wts"},{23,true,1,"b0"},{20,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.2.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=7712u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[8288u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,16,144}},
{"d2bad132937192b2a260a0f6fee45b68200d6607fdc217de40401de2dc7e83a8",{{23,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=27648u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 27648u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[48u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"603fe274c9a5e4ecdb3d39cafc7a91f5341c26f7a51968276f8d38a97228e2b0",{{60,false,4,"coef"},{23,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/27648u;
let channel_scale_shift=coef[48u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"cc7fabe6df8f8d162439096f9b29925e6c25d7e0184af668487565473158341f",{{23,true,4,"raw"}},{64,1,1},{1728,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(0.7623137831687927, 0.47251319885253906, 0.30448591709136963, 0.5249462127685547, 0.35084542632102966, 0.38846078515052795, 0.19160757958889008, 0.5472601652145386, 0.1673624962568283, 0.5664498805999756, 0.5249688029289246, 0.641364574432373, 0.4932307302951813, 0.3702468276023865, 0.5193073749542236, 0.4533976912498474);
    let P1 = array<f32, 16>(-0.14899942278862, -0.041682515293359756, 0.15352407097816467, -0.07781807333230972, -0.083533875644207, 0.011060367338359356, -0.2075349986553192, -0.02444523200392723, -0.06823026388883591, -0.3815551698207855, 0.05100448429584503, 0.0689125657081604, -0.025715690106153488, 0.17757229506969452, -0.045061904937028885, -0.11552629619836807);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"9fa20474c7a66abb9bb1d2114e74679675c387601971abbef8a43e01312ae2ff",{{62,false,4,"wts"},{24,true,1,"b0"},{23,false,1,"b1"},{19,false,1,"b2"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/downs.2.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=144){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=8304u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[9456u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,144}},
{"df8a481a0148921cf72df7ab8acba593c160a2d8db61852bab2a6e30cd9a3e7c",{{62,false,4,"wts"},{26,true,1,"b0"},{27,true,1,"b1"},{25,false,1,"b2"},{24,false,1,"b3"}},{8,8,1},{2,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/gru/convz1_1/Conv, /core/update_block_depth3/unet/gru/convr1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  if (c < 52u) { return S1(c - 20u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=260){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/260;let kx=(q/52)%5;let ch=q%52;let iy=oy+ky-0;let ix=ox+kx-2;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=260 || colIn*4>=40){return vec4<f32>(0.0);}
 let ch=u32(row%52);let ky=u32(row/260);let kx=u32((row/52)%5);let col=u32(colIn*4);let grp=col/20u;let mv=(col%20u)/4u;
 let ix=9472u+(((grp*52u+ch)*1u+ky)*5u+kx)*5u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=40){return;}
 let pix=u32(row);let value=valueIn+wts[12072u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[1u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[2u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[3u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 1u:{
b0[4u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[5u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[6u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[7u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 2u:{
b0[8u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[9u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[10u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[11u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 3u:{
b0[12u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[13u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[14u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[15u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 4u:{
b0[16u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[17u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[18u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[19u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 5u:{
b1[0u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[1u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[2u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[3u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 6u:{
b1[4u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[5u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[6u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[7u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 7u:{
b1[8u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[9u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[10u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[11u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 8u:{
b1[12u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[13u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[14u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[15u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 9u:{
b1[16u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[17u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[18u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[19u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,40,260}},
{"31c1676c07a70e35b3a6159a670df719ab1ed8e450b32da50469f86b472860f3",{{62,false,4,"wts"},{28,true,1,"b0"},{27,false,1,"b1"},{25,false,1,"b2"},{24,false,1,"b3"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/gru/convq1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v * b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  if (c < 52u) { return S1(c - 20u, y, x); }
  return 0.0;
}

fn original_tanh(a: vec4<f32>) -> vec4<f32> {
  let expr = exp(-2 * abs(a));
  return sign(a) * (1 - expr) / (1 + expr);
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=260){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/260;let kx=(q/52)%5;let ch=q%52;let iy=oy+ky-0;let ix=ox+kx-2;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=260 || colIn*4>=20){return vec4<f32>(0.0);}
 let ch=u32(row%52);let ky=u32(row/260);let kx=u32((row/52)%5);let col=u32(colIn*4);let grp=col/20u;let mv=(col%20u)/4u;
 let ix=12096u+(((grp*52u+ch)*1u+ky)*5u+kx)*5u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=20){return;}
 let pix=u32(row);let value=valueIn+wts[13396u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=original_tanh(value).x;
b0[1u*27648u+pix]=original_tanh(value).y;
b0[2u*27648u+pix]=original_tanh(value).z;
b0[3u*27648u+pix]=original_tanh(value).w;
}
case 1u:{
b0[4u*27648u+pix]=original_tanh(value).x;
b0[5u*27648u+pix]=original_tanh(value).y;
b0[6u*27648u+pix]=original_tanh(value).z;
b0[7u*27648u+pix]=original_tanh(value).w;
}
case 2u:{
b0[8u*27648u+pix]=original_tanh(value).x;
b0[9u*27648u+pix]=original_tanh(value).y;
b0[10u*27648u+pix]=original_tanh(value).z;
b0[11u*27648u+pix]=original_tanh(value).w;
}
case 3u:{
b0[12u*27648u+pix]=original_tanh(value).x;
b0[13u*27648u+pix]=original_tanh(value).y;
b0[14u*27648u+pix]=original_tanh(value).z;
b0[15u*27648u+pix]=original_tanh(value).w;
}
case 4u:{
b0[16u*27648u+pix]=original_tanh(value).x;
b0[17u*27648u+pix]=original_tanh(value).y;
b0[18u*27648u+pix]=original_tanh(value).z;
b0[19u*27648u+pix]=original_tanh(value).w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,20,260}},
{"f1b4d12c5c1596f0616cf7257db86ac3e8ec3d5d05fb743b1ba87ab0613e3ebd",{{26,false,4,"z"},{25,false,4,"h"},{28,true,4,"raw"}},{64,1,1},{2160,1,1},R"WGSL_CODE(var<workgroup> sub_scratch:array<vec4<f32>,64>;
var<workgroup> hm_scratch:array<vec4<f32>,64>;
var<workgroup> zq_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(let av = vec4<f32>(1.0);
let bv = z[global_idx];
sub_scratch[local_idx] = av - bv;
workgroupBarrier();
let a_hm = sub_scratch[local_idx];
let b_hm = h[global_idx];
hm_scratch[local_idx] = a_hm * b_hm;
let a_zq = z[global_idx];
let b_zq = raw[global_idx];
zq_scratch[local_idx] = a_zq * b_zq;
workgroupBarrier();
let a = hm_scratch[local_idx];
let b = zq_scratch[local_idx];
raw[global_idx]=a + b;
)WGSL_CODE",0,{0,0,0}},
{"d1a7b0a4aaff17ae36325db0d1f6626a9e529ac0fd348e3cc03f7fb56e9eda27",{{62,false,4,"wts"},{29,true,1,"b0"},{30,true,1,"b1"},{28,false,1,"b2"},{24,false,1,"b3"}},{8,8,1},{2,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/gru/convz2_1/Conv, /core/update_block_depth3/unet/gru/convr2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  if (c < 52u) { return S1(c - 20u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=260){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/52;let kx=(q/52)%1;let ch=q%52;let iy=oy+ky-2;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=260 || colIn*4>=40){return vec4<f32>(0.0);}
 let ch=u32(row%52);let ky=u32(row/52);let kx=u32((row/52)%1);let col=u32(colIn*4);let grp=col/20u;let mv=(col%20u)/4u;
 let ix=13408u+(((grp*52u+ch)*5u+ky)*1u+kx)*5u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=40){return;}
 let pix=u32(row);let value=valueIn+wts[16008u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[1u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[2u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[3u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 1u:{
b0[4u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[5u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[6u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[7u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 2u:{
b0[8u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[9u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[10u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[11u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 3u:{
b0[12u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[13u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[14u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[15u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 4u:{
b0[16u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b0[17u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b0[18u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b0[19u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 5u:{
b1[0u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[1u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[2u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[3u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 6u:{
b1[4u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[5u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[6u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[7u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 7u:{
b1[8u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[9u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[10u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[11u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 8u:{
b1[12u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[13u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[14u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[15u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
case 9u:{
b1[16u*27648u+pix]=1.0 / (1.0 + exp(-value.x));
b1[17u*27648u+pix]=1.0 / (1.0 + exp(-value.y));
b1[18u*27648u+pix]=1.0 / (1.0 + exp(-value.z));
b1[19u*27648u+pix]=1.0 / (1.0 + exp(-value.w));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,40,260}},
{"20958ac5ccb8549a8f37bf7d66e3f9ec72635ace1669878bdb5f1a4838af68a6",{{62,false,4,"wts"},{31,true,1,"b0"},{30,false,1,"b1"},{28,false,1,"b2"},{24,false,1,"b3"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/gru/convq2_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v * b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  if (c < 52u) { return S1(c - 20u, y, x); }
  return 0.0;
}

fn original_tanh(a: vec4<f32>) -> vec4<f32> {
  let expr = exp(-2 * abs(a));
  return sign(a) * (1 - expr) / (1 + expr);
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=260){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/52;let kx=(q/52)%1;let ch=q%52;let iy=oy+ky-2;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=260 || colIn*4>=20){return vec4<f32>(0.0);}
 let ch=u32(row%52);let ky=u32(row/52);let kx=u32((row/52)%1);let col=u32(colIn*4);let grp=col/20u;let mv=(col%20u)/4u;
 let ix=16032u+(((grp*52u+ch)*5u+ky)*1u+kx)*5u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=20){return;}
 let pix=u32(row);let value=valueIn+wts[17332u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=original_tanh(value).x;
b0[1u*27648u+pix]=original_tanh(value).y;
b0[2u*27648u+pix]=original_tanh(value).z;
b0[3u*27648u+pix]=original_tanh(value).w;
}
case 1u:{
b0[4u*27648u+pix]=original_tanh(value).x;
b0[5u*27648u+pix]=original_tanh(value).y;
b0[6u*27648u+pix]=original_tanh(value).z;
b0[7u*27648u+pix]=original_tanh(value).w;
}
case 2u:{
b0[8u*27648u+pix]=original_tanh(value).x;
b0[9u*27648u+pix]=original_tanh(value).y;
b0[10u*27648u+pix]=original_tanh(value).z;
b0[11u*27648u+pix]=original_tanh(value).w;
}
case 3u:{
b0[12u*27648u+pix]=original_tanh(value).x;
b0[13u*27648u+pix]=original_tanh(value).y;
b0[14u*27648u+pix]=original_tanh(value).z;
b0[15u*27648u+pix]=original_tanh(value).w;
}
case 4u:{
b0[16u*27648u+pix]=original_tanh(value).x;
b0[17u*27648u+pix]=original_tanh(value).y;
b0[18u*27648u+pix]=original_tanh(value).z;
b0[19u*27648u+pix]=original_tanh(value).w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,20,260}},
{"7b4e668479e83bfa38945f51758b6a004a31a3759644a62171bf9ceff6b5950d",{{29,false,4,"z"},{28,false,4,"h"},{31,true,4,"raw"}},{64,1,1},{2160,1,1},R"WGSL_CODE(var<workgroup> sub_scratch:array<vec4<f32>,64>;
var<workgroup> hm_scratch:array<vec4<f32>,64>;
var<workgroup> zq_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(let av = vec4<f32>(1.0);
let bv = z[global_idx];
sub_scratch[local_idx] = av - bv;
workgroupBarrier();
let a_hm = sub_scratch[local_idx];
let b_hm = h[global_idx];
hm_scratch[local_idx] = a_hm * b_hm;
let a_zq = z[global_idx];
let b_zq = raw[global_idx];
zq_scratch[local_idx] = a_zq * b_zq;
workgroupBarrier();
let a = hm_scratch[local_idx];
let b = zq_scratch[local_idx];
raw[global_idx]=a + b;
)WGSL_CODE",0,{0,0,0}},
{"5546b42190d7420e8ed6dcdcd37a24434e3cbc864f6470b09e93f563f50482a5",{{62,false,4,"wts"},{32,true,1,"b0"},{31,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/mid/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=180){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/60;let kx=(q/20)%3;let ch=q%20;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=180 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%20);let ky=u32(row/60);let kx=u32((row/20)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=17344u+(((grp*24u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[19072u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,180}},
{"dd48918ec4d266288b1211dfbeadff079d9c6d9143338724498135fcd8c0c258",{{32,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[64u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"c878a2cc45b62a34208750880d6b552c29a86801f2ea1e8d229f828da15df263",{{60,false,4,"coef"},{32,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[64u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"30a196390bdfbd19bbc5a830a6d396703d2384a5078324df4c17b6499ec72a17",{{32,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.0066016912460327, 0.9786734580993652, 0.9980636835098267, 1.383861780166626, 0.5653182864189148, 0.8289008140563965, 1.2726531028747559, 1.3360278606414795, 0.9863739609718323, 0.79414302110672, 0.9148256182670593, 0.5539575219154358, 1.2147337198257446, 1.329681396484375, 0.9587805271148682, 0.9294324517250061, 1.0575658082962036, 1.381798267364502, 0.9386388659477234, 1.329537272453308, 0.515467643737793, 0.9772318601608276, 1.1901066303253174, 1.300421953201294, 1.6998525857925415, 0.549810528755188, 1.4652369022369385, 1.5634791851043701, 0.3340718448162079, 1.1365104913711548, 0.8679242134094238, 0.9671586751937866);
    let P1 = array<f32, 32>(-0.17669640481472015, -0.08895508199930191, -0.16099676489830017, -0.25679975748062134, -0.13155943155288696, -0.239667609333992, -0.3061389625072479, -0.6148698925971985, -0.12502118945121765, -0.13115721940994263, -0.22479499876499176, -0.089290551841259, -0.26162973046302795, -0.10780546814203262, -0.011291858740150928, -0.057862695306539536, -0.18267320096492767, -0.246023029088974, -0.1364057958126068, -0.2677566707134247, -0.22633549571037292, -0.24839939177036285, -0.3910634517669678, -0.17659923434257507, -0.4905271530151367, -0.11707214266061783, -0.2661204934120178, -0.5442976951599121, 0.09538312256336212, -0.16954822838306427, -0.1217859536409378, -0.2531115412712097);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"2c0983d5dead96bdf1e21cb4076cee19851dca714ac0fc6c292cae86494830a1",{{62,false,4,"wts"},{33,true,1,"b0"},{32,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/mid/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=19088u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[21392u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,288}},
{"74a5a11ddfb2eb18c33214972b12e183b3bd5783d196633d1917a82bc6d60a5b",{{33,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[96u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"d0aac7fce0ed8bcd5ffa0f11947c9a69ade85dedd5eb291f801cc2df8e48b087",{{60,false,4,"coef"},{33,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[96u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"def9096997d8a05645d03cca3b986f0dff320e8082ac41318495a346aa81a32a",{{33,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(0.5081897377967834, 0.7644691467285156, 0.8290867805480957, 0.8986796140670776, 0.762510359287262, 0.9900121092796326, 0.7823984026908875, 0.7295272350311279, 0.9870957732200623, 1.130997896194458, 0.875751256942749, 0.527197539806366, 0.796534538269043, 0.9716984033584595, 0.9013031125068665, 0.7536650896072388, 1.3932762145996094, 0.6555570363998413, 1.3508672714233398, 0.9287335872650146, 0.5928181409835815, 0.8442092537879944, 0.7362903952598572, 1.0761736631393433, 0.741619348526001, 1.1173399686813354, 0.3736945390701294, 0.9247899651527405, 0.9568728804588318, 0.8959638476371765, 0.5265315771102905, 0.26163533329963684);
    let P1 = array<f32, 32>(-0.09987767785787582, -0.13438884913921356, -0.05877642333507538, -0.23824474215507507, -0.13941921293735504, 0.12485743314027786, -0.13739053905010223, -0.0823148638010025, -0.1334325075149536, -0.35526829957962036, -0.37519562244415283, -0.05435401573777199, 0.016101744025945663, -0.4732634127140045, -0.1872863918542862, 0.06418627500534058, -0.6200655102729797, -0.21545864641666412, -0.7228708267211914, 0.03446994721889496, -0.2717108726501465, -0.05381655693054199, -0.09330607205629349, -0.281766802072525, -0.07468615472316742, 0.06546895205974579, -0.09822670370340347, -0.04003569483757019, 0.1357114315032959, -0.30019211769104004, -0.2645820081233978, -0.2570459246635437);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"52154eed84dc26e84a0606eb94125386f7f41c5f475ead42c084ba57f733b59c",{{62,false,4,"wts"},{34,true,1,"b0"},{31,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/mid/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 20u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=20){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/20;let kx=(q/20)%1;let ch=q%20;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=20 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%20);let ky=u32(row/20);let kx=u32((row/20)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=21408u+(((grp*24u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[21600u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,20}},
{"173921c8971eb507c2f49da868895f3a21ed2b5e5a3b98d77fb868a44ffb8eb2",{{62,false,4,"wts"},{35,true,1,"b0"},{33,false,1,"b1"},{34,false,1,"b2"},{23,false,1,"b3"},{19,false,1,"b4"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.0.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  v = v + b4[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 48u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=432){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/144;let kx=(q/48)%3;let ch=q%48;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=432 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%48);let ky=u32(row/144);let kx=u32((row/48)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=21616u+(((grp*48u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[25072u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,432}},
{"d7b5153cd486fe4289a2dc5e3d8cc2f200a0cbe4ebb61dbb13a1e08b76bc7d0c",{{35,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[128u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"4b91028e1b87bab03e6df6e124e5203a6dbf60aa0fdf3aadf87d45172b50708d",{{60,false,4,"coef"},{35,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[128u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"36b7ced341fa8d583531b2343115141d17239fc3729622bac7c8c57a23e483ec",{{36,false,1,"t3"},{37,false,1,"t4"},{35,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.3063278198242188, 1.2151318788528442, 1.1210111379623413, 1.3393210172653198, 0.6415340304374695, 1.1968353986740112, 1.2397879362106323, 0.8706600666046143, 1.1680362224578857, 1.0089877843856812, 1.1467055082321167, 1.1902170181274414, 1.1108332872390747, 1.1439334154129028, 1.2504383325576782, 0.506637692451477, 1.229719638824463, 1.1761924028396606, 0.8377634882926941, 1.1697442531585693, 0.9963582754135132, 1.1879781484603882, 1.1819031238555908, 1.084058403968811, 0.9828874468803406, 1.060011863708496, 0.729789674282074, 0.49181991815567017, 0.7541927099227905, 1.0839061737060547, 1.0446860790252686, 1.3604838848114014);
    let P1 = array<f32, 32>(-0.16197475790977478, -0.18183565139770508, 0.11513058096170425, -0.02175852097570896, -0.07427284121513367, 0.07947926968336105, -0.20542098581790924, -0.037493616342544556, -0.00292900949716568, -0.30592742562294006, -0.36552736163139343, -0.09798290580511093, -0.44761160016059875, -0.13710616528987885, -0.1656971126794815, -0.19841228425502777, 0.15257515013217926, -0.20376916229724884, -0.1494400054216385, -0.1402382254600525, 0.13688579201698303, 0.053514011204242706, -0.020509470254182816, -0.07318919152021408, -0.10586881637573242, -0.3925066888332367, 0.08316633850336075, -0.019759323447942734, 0.04890650138258934, -0.297044575214386, -0.017716679722070694, -0.2411116361618042);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"d48b987e01379447cc7e611e058486d296c38acebc460a9d8a4e88c20760c704",{{62,false,4,"wts"},{38,true,1,"b0"},{35,false,1,"b1"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.0.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=288){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=25088u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[27392u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,288}},
{"476f5d3abf588b15083cf5b9f7f965e4a66330a111f9a803c22b228b682f4943",{{38,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=55296u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 55296u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<8u;ci++){coef[160u+g*8u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"e85baf2cb2899882ad012bdeb17136a975fc49df39afe5120c90158b2b7cf91a",{{60,false,4,"coef"},{38,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/55296u;
let channel_scale_shift=coef[160u+channel*8u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"cbab8ce7d1e94ffb9be6d33baee52377a5a34fc9153ad3b3ccb8d2cee73249f0",{{38,true,4,"raw"}},{64,1,1},{3456,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 32>(1.2273502349853516, 1.9275903701782227, 1.3996951580047607, 0.51876300573349, 0.6036960482597351, 0.925854504108429, 2.1969854831695557, 0.9352912902832031, 1.4522594213485718, 1.1071077585220337, 1.263318657875061, 0.8660377264022827, 1.3773139715194702, 0.25398877263069153, 1.1053874492645264, 1.5405441522598267, 0.7302138805389404, 1.4792861938476562, 0.9397917985916138, 1.1901124715805054, 0.561262845993042, 0.8749394416809082, 0.7628927230834961, 0.5252271294593811, 1.0254178047180176, 0.5998544692993164, 0.6686235666275024, 1.03556489944458, 1.750168800354004, 1.4584732055664062, 2.0559194087982178, 0.5781452059745789);
    let P1 = array<f32, 32>(-0.25200989842414856, -1.1468274593353271, 0.09846160560846329, -0.3203883469104767, 0.04753804951906204, 0.04152490198612213, -0.2486402541399002, -0.5093510746955872, -0.2127590924501419, -0.6425765752792358, -0.26733407378196716, -0.023601679131388664, -0.20476870238780975, -0.1414472758769989, -0.36965397000312805, -0.7867390513420105, 0.13105562329292297, -0.8789019584655762, -0.34256643056869507, 0.22001755237579346, -0.09231618791818619, -0.16366301476955414, -0.103691466152668, 0.00915013812482357, -0.3460608422756195, -0.49706703424453735, -0.1851155161857605, 0.08504112809896469, 0.07026352733373642, -0.014422710053622723, -0.1667596697807312, 0.015447761863470078);
let channel=global_idx*4u/27648u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"ad6b9b3a7c8abf985c31da6ec66bcc7483d473a34d82965b8dd48035c402c934",{{62,false,4,"wts"},{39,true,1,"b0"},{33,false,1,"b1"},{34,false,1,"b2"},{23,false,1,"b3"},{19,false,1,"b4"}},{8,8,1},{1,864,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.0.0/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 192u + sx;
  var v = b3[(0u + sc) * 27648u + p];
  v = v + b4[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  if (c < 48u) { return S1(c - 32u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=27648 || q<0 || q>=48){return 0.0;}
 let oy=row/192;let ox=row%192;let ky=q/48;let kx=(q/48)%1;let ch=q%48;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=144 || ix<0 || ix>=192){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=48 || colIn*4>=32){return vec4<f32>(0.0);}
 let ch=u32(row%48);let ky=u32(row/48);let kx=u32((row/48)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=27408u+(((grp*48u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=27648 || colIn*4>=32){return;}
 let pix=u32(row);let value=valueIn+wts[27792u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*27648u+pix]=value.x;
b0[1u*27648u+pix]=value.y;
b0[2u*27648u+pix]=value.z;
b0[3u*27648u+pix]=value.w;
}
case 1u:{
b0[4u*27648u+pix]=value.x;
b0[5u*27648u+pix]=value.y;
b0[6u*27648u+pix]=value.z;
b0[7u*27648u+pix]=value.w;
}
case 2u:{
b0[8u*27648u+pix]=value.x;
b0[9u*27648u+pix]=value.y;
b0[10u*27648u+pix]=value.z;
b0[11u*27648u+pix]=value.w;
}
case 3u:{
b0[12u*27648u+pix]=value.x;
b0[13u*27648u+pix]=value.y;
b0[14u*27648u+pix]=value.z;
b0[15u*27648u+pix]=value.w;
}
case 4u:{
b0[16u*27648u+pix]=value.x;
b0[17u*27648u+pix]=value.y;
b0[18u*27648u+pix]=value.z;
b0[19u*27648u+pix]=value.w;
}
case 5u:{
b0[20u*27648u+pix]=value.x;
b0[21u*27648u+pix]=value.y;
b0[22u*27648u+pix]=value.z;
b0[23u*27648u+pix]=value.w;
}
case 6u:{
b0[24u*27648u+pix]=value.x;
b0[25u*27648u+pix]=value.y;
b0[26u*27648u+pix]=value.z;
b0[27u*27648u+pix]=value.w;
}
case 7u:{
b0[28u*27648u+pix]=value.x;
b0[29u*27648u+pix]=value.y;
b0[30u*27648u+pix]=value.z;
b0[31u*27648u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{27648,32,48}},
{"0e15d57bcdd790f4675a4bbf371fbeb35ec620965068c577f19c9fefb90b5f5e",{{62,false,4,"wts"},{40,true,1,"b0"},{38,false,1,"b1"},{39,false,1,"b2"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.0.1/ups.0.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y) >> 1u; let sx = u32(x) >> 1u;
  let p = sy * 192u + sx;
  var v = b1[(0u + sc) * 27648u + p];
  v = v + b2[(0u + sc) * 27648u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 32u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=288){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/96;let kx=(q/32)%3;let ch=q%32;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=288 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%32);let ky=u32(row/96);let kx=u32((row/32)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=27808u+(((grp*32u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[28960u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,288}},
{"b45d1ae6227df2e2d8e6a43b07d3b504ac84447eb1e78ff41ddb1c754f103b00",{{62,false,4,"wts"},{41,true,1,"b0"},{40,false,1,"b1"},{18,false,1,"b2"},{14,false,1,"b3"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.1.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  v = v + b3[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 24u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=216){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/72;let kx=(q/24)%3;let ch=q%24;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=216 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%24);let ky=u32(row/72);let kx=u32((row/24)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=28976u+(((grp*24u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[29840u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,216}},
{"0ce2ee03d30e3d7dcbc82f959531e8ed835be087dadd4b555fda5a1ee79fe577",{{41,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=110592u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 110592u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[192u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"e73eefeae8587008fac2fd2137cfd83e64b40b394ff0cb20ce5ed5bc973f8425",{{60,false,4,"coef"},{41,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/110592u;
let channel_scale_shift=coef[192u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"c5a7f3b5556c3113b3ac6d442c810e0152ed6186b15dcb5e1f44e5493469e401",{{42,false,1,"t3"},{43,false,1,"t4"},{41,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.071933388710022, 0.6144050359725952, 0.8586381673812866, 1.3540096282958984, 1.3308886289596558, 1.5460915565490723, 0.7262976765632629, 0.9540891051292419, 0.9519808888435364, 1.113931655883789, 0.9017605185508728, 0.9464331865310669, 0.9993612170219421, 1.0123021602630615, 1.3898520469665527, 1.4444743394851685);
    let P1 = array<f32, 16>(-0.03616371005773544, -0.17774008214473724, -0.011179293505847454, -0.560824990272522, -0.21618051826953888, -0.29197508096694946, -0.10306720435619354, -0.01993977651000023, -0.12736448645591736, 0.07968734204769135, 0.0661754459142685, 0.06390447914600372, -0.21356584131717682, 0.05773831158876419, -0.4434376060962677, -0.5049852728843689);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"39423cb086110324a2684aeabb8aac346063d94ea31b0b0d0337c88d4d401192",{{62,false,4,"wts"},{44,true,1,"b0"},{41,false,1,"b1"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.1.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=144){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=29856u+(((grp*16u+ch)*3u+ky)*3u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[30432u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,144}},
{"8e39aa094d9104709a8e0abdcd03030b5d40a72409bd60f927cad8a80683b8f4",{{44,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=110592u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 110592u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<4u;ci++){coef[208u+g*4u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"dc4edce4d041443fc7f2df0b0cf2b860da169d09c4cdf93927276bdaa4bd8f6f",{{60,false,4,"coef"},{44,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/110592u;
let channel_scale_shift=coef[208u+channel*4u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"3a87249bbe476f75768eb2e66f78f1d8dd17069d66707c90cf3aa6541701ca25",{{44,true,4,"raw"}},{64,1,1},{6912,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 16>(1.384238600730896, 1.5721830129623413, 1.3906223773956299, 0.9189033508300781, 1.4390791654586792, 1.2698345184326172, 2.1414549350738525, 2.0327465534210205, 1.8643683195114136, 1.991784930229187, 2.0229380130767822, 1.2356288433074951, 0.9946795701980591, 1.0031911134719849, 1.0363576412200928, 1.3235195875167847);
    let P1 = array<f32, 16>(-0.0020258056465536356, -0.29081201553344727, -0.007741477806121111, 0.26457324624061584, -0.03336023911833763, 0.10371503233909607, -0.8393456339836121, -0.465729296207428, -0.21286973357200623, -0.5399280190467834, -0.4001143276691437, -0.04728317633271217, -0.22817765176296234, -0.6505697965621948, -0.05977911129593849, -0.05523374304175377);
let channel=global_idx*4u/110592u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"44cc4e5b0b2747911412aa1896a95c9f1db4e9fa7ce8e4290972ed0188041626",{{62,false,4,"wts"},{45,true,1,"b0"},{40,false,1,"b1"},{18,false,1,"b2"},{14,false,1,"b3"}},{8,8,1},{1,3456,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.1.0/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 384u + sx;
  var v = b2[(0u + sc) * 110592u + p];
  v = v + b3[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  if (c < 24u) { return S1(c - 16u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=110592 || q<0 || q>=24){return 0.0;}
 let oy=row/384;let ox=row%384;let ky=q/24;let kx=(q/24)%1;let ch=q%24;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=288 || ix<0 || ix>=384){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=24 || colIn*4>=16){return vec4<f32>(0.0);}
 let ch=u32(row%24);let ky=u32(row/24);let kx=u32((row/24)%1);let col=u32(colIn*4);let grp=col/16u;let mv=(col%16u)/4u;
 let ix=30448u+(((grp*24u+ch)*1u+ky)*1u+kx)*4u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=110592 || colIn*4>=16){return;}
 let pix=u32(row);let value=valueIn+wts[30544u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*110592u+pix]=value.x;
b0[1u*110592u+pix]=value.y;
b0[2u*110592u+pix]=value.z;
b0[3u*110592u+pix]=value.w;
}
case 1u:{
b0[4u*110592u+pix]=value.x;
b0[5u*110592u+pix]=value.y;
b0[6u*110592u+pix]=value.z;
b0[7u*110592u+pix]=value.w;
}
case 2u:{
b0[8u*110592u+pix]=value.x;
b0[9u*110592u+pix]=value.y;
b0[10u*110592u+pix]=value.z;
b0[11u*110592u+pix]=value.w;
}
case 3u:{
b0[12u*110592u+pix]=value.x;
b0[13u*110592u+pix]=value.y;
b0[14u*110592u+pix]=value.z;
b0[15u*110592u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{110592,16,24}},
{"b83d87bafc68e43564c33811e57c2e61922ee2abb6cd4993e92a99e8f4eb8c1e",{{62,false,4,"wts"},{46,true,1,"b0"},{44,false,1,"b1"},{45,false,1,"b2"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.1.1/ups.1.1.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y) >> 1u; let sx = u32(x) >> 1u;
  let p = sy * 384u + sx;
  var v = b1[(0u + sc) * 110592u + p];
  v = v + b2[(0u + sc) * 110592u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 16u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=144){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=30560u+(((grp*16u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[30848u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,144}},
{"41d53f92e99abe4d1ef6f7a84c523d2bb409cb73bf0d2214f9819d1ec13c9f7a",{{62,false,4,"wts"},{47,true,1,"b0"},{46,false,1,"b1"},{13,false,1,"b2"},{9,false,1,"b3"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.2.0/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b2[(0u + sc) * 442368u + p];
  v = v + b3[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  if (c < 16u) { return S1(c - 8u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=144){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=30864u+(((grp*16u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31152u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,144}},
{"388ad7319725d3c857e20b8bc26cfc1c0816170865619f68a669e8ab24dcf86f",{{47,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=221184u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 221184u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[224u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"91f0659772e5f1d580f3c32fa4a46d9ff7b183ff442752385c7a2746e4eddcf3",{{60,false,4,"coef"},{47,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/221184u;
let channel_scale_shift=coef[224u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"34eb525516eb109dad53a3a1c6059ddc99856354022a9fc185eb9ee1ae7f6fa1",{{48,false,1,"t3"},{49,false,1,"t4"},{47,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.8507720232009888, 0.6434476971626282, 1.2006244659423828, 0.642288863658905, 0.7959140539169312, 0.9013049006462097, 1.0159988403320312, 1.2116272449493408);
    let P1 = array<f32, 8>(-0.23002560436725616, -0.1665038764476776, -0.4210740029811859, -0.16059842705726624, -0.2862122058868408, -0.2962667942047119, -0.006381908897310495, -0.4407266676425934);
let channel=global_idx*4u/442368u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"9bb46f64c0b0c12fe8467cc7543389a3a18bc0b9898bc9dfbf5baba55e68291f",{{62,false,4,"wts"},{50,true,1,"b0"},{47,false,1,"b1"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.2.0/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=72){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31168u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31312u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,72}},
{"023754cbb3bd1875451090b0b6d70e311e68dce4593f848f06e25c1c8143bead",{{50,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=221184u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 221184u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[232u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"3e89c06b80e54e44b011a755167fc87a07a5f16bb35b3094b6d1c2afbd72c339",{{60,false,4,"coef"},{50,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/221184u;
let channel_scale_shift=coef[232u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"96494c5df961791557087edcd6268592f487c200c18ff432b275102fa8990222",{{50,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.8066206574440002, 0.7152596116065979, 0.7844045162200928, 0.9872947335243225, 0.7914931178092957, 1.1699351072311401, 1.1401056051254272, 0.8229765295982361);
    let P1 = array<f32, 8>(0.17504765093326569, 0.498104453086853, 0.4720593988895416, 0.008328877389431, 0.3995059132575989, 0.18382272124290466, -0.06084665283560753, 0.3168374300003052);
let channel=global_idx*4u/442368u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"3223b260af682cdc2a57f9999ce2f51fdb1548b32f367a92a60d2633cc0886b3",{{62,false,4,"wts"},{51,true,1,"b0"},{46,false,1,"b1"},{13,false,1,"b2"},{9,false,1,"b3"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.2.0/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b2[(0u + sc) * 442368u + p];
  v = v + b3[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  if (c < 16u) { return S1(c - 8u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=16){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/16;let kx=(q/16)%1;let ch=q%16;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=16 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/16);let kx=u32((row/16)%1);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31328u+(((grp*16u+ch)*1u+ky)*1u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31360u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,16}},
{"e8e8ae777fa80db8bd3035fb117d9ab054c5021cf0504edc956e7b839e302f6b",{{62,false,4,"wts"},{52,true,1,"b0"},{50,false,1,"b1"},{51,false,1,"b2"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/ups.2.1_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  v = v + b2[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=72){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31376u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31520u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,72}},
{"e217b597ad20f28f1fb67cbec0882eb84149355101cfeb5d62a49e8009c697cb",{{62,false,4,"wts"},{53,true,1,"b0"},{52,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/final_res_block/block1/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b2[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  if (c < 16u) { return S1(c - 8u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=144){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/48;let kx=(q/16)%3;let ch=q%16;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=144 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/48);let kx=u32((row/16)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31536u+(((grp*16u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31824u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,144}},
{"faf4761c0bd7b6ad72de8d950f71c02db004318d6a625ca1ae19902f80ad8f66",{{53,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=221184u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 221184u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[240u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"0900e84755d92a444cc364acb940e4e0605b8e6880b2a51b53d3beaada93fb10",{{60,false,4,"coef"},{53,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/221184u;
let channel_scale_shift=coef[240u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"675a74971d70f80d888c036a29726138c3347ce57d49c5c536e2c415afd0da5e",{{54,false,1,"t3"},{55,false,1,"t4"},{53,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(var<workgroup> norm_mul_scratch:array<vec4<f32>,64>;
var<workgroup> norm_add_scratch:array<vec4<f32>,64>;
var<workgroup> condition_mul_scratch:array<vec4<f32>,64>;
var<workgroup> shift_add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
var<workgroup> self_mul_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(1.0000128746032715, 1.419042944908142, 1.259379267692566, 1.2294976711273193, 1.2824100255966187, 0.8807469010353088, 1.0456726551055908, 0.506299614906311);
    let P1 = array<f32, 8>(-0.028392791748046875, -0.19860801100730896, 0.11362238973379135, 0.03532786667346954, -0.35307055711746216, 0.020872825756669044, -0.04130525514483452, -0.22246716916561127);
let channel=global_idx*4u/442368u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
norm_mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=norm_mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
norm_add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a_condition=norm_add_scratch[local_idx];let b_condition=vec4<f32>(t3[channel]);
condition_mul_scratch[local_idx]=a_condition*b_condition;
workgroupBarrier();
let a_shift=condition_mul_scratch[local_idx];let b_shift=vec4<f32>(t4[channel]);
shift_add_scratch[local_idx]=a_shift+b_shift;
workgroupBarrier();
let a=shift_add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=shift_add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
self_mul_scratch[local_idx]=final_a*final_b;
workgroupBarrier();
raw[global_idx]=self_mul_scratch[local_idx];
)WGSL_CODE",0,{0,0,0}},
{"7d458ac282674339a2b544f182d8b0c92961b3c658a408fb6d7b729231dee793",{{62,false,4,"wts"},{56,true,1,"b0"},{53,false,1,"b1"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/final_res_block/block2/proj_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=72){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/24;let kx=(q/8)%3;let ch=q%8;let iy=oy+ky-1;let ix=ox+kx-1;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=72 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/24);let kx=u32((row/8)%3);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=31840u+(((grp*8u+ch)*3u+ky)*3u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[31984u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,72}},
{"386334cffba9f7c60a89e200b837c70747e220cad5be5b92a464a8d60def9b3f",{{56,false,4,"raw"},{60,true,4,"coef"}},{256,1,1},{4,1,1},R"WGSL_CODE(var<workgroup> workgroup_shared_sum: array<vec4<f32>,256>;
var<workgroup> workgroup_shared_squared_sum: array<vec4<f32>,256>;
const workgroup_size=256u;
)WGSL_CODE",R"WGSL_CODE(let g=workgroup_id.x; let height=221184u; let channel=g;
let sg = array<f32, 4>(1.0, 1.0, 1.0, 1.0); let bg = array<f32, 4>(0.0, 0.0, 0.0, 0.0);

  var sum = vec4<f32>(0);
  var squared_sum = vec4<f32>(0);
  for (var h = local_idx; h < height; h += workgroup_size) {
    
    let value = vec4<f32>(raw[g * 221184u + h]);
    sum += value;
    squared_sum += value * value;
  }
  workgroup_shared_sum[local_idx] = sum;
  workgroup_shared_squared_sum[local_idx] = squared_sum;
  workgroupBarrier();
  for (var currSize = workgroup_size >> 1; currSize > 0; currSize = currSize >> 1) {
    if (local_idx < u32(currSize)) {
      workgroup_shared_sum[local_idx] = workgroup_shared_sum[local_idx] + workgroup_shared_sum[local_idx + u32(currSize)];
      workgroup_shared_squared_sum[local_idx] = workgroup_shared_squared_sum[local_idx] + workgroup_shared_squared_sum[local_idx + u32(currSize)];
    }
    workgroupBarrier();
  }
  if (local_idx == 0) {
    let sum_final = (workgroup_shared_sum[0].x + workgroup_shared_sum[0].y + workgroup_shared_sum[0].z + workgroup_shared_sum[0].w) / f32(height * 4);
    let squared_sum_final = (workgroup_shared_squared_sum[0].x + workgroup_shared_squared_sum[0].y + workgroup_shared_squared_sum[0].z + workgroup_shared_squared_sum[0].w) / f32(height * 4);
    let inv_std_dev = inverseSqrt(squared_sum_final - sum_final * sum_final + f32(0.000010));
    let channel_scale = inv_std_dev * f32(sg[channel]);
    let channel_shift = f32(bg[channel]) - sum_final * channel_scale;
    for(var ci=0u;ci<2u;ci++){coef[248u+g*2u+ci]=vec4<f32>(channel_scale,channel_shift,0.0,0.0);}
  }
)WGSL_CODE",0,{0,0,0}},
{"bd98a6478497db52fde66a257e1d449ea8f8cde7c850ae852dbd943e76b28665",{{60,false,4,"coef"},{56,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(// Original selected R50 InstanceNorm output expression; same raw buffer in-place.
)WGSL_CODE",R"WGSL_CODE(let channel=global_idx/221184u;
let channel_scale_shift=coef[248u+channel*2u];
let input_value=raw[global_idx];
let output_value=input_value * vec4<f32>(channel_scale_shift.x) + vec4<f32>(channel_scale_shift.y);
raw[global_idx]=output_value;
)WGSL_CODE",0,{0,0,0}},
{"08a15edd46789172dccb0ce1e1d4860b384bdd2832e2d4331b4542021a436887",{{56,true,4,"raw"}},{64,1,1},{13824,1,1},R"WGSL_CODE(var<workgroup> mul_scratch:array<vec4<f32>,64>;
var<workgroup> add_scratch:array<vec4<f32>,64>;
var<workgroup> sigmoid_scratch:array<vec4<f32>,64>;
)WGSL_CODE",R"WGSL_CODE(    let P0 = array<f32, 8>(0.6700785756111145, -0.010936703532934189, 0.29364126920700073, 0.013072769157588482, 0.7185232043266296, -0.004493084270507097, 0.5402140021324158, 0.02248045802116394);
    let P1 = array<f32, 8>(-0.02259894087910652, -0.1038062646985054, -0.1199500635266304, -0.056562311947345734, 0.004977676551789045, 0.030465375632047653, -2.7194950234843418e-05, -0.14645349979400635);
let channel=global_idx*4u/442368u;
let a_mul=raw[global_idx];let b_mul=vec4<f32>(P0[channel]);
mul_scratch[local_idx]=a_mul*b_mul;
workgroupBarrier();
let a_add=mul_scratch[local_idx];let b_add=vec4<f32>(P1[channel]);
add_scratch[local_idx]=a_add+b_add;
workgroupBarrier();
let a=add_scratch[local_idx];
sigmoid_scratch[local_idx]=1.0 / (1.0 + exp(-a));
workgroupBarrier();
let final_a=add_scratch[local_idx];let final_b=sigmoid_scratch[local_idx];
raw[global_idx]=final_a*final_b;
)WGSL_CODE",0,{0,0,0}},
{"e458700ae651ce8a59f42a83a7adaa5db39a87baad1114965820e1ecbbc69184",{{62,false,4,"wts"},{57,true,1,"b0"},{52,false,1,"b1"},{9,false,1,"b2"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/final_res_block/res_conv_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b1[(0u + sc) * 442368u + p];
  return v;
}
fn S1(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b2[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  if (c < 16u) { return S1(c - 8u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=16){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/16;let kx=(q/16)%1;let ch=q%16;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=16 || colIn*4>=8){return vec4<f32>(0.0);}
 let ch=u32(row%16);let ky=u32(row/16);let kx=u32((row/16)%1);let col=u32(colIn*4);let grp=col/8u;let mv=(col%8u)/4u;
 let ix=32000u+(((grp*16u+ch)*1u+ky)*1u+kx)*2u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=8){return;}
 let pix=u32(row);let value=valueIn+wts[32032u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b0[1u*442368u+pix]=value.y;
b0[2u*442368u+pix]=value.z;
b0[3u*442368u+pix]=value.w;
}
case 1u:{
b0[4u*442368u+pix]=value.x;
b0[5u*442368u+pix]=value.y;
b0[6u*442368u+pix]=value.z;
b0[7u*442368u+pix]=value.w;
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,8,16}},
{"fe7ac10aa39e32ba2a9fb400fae7b29fd1c231027211e40826b71fbee299b1eb",{{62,false,4,"wts"},{58,true,1,"b0"},{59,true,1,"b1"},{56,false,1,"b2"},{57,false,1,"b3"}},{8,8,1},{1,13824,1},R"WGSL_CODE(// Fused convolution: /core/update_block_depth3/unet/final_conv_1/Conv, /core/update_block_depth3/unet/conf_1/Conv
fn S0(c: u32, y: i32, x: i32) -> f32 {
  let sc = c; let sy = u32(y); let sx = u32(x);
  let p = sy * 768u + sx;
  var v = b2[(0u + sc) * 442368u + p];
  v = v + b3[(0u + sc) * 442368u + p];
  return v;
}
fn src_val(c: u32, y: i32, x: i32) -> f32 {
  if (c < 8u) { return S0(c - 0u, y, x); }
  return 0.0;
}

// Original ORT Tile32 implicit-GEMM read adapter. Flattened q=(ky*KW+kx)*C+ch, unchanged source order.
fn w6_read_scalar(row:i32,q:i32)->f32{
 if(row<0 || row>=442368 || q<0 || q>=8){return 0.0;}
 let oy=row/768;let ox=row%768;let ky=q/8;let kx=(q/8)%1;let ch=q%8;let iy=oy+ky-0;let ix=ox+kx-0;
 if(iy<0 || iy>=576 || ix<0 || ix>=768){return 0.0;}
 return src_val(u32(ch),iy,ix);
}
fn mm_readA(batch:i32,row:i32,colIn:i32)->vec4<f32>{let q=colIn*4;return vec4<f32>(w6_read_scalar(row,q),w6_read_scalar(row,q+1),w6_read_scalar(row,q+2),w6_read_scalar(row,q+3));}
fn mm_readB(batch:i32,row:i32,colIn:i32)->vec4<f32>{
 if(row<0 || row>=8 || colIn*4>=2){return vec4<f32>(0.0);}
 let ch=u32(row%8);let ky=u32(row/8);let kx=u32((row/8)%1);let col=u32(colIn*4);let grp=col/4u;let mv=(col%4u)/4u;
 let ix=32048u+(((grp*8u+ch)*1u+ky)*1u+kx)*1u+mv;
 return wts[ix];
}
fn mm_write(batch:i32,row:i32,colIn:i32,valueIn:vec4<f32>){
 if(row<0 || row>=442368 || colIn*4>=2){return;}
 let pix=u32(row);let value=valueIn+wts[32056u+u32(colIn)];
 switch u32(colIn){
case 0u:{
b0[0u*442368u+pix]=value.x;
b1[0u*442368u+pix]=1.0 / (1.0 + exp(-value.y));
}
default:{}
}
}
)WGSL_CODE",R"WGSL_CODE()WGSL_CODE",1,{442368,2,8}},
},{{1,20,144,192},{1,1,576,768},{1,1,576,768}},{7077888,7077888,7077888,3538944,3538944,884736,3538944,442368,1769472,884736,884736}},
};