import CoreVideo
import Flutter
import IOSurface
import QuartzCore
import UIKit                                                          

                                                                                                                   
  
                                                                       
                                                                          
                                                                          
                                                        
  
                                                                          
                                                                            

@_silgen_name("aether_scene_renderer_create")
private func aether_scene_renderer_create(
    _ iosurface: UnsafeMutableRawPointer?,
    _ width: UInt32,
    _ height: UInt32
) -> OpaquePointer?

@_silgen_name("aether_scene_renderer_destroy")
private func aether_scene_renderer_destroy(_ renderer: OpaquePointer?)

@_silgen_name("aether_scene_renderer_load_glb")
private func aether_scene_renderer_load_glb(
    _ renderer: OpaquePointer?,
    _ path: UnsafePointer<CChar>
) -> Bool

                                                                  
@_silgen_name("aether_scene_renderer_load_ply")
private func aether_scene_renderer_load_ply(
    _ renderer: OpaquePointer?,
    _ path: UnsafePointer<CChar>
) -> Bool

@_silgen_name("aether_scene_renderer_load_spz")
private func aether_scene_renderer_load_spz(
    _ renderer: OpaquePointer?,
    _ path: UnsafePointer<CChar>
) -> Bool

                                                                          
                                                                      
@_silgen_name("aether_scene_renderer_load_ply_capped")
private func aether_scene_renderer_load_ply_capped(
    _ renderer: OpaquePointer?,
    _ path: UnsafePointer<CChar>,
    _ maxSplats: UInt32,
    _ maxShDegree: UInt8
) -> Bool

@_silgen_name("aether_scene_renderer_load_spz_capped")
private func aether_scene_renderer_load_spz_capped(
    _ renderer: OpaquePointer?,
    _ path: UnsafePointer<CChar>,
    _ maxSplats: UInt32,
    _ maxShDegree: UInt8
) -> Bool

@_silgen_name("aether_scene_renderer_render_full")
private func aether_scene_renderer_render_full(
    _ renderer: OpaquePointer?,
    _ viewMatrix: UnsafePointer<Float>,
    _ modelMatrix: UnsafePointer<Float>
)

                                                                        
                                                                      
@_silgen_name("aether_scene_renderer_set_splat_scale_multiplier")
private func aether_scene_renderer_set_splat_scale_multiplier(
    _ renderer: OpaquePointer?,
    _ multiplier: Float
)

                                                                
                                                              
                                                                   
              
@_silgen_name("aether_scene_renderer_set_max_3d_scale")
private func aether_scene_renderer_set_max_3d_scale(
    _ renderer: OpaquePointer?,
    _ max3dScale: Float
)

                                                                         
                                                                    
@_silgen_name("aether_scene_renderer_get_bounds")
private func aether_scene_renderer_get_bounds(
    _ renderer: OpaquePointer?,
    _ outMin: UnsafeMutablePointer<Float>,
    _ outMax: UnsafeMutablePointer<Float>
) -> Bool

                                                                     
                                                         
struct LoadedBounds {
    let minX: Float
    let minY: Float
    let minZ: Float
    let maxX: Float
    let maxY: Float
    let maxZ: Float
}

                                                                            
                                                                             
                  
enum TextureCreateError: Error {
    case iosurfaceCreate
    case cvpixelbufferCreate(CVReturn)
    case rendererCreate
}

                                                                        
class SharedNativeTexture: NSObject, FlutterTexture {
    private let pixelBuffer: CVPixelBuffer
    private let ioSurface: IOSurfaceRef
    private var rendererHandle: OpaquePointer?

    private var latestView: [Float] = SharedNativeTexture.identityMatrix()
    private var latestModel: [Float] = SharedNativeTexture.identityMatrix()
                                                                       
                                                                      
                                                                    
                                                                           
                                                          
                                                                       
                                                                        
                                                                      
                                                                       
                                                                       
                                                                    
                                                                       
                                   
    private var dirty: Bool = true

                                                                      
                                                                    
                                                                      
                                                                    
                                                                      
                                                                 
                                                        
    private(set) var lastRenderTimestamp: CFTimeInterval = 0.0

                                                                         
    private var copyCount: UInt64 = 0
    private let leakCheckIntervalCalls: UInt64 = 60
    private let leakCheckThresholdRefs: CFIndex = 5

