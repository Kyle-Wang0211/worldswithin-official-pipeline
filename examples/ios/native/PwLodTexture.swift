// Copyright 2013 The Flutter Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ^ Upstream file header, kept as-is for the parts of this file taken from
//   flutter/packages @ fbc80a62002251eaad5c714195bb5ebd22e94b14,
//   packages/camera/camera_avfoundation/darwin/camera_avfoundation/Sources/camera_avfoundation/
//     QueueUtils.swift:17-25     ensureToRunOnMainQueue (verbatim below, renamed)
//     DefaultCamera.swift:28-31  a dedicated serial queue guards the "latest buffer" handle
//     DefaultCamera.swift:1518-1529  copyPixelBuffer: take the latest under that queue,
//                                    Unmanaged.passRetained, never wait
//     CameraPlugin.swift:297-303 on a new buffer, textureFrameAvailable on the main queue
// That LICENSE file (flutter/packages/LICENSE @ fbc80a62002, BSD-3-Clause) is not part of this
// repository, so its full text is reproduced here:
//
//   Copyright 2013 The Flutter Authors
//
//   Redistribution and use in source and binary forms, with or without modification,
//   are permitted provided that the following conditions are met:
//
//       * Redistributions of source code must retain the above copyright
//         notice, this list of conditions and the following disclaimer.
//       * Redistributions in binary form must reproduce the above
//         copyright notice, this list of conditions and the following
//         disclaimer in the documentation and/or other materials provided
//         with the distribution.
//       * Neither the name of Google Inc. nor the names of its
//         contributors may be used to endorse or promote products derived
//         from this software without specific prior written permission.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
//   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  
                                                                                                                                                                                                               
                                                                                           
                                          
                                                                                                
                                                                                                
                                                                                              
                                                                                  
  
                                         
                                                    
                                                                                              
                                                                                               
                                                                                              
                                                                                
                                                      
                                                                                               
                                                                                              
                                                                                     
                                                                                                
                                                                                              
                                                                                                
                                                       
                                                                                        
                                                                                 
  
                                                                                  
                                                                                                
                                                                      
import CoreVideo
import Flutter
import Foundation

                                                                  
                                                        
                                                                      
                                                                                
               
                                                  
func pwLodEnsureToRunOnMainQueue(_ block: @escaping () -> Void) {
  if Thread.isMainThread {
    block()
  } else {
    DispatchQueue.main.async {
      block()
    }
  }
}

                                                                            
func pwLodStatusName(_ s: pwlod_status) -> String {
  switch s {
  case PWLOD_OK: return "PWLOD_OK"
  case PWLOD_ERR_ARG: return "PWLOD_ERR_ARG"
  case PWLOD_ERR_IO: return "PWLOD_ERR_IO"
  case PWLOD_ERR_FORMAT: return "PWLOD_ERR_FORMAT"
  case PWLOD_ERR_GPU: return "PWLOD_ERR_GPU"
  case PWLOD_ERR_STATE: return "PWLOD_ERR_STATE"
  case PWLOD_ERR_NOMEM: return "PWLOD_ERR_NOMEM"
  default: return "PWLOD_STATUS_\(s.rawValue)"
  }
}

                                                                                       
                                                              
func pwLodWireInt(_ v: UInt64) -> NSNumber {
  NSNumber(value: Int64(clamping: v))
}

struct PwLodError: Error {
  let code: String
  let message: String
}

                                                                                              
                                                                                      
final class PwLodFrameSink {
  let textureId: Int64
  weak var registry: FlutterTextureRegistry?
  private let lock = NSLock()
  private var ready: UInt64 = 0

  init(textureId: Int64, registry: FlutterTextureRegistry) {
    self.textureId = textureId
    self.registry = registry
  }

  var framesReady: UInt64 {
    lock.lock()
    defer { lock.unlock() }
    return ready
  }

                                                                                            
                                                                                 
  func frameReady() {
    lock.lock()
    ready &+= 1
    lock.unlock()
    let id = textureId
    pwLodEnsureToRunOnMainQueue { [weak self] in
      self?.registry?.textureFrameAvailable(id)
    }
  }
}

                                                                                     
private let pwLodFrameReady: pwlod_frame_ready_fn = { user, _, _ in
  guard let user = user else { return }
  Unmanaged<PwLodFrameSink>.fromOpaque(user).takeUnretainedValue().frameReady()
}

