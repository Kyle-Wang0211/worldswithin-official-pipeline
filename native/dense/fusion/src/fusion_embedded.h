static constexpr const char kFusionSha256[]="a60e966c9d9fcbf6c2a9da6c75b594a877e1648ae7208a027dd0b2a37ca6c798";
static constexpr const char kDawnHeaderIdentity[]="09f60549925d00e4f3972a9d623fe4aa6a1766c5cec308f8eda6ab2ca794aa47";
static constexpr const char kFusionWgsl[]=R"RECONSTRUCTIONWGSL(// Selected filter.py reprojection + consistency, explicitly f32 execution.
// OpenCV INTER_LINEAR coordinate convention: nearest-even 1/32 map quantization,
// zero border, dyadic bilinear weights. Arithmetic/compiler equivalence is measured.
struct Params { w:u32,h:u32, dmax:f32,dmin:f32, pixel:f32,relative:f32,pad0:f32,pad1:f32 };
@group(0) @binding(0) var<storage,read> reference_depth:array<f32>;
@group(0) @binding(1) var<storage,read> src:array<f32>;
@group(0) @binding(2) var<storage,read> m:array<f32>;
@group(0) @binding(3) var<storage,read_write> out:array<f32>;
@group(0) @binding(4) var<uniform> p:Params;
@group(0) @binding(5) var<storage,read> photo:array<u32>;
@group(0) @binding(6) var<storage,read_write> retained_d16:array<u32>;
@group(0) @binding(7) var<storage,read_write> diagnostics:array<atomic<u32>>;
// Native ordered_dot scalar k-order and explicit FMA, lowered F64 -> F32.
fn row3(o:u32,v:vec3<f32>)->f32 {var a=0.0;a=fma(m[o],v.x,a);a=fma(m[o+1u],v.y,a);a=fma(m[o+2u],v.z,a);return fma(a,1.0,0.0);}
fn row4(o:u32,v:vec3<f32>)->f32 {var a=0.0;a=fma(m[o],v.x,a);a=fma(m[o+1u],v.y,a);a=fma(m[o+2u],v.z,a);a=fma(m[o+3u],1.0,a);return fma(a,1.0,0.0);}
fn mul3(o:u32,v:vec3<f32>)->vec3<f32>{return vec3<f32>(row3(o,v),row3(o+3u,v),row3(o+6u,v));}
fn mul4(o:u32,v:vec3<f32>)->vec3<f32>{return vec3<f32>(row4(o,v),row4(o+4u,v),row4(o+8u,v));}
fn load_depth(x:i32,y:i32)->f32{if(x<0||y<0||x>=i32(p.w)||y>=i32(p.h)){return 0.0;}return src[u32(y)*p.w+u32(x)];}
fn evenround(v:f32)->i32{let b=floor(v);let f=v-b;var r=b;if(f>0.5||(f==0.5&&(i32(b)&1)==1)){r=b+1.0;}return i32(r);}
fn sample(x:f32,y:f32)->f32{if(x!=x||y!=y||abs(x)>1e7||abs(y)>1e7){return 0.0;}let qx=evenround(x*32.0);let qy=evenround(y*32.0);let ix=qx>>5;let iy=qy>>5;let a=f32(qx&31)/32.0;let b=f32(qy&31)/32.0;let w0=(1.0-a)*(1.0-b);let w1=a*(1.0-b);let w2=(1.0-a)*b;let w3=a*b;return ((load_depth(ix,iy)*w0+load_depth(ix+1,iy)*w1)+load_depth(ix,iy+1)*w2)+load_depth(ix+1,iy+1)*w3;}
struct PairResult { mask:bool,depth:f32 };
fn pair_at(i:u32)->PairResult{let x=f32(i%p.w);let y=f32(i/p.w);let d=reference_depth[i];let xyzr=mul3(0,vec3<f32>(x,y,1.0)*d);let xyzs=mul4(9,xyzr);let proj=mul3(25,xyzs);let xy=proj.xy/proj.z;let sd=sample(xy.x,xy.y);let back=mul3(34,vec3<f32>(xy,1.0)*sd);let rp=mul4(43,back);var pr=mul3(59,rp);pr=select(pr,vec3<f32>(1e-5),pr==vec3<f32>(0.0));let rxy=clamp(pr.xy/pr.z,vec2<f32>(-1e8),vec2<f32>(1e8));let dist=length(rxy-vec2<f32>(x,y));let rel=abs(rp.z-d)/d;let gm=dist<p.pixel&&rel<p.relative&&d>p.dmin&&d<p.dmax;return PairResult(gm,rp.z);}

@compute @workgroup_size(128) fn fusion_pair(@builtin(global_invocation_id) id:vec3<u32>){let i=id.x;if(i>=p.w*p.h){return;}
let pair=pair_at(i);var keep=pair.mask;
if(p.pad0>0.5 && keep){let x=f32(i%p.w);let y=f32(i/p.w);let d=reference_depth[i];let xc=vec3<f32>((x-m[61])/m[59]*d,(y-m[64])/m[63]*d,d);let xt=xc-vec3<f32>(m[71],m[75],m[79]);let world=vec3<f32>(dot(xt,vec3<f32>(m[68],m[72],m[76])),dot(xt,vec3<f32>(m[69],m[73],m[77])),dot(xt,vec3<f32>(m[70],m[74],m[78])));var rr=vec3<f32>(m[87],m[88],m[89])-world;var rs=vec3<f32>(m[84],m[85],m[86])-world;rr/=max(length(rr),1e-12);rs/=max(length(rs),1e-12);keep=dot(rr,rs)<=m[90];}
if(p.pad1>0.5){out[i*2u]=0.0;out[i*2u+1u]=0.0;}
if(keep){out[i*2u]+=1.0;out[i*2u+1u]+=pair.depth;}}
// Selected one(): ordered-source acc, (acc+ref)/(gs+1), photo & gs>=2,
// np.round(depth*5000).astype(uint16). F32 averaging is an explicit V3 adaptation.
@compute @workgroup_size(128) fn fusion_final(@builtin(global_invocation_id) id:vec3<u32>){
 let i=id.x;if(i>=p.w*p.h){return;}
 let count=u32(out[i*2u]);let avg=(out[i*2u+1u]+reference_depth[i])/(f32(count)+1.0);
 if(avg!=avg || abs(avg)>3.402823e38){atomicAdd(&diagnostics[1],1u);}
 let keep=photo[i]!=0u && count>=2u;
 var d=0.0;if(keep){d=avg;atomicAdd(&diagnostics[0],1u);}
 if(photo[i]!=0u){atomicAdd(&diagnostics[2u+min(count,10u)],1u);}
 // Host domain guard requires effective dmax*5000 < 2^31 before launch.
 retained_d16[i]=u32(evenround(d*5000.0))&65535u;
}
)RECONSTRUCTIONWGSL";