    private static func identityMatrix() -> [Float] {
        [
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1,
        ]
    }

    init(width: Int = 256, height: Int = 256) throws {
        let ioProps: [IOSurfacePropertyKey: Any] = [
            .width: width,
            .height: height,
            .pixelFormat: Int(kCVPixelFormatType_32BGRA),
            .bytesPerElement: 4,
            .bytesPerRow: width * 4,
        ]
        guard let surface = IOSurface(properties: ioProps) else {
            throw TextureCreateError.iosurfaceCreate
        }

        var pbUnmanaged: Unmanaged<CVPixelBuffer>?
        let cvStatus = CVPixelBufferCreateWithIOSurface(
            kCFAllocatorDefault,
            surface,
            nil,
            &pbUnmanaged
        )
        guard cvStatus == kCVReturnSuccess,
              let buffer = pbUnmanaged?.takeRetainedValue() else {
            throw TextureCreateError.cvpixelbufferCreate(cvStatus)
        }

        let surfaceVoidPtr = unsafeBitCast(surface, to: UnsafeMutableRawPointer.self)
                                                                      
                                                                     
                                                               
                                                                    
                                                                     
                                                              
                                           
        NSLog("[SharedNativeTexture iOS] importing IOSurface %dx%d into Dawn",
              width, height)
        guard let renderer = aether_scene_renderer_create(
            surfaceVoidPtr,
            UInt32(width),
            UInt32(height)
        ) else {
            throw TextureCreateError.rendererCreate
        }

        self.pixelBuffer = buffer
        self.ioSurface = surface
        self.rendererHandle = renderer
        super.init()

        NSLog("[SharedNativeTexture iOS] scene renderer created %dx%d BGRA8",
              width,
              height)
    }

    deinit {
        aether_scene_renderer_destroy(rendererHandle)
        rendererHandle = nil
    }

                                                                  
                                                                  
                                                                    
                             
    @discardableResult
    func render() -> Double {
        guard let rendererHandle else { return 0.0 }
        let frameStart = CACurrentMediaTime()
        lastRenderTimestamp = frameStart                           
        latestView.withUnsafeBufferPointer { viewBuf in
            latestModel.withUnsafeBufferPointer { modelBuf in
                guard let viewPtr = viewBuf.baseAddress,
                      let modelPtr = modelBuf.baseAddress else { return }
                aether_scene_renderer_render_full(rendererHandle, viewPtr, modelPtr)
            }
        }
        return (CACurrentMediaTime() - frameStart) * 1000.0
    }

    func setMatrices(view: [Float], model: [Float]) {
        if view.count == 16 { latestView = view }
        if model.count == 16 { latestModel = model }
                                                                         
                                                                   
                                                                       
                                                                        
                                                                    
        dirty = true
    }

                                                                      
                                                                      
                                                              
                                                
    func consumeIfDirty() -> Bool {
        if dirty {
            dirty = false
            return true
        }
        return false
    }

    func loadGlb(path: String) -> Bool {
        guard let rendererHandle else { return false }
        return path.withCString { cPath in
            aether_scene_renderer_load_glb(rendererHandle, cPath)
        }
    }

                                                                     
                                                                      
                                                                      
                            
    func loadPly(path: String, maxSplats: UInt32 = 0, maxShDegree: UInt8 = 3) -> Bool {
        guard let rendererHandle else { return false }
        return path.withCString { cPath in
            aether_scene_renderer_load_ply_capped(
                rendererHandle, cPath, maxSplats, maxShDegree)
        }
    }

                                                               
    func loadSpz(path: String, maxSplats: UInt32 = 0, maxShDegree: UInt8 = 3) -> Bool {
        guard let rendererHandle else { return false }
        return path.withCString { cPath in
            aether_scene_renderer_load_spz_capped(
                rendererHandle, cPath, maxSplats, maxShDegree)
        }
    }

                                                                         
                                                                       
                                                                   
                                                                 
                             
    func setSplatScaleMultiplier(_ multiplier: Float) {
        guard let rendererHandle else { return }
        aether_scene_renderer_set_splat_scale_multiplier(rendererHandle, multiplier)
    }

                                                                 
                                                               
                                                                 
                             
