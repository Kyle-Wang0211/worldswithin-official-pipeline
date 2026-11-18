                                                                                
  
                                                                              
                                                                                    
                                                                               
                                                                              
                                                                              
                                                                             
                                                                                   
                                                                         
                                       
  
enable f16;
  
                                                                             
                                                                                  
                                                                           
  
                                            
                                                                                            
                                                                                       
                                                                                
                                                                                  
                                                  
                                                                                  
                                                                           
                                                
                                         
                                                                  
  
                                                                                
                                                          
  
                                                                        
                                                                      
                                                                                  
                                                       
  
                                                                                 
                                                                                 
                                                                       

const SIDE : u32 = 31u;                  
const NPIX : u32 = 961u;
const RES  : f32 = 15.0;                         
const EXTENT : f32 = 7.5;                            
const SMOOTH : f32 = 1.0;                                                      
const KSIGMA : f32 = 2.0;                                                   
const MAGNIF : f32 = 3.0;
const NBO : i32 = 8;
const NBP : i32 = 4;
const WINSIZE : f32 = 2.0;                         
const DSP_NUM : u32 = 6u;                                                                              
const KP_STRIDE : u32 = 8u;
const WG : u32 = 64u;
const PI : f32 = 3.14159265358979323846;
const EPS_D : f32 = 2.220446049250313e-16;
const EPS_F : f32 = 1.19209290e-07;

struct LevelMeta { offset : u32, width : u32, height : u32, pad : u32 };
struct Params {
  count        : u32,
  first_octave : i32,
  last_octave  : i32,
  oct_res      : i32,
  base_scale   : f32,
  oct_first_sub: i32,
  oct_last_sub : i32,
  levels_per_oct : u32,
  dsp_min_scale  : f32,
  dsp_scale_step : f32,
  pad0 : u32, pad1 : u32,
};

@group(0) @binding(0) var<storage, read>       packed_gss : array<f32>;
@group(0) @binding(1) var<storage, read>       level_meta : array<LevelMeta>;
@group(0) @binding(2) var<storage, read>       kp_in      : array<u32>;                     
@group(0) @binding(3) var<storage, read_write> raw_desc   : array<f32>;                          
@group(0) @binding(4) var<uniform>             P          : Params;

var<workgroup> wpatch : array<f16, 961>;                                     
var<workgroup> A_sh   : array<f32, 4>;                                        
var<workgroup> T_sh   : array<f32, 2>;
var<workgroup> dsh    : array<f32, 2>;                 
var<workgroup> hist   : array<f32, 128>;                                             
var<workgroup> accum  : array<f32, 128>;                                         
                                                                           
                                                                               
                                                                             
