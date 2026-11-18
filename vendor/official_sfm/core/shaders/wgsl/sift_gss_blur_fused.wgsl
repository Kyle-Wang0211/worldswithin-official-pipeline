                                                                                    
                                                                                      
                                                                                     
                                                                                                 
                                                                                 
                                                                                 
                                                                             
                                                                     

struct Params {
  width   : u32,
  height  : u32,
  radius  : u32,
  src_off : u32,
  dst_off : u32,
  _pad0   : u32,
  _pad1   : u32,
  _pad2   : u32,
};

@group(0) @binding(0) var<storage, read_write> data : array<f32>;
@group(0) @binding(1) var<storage, read>       taps : array<f32>;
@group(0) @binding(2) var<uniform>             P    : Params;

const BR  : u32 = 32u;                                                             
const RS  : u32 = 64u;                                                                                                            
const SEG : u32 = 40u;                                                   
var<workgroup> ring : array<f32, 512>;                   
var<workgroup> seg  : array<f32, 1280>;                                              
var<workgroup> tp   : array<f32, 33>;                     

fn clampi(v : i32, lo : i32, hi : i32) -> i32 {
  return max(lo, min(v, hi));
}

                                                                                 
                                                             
fn stage_rows(row0 : i32, n : i32, x0 : i32, tx : u32, ty : u32) {
  let w   : i32 = i32(P.width);
  let r   : i32 = i32(P.radius);
  let cols : i32 = 8 + 2 * r;
  if (i32(ty) < n) {
    let base : i32 = (row0 + i32(ty)) * w;
    for (var j : i32 = i32(tx); j < cols; j = j + 8) {
      let sx : i32 = clampi(x0 + j - r, 0, w - 1);
      seg[ty * SEG + u32(j)] = data[P.src_off + u32(base + sx)];
    }
  }
}

                                                                                             
fn h_blur_seg(tx : u32, ty : u32) -> f32 {
  let r   : i32 = i32(P.radius);
  let len : i32 = 2 * r + 1;
  let sb  : u32 = ty * SEG + tx;
  var acc : f32 = 0.0;
  for (var k : i32 = 0; k < len; k = k + 1) {
    acc = acc + tp[k] * seg[sb + u32(k)];
  }
  return acc;
}

@compute @workgroup_size(256, 1, 1)
fn main(@builtin(workgroup_id) wid : vec3<u32>,
        @builtin(local_invocation_id) lid : vec3<u32>) {
  let tx : u32 = lid.x % 8u;
  let ty : u32 = lid.x / 8u;
  let x0 : i32 = i32(wid.x * 8u);
  let x  : u32 = wid.x * 8u + tx;
  let w  : u32 = P.width;
  let h  : i32 = i32(P.height);
  let r  : i32 = i32(P.radius);
  let len : i32 = 2 * r + 1;
  if (i32(lid.x) < len) { tp[lid.x] = taps[lid.x]; }

                                                                                   
  let pre : i32 = min(i32(BR) + r, h);
  var row0 : i32 = 0;
  loop {
    if (row0 >= pre) { break; }
    let n : i32 = min(i32(BR), pre - row0);
    stage_rows(row0, n, x0, tx, ty);
    workgroupBarrier();
    if (i32(ty) < n && x < w) {
      ring[(u32(row0 + i32(ty)) & (RS - 1u)) * 8u + tx] = h_blur_seg(tx, ty);
    }
    workgroupBarrier();
    row0 = row0 + i32(BR);
  }
  var loaded : i32 = pre;

  var yb : i32 = 0;
  loop {
    if (yb >= h) { break; }
    workgroupBarrier();                                          

                                                                                     
    let y : i32 = yb + i32(ty);
    if (y < h && x < w) {
      var acc : f32 = 0.0;
      for (var k : i32 = 0; k < len; k = k + 1) {
        let sy : i32 = clampi(y + (k - r), 0, h - 1);
        acc = acc + tp[k] * ring[(u32(sy) & (RS - 1u)) * 8u + tx];
      }
      data[P.dst_off + u32(y) * w + x] = acc;
    }
    workgroupBarrier();                                                  

                                                                                        
    let need : i32 = min(h, yb + 2 * i32(BR) + r);
    let n : i32 = need - loaded;
    if (n > 0) {
      stage_rows(loaded, n, x0, tx, ty);
      workgroupBarrier();
      if (i32(ty) < n && x < w) {
        ring[(u32(loaded + i32(ty)) & (RS - 1u)) * 8u + tx] = h_blur_seg(tx, ty);
      }
      loaded = need;
    }
    yb = yb + i32(BR);
  }
}