    func setMax3dScale(_ max3dScale: Float) {
        guard let rendererHandle else { return }
        aether_scene_renderer_set_max_3d_scale(rendererHandle, max3dScale)
    }

                                                                     
                                                             
    func getBounds() -> LoadedBounds? {
        guard let rendererHandle else { return nil }
        var minBuf: [Float] = [0, 0, 0]
        var maxBuf: [Float] = [0, 0, 0]
        let ok = minBuf.withUnsafeMutableBufferPointer { minBp -> Bool in
            maxBuf.withUnsafeMutableBufferPointer { maxBp -> Bool in
                guard let minPtr = minBp.baseAddress,
                      let maxPtr = maxBp.baseAddress else { return false }
                return aether_scene_renderer_get_bounds(rendererHandle,
                                                        minPtr, maxPtr)
            }
        }
        guard ok else { return nil }
        return LoadedBounds(
            minX: minBuf[0], minY: minBuf[1], minZ: minBuf[2],
            maxX: maxBuf[0], maxY: maxBuf[1], maxZ: maxBuf[2]
        )
    }

                                                                       
       
                                                                         
                                                                         
                                                                         
                                                                  
                                                                                   
       
                 
                                                                           
                                                                       
                                                        
                                                                     
                                                                         
                                                
                                                                 
       
                                                                       
                                                                       
                                                                       
                                                                     
                          
    func captureAsJPEG(quality: CGFloat = 0.85) -> Data? {
        let width = IOSurfaceGetWidth(ioSurface)
        let height = IOSurfaceGetHeight(ioSurface)
        let bytesPerRow = IOSurfaceGetBytesPerRow(ioSurface)

                                                                         
                                                                      
                                                                         
                                                                        
                                                                        
                                                                  
        let lockOpts: IOSurfaceLockOptions = [.readOnly]
        let lockResult = IOSurfaceLock(ioSurface, lockOpts, nil)
        guard lockResult == kern_return_t(0) else {
            NSLog("[SharedNativeTexture iOS] captureAsJPEG: IOSurfaceLock failed (%d)",
                  lockResult)
            return nil
        }
        defer { IOSurfaceUnlock(ioSurface, lockOpts, nil) }

                                                                        
                                                                        
                                               
        let baseAddress = IOSurfaceGetBaseAddress(ioSurface)

        let colorSpace = CGColorSpaceCreateDeviceRGB()
                                                                            
                                                                         
                                         
        let bitmapInfo: UInt32 = CGImageAlphaInfo.premultipliedFirst.rawValue
            | CGBitmapInfo.byteOrder32Little.rawValue
        guard let context = CGContext(
            data: baseAddress,
            width: width,
            height: height,
            bitsPerComponent: 8,
            bytesPerRow: bytesPerRow,
            space: colorSpace,
            bitmapInfo: bitmapInfo
        ) else {
            NSLog("[SharedNativeTexture iOS] captureAsJPEG: CGContext create failed")
            return nil
        }
        guard let cgImage = context.makeImage() else {
            NSLog("[SharedNativeTexture iOS] captureAsJPEG: makeImage failed")
            return nil
        }

        let uiImage = UIImage(cgImage: cgImage)
        let data = uiImage.jpegData(compressionQuality: quality)
        if let bytes = data?.count {
            NSLog("[SharedNativeTexture iOS] captureAsJPEG: %dx%d → %.1f KB",
                  width, height, Double(bytes) / 1024.0)
        }
        return data
    }

    func copyPixelBuffer() -> Unmanaged<CVPixelBuffer>? {
        _ = ioSurface                                                      
        copyCount &+= 1
        if copyCount % leakCheckIntervalCalls == 0 {
            let rc = CFGetRetainCount(pixelBuffer)
            if rc > leakCheckThresholdRefs {
                NSLog("[SharedNativeTexture iOS] passRetained contract WARNING: pixelBuffer retainCount=%ld after %llu copyPixelBuffer calls",
                      rc,
                      copyCount)
            }
        }
        return Unmanaged.passRetained(pixelBuffer)
    }
}
