import UIKit
import Flutter
import Darwin
import os

                                                                                                                           
  
                                                                         
                                                                     
                                                                      
                                                
  
                                                                       
                                                   
                                               
                                             
                                                                 
                                                                          
                                                                        
                                                          
                                                                       
                                                                            
                                              
  
                                                                         
                
                                      
                                          
                                                        
                                                               
  
                                                                         
                                                                      
                                            

class AetherTexturePlugin: NSObject, FlutterPlugin {
    private let textures: FlutterTextureRegistry
                                                                        
                                             
    private var registered: [Int64: SharedNativeTexture] = [:]

                                                                      
                                                                              
      
                                                                      
                                                                         
                                                                     
                                                                     
                                                              
                                                                     
                 
    private var displayLink: CADisplayLink?
    private var animationStart: CFTimeInterval = 0
    private var animatedTextureId: Int64?
    private var animatedTexture: SharedNativeTexture?
    private var frameCount: Int = 0
    private var frameStatsLogTime: CFTimeInterval = 0

                                        
      
                                                               
                                              
                                                                            
                                                      
                                                                         
                                                                      
    private var targetFps: Int = 60
                                                                            
                                                                               
                                                                         
                         
    private var isPaused: Bool = false
                                                                             
                                                                            
                                         
    private var warningChannel: FlutterMethodChannel?

    static func register(with registrar: FlutterPluginRegistrar) {
                                                                      
                                               
        let channel = FlutterMethodChannel(
            name: "aether_texture",
            binaryMessenger: registrar.messenger()
        )
                                                                          
                                                                              
        let warningChannel = FlutterMethodChannel(
            name: "aether_texture/warning",
            binaryMessenger: registrar.messenger()
        )
        let instance = AetherTexturePlugin(
            textures: registrar.textures(),
            warningChannel: warningChannel
        )
        registrar.addMethodCallDelegate(instance, channel: channel)
    }

    init(textures: FlutterTextureRegistry,
         warningChannel: FlutterMethodChannel? = nil) {
        self.textures = textures
        self.warningChannel = warningChannel
        super.init()

                                                                           
                                                                         
                                                                        
                                                  
        let nc = NotificationCenter.default
        nc.addObserver(self, selector: #selector(handleBackground),
                       name: UIApplication.didEnterBackgroundNotification, object: nil)
        nc.addObserver(self, selector: #selector(handleForeground),
                       name: UIApplication.willEnterForegroundNotification, object: nil)
        nc.addObserver(self, selector: #selector(handleMemoryWarning),
                       name: UIApplication.didReceiveMemoryWarningNotification, object: nil)
        nc.addObserver(self, selector: #selector(handleThermalChange),
                       name: ProcessInfo.thermalStateDidChangeNotification, object: nil)
                                                                                  
                                                       
        applyThermalPolicy(ProcessInfo.processInfo.thermalState, sourceLog: "init")
    }

    deinit {
                                                                            
                                                                          
        NotificationCenter.default.removeObserver(self)
    }

                                                                        
                                                                           
                                                                       
                                                                   
                                                                        
                                                                    
                                               
    private func parseTextureDimension(_ raw: Any?, default fallback: Int) -> Int {
        let n: Int?
        switch raw {
        case let v as Int:    n = v
        case let v as Int32:  n = Int(v)
        case let v as Int64:  n = Int(v)
        case let v as NSNumber: n = v.intValue
        default: n = nil
        }
        guard let v = n, v > 0, v <= 4096 else { return fallback }
        return v
    }

