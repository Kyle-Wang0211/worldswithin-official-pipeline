                                                                                     
  
                                                                                
                                                                       
                                                                                
                                                          
  
                                                                            
                                                                            
                                                                                
                                                                            
                                                                    
                                         
  
                                                                             
                                                                               
                                                                   
  
                                                                               
                                                                                 
                                                                                
                                                                                  
  
                                                                         
                                                
                                                                                       
                                                 
  
                                                                                  
                                                                                   
                                                                           
                                                                                   
                                                                              
  
                                                                               
                                                                              
                                                                          
                                             
  
                                                                                   
                                                                                 
                                                                

                                                                                   
                                                      
                                                                               
                  
                  
                                                
                                                 
const KP_STRIDE : u32 = 8u;                                

struct Params {
  width   : u32,                                                      
  height  : u32,                              
  cap     : u32,                                  
  octave  : u32,                       
  peak_threshold : f32,                                                        
  edge_threshold : f32,                                      
  base_scale     : f32,                   
  oct_resolution : f32,                              
                                                                                        
  off0 : u32,
  off1 : u32,
  off2 : u32,
  off3 : u32,
  off4 : u32,
  off5 : u32,
  z0 : u32,                                                                                    
  _pad1 : u32,
};

                                                            
                                                                                      
                                                                                         
@group(0) @binding(0) var<storage, read> packed_gss : array<f32>;
@group(0) @binding(1) var<storage, read_write> kp_counter : atomic<u32>;
@group(0) @binding(2) var<storage, read_write> kp_buffer  : array<u32>;                   
@group(0) @binding(3) var<uniform>             P          : Params;
                                                                                               
                                                                                                         
                                                                                      
@group(0) @binding(4) var<storage, read> packed_gss_hi : array<f32>;

                                                                     
fn gss_at(level_idx : i32, x : i32, y : i32) -> f32 {
  let w : i32 = i32(P.width);
  let h : i32 = i32(P.height);
  let cx : i32 = max(0, min(x, w - 1));
  let cy : i32 = max(0, min(y, h - 1));
  let k : i32 = cy * w + cx;
  switch (level_idx) {
    case 0: { return packed_gss[P.off0 + u32(k)]; }
    case 1: { return packed_gss[P.off1 + u32(k)]; }
    case 2: { return packed_gss[P.off2 + u32(k)]; }
    case 3: { return packed_gss_hi[P.off3 + u32(k)]; }
    case 4: { return packed_gss_hi[P.off4 + u32(k)]; }
    default: { return packed_gss_hi[P.off5 + u32(k)]; }
  }
}

                                                                               
                                                                               
                                                
fn dog_at(zc : i32, x : i32, y : i32) -> f32 {
  return gss_at(zc, x, y) - gss_at(zc + 1, x, y);
}

                                                                            
                                                                               
                                                          
fn solve3(Ain : array<f32, 9>, bin : array<f32, 3>, out_x : ptr<function, array<f32,3>>) -> bool {
                                                                             
  var M : array<f32, 12>;
                                                                              
  M[0] = Ain[0]; M[1] = Ain[1]; M[2] = Ain[2];
  M[3] = Ain[3]; M[4] = Ain[4]; M[5] = Ain[5];
  M[6] = Ain[6]; M[7] = Ain[7]; M[8] = Ain[8];
  M[9] = bin[0]; M[10] = bin[1]; M[11] = bin[2];

  let numRows : i32 = 3;
  let numCols : i32 = 4;
  for (var j : i32 = 0; j < numRows; j = j + 1) {
    var maxa : f32 = 0.0;
    var maxabsa : f32 = 0.0;
    var maxi : i32 = -1;
    for (var i : i32 = j; i < numRows; i = i + 1) {
      let a : f32 = M[i + j * numRows];
      let absa : f32 = abs(a);
      if (absa > maxabsa) { maxa = a; maxabsa = absa; maxi = i; }
    }
    if (maxabsa < 1e-10) { return false; }
    let ip : i32 = maxi;
                                                                            
    for (var jj : i32 = j; jj < numCols; jj = jj + 1) {
      let tmp : f32 = M[ip + jj * numRows];
      M[ip + jj * numRows] = M[j + jj * numRows];
      M[j + jj * numRows] = tmp;
      M[j + jj * numRows] = M[j + jj * numRows] / maxa;
    }
                                 
    for (var ii : i32 = j + 1; ii < numRows; ii = ii + 1) {
      let x : f32 = M[ii + j * numRows];
      for (var jj : i32 = j; jj < numCols; jj = jj + 1) {
        M[ii + jj * numRows] = M[ii + jj * numRows] - x * M[j + jj * numRows];
      }
    }
  }
                                                                      
  for (var i : i32 = numRows - 1; i > 0; i = i - 1) {
    for (var ii : i32 = i - 1; ii >= 0; ii = ii - 1) {
      let x : f32 = M[ii + i * numRows];
      for (var jc : i32 = numRows; jc < numCols; jc = jc + 1) {
        M[ii + jc * numRows] = M[ii + jc * numRows] - x * M[i + jc * numRows];
      }
    }
  }
  (*out_x)[0] = M[9];
  (*out_x)[1] = M[10];
  (*out_x)[2] = M[11];
  return true;
}

