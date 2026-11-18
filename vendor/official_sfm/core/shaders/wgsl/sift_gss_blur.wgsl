                                                                                 
  
                                                                             
                                                                                
                                                                               
                                                                             
                                                                                 
                                                                               
  
                                                                              
                                                                            
                                                                           
  
                                                                    

struct Params {
  width  : u32,
  height : u32,
  radius : u32,                                                   
  axis   : u32,                                  
                                                                                       
  src_off : u32,
  dst_off : u32,
};

                                                                                            
                                                                                                    
                                                                               
@group(0) @binding(0) var<storage, read_write> data  : array<f32>;
@group(0) @binding(1) var<storage, read>       taps  : array<f32>;                            
@group(0) @binding(2) var<uniform>             P     : Params;

fn clampi(v : i32, lo : i32, hi : i32) -> i32 {
  return max(lo, min(v, hi));
}

@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let x : u32 = gid.x;
  let y : u32 = gid.y;
  if (x >= P.width || y >= P.height) { return; }

  let r   : i32 = i32(P.radius);
  let len : i32 = 2 * r + 1;
  var acc : f32 = 0.0;

  if (P.axis == 0u) {
                                                        
    let base : i32 = i32(y) * i32(P.width);
    let cx   : i32 = i32(x);
    for (var k : i32 = 0; k < len; k = k + 1) {
      let sx : i32 = clampi(cx + (k - r), 0, i32(P.width) - 1);
      acc = acc + taps[k] * data[P.src_off + u32(base + sx)];
    }
  } else {
                                                       
    let cy : i32 = i32(y);
    for (var k : i32 = 0; k < len; k = k + 1) {
      let sy : i32 = clampi(cy + (k - r), 0, i32(P.height) - 1);
      acc = acc + taps[k] * data[P.src_off + u32(sy * i32(P.width) + i32(x))];
    }
  }

  data[P.dst_off + y * P.width + x] = acc;
}
