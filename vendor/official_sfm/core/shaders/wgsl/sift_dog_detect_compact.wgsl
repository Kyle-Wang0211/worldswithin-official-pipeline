                                                                                     
  
                                                                                
                                                                       
                                                                                
                                                          
  
                                                                            
                                                                            
                                                                                
                                                                            
                                                                    
                                         
  
                                                                             
                                                                               
                                                                   
  
                                                                               
                                                                                 
                                                                                
                                                                                  
  
                                                                         
                                                
                                                                                       
                                                 
  
                                                                                  
                                                                                   
                                                                           
                                                                                   
                                                                              
  
                                                                               
                                                                              
                                                                          
                                             
  
                                                                                   
                                                                                 
                                                                

                                                                                   
                                                      
                                                                               
                  
                  
                                                
                                                 
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
@group(0) @binding(1) var<uniform>             P          : Params;
                                                                                               
                                                                                                         
                                                                                      
@group(0) @binding(2) var<storage, read> packed_gss_hi : array<f32>;
                                                                                                     
                                                                                                   
                                                                                
@group(0) @binding(3) var<storage, read_write> cand_buf : array<u32>;                        
@group(0) @binding(4) var<storage, read_write> cand_counter : atomic<u32>;

                                                                     
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

                                                                                     
  let ci : u32 = atomicAdd(&cand_counter, 1u);
  if (ci < P.cap) {
    cand_buf[ci * 4u + 0u] = u32(x0);
    cand_buf[ci * 4u + 1u] = u32(y0);
    cand_buf[ci * 4u + 2u] = u32(zc);
    cand_buf[ci * 4u + 3u] = 0u;
  }
  return;
}