final class PwLodTexture: NSObject, FlutterTexture {
  let widthPx: UInt32
  let heightPx: UInt32
  let version: String
  let backend: UInt32
                                                                                          
                            
  let ioQueue: DispatchQueue

                                                                             
  private let gpu: UnsafeMutablePointer<pwlod_gpu>
  private var viewer: OpaquePointer?
  private var ring: OpaquePointer?
  private let pixelBuffers: [CVPixelBuffer]
  private var params = pwlod_params()
  private var sink: Unmanaged<PwLodFrameSink>?
  private var running = false

                                                                                           
                        
  private let pixelBufferSynchronizationQueue = DispatchQueue(
    label: "io.worldswithin.lod.pixelBufferSynchronizationQueue")

                                                                                        
  private var copyCalls: UInt64 = 0
  private var copyEmpty: UInt64 = 0
  private var lastAcquiredFrame: UInt64 = 0
  private var copyMaxNs: UInt64 = 0

  private init(
    widthPx: UInt32, heightPx: UInt32, gpu: UnsafeMutablePointer<pwlod_gpu>,
    viewer: OpaquePointer, ring: OpaquePointer, pixelBuffers: [CVPixelBuffer]
  ) {
    self.widthPx = widthPx
    self.heightPx = heightPx
    self.gpu = gpu
    self.viewer = viewer
    self.ring = ring
    self.pixelBuffers = pixelBuffers
    self.version = pwlod_version().map { String(cString: $0) } ?? ""
    self.backend = gpu.pointee.backend.rawValue
    self.ioQueue = DispatchQueue(label: "io.worldswithin.lod.texture.io", qos: .userInitiated)
    pwlod_params_default(&params)
    super.init()
  }

                                                                                               
                                                          
  static func make(
    widthPx: UInt32, heightPx: UInt32, gpu: UnsafeMutablePointer<pwlod_gpu>
  ) throws -> PwLodTexture {
    var viewer: OpaquePointer?
    var s = pwlod_viewer_create(gpu, &viewer)
    guard s == PWLOD_OK, let viewer = viewer else {
      throw PwLodError(code: pwLodStatusName(s), message: "pwlod_viewer_create")
    }
    var err = [CChar](repeating: 0, count: 256)
    guard let ring = PwLodSurfaceRingCreate(gpu.pointee.device, widthPx, heightPx, &err, UInt32(err.count)) else {
      pwlod_viewer_destroy(viewer)
      throw PwLodError(code: "PWLOD_SHELL_SURFACE", message: String(cString: err))
    }
    let count = Int(PWLOD_TARGET_COUNT)
    var buffers: [CVPixelBuffer] = []
    for i in 0..<count {
      if let pb = PwLodSurfaceRingPixelBuffer(ring, UInt32(i)) { buffers.append(pb) }
    }
    s = pwlod_viewer_set_targets(
      viewer, PwLodSurfaceRingTargets(ring), UInt32(count), PwLodSurfaceRingFormat(), widthPx, heightPx)
    guard s == PWLOD_OK, buffers.count == count else {
      pwlod_viewer_destroy(viewer)
      PwLodSurfaceRingDestroy(ring)
      throw PwLodError(code: pwLodStatusName(s), message: "pwlod_viewer_set_targets")
    }
    return PwLodTexture(
      widthPx: widthPx, heightPx: heightPx, gpu: gpu, viewer: viewer, ring: ring,
      pixelBuffers: buffers)
  }

  deinit {
    close()
  }

                           

                                                                                           
                                                                                     
  func copyPixelBuffer() -> Unmanaged<CVPixelBuffer>? {
    let t0 = DispatchTime.now().uptimeNanoseconds
    var pixelBuffer: CVPixelBuffer?
    pixelBufferSynchronizationQueue.sync {
      copyCalls &+= 1
      if let viewer = viewer {
        var index: UInt32 = 0
        var frame: UInt64 = 0
        if pwlod_viewer_acquire_latest(viewer, &index, &frame) == PWLOD_OK,
          Int(index) < pixelBuffers.count
        {
          pixelBuffer = pixelBuffers[Int(index)]
          lastAcquiredFrame = frame
        }
      }
      if pixelBuffer == nil { copyEmpty &+= 1 }
      let dt = DispatchTime.now().uptimeNanoseconds &- t0
      if dt > copyMaxNs { copyMaxNs = dt }
    }

    if let buffer = pixelBuffer {
      return Unmanaged.passRetained(buffer)
    } else {
      return nil
    }
  }

                                                 

                                                                                    
  func attach(textureId: Int64, registry: FlutterTextureRegistry) {
    if sink == nil {
      sink = Unmanaged.passRetained(PwLodFrameSink(textureId: textureId, registry: registry))
    }
  }

                                                                                                
  func pause() {
    guard running, let viewer = viewer else { return }
    _ = pwlod_viewer_stop(viewer)
    running = false
  }

