                                                                                   
  
                                                                                     
                                                                                 
                                                                                   
                                          
  
                                                                             
                                                                                   
                                                                                

struct Params {
  src_width  : u32,
  dst_width  : u32,
  dst_height : u32,
  src_off    : u32,
  dst_off    : u32,
  _pad0      : u32,
  _pad1      : u32,
  _pad2      : u32,
};

                                                                                         
                                                                                
                                                                                                     
                                                                                 
                                                                                                       
                                                                                          
                                                                                  
@group(0) @binding(0) var<storage, read_write> src  : array<f32>;
@group(0) @binding(1) var<storage, read_write> dst  : array<f32>;
@group(0) @binding(2) var<uniform>             P    : Params;

@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let dx : u32 = gid.x;
  let dy : u32 = gid.y;
  if (dx >= P.dst_width || dy >= P.dst_height) { return; }

  let sx : u32 = dx << 1u;
  let sy : u32 = dy << 1u;
  dst[P.dst_off + dy * P.dst_width + dx] = src[P.src_off + sy * P.src_width + sx];
}
