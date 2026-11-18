                                                                                 
  
                                                                                 
                                                                                 
                                                                               
                                                                                      
                                                                     
                                                 
  
                                                           

const DSP_NUM : u32 = 6u;                                                                              

struct Params { count : u32 };

@group(0) @binding(0) var<storage, read>       scale_desc : array<f32>;                      
@group(0) @binding(1) var<storage, read_write> raw_desc   : array<f32>;              
@group(0) @binding(2) var<uniform>             P          : Params;

@compute @workgroup_size(64,1,1)
fn main(@builtin(workgroup_id) wid:vec3<u32>,
        @builtin(local_invocation_id) lid:vec3<u32>){
                                                                                           
  let idx = (wid.y*65535u + wid.x)*64u + lid.x;                  
  if (idx >= P.count * 128u) { return; }
  let kp  = idx / 128u;
  let bin = idx % 128u;
                                                                                  
  var acc : f32 = 0.0;
  for (var sc : u32 = 0u; sc < DSP_NUM; sc = sc + 1u) {
    acc = acc + scale_desc[(kp * DSP_NUM + sc) * 128u + bin];
  }
  raw_desc[idx] = acc * (1.0 / f32(DSP_NUM));
}