  @discardableResult
  func resume() -> pwlod_status {
    guard !running, let viewer = viewer, let sink = sink else { return PWLOD_OK }
    let s = pwlod_viewer_start(viewer, pwLodFrameReady, sink.toOpaque())
    running = s == PWLOD_OK
    return s
  }

                                                                      
  func close() {
    var v: OpaquePointer?
    pixelBufferSynchronizationQueue.sync {
      v = viewer
      viewer = nil
    }
    if let v = v { pwlod_viewer_destroy(v) }                                     
    running = false
    sink?.release()
    sink = nil
    if let r = ring { PwLodSurfaceRingDestroy(r) }
    ring = nil
                                                                                   
  }

                         

              
  func loadOctree(_ dir: String) -> pwlod_status {
    guard let viewer = viewer else { return PWLOD_ERR_STATE }
    return dir.withCString { pwlod_viewer_load_octree(viewer, $0) }
  }

  func setCamera(
    viewProjRowMajor vp: [Double], eyeWorld eye: [Double], focalPx: Double,
    orbitDistance: Double, orthoMix: Double,
    viewportWidthPx: UInt32, viewportHeightPx: UInt32
  ) -> pwlod_status {
    guard let viewer = viewer else { return PWLOD_ERR_STATE }
    guard vp.count == 16, eye.count == 3 else { return PWLOD_ERR_ARG }
    var cam = pwlod_camera()
                                                                       
    withUnsafeMutableBytes(of: &cam.view_proj_row_major) { dst in
      vp.withUnsafeBytes { dst.copyMemory(from: $0) }
    }
    withUnsafeMutableBytes(of: &cam.eye_world) { dst in
      eye.withUnsafeBytes { dst.copyMemory(from: $0) }
    }
                                                                 
    cam.focal_px = focalPx
    cam.orbit_distance = orbitDistance
    cam.ortho_mix = orthoMix
    cam.viewport_width_px = viewportWidthPx
    cam.viewport_height_px = viewportHeightPx
    return pwlod_viewer_set_camera(viewer, &cam)
  }

                                                                          
                                                        
  func setParams(_ a: [String: Any]) -> pwlod_status {
    guard let viewer = viewer else { return PWLOD_ERR_STATE }
    if let v = a["point_budget"] as? NSNumber { params.point_budget = v.int64Value }
    if let v = a["target_frame_ms"] as? NSNumber { params.target_frame_ms = v.doubleValue }
    if let v = a["point_size_mode"] as? NSNumber {
      params.point_size_mode = pwlod_point_size_mode(rawValue: v.uint32Value)
    }
    if let v = a["async_loading"] as? NSNumber { params.async_loading = v.int32Value }
    if let v = a["cache_bytes"] as? NSNumber { params.cache_bytes = v.uint64Value }
    if let bg = PwLodArgs.float64s(a, "background_rgba", count: 4) {
      params.background_rgba = (Float(bg[0]), Float(bg[1]), Float(bg[2]), Float(bg[3]))
    }
    if let v = a["debug_render_sleep_ms"] as? NSNumber { params.debug_render_sleep_ms = v.int32Value }
    if let v = a["debug_publish_before_done"] as? NSNumber {
      params.debug_publish_before_done = v.int32Value
    }
    return pwlod_viewer_set_params(viewer, &params)
  }

                                                                                                
                                       
