                                                                                       
  
                                                                                   
                                                                               
                                                                                
  
                                                                              
                                                                         
                                                                     

struct Params {
  width   : u32,
  height  : u32,
  dst_off : u32,                                                           
  _pad    : u32,
};

@group(0) @binding(0) var<storage, read>       src_u8 : array<u32>;                    
@group(0) @binding(1) var<storage, read_write> dst    : array<f32>;
@group(0) @binding(2) var<uniform>             P      : Params;

@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let x : u32 = gid.x;
  let y : u32 = gid.y;
  if (x >= P.width || y >= P.height) { return; }

  let lin   : u32 = y * P.width + x;
  let word  : u32 = src_u8[lin >> 2u];
  let shift : u32 = (lin & 3u) * 8u;
  let byte  : u32 = (word >> shift) & 0xFFu;

  dst[P.dst_off + lin] = f32(byte) / 255.0;
}