const BLK : u32 = 32u;
var<workgroup> redM : array<f32, 2048>;                  

                                                 
fn fsign(x : f32) -> f32 { if (x < 0.0) { return -1.0; } return 1.0; }
struct Dlasv2 { smin:f32, smax:f32, sv:f32, cv:f32, su:f32, cu:f32 };
fn dlasv2(f : f32, g : f32, h : f32) -> Dlasv2 {
  var o : Dlasv2;
  var ft=f; var gt=g; var ht=h;
  var fa=abs(f); var ga=abs(g); var ha=abs(h);
  var pmax:i32=1; var swap:i32=0; var glarge:i32=0;
  var svt:f32=0.0; var cvt:f32=1.0; var sut:f32=0.0; var cut:f32=1.0;
  var smin:f32=0.0; var smax:f32=0.0;
  if (fa<ha){pmax=3; let t1=ft;ft=ht;ht=t1; let t2=fa;fa=ha;ha=t2; swap=1;}
  if (ga==0.0){smin=ha;smax=fa;cut=1.0;sut=0.0;cvt=1.0;svt=0.0;}
  else {
    if (ga>fa){pmax=2; if((fa/ga)<EPS_D){glarge=1;smax=ga;
      if(ha>1.0){smin=fa/(ga/ha);}else{smin=(fa/ga)*ha;}
      cut=1.0;sut=ht/gt;cvt=1.0;svt=ft/gt;}}
    if (glarge==0){
      var d:f32; let fmh=fa-ha;
      if(fmh==fa){d=1.0;}else{d=fmh/fa;}
      let q=gt/ft; let s=2.0-d; let dd=d*d; let qq=q*q; let ss=s*s;
      let spq=sqrt(ss+qq); var dpq:f32; if(d==0.0){dpq=abs(q);}else{dpq=sqrt(dd+qq);}
      let a=0.5*(spq+dpq); smin=ha/a; smax=fa*a;
      var tmp:f32;
      if(qq==0.0){ if(d==0.0){tmp=fsign(ft)*2.0*fsign(gt);}else{tmp=gt/(fsign(ft)*fmh)+q/s;} }
      else { tmp=(q/(spq+s)+q/(dpq+d))*(1.0+a); }
      let tt=sqrt(tmp*tmp+4.0);
      cvt=2.0/tt; svt=tmp/tt; cut=(cvt+svt*q)/a; sut=(ht/ft)*svt/a;
    }
  }
  if(swap==1){o.cu=svt;o.su=cvt;o.cv=sut;o.sv=cut;}else{o.cu=cut;o.su=sut;o.cv=cvt;o.sv=svt;}
  var tsign:f32=1.0;
  if(pmax==1){tsign=fsign(o.cv)*fsign(o.cu)*fsign(f);}
  if(pmax==2){tsign=fsign(o.sv)*fsign(o.cu)*fsign(g);}
  if(pmax==3){tsign=fsign(o.sv)*fsign(o.su)*fsign(h);}
  o.smax=tsign*smax; o.smin=(tsign*fsign(f)*fsign(h))*smin;
  return o;
}
struct Svd2 { s0:f32, s3:f32, u0:f32, u1:f32, u2:f32, u3:f32, v0:f32, v1:f32, v2:f32, v3:f32 };
fn svd2(m11:f32,m21:f32,m12:f32,m22:f32)->Svd2{
  var r:Svd2; var cu1=m11; var su1=m21;
  let norm=sqrt(cu1*cu1+su1*su1); cu1=cu1/norm; su1=su1/norm;
  let f=cu1*m11+su1*m21; let g=cu1*m12+su1*m22; let h=-su1*m12+cu1*m22;
  let d=dlasv2(f,g,h);
  r.s0=d.smax; r.s3=d.smin;
  r.u0=d.cu*cu1-d.su*su1; r.u1=d.su*cu1+d.cu*su1;
  r.u2=-d.cu*su1-d.su*cu1; r.u3=-d.su*su1+d.cu*cu1;
  r.v0=d.cv; r.v1=d.sv; r.v2=-d.sv; r.v3=d.cv;
  return r;
}
fn level_at(li:u32,x:i32,y:i32)->f32{
  let m=level_meta[li]; let w=i32(m.width); let h=i32(m.height);
  let cx=max(0,min(x,w-1)); let cy=max(0,min(y,h-1));
  return packed_gss[m.offset+u32(cy*w+cx)];
}
struct LevelPick { li:u32, step:f32 };
fn pick_level(d1in:f32,d2in:f32)->LevelPick{
  var r:LevelPick;
                                                                                
                                                                                 
                                                                            
                                                                                     
                                                                               
                         
  let d1=abs(d1in); let d2=abs(d2in);
  let factor=1.0/max(min(d1,d2),1e-12);
  let lg2=log2(SMOOTH/(factor*P.base_scale));
  var o=P.first_octave+1;
  loop{ if(o>P.last_octave){break;}
    var s=i32(floor(lg2-f32(o))); s=max(s,P.oct_first_sub); s=min(s,P.oct_last_sub);
    let sigma_=P.base_scale*exp2(f32(o)+f32(s)/f32(P.oct_res));
    if(factor*sigma_>SMOOTH){o=o-1;break;} o=o+1;
  }
  o=min(o,P.last_octave); o=max(o,P.first_octave);
  var s=i32(floor(lg2-f32(o))); s=max(s,P.oct_first_sub); s=min(s,P.oct_last_sub);
  r.li=u32(o)*P.levels_per_oct+u32(s-P.oct_first_sub);
  r.step=exp2(f32(o));
  return r;
}