@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let gx : u32 = gid.x;
  let gy : u32 = gid.y;
  let zc : i32 = i32(gid.z) + i32(P.z0) + 1;                                                            
  let W : i32 = i32(P.width);
  let H : i32 = i32(P.height);

                                                                          
  if (i32(gid.z) + i32(P.z0) > 2) { return; }
                                                  
  if (gx < 1u || gy < 1u || gx >= u32(W - 1) || gy >= u32(H - 1)) { return; }

  let x0 : i32 = i32(gx);
  let y0 : i32 = i32(gy);
  let v : f32 = dog_at(zc, x0, y0);

                                                                                   
                                                                                     
  let t08 : f32 = 0.8 * P.peak_threshold;
  var is_max : bool = (v >= t08);
  var is_min : bool = (v <= -t08);
  if (is_max || is_min) {
                                                                                           
                                                                                          
    for (var kz : i32 = 0; kz <= 2; kz = kz + 1) {
      let dz : i32 = select(select(1, -1, kz == 1), 0, kz == 0);
      for (var dy : i32 = -1; dy <= 1; dy = dy + 1) {
        for (var dx : i32 = -1; dx <= 1; dx = dx + 1) {
          if (dx == 0 && dy == 0 && dz == 0) { continue; }
          let nv : f32 = dog_at(zc + dz, x0 + dx, y0 + dy);
          if (!(v > nv)) { is_max = false; }
          if (!(v < nv)) { is_min = false; }
                                                                                                                                       
          if (!(is_max || is_min)) { break; }
        }
        if (!(is_max || is_min)) { break; }
      }
      if (!(is_max || is_min)) { break; }
    }
  }
  if (!(is_max || is_min)) { return; }

                                                                  
  var x : i32 = x0;
  var y : i32 = y0;
  let z : i32 = zc;                                  
  var dxc : i32 = 0;
  var dyc : i32 = 0;
  var b : array<f32, 3>;
  b[0] = 0.0; b[1] = 0.0; b[2] = 0.0;
  var Dx : f32 = 0.0; var Dy : f32 = 0.0; var Dz : f32 = 0.0;
  var ok_solve : bool = true;

  for (var iter : i32 = 0; iter < 5; iter = iter + 1) {
    x = x + dxc;
    y = y + dyc;
                                                      
    let c   : f32 = dog_at(z,   x,   y);
    let xp  : f32 = dog_at(z,   x+1, y);
    let xm  : f32 = dog_at(z,   x-1, y);
    let yp  : f32 = dog_at(z,   x,   y+1);
    let ym  : f32 = dog_at(z,   x,   y-1);
    let zp  : f32 = dog_at(z+1, x,   y);
    let zm  : f32 = dog_at(z-1, x,   y);
    let xyp : f32 = dog_at(z,   x+1, y+1);
    let xym : f32 = dog_at(z,   x-1, y-1);
    let xpym: f32 = dog_at(z,   x+1, y-1);
    let xmyp: f32 = dog_at(z,   x-1, y+1);
    let xzp : f32 = dog_at(z+1, x+1, y);
    let xzm : f32 = dog_at(z-1, x-1, y);
    let xpzm: f32 = dog_at(z-1, x+1, y);
    let xmzp: f32 = dog_at(z+1, x-1, y);
    let yzp : f32 = dog_at(z+1, x,   y+1);
    let yzm : f32 = dog_at(z-1, x,   y-1);
    let ypzm: f32 = dog_at(z-1, x,   y+1);
    let ymzp: f32 = dog_at(z+1, x,   y-1);

    Dx = 0.5 * (xp - xm);
    Dy = 0.5 * (yp - ym);
    Dz = 0.5 * (zp - zm);
    let Dxx : f32 = xp + xm - 2.0 * c;
    let Dyy : f32 = yp + ym - 2.0 * c;
    let Dzz : f32 = zp + zm - 2.0 * c;
    let Dxy : f32 = 0.25 * (xyp + xym - xmyp - xpym);
    let Dxz : f32 = 0.25 * (xzp + xzm - xmzp - xpzm);
    let Dyz : f32 = 0.25 * (yzp + yzm - ypzm - ymzp);

                                    
    var A : array<f32, 9>;
    A[0] = Dxx; A[4] = Dyy; A[8] = Dzz;
    A[1] = Dxy; A[3] = Dxy;                           
    A[2] = Dxz; A[6] = Dxz;                           
    A[5] = Dyz; A[7] = Dyz;                           
    var rhs : array<f32, 3>;
    rhs[0] = -Dx; rhs[1] = -Dy; rhs[2] = -Dz;

    var sol : array<f32, 3>;
    ok_solve = solve3(A, rhs, &sol);
    if (!ok_solve) {
      b[0] = 0.0; b[1] = 0.0; b[2] = 0.0;
      break;
    }
    b[0] = sol[0]; b[1] = sol[1]; b[2] = sol[2];

                                                                   
    dxc = 0; dyc = 0;
    if (b[0] > 0.6 && x < W - 2) { dxc = 1; }
    if (b[0] < -0.6 && x > 1) { dxc = -1; }
    if (b[1] > 0.6 && y < H - 2) { dyc = dyc + 1; }
    if (b[1] < -0.6 && y > 1) { dyc = dyc - 1; }
    if (dxc == 0 && dyc == 0) { break; }
  }

                                                                    
                                                                                 
                                                                                 
                                                                                 
                                                                                  
  let c2   : f32 = dog_at(z,   x,   y);
  let xp2  : f32 = dog_at(z,   x+1, y);
  let xm2  : f32 = dog_at(z,   x-1, y);
  let yp2  : f32 = dog_at(z,   x,   y+1);
  let ym2  : f32 = dog_at(z,   x,   y-1);
  let xyp2 : f32 = dog_at(z,   x+1, y+1);
  let xym2 : f32 = dog_at(z,   x-1, y-1);
  let xpym2: f32 = dog_at(z,   x+1, y-1);
  let xmyp2: f32 = dog_at(z,   x-1, y+1);
  let Dxx2 : f32 = xp2 + xm2 - 2.0 * c2;
  let Dyy2 : f32 = yp2 + ym2 - 2.0 * c2;
  let Dxy2 : f32 = 0.25 * (xyp2 + xym2 - xmyp2 - xpym2);

  let peakScore : f32 = c2 + 0.5 * (Dx * b[0] + Dy * b[1] + Dz * b[2]);
  let denom : f32 = Dxx2 * Dyy2 - Dxy2 * Dxy2;
  let alpha : f32 = (Dxx2 + Dyy2) * (Dxx2 + Dyy2) / denom;
  var edgeScore : f32;
  if (alpha < 0.0) {
    edgeScore = 3.4e38;                                                  
  } else {
    edgeScore = (0.5 * alpha - 1.0) + sqrt(max(0.25 * alpha - 1.0, 0.0) * alpha);
  }

  let rx : f32 = f32(x) + b[0];
  let ry : f32 = f32(y) + b[1];
  let rz : f32 = f32(z) + b[2];
  let depth : i32 = 5;

                                                                            
  let stable : bool =
      ok_solve &&
      abs(b[0]) < 1.5 && abs(b[1]) < 1.5 && abs(b[2]) < 1.5 &&
      rx >= 0.0 && rx <= f32(W - 1) &&
      ry >= 0.0 && ry <= f32(H - 1) &&
      rz >= 0.0 && rz <= f32(depth - 1);

                                                                                    
  let keep : bool = stable &&
      abs(peakScore) > P.peak_threshold &&
      edgeScore < P.edge_threshold;
  if (!keep) { return; }

                             
                                                                        
  let step : f32 = exp2(f32(P.octave));
  let fx : f32 = rx * step;
  let fy : f32 = ry * step;
                                                                             
                                                             
  let s_round : i32 = i32(round(rz));
  let sigma : f32 = P.base_scale *
      exp2(f32(P.octave) + (f32(s_round) + (-1.0)) / P.oct_resolution);

  let slot : u32 = atomicAdd(&kp_counter, 1u);
  if (slot < P.cap) {
    let base : u32 = slot * KP_STRIDE;
    kp_buffer[base + 0u] = bitcast<u32>(fx);
    kp_buffer[base + 1u] = bitcast<u32>(fy);
    kp_buffer[base + 2u] = bitcast<u32>(sigma);
    kp_buffer[base + 3u] = bitcast<u32>(peakScore);
    kp_buffer[base + 4u] = bitcast<u32>(edgeScore);
    kp_buffer[base + 5u] = bitcast<u32>(i32(P.octave));
    kp_buffer[base + 6u] = bitcast<u32>(s_round);
    kp_buffer[base + 7u] = 0u;
  }
}
