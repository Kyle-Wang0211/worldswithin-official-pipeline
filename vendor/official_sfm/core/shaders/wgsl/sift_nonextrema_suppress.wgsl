                                                                                
  
                                                                                
                                                                                    
                                                                               
                                                                            
                                                  
                                                                                    
                                                                                
                                                                        
  
                                                                              
                                                                               
                                                                                 
                                                                                
                                                                              
                                                                                 
  
                                                                            
                                                                                 
                                                                            
                                                    
  
                                                                 
                                                                          
const KP_STRIDE : u32 = 8u;

struct Params {
  count : u32,                                      
  tol   : f32,                                         
};

@group(0) @binding(0) var<storage, read>       kp_buffer : array<u32>;                      
@group(0) @binding(1) var<storage, read_write> keep      : array<u32>;                
@group(0) @binding(2) var<uniform>             P         : Params;

fn kp_x(i : u32)     -> f32 { return bitcast<f32>(kp_buffer[i * KP_STRIDE + 0u]); }
fn kp_y(i : u32)     -> f32 { return bitcast<f32>(kp_buffer[i * KP_STRIDE + 1u]); }
fn kp_sigma(i : u32) -> f32 { return bitcast<f32>(kp_buffer[i * KP_STRIDE + 2u]); }
fn kp_score(i : u32) -> f32 { return bitcast<f32>(kp_buffer[i * KP_STRIDE + 3u]); }

@compute @workgroup_size(64, 1, 1)
fn main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let j : u32 = gid.x;
  if (j >= P.count) { return; }

  let xj : f32 = kp_x(j);
  let yj : f32 = kp_y(j);
  let sj : f32 = kp_sigma(j);
  let scj : f32 = abs(kp_score(j));
  let onePlusTol : f32 = 1.0 + P.tol;

  var suppressed : bool = false;
  for (var i : u32 = 0u; i < P.count; i = i + 1u) {
    if (i == j) { continue; }
    let si : f32 = kp_sigma(i);
    let sci : f32 = abs(kp_score(i));
                                                                               
                                                               
                                                                                 
                              
    if (sj < onePlusTol * si && si < onePlusTol * sj &&
        abs(kp_x(i) - xj) < P.tol * si && abs(kp_y(i) - yj) < P.tol * si &&
        sci > scj) {
      suppressed = true;
      break;
    }
  }
  keep[j] = select(1u, 0u, suppressed);
}