  func setStyle(_ a: [String: Any]) -> pwlod_status {
    guard let viewer = viewer else { return PWLOD_ERR_STATE }
    var st = pwlod_style()
    pwlod_style_default(&st)
    if let v = a["point_size"] as? NSNumber { st.point_size = v.floatValue }
    if let v = a["sprite_px"] as? NSNumber { st.sprite_px = v.floatValue }
    if let v = a["disc_radius_px_at_scale1"] as? NSNumber { st.disc_radius_px_at_scale1 = v.floatValue }
    if let v = a["max_sprite_scale"] as? NSNumber { st.max_sprite_scale = v.floatValue }
    if let v = a["tone"] as? NSNumber { st.tone = pwlod_tone(rawValue: v.uint32Value) }
    if let v = a["exposure"] as? NSNumber { st.exposure = v.floatValue }
    if let v = a["uncolored_min_y"] as? NSNumber { st.uncolored_min_y = v.floatValue }
    if let v = a["uncolored_inv_y_span"] as? NSNumber { st.uncolored_inv_y_span = v.floatValue }
    if let v = a["selection_mode"] as? NSNumber {
      st.selection_mode = pwlod_selection_mode(rawValue: v.uint32Value)
    }
    if let c = PwLodArgs.float64s(a, "selection_center", count: 3) {
      st.selection_center = (c[0], c[1], c[2])
    }
    if let z = PwLodArgs.float64s(a, "selection_size", count: 3) {
      st.selection_size = (z[0], z[1], z[2])
    }
    if let r = PwLodArgs.float64s(a, "selection_rot_row_major", count: 9) {
      st.selection_rot_row_major = (r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8])
    }
    if let v = a["selection_out_argb"] as? NSNumber { st.selection_out_argb = v.uint32Value }
    return pwlod_viewer_set_style(viewer, &st)
  }

                                                                                              
                                                                                              
                                                              
  func setPoints(xyz: Data, rgb: Data, count: UInt64, colored: Bool, visibility: Data?) -> pwlod_status {
    guard let viewer = viewer else { return PWLOD_ERR_STATE }
    guard xyz.count == Int(count) * 12, rgb.count >= Int(count) * 3 else { return PWLOD_ERR_ARG }
    if let v = visibility, v.count != Int(count) { return PWLOD_ERR_ARG }
    return xyz.withUnsafeBytes { xp in
      rgb.withUnsafeBytes { rp in
        let x = xp.bindMemory(to: Float.self).baseAddress
        let r = rp.bindMemory(to: UInt8.self).baseAddress
        if let vis = visibility {
          return vis.withUnsafeBytes { vp in
            pwlod_viewer_set_points(
              viewer, x, r, count, colored ? 1 : 0, vp.bindMemory(to: UInt8.self).baseAddress)
          }
        }
        return pwlod_viewer_set_points(viewer, x, r, count, colored ? 1 : 0, nil)
      }
    }
  }

                                                                                              
                      
  func stats() -> [String: Any]? {
    guard let viewer = viewer else { return nil }
    var st = pwlod_frame_stats()
    guard pwlod_viewer_get_stats(viewer, &st) == PWLOD_OK else { return nil }
    var shell: [String: Any] = [:]
    pixelBufferSynchronizationQueue.sync {
      shell = [
        "copy_calls": pwLodWireInt(copyCalls),
        "copy_empty": pwLodWireInt(copyEmpty),
        "last_acquired_frame_number": pwLodWireInt(lastAcquiredFrame),
        "copy_max_us": Double(copyMaxNs) / 1000.0,
      ]
    }
    shell["frames_ready"] = pwLodWireInt(sink?.takeUnretainedValue().framesReady ?? 0)
    shell["running"] = running
    return [
      "frame_number": pwLodWireInt(st.frame_number),
      "completed_frame_number": pwLodWireInt(st.completed_frame_number),
      "points_drawn": NSNumber(value: st.points_drawn),
      "nodes_drawn": NSNumber(value: st.nodes_drawn),
      "nodes_loading": NSNumber(value: st.nodes_loading),
      "uploads_this_frame": NSNumber(value: st.uploads_this_frame),
      "dropped_for_cache": NSNumber(value: st.dropped_for_cache),
      "min_node_pixel_size": st.min_node_pixel_size,
      "cpu_ms": st.cpu_ms,
      "gpu_ms": st.gpu_ms,
      "lowest_spacing": st.lowest_spacing,                                    
      "source": NSNumber(value: st.source),                                                  
      "shell": shell,
    ]
  }
}

                                                                                        
                                              
enum PwLodArgs {
  static func float64s(_ a: [String: Any], _ key: String, count: Int) -> [Double]? {
    guard let td = a[key] as? FlutterStandardTypedData, td.type == .float64,
      Int(td.elementCount) == count
    else { return nil }
    return td.data.withUnsafeBytes { Array($0.bindMemory(to: Double.self)) }
  }

                                                                                         
                                                             
  static func typedData(
    _ a: [String: Any], _ key: String, _ type: FlutterStandardDataType
  ) -> Data? {
    guard let td = a[key] as? FlutterStandardTypedData, td.type == type else { return nil }
    return td.data
  }

  static func string(_ a: [String: Any], _ key: String) -> String? {
    guard let s = a[key] as? String, !s.isEmpty else { return nil }
    return s
  }

  static func int(_ a: [String: Any], _ key: String) -> Int? {
    (a[key] as? NSNumber)?.intValue
  }

  static func double(_ a: [String: Any], _ key: String) -> Double? {
    (a[key] as? NSNumber)?.doubleValue
  }
}