    func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        switch call.method {

        case "createSharedNativeTexture":
                                                                           
                                                                        
                                                                         
                                                                   
            let args = call.arguments as? [String: Any] ?? [:]
            let width  = parseTextureDimension(args["width"],  default: 256)
            let height = parseTextureDimension(args["height"], default: 256)
            do {
                let texture = try SharedNativeTexture(width: width, height: height)
                let id = textures.register(texture)
                registered[id] = texture
                                                                      
                                                                       
                                                                          
                                                                          
                                                                       
                                                                        
                                           
                let isBackgrounded = UIApplication.shared.applicationState
                    != .active
                if !isPaused && !isBackgrounded {
                    texture.render()
                    textures.textureFrameAvailable(id)
                }
                startAnimation(textureId: id, texture: texture)
                Self.logMemoryFootprint("register id=\(id) total=\(registered.count)")
                result(NSNumber(value: id))
            } catch TextureCreateError.iosurfaceCreate {
                result(FlutterError(
                    code: "IOSURFACE_FAILED",
                    message: "IOSurface(properties:) returned nil",
                    details: nil))
            } catch TextureCreateError.cvpixelbufferCreate(let cvret) {
                result(FlutterError(
                    code: "CVPIXELBUFFER_FAILED",
                    message: "CVPixelBufferCreateWithIOSurface returned CVReturn=\(cvret)",
                    details: nil))
            } catch TextureCreateError.rendererCreate {
                result(FlutterError(
                    code: "RENDERER_FAILED",
                    message: "aether_scene_renderer_create returned NULL — see Xcode Console for [Aether3D][scene_renderer] diagnostic",
                    details: nil))
            } catch {
                result(FlutterError(
                    code: "UNKNOWN_TEXTURE_ERROR",
                    message: "\(error)",
                    details: nil))
            }

        case "disposeTexture":
            guard let args = call.arguments as? [String: Any],
                  let id = (args["textureId"] as? NSNumber)?.int64Value else {
                result(FlutterError(
                    code: "BAD_ARGS",
                    message: "disposeTexture requires {textureId: int}",
                    details: nil))
                return
            }
            disposeTexture(id: id)
            result(nil)

        case "loadGlb":
            guard let args = call.arguments as? [String: Any],
                  let id = (args["textureId"] as? NSNumber)?.int64Value,
                  let path = args["path"] as? String else {
                result(FlutterError(
                    code: "BAD_ARGS",
                    message: "loadGlb requires {textureId: int, path: String}",
                    details: nil))
                return
            }
            guard let texture = registered[id] else {
                result(FlutterError(
                    code: "NO_SUCH_TEXTURE",
                    message: "loadGlb called with textureId=\(id) which is not registered",
                    details: nil))
                return
            }
            guard texture.loadGlb(path: path) else {
                result(FlutterError(
                    code: "GLB_LOAD_FAILED",
                    message: "aether_scene_renderer_load_glb returned false for path=\(path)",
                    details: nil))
                return
            }
                                                                         
                                                                        
                                                                            
            let bounds = texture.getBounds()
            if let b = bounds {
                NSLog("[AetherTexture iOS] loadGlb succeeded: %@ bounds=([%.2f..%.2f],[%.2f..%.2f],[%.2f..%.2f])",
                      path, b.minX, b.maxX, b.minY, b.maxY, b.minZ, b.maxZ)
                result([
                    "bounds": [
                        "minX": Double(b.minX),
                        "minY": Double(b.minY),
                        "minZ": Double(b.minZ),
                        "maxX": Double(b.maxX),
                        "maxY": Double(b.maxY),
                        "maxZ": Double(b.maxZ),
                    ]
                ])
            } else {
                NSLog("[AetherTexture iOS] loadGlb succeeded: %@ (no bounds)", path)
                result([:])
            }

                                                                     
                                                                    
                                                                    
                                                                       
                                                        
        case "loadPly":
            guard let args = call.arguments as? [String: Any],
                  let id = (args["textureId"] as? NSNumber)?.int64Value,
                  let path = args["path"] as? String else {
                result(FlutterError(
                    code: "BAD_ARGS",
                    message: "loadPly requires {textureId: int, path: String}",
                    details: nil))
                return
            }
            guard let texture = registered[id] else {
                result(FlutterError(
                    code: "NO_SUCH_TEXTURE",
                    message: "loadPly called with textureId=\(id) which is not registered",
                    details: nil))
                return
            }
                                                                    
            let maxSplats = (args["maxSplats"] as? NSNumber)?.uint32Value ?? 0
            let maxShDegree = (args["maxShDegree"] as? NSNumber)?.uint8Value ?? 3
                                                                          
            let splatScale = (args["splatScaleMultiplier"] as? NSNumber)?.floatValue ?? 1.0
            texture.setSplatScaleMultiplier(splatScale)
                                                                         
            let max3dScale = (args["max3dScale"] as? NSNumber)?.floatValue ?? 0.0
            texture.setMax3dScale(max3dScale)
            if !texture.loadPly(path: path,
                                maxSplats: maxSplats,
                                maxShDegree: maxShDegree) {
                result(FlutterError(
                    code: "PLY_LOAD_FAILED",
                    message: "aether_scene_renderer_load_ply returned false for path=\(path)",
                    details: nil))
                return
            }
                                                                    
                                                             
            let plyBounds = texture.getBounds()
            if let b = plyBounds {
                NSLog("[AetherTexture iOS] loadPly succeeded: %@ bounds=([%.2f..%.2f],[%.2f..%.2f],[%.2f..%.2f])",
                      path, b.minX, b.maxX, b.minY, b.maxY, b.minZ, b.maxZ)
                result([
                    "bounds": [
                        "minX": Double(b.minX),
                        "minY": Double(b.minY),
                        "minZ": Double(b.minZ),
                        "maxX": Double(b.maxX),
                        "maxY": Double(b.maxY),
                        "maxZ": Double(b.maxZ),
                    ]
                ])
            } else {
                NSLog("[AetherTexture iOS] loadPly succeeded: %@ (no bounds)", path)
                result([:])
            }

        case "loadSpz":
            guard let args = call.arguments as? [String: Any],
                  let id = (args["textureId"] as? NSNumber)?.int64Value,
                  let path = args["path"] as? String else {
                result(FlutterError(
                    code: "BAD_ARGS",
                    message: "loadSpz requires {textureId: int, path: String}",
                    details: nil))
                return
            }
            guard let texture = registered[id] else {
                result(FlutterError(
                    code: "NO_SUCH_TEXTURE",
                    message: "loadSpz called with textureId=\(id) which is not registered",
                    details: nil))
                return
            }
                                                                    
            let maxSplats = (args["maxSplats"] as? NSNumber)?.uint32Value ?? 0
            let maxShDegree = (args["maxShDegree"] as? NSNumber)?.uint8Value ?? 3
                                                                        
                                                                        
                                                                    
            let splatScale = (args["splatScaleMultiplier"] as? NSNumber)?.floatValue ?? 1.0
            texture.setSplatScaleMultiplier(splatScale)
                                                                      
                                                                          
            let max3dScale = (args["max3dScale"] as? NSNumber)?.floatValue ?? 0.0
            texture.setMax3dScale(max3dScale)
            if !texture.loadSpz(path: path,
                                maxSplats: maxSplats,
                                maxShDegree: maxShDegree) {
                result(FlutterError(
                    code: "SPZ_LOAD_FAILED",
                    message: "aether_scene_renderer_load_spz returned false for path=\(path)",
                    details: nil))
                return
            }
            Self.logMemoryFootprint("loadSpz id=\(id) path=\((path as NSString).lastPathComponent)")
                                                                    
                                                                      
                                                                   
                                                                    
                                                                 
            let spzBounds = texture.getBounds()
            if let b = spzBounds {
                NSLog("[AetherTexture iOS] loadSpz succeeded: %@ bounds=([%.2f..%.2f],[%.2f..%.2f],[%.2f..%.2f])",
                      path, b.minX, b.maxX, b.minY, b.maxY, b.minZ, b.maxZ)
                result([
                    "bounds": [
                        "minX": Double(b.minX),
                        "minY": Double(b.minY),
                        "minZ": Double(b.minZ),
                        "maxX": Double(b.maxX),
                        "maxY": Double(b.maxY),
                        "maxZ": Double(b.maxZ),
                    ]
                ])
            } else {
                NSLog("[AetherTexture iOS] loadSpz succeeded: %@ (no bounds)", path)
                result([:])
            }

        case "setMatrices":
            guard let args = call.arguments as? [String: Any],
                  let id = (args["textureId"] as? NSNumber)?.int64Value,
                  let viewData = args["view"] as? FlutterStandardTypedData,
                  let modelData = args["model"] as? FlutterStandardTypedData else {
                result(FlutterError(
                    code: "BAD_ARGS",
                    message: "setMatrices requires {textureId: int, view: Float32List(16), model: Float32List(16)}",
                    details: nil))
                return
            }
            guard let texture = registered[id] else {
                                                          
                result(nil)
                return
            }
            texture.setMatrices(
                view: viewData.data.toFloatArray(),
                model: modelData.data.toFloatArray()
            )
            result(nil)

        case "captureThumb":
                                                                           
                                                                      
                                                            
              
                                                                       
                                                                        
                                                                    
                                                                    
            guard let args = call.arguments as? [String: Any],
                  let id = (args["textureId"] as? NSNumber)?.int64Value else {
                result(FlutterError(
                    code: "BAD_ARGS",
                    message: "captureThumb requires {textureId: int}",
                    details: nil))
                return
            }
            guard let texture = registered[id] else {
                                                                        
                                                                      
                                                             
                result(nil)
                return
            }
            let quality = (args["quality"] as? NSNumber)?.doubleValue ?? 0.85
            let data = texture.captureAsJPEG(quality: CGFloat(quality))
            if let data = data {
                result(FlutterStandardTypedData(bytes: data))
            } else {
                result(nil)
            }

        case "pauseRendering":
            pauseAnimation(reason: "dart lifecycle")
            result(nil)

        case "resumeRendering":
            applyThermalPolicy(ProcessInfo.processInfo.thermalState, sourceLog: "dart lifecycle")
            if !isCriticalThermal(ProcessInfo.processInfo.thermalState) {
                resumeAnimation(reason: "dart lifecycle")
            }
            result(nil)

        default:
            result(FlutterMethodNotImplemented)
        }
    }

                                                             
                                                                        
                           
       
                                                                 
                                                                     
                                                                    
    private func disposeTexture(id: Int64) {
        textures.unregisterTexture(id)
        registered.removeValue(forKey: id)
        if animatedTextureId == id {
            animatedTextureId = nil
            animatedTexture = nil
        }
        if registered.isEmpty {
            stopAnimation()
        }
        Self.logMemoryFootprint("dispose id=\(id) remaining=\(registered.count)")
    }

    private func startAnimation(textureId: Int64, texture: SharedNativeTexture) {
        guard displayLink == nil else { return }                    
        animatedTextureId = textureId
        animatedTexture   = texture
        animationStart    = CACurrentMediaTime()
        frameStatsLogTime = animationStart
        frameCount        = 0
        isPaused          = false

                                                                          
                                                                        
                                               
        let dl = CADisplayLink(target: self, selector: #selector(displayLinkTick))
        dl.preferredFramesPerSecond = targetFps                             
        dl.add(to: .main, forMode: .common)
        displayLink = dl
        NSLog("[AetherTexture] startAnimation targetFps=%d", targetFps)
    }

    private func stopAnimation() {
        displayLink?.invalidate()
        displayLink = nil
        animatedTextureId = nil
        animatedTexture = nil
        isPaused = false
    }

                                                                                                                                   

                                                                           
                                                                          
                                                                           
                                                                
    private func pauseAnimation(reason: String) {
        guard let dl = displayLink, !isPaused else { return }
        dl.isPaused = true
        isPaused = true
        NSLog("[AetherTexture] pauseAnimation reason=%@", reason)
    }

    private func resumeAnimation(reason: String) {
        guard let dl = displayLink, isPaused else { return }
        dl.isPaused = false
        isPaused = false
        NSLog("[AetherTexture] resumeAnimation reason=%@", reason)
    }

    @objc private func handleBackground() {
        pauseAnimation(reason: "background")
    }

    @objc private func handleForeground() {
                                                                                    
                                                            
        applyThermalPolicy(ProcessInfo.processInfo.thermalState, sourceLog: "foreground")
        if !isCriticalThermal(ProcessInfo.processInfo.thermalState) {
            resumeAnimation(reason: "foreground")
        }
    }

                                                                                                                                       
      
                                                                   
                                                             
                                                                      
                                                                 
                                                                       
                                                                     
                           
      
                                                                     
                                                                        
                                                                     
                                                                
                                                               
                                                    
    static func logMemoryFootprint(_ tag: String) {
        var info = task_vm_info_data_t()
        var count = mach_msg_type_number_t(
            MemoryLayout<task_vm_info_data_t>.size / MemoryLayout<integer_t>.size)
        let kr: kern_return_t = withUnsafeMutablePointer(to: &info) { ptr in
            ptr.withMemoryRebound(to: integer_t.self, capacity: Int(count)) { reb in
                task_info(mach_task_self_, task_flavor_t(TASK_VM_INFO), reb, &count)
            }
        }
        let footprintMB: Double =
            (kr == KERN_SUCCESS) ? Double(info.phys_footprint) / 1_048_576.0 : -1.0
        let availableMB: Int = os_proc_available_memory() / (1024 * 1024)
        NSLog("[AetherTexture] mem[%@] phys_footprint=%.1fMB available=%dMB",
              tag, footprintMB, availableMB)
    }

    @objc private func handleMemoryWarning() {
                                                                 
                                                                       
                                                                      
                                                                      
                                                               
                                                         
          
                                                                    
                                                                   
                                                                         
                                                                   
                                                                   
                                        
          
                                                                  
                                                                  
                                                                     
                                                                   
                          
        let total = registered.count
        guard total > 0 else { return }
                                                                  
                                                                 
                                                                   
                                         
        let sortedByRecency: [(key: Int64, value: SharedNativeTexture)] =
            registered.sorted { (lhs: (key: Int64, value: SharedNativeTexture),
                                 rhs: (key: Int64, value: SharedNativeTexture)) -> Bool in
                lhs.value.lastRenderTimestamp > rhs.value.lastRenderTimestamp
            }
                                                                       
                                                                       
                                                                        
                                                                   
                                                                   
                                                   
          
                                                                      
                                                                      
                                                                  
                                                                  
                                                                      
                      
        let availableBytes = os_proc_available_memory()
        let availableMB = availableBytes / (1024 * 1024)
        let keepCount = (availableMB > 0 && availableMB < 600) ? 2 : 3
        let disposeIds: [Int64] = sortedByRecency.dropFirst(keepCount).map { $0.key }
        NSLog("[AetherTexture] memory warning — keepCount=%d (available=%dMB) disposing %d/%d textures (keeping focused)",
              keepCount, availableMB, disposeIds.count, total)
        Self.logMemoryFootprint("memWarning")
        warningChannel?.invokeMethod("warning", arguments: [
            "kind": "memory",
            "disposedIds": disposeIds.map { NSNumber(value: $0) },
            "message": "Memory warning — non-focused textures released"
        ])
        for id in disposeIds {
            disposeTexture(id: id)
        }
    }

    @objc private func handleThermalChange() {
        applyThermalPolicy(ProcessInfo.processInfo.thermalState, sourceLog: "thermalDidChange")
    }

                                                                               
                                                                             
                                                                                
    private func applyThermalPolicy(_ state: ProcessInfo.ThermalState, sourceLog: String) {
        let newFps: Int
        let warning: String?
        switch state {
        case .nominal, .fair:
            newFps = 60
            warning = nil
        case .serious:
            newFps = 30
            warning = "Performance reduced (device warm)"
        case .critical:
            newFps = 0
            warning = "Animation paused (device too hot)"
        @unknown default:
                                                                              
                                                                
            newFps = 60
            warning = nil
        }
        let stateName = thermalStateName(state)
        NSLog("[AetherTexture] thermal=%@ targetFps=%d source=%@", stateName, newFps, sourceLog)

        targetFps = newFps
        if let dl = displayLink {
            dl.preferredFramesPerSecond = max(newFps, 1)                
        }
        if newFps == 0 {
            pauseAnimation(reason: "thermal=\(stateName)")
        } else if !isPaused {
                                                                              
                                           
                                                          
        }
        if let w = warning {
            warningChannel?.invokeMethod("warning", arguments: [
                "kind": "thermal",
                "state": stateName,
                "message": w
            ])
        }
    }

    private func isCriticalThermal(_ state: ProcessInfo.ThermalState) -> Bool {
        if case .critical = state { return true }
        return false
    }

    private func thermalStateName(_ state: ProcessInfo.ThermalState) -> String {
        switch state {
        case .nominal:  return "nominal"
        case .fair:     return "fair"
        case .serious:  return "serious"
        case .critical: return "critical"
        @unknown default: return "unknown(\(state.rawValue))"
        }
    }

    @objc private func displayLinkTick() {
                                                                         
                                                                    
                                        
          
                                                                    
                                                                 
                                                                     
                                                                         
                                                                        
                                                             
                                                                          
                                                                  
                                                                     
                                                                    
                                                                   
                              
        if registered.isEmpty { return }
        let now = CACurrentMediaTime()
        var totalRenderMs: Double = 0
        var rendered = 0
        let count = registered.count
        for (id, texture) in registered {
            if !texture.consumeIfDirty() { continue }
            totalRenderMs += texture.render()
            textures.textureFrameAvailable(id)
            rendered += 1
        }

                                                                        
                                                                     
                                                                    
        frameCount += 1
        let dt = now - frameStatsLogTime
        if dt >= 1.0 {
            let fps = Double(frameCount) / dt
            NSLog("[AetherTexture] %.1f fps (frames=%d, dt=%.3f, totalRenderMs=%.2f, textures=%d, rendered=%d)",
                  fps,
                  frameCount,
                  dt,
                  totalRenderMs,
                  count,
                  rendered)
            frameStatsLogTime = now
            frameCount = 0
        }
    }
}

private extension Data {
    func toFloatArray() -> [Float] {
        let count = self.count / MemoryLayout<Float>.size
        return self.withUnsafeBytes { raw -> [Float] in
            let base = raw.bindMemory(to: Float.self)
            return Array(UnsafeBufferPointer(start: base.baseAddress, count: count))
        }
    }
}