@compute @workgroup_size(64,1,1)
fn main(@builtin(workgroup_id) wid:vec3<u32>,
        @builtin(local_invocation_id) lid:vec3<u32>){
                                                                      
  let kp=wid.y*65535u+wid.x; let lane=lid.x;
  if(kp>=P.count){return;}
  let base=kp*KP_STRIDE;
  let fx=bitcast<f32>(kp_in[base+0u]);
  let fy=bitcast<f32>(kp_in[base+1u]);
  let a11=bitcast<f32>(kp_in[base+2u]);                                                        
  let a12=bitcast<f32>(kp_in[base+3u]);
  let a21=bitcast<f32>(kp_in[base+4u]);
  let a22=bitcast<f32>(kp_in[base+5u]);

                                        
  for(var b=lane; b<128u; b=b+WG){ accum[b]=0.0; }
  workgroupBarrier();

                                     
  for(var sc=0u; sc<DSP_NUM; sc=sc+1u){
    let dsp_scale=P.dsp_min_scale + f32(sc)*P.dsp_scale_step;
                                                                
    let sa11=a11*dsp_scale; let sa21=a21*dsp_scale; let sa12=a12*dsp_scale; let sa22=a22*dsp_scale;
    if(lane==0u){
                                                                                  
                                                                          
                                                                                  
      let sv=svd2(sa11,sa21,sa12,sa22);
      A_sh[0]=sa11; A_sh[1]=sa21; A_sh[2]=sa12; A_sh[3]=sa22;                       
      T_sh[0]=fx; T_sh[1]=fy; dsh[0]=sv.s0; dsh[1]=sv.s3;
    }
    workgroupBarrier();

                                                   
    let pk=pick_level(dsh[0],dsh[1]);
    let A0=A_sh[0]/pk.step; let A1=A_sh[1]/pk.step;
    let A2=A_sh[2]/pk.step; let A3=A_sh[3]/pk.step;
    let t0=T_sh[0]/pk.step; let t1=T_sh[1]/pk.step;
    let stephat=EXTENT/RES;
    for(var idx=lane; idx<NPIX; idx=idx+WG){
      let yyi=i32(idx/SIDE); let xxi=i32(idx%SIDE);
      let yhat=-EXTENT+f32(yyi)*stephat; let xhat=-EXTENT+f32(xxi)*stephat;
      let rx=A2*yhat+t0; let ry=A3*yhat+t1;
      let x=A0*xhat+rx; let y=A1*xhat+ry;
      let xi=i32(floor(x)); let yi=i32(floor(y));
      let wx=x-f32(xi); let wy=y-f32(yi);
      let i00=level_at(pk.li,xi,yi); let i10=level_at(pk.li,xi+1,yi);
      let i01=level_at(pk.li,xi,yi+1); let i11=level_at(pk.li,xi+1,yi+1);
                                                            
      wpatch[idx]=f16((1.0-wy)*((1.0-wx)*i00+wx*i10)+wy*((1.0-wx)*i01+wx*i11));
    }
    workgroupBarrier();

                                                                           
                                                                                 
    let SBP = MAGNIF*KSIGMA + EPS_D;
    let Wb = i32(floor(sqrt(2.0)*SBP*f32(NBP+1)/2.0 + 0.5));
    let xc = i32(RES + 0.5);        
    let yc = i32(RES + 0.5);
                                                                    
                                                                                
                                                                                   
    var lh:array<f32,128>;
    for(var b=0u;b<128u;b=b+1u){ lh[b]=0.0; }
                                                                     
    let dlo_y = max(-Wb, -yc); let dhi_y = min(Wb, i32(SIDE)-1-yc);
    let dlo_x = max(-Wb, -xc); let dhi_x = min(Wb, i32(SIDE)-1-xc);
    let nwin = u32((dhi_y-dlo_y+1)*(dhi_x-dlo_x+1));
                                                 
    for(var t=lane; t<nwin; t=t+WG){
      let wcols = u32(dhi_x-dlo_x+1);
      let dyi = dlo_y + i32(t / wcols);
      let dxi = dlo_x + i32(t % wcols);
      let px = xc+dxi; let py = yc+dyi;
                                                                                   
                                                                          
      let c = f32(wpatch[u32(py)*SIDE + u32(px)]);
      var gx:f32; var gy:f32;
      if(px==0){gx=f32(wpatch[u32(py)*SIDE+u32(px+1)])-c;}
      else if(px==i32(SIDE)-1){gx=c-f32(wpatch[u32(py)*SIDE+u32(px-1)]);}
      else{gx=0.5*(f32(wpatch[u32(py)*SIDE+u32(px+1)])-f32(wpatch[u32(py)*SIDE+u32(px-1)]));}
      if(py==0){gy=f32(wpatch[u32(py+1)*SIDE+u32(px)])-c;}
      else if(py==i32(SIDE)-1){gy=c-f32(wpatch[u32(py-1)*SIDE+u32(px)]);}
      else{gy=0.5*(f32(wpatch[u32(py+1)*SIDE+u32(px)])-f32(wpatch[u32(py-1)*SIDE+u32(px)]));}
      let modulus=sqrt(gx*gx+gy*gy);
                                                                              
                                                                                
                                                                                  
      if (modulus <= 0.0) { continue; }
      var angle=atan2(gy,gx)+2.0*PI;
      angle=angle-2.0*PI*floor(angle/(2.0*PI));
                                                   
      let theta=angle;
                                   
      let dx=f32(xc+dxi)-RES; let dy=f32(yc+dyi)-RES;
      let nx=dx/SBP; let ny=dy/SBP;
      let nt=f32(NBO)*theta/(2.0*PI);
      let win=exp(-(nx*nx+ny*ny)/(2.0*WINSIZE*WINSIZE));
      let binx=i32(floor(nx-0.5));
      let biny=i32(floor(ny-0.5));
      let bint=i32(floor(nt));
      let rbinx=nx-(f32(binx)+0.5);
      let rbiny=ny-(f32(biny)+0.5);
      let rbint=nt-f32(bint);
      for(var dbx=0;dbx<2;dbx=dbx+1){
        for(var dby=0;dby<2;dby=dby+1){
          for(var dbt=0;dbt<2;dbt=dbt+1){
            if(binx+dbx >= -(NBP/2) && binx+dbx < (NBP/2) &&
               biny+dby >= -(NBP/2) && biny+dby < (NBP/2)){
              let weight = win*modulus*
                abs(1.0-f32(dbx)-rbinx)*abs(1.0-f32(dby)-rbiny)*abs(1.0-f32(dbt)-rbint);
              let bx = binx+dbx+(NBP/2);           
              let by = biny+dby+(NBP/2);           
              let bt = (bint+dbt)%NBO;                               
              let bi = u32(bt + by*(NBO*NBP) + bx*NBO);
              lh[bi]=lh[bi]+weight;
            }
          }
        }
      }
    }
                                                                           
                                                                                 
                                                                                
                                                                             
    for(var blk=0u; blk<128u; blk=blk+BLK){
      for(var k=0u;k<BLK;k=k+1u){ redM[lane*BLK + k] = lh[blk + k]; }
      workgroupBarrier();
      var off=WG/2u;
      loop{
        if(off==0u){break;}
        if(lane<off){
          for(var k=0u;k<BLK;k=k+1u){ redM[lane*BLK+k] = redM[lane*BLK+k] + redM[(lane+off)*BLK+k]; }
        }
        workgroupBarrier();
        off=off/2u;
      }
      if(lane==0u){ for(var k=0u;k<BLK;k=k+1u){ hist[blk+k] = redM[k]; } }
      workgroupBarrier();
    }

                                                                                   
    if(lane==0u){
      var norm:f32=0.0;
      for(var b=0u;b<128u;b=b+1u){ norm=norm+hist[b]*hist[b]; }
      norm=sqrt(norm)+EPS_F;
      for(var b=0u;b<128u;b=b+1u){ hist[b]=hist[b]/norm; }
                                                                                       
      for(var b=0u;b<128u;b=b+1u){ if(hist[b]>0.2){ hist[b]=0.2; } }
      var norm2:f32=0.0;
      for(var b=0u;b<128u;b=b+1u){ norm2=norm2+hist[b]*hist[b]; }
      norm2=sqrt(norm2)+EPS_F;
      for(var b=0u;b<128u;b=b+1u){ hist[b]=hist[b]/norm2; }
    }
    workgroupBarrier();
                                             
    for(var b=lane;b<128u;b=b+WG){ accum[b]=accum[b]+hist[b]; }
    workgroupBarrier();
  }

                                                        
  let inv=1.0/f32(DSP_NUM);
  for(var b=lane;b<128u;b=b+WG){
    raw_desc[kp*128u+b]=accum[b]*inv;
  }
}
