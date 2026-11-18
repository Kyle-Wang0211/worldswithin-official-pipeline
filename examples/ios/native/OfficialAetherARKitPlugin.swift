import ARKit
@preconcurrency import AVFoundation
import CoreImage
import CoreMedia
import Flutter
import Foundation
import ImageIO
import simd
import UIKit

                                                                 
                                                                            
                                                                             
                                                                               
                                                                          
                                         
@_silgen_name("aether_gpu_match_set_capture_active")
func aether_gpu_match_set_capture_active(_ active: Int32)

enum OfficialARKitIdentifiers {
  static let methodChannel = "worldswithin_official_arkit"
  static let poseEventChannel = "worldswithin_official_arkit/pose_stream"
  static let previewView = "worldswithin_official_arkit_preview"
  static let cameraOwner = "official"
}

struct LiveCloudAnchorDeltaV1 {
  let translation: SIMD3<Float>
  let translationMeters: Double
  let rotation: simd_quatf
  let rotationDegrees: Double
}

enum LiveCloudAnchorDiagnostics {
  static func severity(translationMeters: Double) -> String {
    if translationMeters >= 0.10 { return "severe" }
    if translationMeters >= 0.05 { return "warning" }
    return "normal"
  }

  static func delta(
    lock: simd_float4x4,
    current: simd_float4x4
  ) -> LiveCloudAnchorDeltaV1 {
                                                                             
                                                                           
                                                                           
    let relative = lock.inverse * current
    let translation = SIMD3<Float>(
      relative.columns.3.x,
      relative.columns.3.y,
      relative.columns.3.z
    )
    let rotation = simd_normalize(simd_quatf(relative))
    let halfAngle = min(1.0, max(0.0, abs(Double(rotation.real))))
    return LiveCloudAnchorDeltaV1(
      translation: translation,
      translationMeters: Double(simd_length(translation)),
      rotation: rotation,
      rotationDegrees: 2.0 * acos(halfAngle) * 180.0 / Double.pi
    )
  }
}

struct LiveCloudRenderMetadataV1 {
  let contract: String
  let sourceReceiveSequence: Int
  let receiveSequence: Int
  let channelPushSequence: Int
  let source: String
  let publishVersion: Int
  let pointCount: Int
  let receiveEpochMs: Int64
  let computeDoneEpochMs: Int64
}

                                                 
  
                   
                                               
                                                               
                                                                     
                                                                    
                                                                 
                                                                         
                                                       
                                                                         
                                   
                                                                     
                                                                  
                                                                      
                                                          
                                                           
                                                                           
                                              
  
                                                                                  
               
          
                                                                        
                                                                        
                                                                    
                                                  
                                                                           
                                                                      
                                                                
                                                                
                                                                    
                                                                         
                                                                  
                                                                         
                                                                        
                                                                       
                                                                         
                                                                                
          
  
                                                                   
                                                                   
                                                                  
                                                                
                                                
  
                                                                   
                                                                     
                                                                   
                                                                   
                                          

@available(iOS 11.0, *)
class OfficialAetherARKitPlugin: NSObject {
                           

  private static var sharedInstance: OfficialAetherARKitPlugin?

                                                                                
                                                                      
                                                                     
                                            
  static func currentSession() -> ARSession? {
    return sharedInstance?.arSession
  }

  static func register(with registrar: FlutterPluginRegistrar) {
                                                                                                    
                                                                                                              
                                                                                        
      
                                                                                               
                                                                                
                                                                               
      
                                                                                              
                                                                                                        
                                                                                                    
                                                                                                        
      
                                                                                                  
                                                                                                        
                                                                                                         
                                                                                                     
                                                                  
      
                                                                                                            
                                                                                             
                                                                                         
                                                                                            
                                                                         
                                                                                                   
                                                                                                   
                                                                                                              
                                                                                                    
      
                                                                                                         
                                                                                                      
                                                                                                  
                                                                                        
                                         
                                                                                         
                                                                                            
                                                                                                  
                                                                                                     
                                                                                        
                                                                                                              
                                                                                       
    setenv("OFFICIAL_AETHER_STREAM_TEMPORAL_ONLY", "0", 0)
    setenv("OFFICIAL_AETHER_DESCRIPTOR_RESIDENCY_V1", "1", 0)
                                                             
                                                                           
                                                                                               
                                                                                          
                                                                                             
                                                                                             
                                                                                                    
                                                                                                                    
                                                                                          
                                                                                                     
                                                                                                            
                                                                                  
    setenv("OFFICIAL_AETHER_DESCRIPTOR_RESIDENCY_BYTES", "419430400", 0)            
                                                                                           
                                                                                                  
                                                                                    
                                                                                   
                                                                  
                                                                                             
                                                                                           
                                                                                           
                                                               
                                                                                          
                                                                                              
                                                                                           
                                                                                                   
                                                               
                                                                                                    
                                                                                             
                                                                      
                                                                                                             
                                                                                                               
                                                                                              
                                                                                      
                                                                                                   
                                                                                                              
                                                                                                             
                                                                                                    
                                                                            
                                                                                   
                                                                  
      
                                    
                                                 
      
                                                                                  
                                                                                                   
                                                         
                                                                                                   
                                                     
                                                                           
                                                                             
                                                                          
                                                                               
                                                                                                     
                                                         
      
                                                                             
                                                                                
                                                                                               
                                                                                                             
                                                                                            
                                                                                        
                                                                                              
                                                                                                 
                                                                                               
                                                                                                      
                                                                              
                                                                                      
                                                                                  
                                                                                            
                                                                                       
                                                                                       
                                                                            
                                                                                           
                                                            
                                                                              
                                                                        
                                                                                                 
                                                                                           
                                                                                       
                                                                                            
                                                                                  
                                                                                   
                                                                                 
                                        
                                                                      
                                                                                    
                                                                                      
                                                                               
                                                                 
                                                                                     
                                                                                               
                                                                                                         
                                                                                                                    
                                                                                                 
                                                                                                       
                                                                                              
                                                                                                
    setenv("OFFICIAL_AETHER_GPU_TIMESTAMPS", "1", 0)
                                                                                            
                                                                                                
                                                                                                                 
                                                                                                           
    setenv("OFFICIAL_AETHER_TAIL_CACHE_V1", "1", 0)
    setenv("OFFICIAL_AETHER_QUADRATIC_OVERLAP", "0", 0)
                                                                            
                                                                                     
                                                                                             
                                                                                      
                                                                                 
    setenv("OFFICIAL_AETHER_QUADRATIC_PREPAY", "0", 0)
                                                                                            
                                                                                     
                                                                                                
                                                                                          
                                                                              
                                                                             
                                                                                          
                                                                                                       
                                                                                   
                                                                                                   
                                                                                      
                                                                                           
                                                
                                                                                       
                                                                                         
                                                                                               
                                                                                                            
                                                                                            
                                                                                                             
                                                                                                  
                                                                                              
                                                                                 
                                                                                    
    setenv("OFFICIAL_AETHER_MATCH_GAP_SERIOUS_PCT", "12", 0)
                                                     
                                                                         
                                                                             
                                                                     
                                                                  
                                                                        
                                                                             
                                                                            
                                                                             
                                                                               
                                                                           
                                                                            
                                                                       
                                      
    setenv("OFFICIAL_AETHER_OFFICIAL_TRIANGULATE", "1", 0)
    setenv("OFFICIAL_AETHER_SELFDEV_TRIANGULATE", "0", 0)
    setenv("OFFICIAL_AETHER_TRI_IGNORE_2VIEW", "0", 0)
                                                                                        
                                                                                          
                                                                                
                                                                          
                                                                                                     
                                                                          
                                                                  
                                                                    
                                                                  
    setenv("OFFICIAL_AETHER_SCALE_ANCHOR", "1", 0)
                                                                                           
                                                                                              
                                                                                                 
                                                                                            
                                                                         
                                                                                                          
    setenv("OFFICIAL_AETHER_AR_EVERY_FRAME", "1", 0)
                                                                                   
                                                                                            
                                                                                         
                                                                          
                                                                                                  
      
                                                                                                               
                                                       
                                  
                                                                    
                                                                          
                                                                           
                                                                                                      
                                                                                                       
                                                                    
      
                                                                                                             
                                                                                                
                                                                                         
                                                                                                    
                                                                                                                    
      
                                                                                                   
                                                                                                
                                                                                                      
                                                                                                               
                                                                                                            
                                                                         
    setenv("OFFICIAL_AETHER_AR_SPLAT_MAX_PX", "6", 0)
                                                                        
                                                                             
                                                                           
                                                                              
                                                                                         
                                                                                         
                                                                                
                                                                          
                                                                                 
                                                                                      
                                                                                        
                                                                                        
                                                                                     
                                                                                        
                                                                        
    setenv("OFFICIAL_AETHER_STAGE1_ROUNDS_CAP", "2", 0)
                                                                                                          
                                                                                         
                                                                                                    
      
                                                                                  
                                                                                   
                                   
      
                                                                                                       
                                                                                                     
      
                                                                                          
                                                                                         
                                                                                   
      
                                                                                        
                                                                                       
                                                         
    let plugin = OfficialAetherARKitPlugin(messenger: registrar.messenger())
    sharedInstance = plugin
    let factory = OfficialAetherARKitPreviewFactory(getSession: {
      OfficialAetherARKitPlugin.currentSession()
    })
    registrar.register(factory, withId: OfficialARKitIdentifiers.previewView)
                                                                                             
                                                                            
    OfficialPwNativeTelemetry.shared.logSession()
  }

                                                                      

                                                         
                                                                                   
                                                                             
                                                                              
                                                                                  
                                                                                  
  struct PhotoCardSpec {
                                                                    
                                                                                               
                                                                                                   
                                                                                                  
                                                                                                     
                                                      
                                                                                            
                                                                                                            
                                                                                                  
                                                                                   
                                                                                
                                                                                      
                                                                                                  
                                                                                               
                                                                                       
                                                            
    let textureCGImage: CGImage?
    let texturePath: String
    let evidencePath: String
                                                                                                 
                                                                                            
                                                                                           
                                                                                      
                                                                                      
    let shutterFeedbackEpochMs: Double?
                                                                                                 
    let addCalledEpochMs: Double
    let localCorners: [SCNVector3]                                                       
    let captureDistance: Float                                          
    let worldCentroid: simd_float3                                                        
    let captureCamPos: simd_float3                                                        
  }
  static var photoCardSpecs: [String: PhotoCardSpec] = [:]
                                                                                              
                                                                               
                                                                                                 
                                                      
     
                                                                                             
                                                                                               
                                                                                               
                                                                                                        
                                                                                                      
                                                                                          
                                                                      
                                                                                            
  static var photoBasisByEvidencePath:
    [String: (invView: simd_float4x4, transform: simd_float4x4)] = [:]
                                                                                        
                                                                               
  static var photoCardThumbByEvidencePath: [String: CGImage] = [:]
                                                                                              
                                                                                                 
  static var photoCardThumbRetries: [String: Int] = [:]
  private static var photoCardAnchors: [ARAnchor] = []
                                                                                                                    
                                                                                            
                                   
  static var photoCardPendingAnchors:
    [String: (anchor: ARAnchor, z: Float, halfX: Float, halfY: Float,
              evidencePath: String, since: CFTimeInterval)] = [:]
  private static var photoCardCounter = 0

                                                                                 
                                                                                          
                                                                                 
                                                                     
  static var featurePointsVisible: Bool = false

                                                                                            
                                                                                                      
                                                                                                
                                                                                      
  static var photoCardsVisible: Bool = true

                                                                                        
                                                                          
  static var videoFormatMode: String = "4k"

                                                                         
                                                                               
                                                                                
                                                                             
                                                                       
                                                                         
                                                                        
                                                                            
                                                        
  static let coverageCloudLock = NSLock()
  static var coverageCloudXyz: [Float] = []
  static var coverageCloudRgb: [UInt8] = []
  static var coverageCloudTransform = matrix_identity_float4x4
  static var coverageCloudMetadata = LiveCloudRenderMetadataV1(
    contract: "UNSTAMPED",
    sourceReceiveSequence: 0,
    receiveSequence: 0,
    channelPushSequence: 0,
    source: "unknown",
    publishVersion: 0,
    pointCount: 0,
    receiveEpochMs: 0,
    computeDoneEpochMs: 0
  )
  static var coverageCloudDirty = false

  static func floatArray(_ matrix: simd_float4x4) -> [Float] {
    [
      matrix.columns.0.x, matrix.columns.0.y,
      matrix.columns.0.z, matrix.columns.0.w,
      matrix.columns.1.x, matrix.columns.1.y,
      matrix.columns.1.z, matrix.columns.1.w,
      matrix.columns.2.x, matrix.columns.2.y,
      matrix.columns.2.z, matrix.columns.2.w,
      matrix.columns.3.x, matrix.columns.3.y,
      matrix.columns.3.z, matrix.columns.3.w,
    ]
  }

  static func setCoverageCloudTransform(_ transform: simd_float4x4) {
    coverageCloudLock.lock()
    coverageCloudTransform = transform
    coverageCloudDirty = true
    coverageCloudLock.unlock()
  }

  static func setCoverageCloud(
    xyz: [Float],
    rgb: [UInt8],
    metadata: LiveCloudRenderMetadataV1
  ) {
    coverageCloudLock.lock()
    coverageCloudXyz = xyz
    coverageCloudRgb = rgb
    coverageCloudMetadata = metadata
    coverageCloudDirty = true
    coverageCloudLock.unlock()
  }

                                                                           
                                                                           
  static func takeCoverageCloudIfDirty()
    -> (xyz: [Float], rgb: [UInt8], metadata: LiveCloudRenderMetadataV1,
        transform: simd_float4x4)?
  {
    coverageCloudLock.lock()
    defer { coverageCloudLock.unlock() }
    if !coverageCloudDirty { return nil }
    coverageCloudDirty = false
    return (
      coverageCloudXyz,
      coverageCloudRgb,
      coverageCloudMetadata,
      coverageCloudTransform
    )
  }

                                                                                 
                                                                              
                                                                               
                                                                                          
                                                                           
                                                                                    
                                         
  static let photoCardStateLock = NSLock()
  static var photoCardStates: [String: Int] = [:]                       
  static var photoCardStatesDirty = false

  static func mergePhotoCardStates(_ update: [String: Int]) {
    photoCardStateLock.lock()
    photoCardStates.merge(update) { _, new in new }
    photoCardStatesDirty = true
    photoCardStateLock.unlock()
  }

                                                                          
  static func photoCardState(forPath path: String) -> Int {
    photoCardStateLock.lock()
    defer { photoCardStateLock.unlock() }
    return photoCardStates[path] ?? 0
  }

                                                                         
                                                                             
                                                                        
  static func takePhotoCardStatesIfDirty() -> [String: Int]? {
    photoCardStateLock.lock()
    defer { photoCardStateLock.unlock() }
    if !photoCardStatesDirty { return nil }
    photoCardStatesDirty = false
    return photoCardStates
  }
                                                                                  
                                                                                
                                                                                
                                                                 
  static let photoCardCloseZ: Float = 0.05
                                                                               
                                                                                 
                                                                                    
                                                             
  static let photoCardThumbMaxPx = 96

                   

  private let methodChannel: FlutterMethodChannel
  private let poseEventChannel: FlutterEventChannel
  private let poseStreamHandler = OfficialPoseStreamHandler()

                      

  private var arSession: ARSession?
  private let sessionDelegate = OfficialARSessionForwarder()

                                                                     
                                                                           
                                                                         
                                                                      
                                                                  
  private var worldOrigin: simd_float3?
  private var worldYaw: Float = 0

                                                                 
                                                                      
                                                                     
                                                                    
                                                                      
                                                                  
                                                                   
                                                                   
                                                                      
                                            
  private var worldSubjectAnchor: ARAnchor?

                                                                     
                                                                    
                                                                      
                                                                          
                                                                       
                                       
  private var lockTimeOrigin: simd_float3?
  private var lockTimeAnchorTransform: simd_float4x4?
  private var lastLoggedAnchorTransform: simd_float4x4?
  private var lastAnchorSeverity: String?
  private var lastDriftLogTime: TimeInterval = 0
  private var lastAnchorV2Transform: simd_float4x4?
  private var lastAnchorV2LogTime: TimeInterval = 0

                                                                      
                                                                       
                                                                     
                   
  private var lastQualityComputeTime: TimeInterval = 0
                                                            
     
                                                              
                                                                           
     
                                                                                                     
                                                                              
                                                                                
                                                                                  
                                                                                                           
                                                         
     
                                                                                         
                                                                                              
                                                                                                      
                               
                                                                                   
                                                                                 
                                                 
                         
                            
                                                                       
                                                                                                           
                                                                              
                                                                                                
                                                 
                                                      
                                                                                                
                                                                                                     
                                                                                                       
                                                                                              
                                                                                                    
                                                        
                                                                                               
                                                                                
  static var currentQualityHz: Double = qualityHzArms[0]
  private static let qualityHzArms: [Double] = {
    guard let raw = getenv("OFFICIAL_AETHER_QUALITY_HZ"),
          let text = String(validatingUTF8: raw) else { return [6.0] }
    let arms = text.split(separator: ",").compactMap { part -> Double? in
      guard let v = Double(part.trimmingCharacters(in: .whitespaces)),
            v.isFinite, v > 0 else { return nil }
      return v
    }
    return arms.isEmpty ? [6.0] : arms
  }()
                                                                                                    
                                                                             
  private var qualityArmIndex = 0 {
    didSet { OfficialAetherARKitPlugin.currentQualityHz = 1.0 / qualityInterval }
  }
  private var qualityInterval: TimeInterval {
    1.0 / Self.qualityHzArms[qualityArmIndex % Self.qualityHzArms.count]
  }

                                                                   
                                                                 
                                                                         
                                                        
  private var lastPreviewPointPayloadTime: TimeInterval = 0

                                                                                                     
                                                                                       
                                                                                       
                                                                                               
                                                                                 
                                                                     
  private var lastCenterRayDepthM: Float = -1
  private var lastCenterRayAt: TimeInterval = 0
  private static let centerRayInterval: TimeInterval = 0.5
  private static let previewPointInterval: TimeInterval = 1.0 / 8.0
  private static let previewPointMaxCount: Int = 220

                                                                    
                                                                    
                                                                      
                                                                      
                                                                    
                                                               
                                                                     
                                               
  private let qualityQueue = DispatchQueue(
    label: "com.worldswithin.official.arkit.quality",
    qos: .userInitiated
  )
                                                                     
                                                                     
                                                                
                                                                             
                                                                               
     
                                                                        
                                                     
                                                                      
                                                                      
                             
  private var pendingGray128: Data?
  private var pendingGraySourceTimestamp: TimeInterval?
  private var pendingGraySourceFocalX: Double?
  private var pendingGraySourceFocalY: Double?
                                                                   
                                                                        
                                                                     
                                                                
  private var qualityComputeInFlight: Bool = false

                                                                        
                                                                       
                                                              
                                                                      
                                                   
                                                                     
                                                                     
                                                          
  private var qDiagWindowStart: TimeInterval = 0
  private var qDiagFires: Int = 0
  private var qDiagSkips: Int = 0
  private var qDiagElapsedMsSum: Double = 0
  private var qDiagAttached: Int = 0
  private var qDiagPoseEvents: Int = 0

                                                                
    
                                                                         
                                                                          
                                                                    
                                                                     
                                                                          
                                                                      
                                                                         
                                                                     
                                                                       
                                 
    
                                                                     
                      
                                                 
                                        
                                                              
                                           
                                           
                                     
                                         
                                                      
  private struct ScaleAlignPremetrics {
    let anchorDepthCount: Int
    let anchorDepthMinM: Float
    let anchorDepthMaxM: Float
    let anchorDepthSpanM: Float
    let reliabilityPrior: Float
  }

  private struct LatestFrameSnapshot {
    let pixelBuffer: CVPixelBuffer
    let timestamp: TimeInterval
    let extrinsic: [Float]
    let intrinsicsFxFyCxCy: [Float]
    let imageW: Int
    let imageH: Int
    let trackingStateName: String
    let isTracking: Bool
    let anchorsWorld: [[Float]]
    let anchorIds: [UInt64]
    let scaleAlignPremetrics: ScaleAlignPremetrics
  }
  private var lastFrameSnapshot: LatestFrameSnapshot?
  private var recentFrameSnapshots: [LatestFrameSnapshot] = []
                                                                   
                                                                       
                                                                       
                                                                      
                                                             
  private static let maxRecentFrameSnapshots = 4
  private static let defaultSaveMaxTimestampDelta: TimeInterval = 0.18

                                                                   
                                                                  
                                                   
  private let jpegEncodeQueue = DispatchQueue(
    label: "com.worldswithin.official.arkit.jpeg",
    qos: .userInitiated
  )

                                                                        
                                                                      
                                                                     
                                                        
  private lazy var ciContext: CIContext = CIContext(options: nil)

                                                                         
                                                                         
                                                                             
                                                                         
                                                                            
                                                                           
                                                                             
                                                                              
                                                                           
                                                                         
                                 
                                                                       
                                                                                          
                                                                               
                                                                                      
                                                                                  
  private let colorizeQueue = DispatchQueue(
    label: "com.worldswithin.official.arkit.colorize",
    qos: .utility,
    attributes: .concurrent
  )



               

  private init(messenger: FlutterBinaryMessenger) {
    self.methodChannel = FlutterMethodChannel(
      name: OfficialARKitIdentifiers.methodChannel,
      binaryMessenger: messenger
    )
    self.poseEventChannel = FlutterEventChannel(
      name: OfficialARKitIdentifiers.poseEventChannel,
      binaryMessenger: messenger
    )
    super.init()
    methodChannel.setMethodCallHandler { [weak self] call, result in
      self?.handle(call: call, result: result)
    }
    poseEventChannel.setStreamHandler(poseStreamHandler)
    sessionDelegate.onFrame = { [weak self] frame in
      self?.broadcast(frame: frame)
    }
    sessionDelegate.onSessionFailure = {
      PwARCameraLease.shared.release(
        owner: OfficialARKitIdentifiers.cameraOwner
      )
    }
  }

                                

  private func handle(call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "isAvailable":
      result(ARWorldTrackingConfiguration.isSupported)
    case "startSession":
      guard PwARCameraLease.shared.acquire(
        owner: OfficialARKitIdentifiers.cameraOwner
      ) else {
        result(FlutterError(
          code: "ar_camera_busy",
          message: "The camera is already owned by another capture pipeline",
          details: nil
        ))
        return
      }
      do {
        let resume =
          (call.arguments as? [String: Any])?["resume"] as? Bool ?? false
                                                                           
                                                                                       
                                                                                          
                                                                                             
                                                                                            
        OfficialAetherARKitPlugin.videoFormatMode =
          ((call.arguments as? [String: Any])?["videoFormatMode"] as? String)
            ?? OfficialAetherARKitPlugin.videoFormatMode
        guard OfficialAetherARKitPlugin.videoFormatMode == "hires43" else {
          throw NSError(
            domain: "OfficialAetherARKit",
            code: 217,
            userInfo: [NSLocalizedDescriptionKey:
              "Official capture supports only the locked 1920x1440 preview mode"]
          )
        }
        try startSession(resetWorld: !resume)
        result(nil)
      } catch {
        PwARCameraLease.shared.release(
          owner: OfficialARKitIdentifiers.cameraOwner
        )
        result(FlutterError(
          code: "ar_start_failed",
          message: error.localizedDescription,
          details: nil
        ))
      }
    case "stopSession":
      stopSession()
      result(nil)
    case "lockOrigin":
      let distance: Float
      if let args = call.arguments as? [String: Any],
         let d = (args["distanceMeters"] as? NSNumber)?.floatValue {
        distance = d
      } else {
        distance = 0.5
      }
      let lockResult = lockOrigin(distanceMeters: distance)
      if let payload = lockResult {
        result(payload)
      } else {
        result(FlutterError(
          code: "ar_no_frame",
          message: "ARSession has no current frame to lock against",
          details: nil
        ))
      }
    case "saveCurrentFrameAsJpeg":
                                                                         
                                                                           
                                                                         
                                                                      
                                                                      
      guard let args = call.arguments as? [String: Any],
            let jpegPath = args["jpegPath"] as? String,
            let metadataPath = args["metadataPath"] as? String else {
        result(FlutterError(
          code: "ar_save_jpeg_bad_args",
          message: "saveCurrentFrameAsJpeg requires {jpegPath: String, metadataPath: String, quality?: Float}",
          details: nil
        ))
        return
      }
      let quality = (args["quality"] as? NSNumber)?.floatValue ?? 0.9
      let targetTimestamp = (args["targetTimestamp"] as? NSNumber)?.doubleValue
      let maxTimestampDelta = (args["maxTimestampDelta"] as? NSNumber)?.doubleValue
        ?? Self.defaultSaveMaxTimestampDelta
      let metadataSchemaVersion = (args["metadataSchemaVersion"] as? NSNumber)?.intValue ?? 1
      let dartSaveContract = args["dartSaveContract"] as? [String: Any]
      let includeSfmFeed = (args["includeSfmFeed"] as? NSNumber)?.boolValue ?? true
      saveCurrentFrameAsJpeg(
        jpegPath: jpegPath,
        metadataPath: metadataPath,
        targetTimestamp: targetTimestamp,
        maxTimestampDelta: maxTimestampDelta,
        quality: quality,
        metadataSchemaVersion: metadataSchemaVersion,
        dartSaveContract: dartSaveContract,
        includeSfmFeed: includeSfmFeed
      ) { payload, error in
        if let error = error {
          result(FlutterError(
            code: "ar_save_jpeg_failed",
            message: error.localizedDescription,
            details: nil
          ))
        } else {
                                                                            
                                                                               
          result(payload)
        }
      }
    case "captureHighResolutionStill":
      guard let args = call.arguments as? [String: Any],
            let highresPath = args["highresPath"] as? String,
            let previewPath = args["previewPath"] as? String else {
        result(FlutterError(
          code: "ar_highres_bad_args",
          message: "captureHighResolutionStill requires {highresPath: String, previewPath: String, quality?: Float}",
          details: nil
        ))
        return
      }
      let quality = (args["quality"] as? NSNumber)?.floatValue ?? 0.92
      let metadataPath = args["metadataPath"] as? String
      let targetTimestamp = (args["triggerTimestamp"] as? NSNumber)?.doubleValue
      let metadataSchemaVersion = (args["metadataSchemaVersion"] as? NSNumber)?.intValue ?? 1
      let dartSaveContract = args["dartSaveContract"] as? [String: Any]
                                                                                                 
                                                               
      let feedSfm = (args["feedSfm"] as? NSNumber)?.boolValue ?? false
      let deriveAuxiliary =
        (args["deriveAuxiliary"] as? NSNumber)?.boolValue ?? true
      captureHighResolutionStill(
        highresPath: highresPath,
        previewPath: previewPath,
        quality: quality,
        metadataPath: metadataPath,
        targetTimestamp: targetTimestamp,
        metadataSchemaVersion: metadataSchemaVersion,
        dartSaveContract: dartSaveContract,
        feedSfm: feedSfm,
        deriveAuxiliary: deriveAuxiliary
      ) { payload, error in
        if let error = error {
          let nsError = error as NSError
          OfficialPwNativeTelemetry.shared.log("highres_capture", [
            "outcome": "error",
            "domain": nsError.domain,
            "code": nsError.code,
            "message": error.localizedDescription,
          ])
          result(FlutterError(
            code: "ar_highres_failed",
            message: error.localizedDescription,
            details: nil
          ))
        } else {
          OfficialPwNativeTelemetry.shared.log("highres_capture", [
            "outcome": "ok",
            "request_to_capture_dt":
              (payload?["timestampDelta"] as? NSNumber)?.doubleValue ?? -1,
            "image_w": (payload?["imageWidth"] as? NSNumber)?.intValue ?? 0,
            "image_h": (payload?["imageHeight"] as? NSNumber)?.intValue ?? 0,
          ])
          result(payload)
        }
      }
    case "beginFullPipelineBackground", "full_pipelineBackgroundStatus",
         "updateFullPipelineBackground", "endFullPipelineBackground":
      let args = call.arguments as? [String: Any]
      let jobID = args?["jobId"] as? String ?? ""
      func reply(_ status: String, _ reason: String) {
        result(["schema": "RECONSTRUCTION_BACKGROUND_V1", "job_id": jobID,
                "status": status, "reason": reason])
      }
      guard #available(iOS 26.0, *) else {
        reply("unsupported", "ios26_required")
        return
      }
      guard !jobID.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else {
        reply("rejected", "invalid_job_id")
        return
      }
      let umbrella = OfficialReconUmbrella.shared
      switch call.method {
      case "beginFullPipelineBackground":
                                                                           
                                                                      
        var requiresGPU = true
        if let raw = args?["requiresGPU"] {
          guard let value = raw as? NSNumber,
                CFGetTypeID(value) == CFBooleanGetTypeID() else {
            reply("rejected", "invalid_requires_gpu")
            return
          }
          requiresGPU = value.boolValue
        }
        result(umbrella.beginFullPipeline(jobID: jobID, requiresGPU: requiresGPU))
      case "full_pipelineBackgroundStatus":
        result(umbrella.full_pipelineStatus(jobID: jobID))
      case "updateFullPipelineBackground":
                                                                               
        func integer(_ key: String) -> Int? {
          guard let number = args?[key] as? NSNumber,
                CFGetTypeID(number) != CFBooleanGetTypeID() else { return nil }
          let value = number.doubleValue
          guard value.isFinite, value.rounded() == value,
                value >= 0, value < Double(Int.max) else { return nil }
          return Int(value)
        }
        guard let stages = integer("completedStages"), let done = integer("done"),
              let total = integer("total"), let phase = args?["phase"] as? String,
              !phase.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else {
          reply("rejected", "invalid_progress")
          return
        }
        result(umbrella.updateFullPipeline(jobID: jobID, completedStages: stages,
                                     phase: phase, done: done, total: total))
      case "endFullPipelineBackground":
        guard let value = args?["success"] as? NSNumber,
              CFGetTypeID(value) == CFBooleanGetTypeID() else {
          reply("rejected", "invalid_success")
          return
        }
        result(umbrella.endFullPipeline(jobID: jobID, success: value.boolValue))
      default:
        reply("rejected", "unknown_method")
      }
    case "beginReconUmbrella":
                                                                            
                                                                           
                                                                                 
                                                          
      let args = call.arguments as? [String: Any]
      let jobID = args?["jobId"] as? String ?? "legacy"
      if #available(iOS 26.0, *) {
        OfficialReconUmbrella.shared.begin(jobID: jobID)
      }
      result(nil)
    case "endReconUmbrella":
                                                                                
      let args = call.arguments as? [String: Any]
      let jobID = args?["jobId"] as? String ?? "legacy"
      if #available(iOS 26.0, *) {
        OfficialReconUmbrella.shared.end(jobID: jobID)
      }
      result(nil)
    case "setReconProgress":
                                                                                 
                                                                   
      let args = call.arguments as? [String: Any]
      let fraction = (args?["fraction"] as? NSNumber)?.doubleValue ?? 0
      let subtitle = args?["subtitle"] as? String
      if #available(iOS 26.0, *) {
        OfficialReconUmbrella.shared.setRealProgress(fraction: fraction, subtitle: subtitle)
      }
      result(nil)
    case "decodeJpegForColor":
                                                                              
                                                                           
                                                                               
                                                                               
                                                                     
                                                                   
                                                       
                                                                         
        
                                                                              
                                                                               
                                                                               
                                                                                    
                                                                               
                                                                                
                                                                             
                                                                                
                                                                                 
      colorizeQueue.async { [weak self] in
        let mainResult: FlutterResult = { value in
          DispatchQueue.main.async { result(value) }
        }
        guard let self = self else { mainResult(nil); return }
        self.handleDecodeJpegForColor(call: call, result: mainResult)
      }
    case "addPhotoCard":
                                                                              
                                                                                
                                                                                    
      guard let args = call.arguments as? [String: Any],
            let texturePath = args["textureJpegPath"] as? String,
            let evidencePath = args["evidenceJpegPath"] as? String else {
        result(FlutterError(
          code: "bad_args",
          message: "addPhotoCard requires textureJpegPath and evidenceJpegPath",
          details: nil))
        return
      }
      guard let session = arSession,
            let frame = session.currentFrame else {
        result(FlutterError(
          code: "ar_no_frame", message: "addPhotoCard: no current ARFrame",
          details: nil))
        return
      }
      let camera = frame.camera
                                                                                   
                                                                                  
                                                                                  
                                                                                     
                                                                                         
                                                                                    
                                                                                  
                                                                                   
                                                                           
                                                                                        
        
                                                                                       
                                                                                                 
                                                                                            
                                       
                                                                                         
                                                                                                    
                                                                 
                                                                                                    
                                                                                      
                                                                           
        
                                                                                         
                                                               
                                                                                                 
                                                                                             
                                                                                 
      let liveT = camera.transform
      let photoBasis = OfficialAetherARKitPlugin
        .photoBasisByEvidencePath.removeValue(forKey: evidencePath)
                                                                                         
        
                                                                                           
                                                                                         
                                                                                                              
        
                                                                                                      
                                                                                        
                                                                                    
                                                                                                 
                                                                                          
                                                                                                          
                                           
      let camT = photoBasis?.transform ?? liveT
      let camPos = simd_make_float3(camT.columns.3)
      OfficialPwNativeTelemetry.shared.log("photocard_pose_source", [
        "source": photoBasis == nil ? "live_fallback" : "photo_frame_basis",
                                                                                   
        "shift_m": Double(simd_length(
          simd_make_float3(camT.columns.3) - simd_make_float3(liveT.columns.3))),
      ])
      let z: Float = Self.photoCardCloseZ
      NSLog("[PHOTOCARD] addPhotoCard close-anchor z=%.3f", z)
                                                                                    
                                                                               
                                                                                     
                                                                                  
                                                                              
                                                                                  
                                                                                        
                                                                                             
                                                                                                       
                                                            
      let screenSize = UIScreen.main.bounds.size
      let viewportSize: CGSize =
        OfficialAetherARKitPlugin.videoFormatMode == "hires43"
          ? CGSize(width: screenSize.width, height: screenSize.width * 4.0 / 3.0)
          : screenSize
      let proj = camera.projectionMatrix(for: .portrait,
                                         viewportSize: viewportSize,
                                         zNear: 0.001, zFar: 1000)
                                                                                                    
                                                                                                              
                                 
                                                                                     
                                                                              
      let liveInvView = camera.viewMatrix(for: .portrait).inverse
      let invView = photoBasis?.invView ?? liveInvView
                                                                              
                                                                                                 
      let halfX = z / proj.columns.0.x
      let halfY = z / proj.columns.1.y
      NSLog("[PHOTOCARD] addPhotoCard viewport=%.0fx%.0f z=%.2f halfX=%.3f halfY=%.3f",
            viewportSize.width, viewportSize.height, z, halfX, halfY)
                                                                      
      let viewCornersV: [simd_float4] = [
        simd_float4(-halfX,  halfY, -z, 1),        
        simd_float4( halfX,  halfY, -z, 1),        
        simd_float4( halfX, -halfY, -z, 1),        
        simd_float4(-halfX, -halfY, -z, 1),        
      ]
      let worldCorners: [simd_float3] = viewCornersV.map {
        simd_make_float3(invView * $0)
      }
      let centroid = (worldCorners[0] + worldCorners[1]
                      + worldCorners[2] + worldCorners[3]) / 4
                                                                            
                                                                                                   
                                                                
                                                                                                  
                                                                                               
                                                                                   
      let liveUp = simd_normalize(simd_make_float3(liveInvView.columns.1))
      let cardUp = simd_normalize(simd_make_float3(invView.columns.1))
      let liveRight = simd_normalize(simd_make_float3(liveInvView.columns.0))
      let cardRight = simd_normalize(simd_make_float3(invView.columns.0))
      let degOf: (simd_float3, simd_float3) -> Double = { a, b in
        Double(acos(max(-1, min(1, simd_dot(a, b)))) * 180 / .pi)
      }
      let qW = simd_length(worldCorners[1] - worldCorners[0])
      let qH = simd_length(worldCorners[3] - worldCorners[0])
      OfficialPwNativeTelemetry.shared.log("photocard_orientation", [
        "card_up_vs_live_deg": degOf(cardUp, liveUp),
        "card_right_vs_live_deg": degOf(cardRight, liveRight),
        "quad_aspect": Double(qH > 0 ? qW / qH : -1),
        "half_x": Double(halfX),
        "half_y": Double(halfY),
      ])
      let localCorners = worldCorners.map {
        SCNVector3($0.x - centroid.x, $0.y - centroid.y, $0.z - centroid.z)
      }
                                                                                
                                                                                 
                                        
      let cardName = "official_photo_card_\(OfficialAetherARKitPlugin.photoCardCounter)"
      OfficialAetherARKitPlugin.photoCardCounter += 1
      let cardThumb = OfficialAetherARKitPlugin
        .photoCardThumbByEvidencePath.removeValue(forKey: evidencePath)
      OfficialAetherARKitPlugin.photoCardSpecs[cardName] =
        PhotoCardSpec(textureCGImage: cardThumb,
                      texturePath: texturePath,
                      evidencePath: evidencePath,
                      shutterFeedbackEpochMs:
                        (args["shutterFeedbackEpochMs"] as? NSNumber)?.doubleValue,
                      addCalledEpochMs: Date().timeIntervalSince1970 * 1000.0,
                      localCorners: localCorners,
                      captureDistance: z, worldCentroid: centroid, captureCamPos: camPos)
      var anchorT = matrix_identity_float4x4
      anchorT.columns.3 = simd_float4(centroid, 1)
      let cardAnchor = ARAnchor(name: cardName, transform: anchorT)
      OfficialAetherARKitPlugin.photoCardAnchors.append(cardAnchor)
                                                           
        
                                                                                            
                                                                                          
                                                                                          
                                                                                               
                                                                     
                                                                  
        
                                                                                                        
                                                                                               
                                                                                                    
                                                                      
        
                                                                                         
                                                     
                                                                                     
                                                          
                                                                    
        
                                                                                               
                                                                                              
                                                            
                                                                                       
                                                                                             
                                                                                             
                                                                                                    
                                                                                                   
                                                                                                       
                                                                                               
                                                                                  
      let mapStatus = frame.worldMappingStatus
      let mature = true
      OfficialPwNativeTelemetry.shared.log("photocard_anchor_decision", [
        "name": cardName,
        "world_mapping_status": mapStatus.rawValue,
        "deferred": 0,
        "world_mapping_status_observed": mapStatus.rawValue,
      ])
      if mature {
        session.add(anchor: cardAnchor)
      } else {
        OfficialAetherARKitPlugin.photoCardPendingAnchors[cardName] =
          (anchor: cardAnchor, z: z, halfX: halfX, halfY: halfY,
           evidencePath: evidencePath, since: CACurrentMediaTime())
      }
      result(nil)
                                                                                            
                                                                                                        
                                                                                                
                                                                                       
                                                                                           
                                                                                 
    case "focusNudge":
      guard #available(iOS 16.0, *),
            let device =
              ARWorldTrackingConfiguration.configurableCaptureDeviceForPrimaryCamera
      else { result(false); return }
      do {
        try device.lockForConfiguration()
        if device.isFocusPointOfInterestSupported {
          device.focusPointOfInterest = CGPoint(x: 0.5, y: 0.5)
        }
        if device.isFocusModeSupported(.autoFocus) {
          device.focusMode = .autoFocus
        }
        device.unlockForConfiguration()
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.2) {
          guard let d =
                  ARWorldTrackingConfiguration.configurableCaptureDeviceForPrimaryCamera
          else { return }
          do {
            try d.lockForConfiguration()
            if d.isFocusModeSupported(.continuousAutoFocus) {
              d.focusMode = .continuousAutoFocus
            }
            d.unlockForConfiguration()
          } catch {}
        }
        NSLog("[AF-SELFHEAL] center one-shot nudge fired")
        result(true)
      } catch {
        result(false)
      }
                                                                                              
                                                                                           
                                                                                                      
                                                                                          
                                                                                              
    case "setPreviewFps":
      guard #available(iOS 16.0, *),
            let args = call.arguments as? [String: Any],
            let fps = args["fps"] as? Int, fps > 0,
            let device =
              ARWorldTrackingConfiguration.configurableCaptureDeviceForPrimaryCamera
      else { result(false); return }
      let supported = device.activeFormat.videoSupportedFrameRateRanges
        .contains { $0.minFrameRate <= Double(fps) &&
                    Double(fps) <= $0.maxFrameRate }
      guard supported else {
        NSLog("[ADAPTIVE-FPS] fps=%d out of active format ranges — refused", fps)
        result(false)
        return
      }
      do {
        try device.lockForConfiguration()
        let dur = CMTime(value: 1, timescale: CMTimeScale(fps))
        device.activeVideoMinFrameDuration = dur
        device.activeVideoMaxFrameDuration = dur
        device.unlockForConfiguration()
        NSLog("[ADAPTIVE-FPS] preview fps set to %d (in-session)", fps)
        result(true)
      } catch {
        NSLog("[ADAPTIVE-FPS] lockForConfiguration failed: \(error)")
        result(false)
      }
    case "clearPhotoCards":
      OfficialAetherARKitPlugin.clearPhotoCards(in: arSession)
      result(nil)
    case "removePhotoCard":
      guard let args = call.arguments as? [String: Any],
            let evidencePath = args["evidenceJpegPath"] as? String else {
        result(FlutterError(
          code: "bad_args",
          message: "removePhotoCard requires evidenceJpegPath",
          details: nil
        ))
        return
      }
      OfficialAetherARKitPlugin.removePhotoCard(
        evidencePath: evidencePath,
        from: arSession
      )
      result(nil)
    case "setPhotoCardStates":
                                                                           
                                                                       
                                                                                   
                                                                       
      guard let args = call.arguments as? [String: Any],
            let states = args["states"] as? [String: NSNumber] else {
        result(FlutterError(
          code: "photo_card_states_bad_args",
          message: "setPhotoCardStates requires {states: {jpegPath: Int}}",
          details: nil))
        return
      }
      OfficialAetherARKitPlugin.mergePhotoCardStates(states.mapValues { $0.intValue })
                                                                                          
      OfficialPwNativeTelemetry.shared.noteCardPush(diffCount: states.count)
      result(nil)
    case "setLogicalWorldDisplayTransform":
      guard let args = call.arguments as? [String: Any],
            let raw = args["platformWorldFromLogicalWorld"] as? [NSNumber],
            raw.count == 16 else {
        result(FlutterError(
          code: "logical_world_transform_bad_args",
          message: "setLogicalWorldDisplayTransform requires 16 values",
          details: nil))
        return
      }
      let values = raw.map { $0.floatValue }
      guard values.allSatisfy({ $0.isFinite }) else {
        result(FlutterError(
          code: "logical_world_transform_non_finite",
          message: "display transform values must be finite",
          details: nil))
        return
      }
      let transform = simd_float4x4(columns: (
        simd_float4(values[0], values[1], values[2], values[3]),
        simd_float4(values[4], values[5], values[6], values[7]),
        simd_float4(values[8], values[9], values[10], values[11]),
        simd_float4(values[12], values[13], values[14], values[15])
      ))
      OfficialAetherARKitPlugin.setCoverageCloudTransform(transform)
      result(nil)
    case "setCoveragePointCloud":
                                                                          
                                                                        
      guard let args = call.arguments as? [String: Any] else {
        result(FlutterError(
          code: "coverage_cloud_bad_args",
          message: "setCoveragePointCloud requires {xyz: Float32List, rgb: Uint8List}",
          details: nil))
        return
      }
      var xyz: [Float] = []
      if let t = args["xyz"] as? FlutterStandardTypedData {
        xyz = t.data.withUnsafeBytes { Array($0.bindMemory(to: Float32.self)) }
      }
      var rgb: [UInt8] = []
      if let t = args["rgb"] as? FlutterStandardTypedData {
        rgb = [UInt8](t.data)
      }
      let metadata = LiveCloudRenderMetadataV1(
        contract: args["diagContract"] as? String ?? "UNSTAMPED",
        sourceReceiveSequence:
          (args["diagSourceReceiveSeq"] as? NSNumber)?.intValue ?? 0,
        receiveSequence:
          (args["diagReceiveSeq"] as? NSNumber)?.intValue ?? 0,
        channelPushSequence:
          (args["diagChannelPushSeq"] as? NSNumber)?.intValue ?? 0,
        source: args["diagSource"] as? String ?? "unknown",
        publishVersion:
          (args["diagVersion"] as? NSNumber)?.intValue ?? 0,
        pointCount:
          (args["diagPointCount"] as? NSNumber)?.intValue ?? xyz.count / 3,
        receiveEpochMs:
          (args["diagReceiveEpochMs"] as? NSNumber)?.int64Value ?? 0,
        computeDoneEpochMs:
          (args["diagComputeDoneEpochMs"] as? NSNumber)?.int64Value ?? 0
      )
      OfficialAetherARKitPlugin.setCoverageCloud(
        xyz: xyz,
        rgb: rgb,
        metadata: metadata
      )
      OfficialPwNativeTelemetry.shared.log("live_cloud_native_receive_v2", [
        "contract": "PW_LIVE_CLOUD_DIAG_RUNTIME_V2_STREAM",
        "source_receive_seq": metadata.sourceReceiveSequence,
        "receive_seq": metadata.receiveSequence,
        "channel_push_seq": metadata.channelPushSequence,
        "source": metadata.source,
        "publish_version": metadata.publishVersion,
        "declared_points": metadata.pointCount,
        "received_points": xyz.count / 3,
        "observation_only": true,
      ])
      result(nil)
    case "setFeaturePointsVisible":
      let visible =
        ((call.arguments as? [String: Any])?["visible"] as? NSNumber)?.boolValue
          ?? false
      OfficialAetherARKitPlugin.featurePointsVisible = visible
      NSLog("[OfficialAetherARKit] setFeaturePointsVisible=\(visible)")
      result(nil)
    case "setPhotoCardsVisible":
      let visible =
        ((call.arguments as? [String: Any])?["visible"] as? NSNumber)?.boolValue
          ?? true
      OfficialAetherARKitPlugin.photoCardsVisible = visible
      NSLog("[OfficialAetherARKit] setPhotoCardsVisible=\(visible)")
      result(nil)
    case "telemetryCaptureBegin":
                                                                          
                                                                                           
      OfficialPwNativeTelemetry.shared.startResourceSampling()
      OfficialPwNativeTelemetry.shared.logCaptureIdentity()
                                                                                               
                                                    
      result(nil)
    case "telemetryCaptureEnd":
                                                                            
      OfficialPwNativeTelemetry.shared.stopResourceSampling()
      result(nil)
    default:
      result(FlutterMethodNotImplemented)
    }
  }

                            

                                                                                 
                                                                                      
                                                                                     
                                                                                 
                                                
  static func clearPhotoCards(in session: ARSession?) {
                                                                                        
                                                                
    photoBasisByEvidencePath.removeAll()
    photoCardThumbByEvidencePath.removeAll()
    if let session = session {
      for a in photoCardAnchors { session.remove(anchor: a) }
    }
    photoCardAnchors.removeAll()
    photoCardSpecs.removeAll()
                                                                                        
    photoCardStateLock.lock()
    photoCardStates.removeAll()
    photoCardStatesDirty = false
    photoCardStateLock.unlock()
  }

  static func removePhotoCard(evidencePath: String, from session: ARSession?) {
    let names = Set(
      photoCardSpecs.compactMap { name, spec in
        spec.evidencePath == evidencePath ? name : nil
      }
    )
    guard !names.isEmpty else { return }
    let removedAnchors = photoCardAnchors.filter {
      guard let name = $0.name else { return false }
      return names.contains(name)
    }
    if let session {
      for anchor in removedAnchors { session.remove(anchor: anchor) }
    }
    photoCardAnchors.removeAll {
      guard let name = $0.name else { return false }
      return names.contains(name)
    }
    for name in names {
      photoCardSpecs.removeValue(forKey: name)
      photoCardThumbRetries.removeValue(forKey: name)
    }
  }

  private func startSession(resetWorld: Bool = true) throws {
    NSLog("[OfficialAetherARKit] startSession(resetWorld=\(resetWorld)) — isSupported=\(ARWorldTrackingConfiguration.isSupported)")
    guard ARWorldTrackingConfiguration.isSupported else {
      throw NSError(
        domain: "OfficialAetherARKit",
        code: -1,
        userInfo: [NSLocalizedDescriptionKey:
          "ARWorldTracking not supported on this device"]
      )
    }
    let configuration = ARWorldTrackingConfiguration()
                                                                             
                                                                            
                                                                            
    configuration.isAutoFocusEnabled = true
                                                                      
                                                                  
                                                     
    configuration.worldAlignment = .gravity
                                                                                     
                                                                              
                                                                                 
                                                                               
                                                                            
                                                                                                      
    configuration.isLightEstimationEnabled = false
                                                 
                                                                  
                                                                    
                                                                  
                                                                   
                                                                
                                                                    
                                                            
      
                                                                                         
                                                                                        
                                                                                                      
                                                                                                      
                                                                                                 
                                                                                          
    configuration.planeDetection = [.horizontal]

                                                                          
      
                                               
                                                                     
                                                                     
                                                                       
                                                                         
                                                                     
                                                                
                                                                     
                                                                       
                                                                    
                                                                     
                               
      
                                                                    
                                                                   
      
                                                                          
      
                                                   
                                                                                 
                                                                                
                                                                                    
                                                                      
                                                                                 
                                                                                  
                                                                                    
                                                                              
                                                                
      
                                                                              
                                                                                 
                                                          
                                                 
    let physMemBytes = ProcessInfo.processInfo.physicalMemory
    let physMemGB = Double(physMemBytes) / (1024.0 * 1024.0 * 1024.0)
    let kFourKMemThresholdBytes: UInt64 = 5_000_000_000           
                                                                                         
                                                                                
                                                                             
                                                                                               
    let allow4K = physMemBytes >= kFourKMemThresholdBytes
      && OfficialAetherARKitPlugin.videoFormatMode != "default43"
      && OfficialAetherARKitPlugin.videoFormatMode != "hires43"
    if OfficialAetherARKitPlugin.videoFormatMode == "default43" {
      NSLog("[OfficialAetherARKit] [E24] videoFormatMode=default43 — skipping 4K override")
    }
                                                                                              
                                                                                       
                                                                                                
    if OfficialAetherARKitPlugin.videoFormatMode == "hires43", #available(iOS 16.0, *) {
      let hires = ARWorldTrackingConfiguration.supportedVideoFormats
        .filter {
          Int($0.imageResolution.width) == 1920 &&
          Int($0.imageResolution.height) == 1440 &&
          $0.isRecommendedForHighResolutionFrameCapturing
        }
        .max { $0.framesPerSecond < $1.framesPerSecond }
      guard let hires else {
        throw NSError(
          domain: "OfficialAetherARKit",
          code: 215,
          userInfo: [NSLocalizedDescriptionKey:
            "Official capture requires a high-resolution-capable 1920x1440 AR preview format"]
        )
      }
      configuration.videoFormat = hires
      NSLog("[OfficialAetherARKit] hires43 locked preview: \(hires.imageResolution) @ \(hires.framesPerSecond) fps")
    }
    if #available(iOS 16.0, *), allow4K {
      if let fourK = ARWorldTrackingConfiguration.recommendedVideoFormatFor4KResolution {
        configuration.videoFormat = fourK
        NSLog("[OfficialAetherARKit] device tier HIGH (\(String(format: "%.2f", physMemGB)) GB RAM), using 4K videoFormat: \(fourK.imageResolution) @ \(fourK.framesPerSecond) fps")
      } else {
        NSLog("[OfficialAetherARKit] device tier HIGH (\(String(format: "%.2f", physMemGB)) GB RAM) but recommendedVideoFormatFor4KResolution returned nil; using system default \(configuration.videoFormat.imageResolution)")
      }
    } else {
      let res = configuration.videoFormat.imageResolution
      NSLog("[OfficialAetherARKit] device tier LOW (\(String(format: "%.2f", physMemGB)) GB RAM), staying on default videoFormat \(res) to avoid 4K jetsam risk")
    }

                                                                                     
                                                                                                    
                                                                                                 
                                                                                             
                                                                                              
    if ProcessInfo.processInfo.environment["OFFICIAL_AETHER_AR_30FPS"] == "1",
       #available(iOS 16.0, *) {
                                                                                                  
                                                                                              
                                                                                          
                                                                        
      let want = configuration.videoFormat.imageResolution
      let needHires =
        configuration.videoFormat.isRecommendedForHighResolutionFrameCapturing
      let half = ARWorldTrackingConfiguration.supportedVideoFormats
        .filter {
          $0.imageResolution == want && $0.framesPerSecond == 30 &&
          $0.isRecommendedForHighResolutionFrameCapturing == needHires
        }
        .first
      if let half {
        configuration.videoFormat = half
        NSLog("[OfficialAetherARKit] [AR-30FPS] locked \(half.imageResolution) @ 30 fps (hires=\(needHires))")
      } else {
        NSLog("[OfficialAetherARKit] [AR-30FPS] no matching 30fps format at \(want) hires=\(needHires) — staying at \(configuration.videoFormat.framesPerSecond) fps")
      }
    }

                                                                                          
                                                                                          
                         
    do {
      var rows: [[String: Any]] = []
      for f in ARWorldTrackingConfiguration.supportedVideoFormats {
        var row: [String: Any] = [
          "width": Int(f.imageResolution.width),
          "height": Int(f.imageResolution.height),
          "fps": f.framesPerSecond,
        ]
        if #available(iOS 16.0, *) {
          row["recommendedForHighResCapture"] =
            f.isRecommendedForHighResolutionFrameCapturing
        }
        rows.append(row)
      }
      var dump: [String: Any] = [
        "mode": OfficialAetherARKitPlugin.videoFormatMode,
        "chosenWidth": Int(configuration.videoFormat.imageResolution.width),
        "chosenHeight": Int(configuration.videoFormat.imageResolution.height),
        "chosenFps": configuration.videoFormat.framesPerSecond,
        "formats": rows,
      ]
                                                                                            
                                                                                        
      if #available(iOS 16.0, *),
         let dev = ARWorldTrackingConfiguration
           .configurableCaptureDeviceForPrimaryCamera {
        dump["activeFormatFpsRanges"] =
          dev.activeFormat.videoSupportedFrameRateRanges.map {
            ["min": $0.minFrameRate, "max": $0.maxFrameRate]
          }
      }
      let docs = FileManager.default.urls(
        for: .documentDirectory, in: .userDomainMask)[0]
      let data = try JSONSerialization.data(
        withJSONObject: dump, options: [.prettyPrinted])
      try data.write(to: docs.appendingPathComponent("official_ar_video_formats.json"))
    } catch {
      NSLog("[OfficialAetherARKit] [E24] formats dump failed: \(error)")
    }

    let session = arSession ?? ARSession()
                                                                              
                                                                              
    session.delegateQueue = PwVioSensorIngress.dispatchQueue
    session.delegate = sessionDelegate
    if resetWorld {
                                                                                  
      session.run(configuration,
                  options: [.resetTracking, .removeExistingAnchors])
    } else {
                                                                                
                                                                                
                                                                                 
                                                                                  
                                                                                     
      session.run(configuration)
      OfficialAetherARKitPlugin.clearPhotoCards(in: session)
    }
    arSession = session
                                                                             
                                                                              
    PwVioTimebase.shared.resumeShadowPipeline()
                                                                        
                                                                     
    aether_gpu_match_set_capture_active(1)
    if #available(iOS 16.0, *) {
      restoreContinuousExposureFocus(
        reason: resetWorld ? "session start" : "session resume")
    }
    if resetWorld {
      worldOrigin = nil
      worldYaw = 0
      worldSubjectAnchor = nil
      lockTimeOrigin = nil
      lockTimeAnchorTransform = nil
      lastLoggedAnchorTransform = nil
      lastAnchorSeverity = nil
      lastAnchorV2Transform = nil
    }
    lastDriftLogTime = 0
    lastAnchorV2LogTime = 0
    lastFrameSnapshot = nil
    recentFrameSnapshots.removeAll()
  }

  private func stopSession() {
    PwVioTimebase.shared.suspendShadowPipeline()
    defer {
      PwARCameraLease.shared.release(
        owner: OfficialARKitIdentifiers.cameraOwner
      )
    }
    if #available(iOS 16.0, *) {
      restoreContinuousExposureFocus(reason: "session stop")
    }
    if let anchor = worldSubjectAnchor {
      arSession?.remove(anchor: anchor)
    }
                                                                                                     
    sessionDelegate.disarmStallWatchdog()
    arSession?.pause()
                                                                           
                                                                           
                                                                            
                                                                       
                                                
    aether_gpu_match_set_capture_active(0)
    worldOrigin = nil
    worldYaw = 0
    worldSubjectAnchor = nil
    lockTimeOrigin = nil
    lockTimeAnchorTransform = nil
    lastLoggedAnchorTransform = nil
    lastAnchorSeverity = nil
    lastDriftLogTime = 0
    lastAnchorV2Transform = nil
    lastAnchorV2LogTime = 0
    lastFrameSnapshot = nil
    recentFrameSnapshots.removeAll()
  }

                                                             

                                                                       
                                                                      
                                                                 
                                                                     
                                                                  
                                     
     
                                                                      
                                                                       
                                                                      
                                                                    
                                                               
                                                     
  private func lockOrigin(distanceMeters: Float) -> [String: Any]? {
    guard let frame = arSession?.currentFrame else {
      NSLog("[OfficialAetherARKit] lockOrigin: no currentFrame yet")
      return nil
    }
    switch frame.camera.trackingState {
    case .normal:
      break
    case .limited(let reason):
      NSLog("[OfficialAetherARKit] lockOrigin: tracking is .limited(\(reason)) — retrying")
      return nil
    case .notAvailable:
      NSLog("[OfficialAetherARKit] lockOrigin: tracking .notAvailable — retrying")
      return nil
    @unknown default:
      NSLog("[OfficialAetherARKit] lockOrigin: unknown trackingState — retrying")
      return nil
    }
    let t = frame.camera.transform
    let camPos = simd_float3(t.columns.3.x, t.columns.3.y, t.columns.3.z)
                                                               
                                                                       
                                                                        
                                                                       
                                               
    let forward = -simd_float3(t.columns.2.x, t.columns.2.y, t.columns.2.z)

                                                            
      
                                                                      
                                                                      
                                                                      
                                                                    
                                                                      
                                                                
                                                                     
                                                      
                                                                      
                                                                     
                                                                       
                                             
                                                                   
                                  
      
                                                                     
                                                                     
                                                                    
                       
    let subjectAnchorMaxRange: Float = 2.5
    var origin: simd_float3
    var positionSource: String
    if let session = arSession {
      var hits: [ARRaycastResult] = []
      var raycastSource: String = ""
      if #available(iOS 13.0, *) {
        let estimateQuery = ARRaycastQuery(
          origin: camPos,
          direction: simd_normalize(forward),
          allowing: .estimatedPlane,
          alignment: .any
        )
        hits = session.raycast(estimateQuery)
        if !hits.isEmpty { raycastSource = "estimated plane" }
      }
      if hits.isEmpty {
        let infQuery = ARRaycastQuery(
          origin: camPos,
          direction: simd_normalize(forward),
          allowing: .existingPlaneInfinite,
          alignment: .horizontal
        )
        hits = session.raycast(infQuery)
        if !hits.isEmpty { raycastSource = "existing horizontal plane" }
      }
      if let hit = hits.first {
        let hitPos = simd_float3(
          hit.worldTransform.columns.3.x,
          hit.worldTransform.columns.3.y,
          hit.worldTransform.columns.3.z
        )
        let hitDistance = simd_distance(camPos, hitPos)
        if hitDistance <= subjectAnchorMaxRange {
          origin = hitPos
          positionSource = "\(raycastSource) (\(String(format: "%.2f", hitDistance)) m)"
        } else {
          origin = camPos + simd_normalize(forward) * distanceMeters
          positionSource = "forward fallback (\(raycastSource) hit \(String(format: "%.2f", hitDistance)) m > cap \(subjectAnchorMaxRange) m)"
        }
      } else {
        origin = camPos + simd_normalize(forward) * distanceMeters
        positionSource = "forward fallback (no raycast hit)"
      }
    } else {
      origin = camPos + simd_normalize(forward) * distanceMeters
      positionSource = "forward fallback (no session)"
    }

                                                                    
                     
    if let oldAnchor = worldSubjectAnchor, let session = arSession {
      session.remove(anchor: oldAnchor)
      worldSubjectAnchor = nil
    }
    lockTimeAnchorTransform = nil
    lastLoggedAnchorTransform = nil
    lastAnchorSeverity = nil
    lastAnchorV2Transform = nil
    lastAnchorV2LogTime = 0

                                                                  
                                                             
                                                                   
                                                                    
                                                             
    if let session = arSession {
      var transform = matrix_identity_float4x4
      transform.columns.3 = simd_float4(origin.x, origin.y, origin.z, 1)
      let anchor = ARAnchor(name: "worldswithin_official_subject_origin",
                            transform: transform)
      session.add(anchor: anchor)
      worldSubjectAnchor = anchor
      lockTimeAnchorTransform = transform
      lastLoggedAnchorTransform = nil
      lastAnchorSeverity = nil
      lastAnchorV2Transform = nil
    }

                                                                 
                                                                       
    let relInitial = camPos - origin
    let yaw = atan2(relInitial.z, relInitial.x)

    worldOrigin = origin
    worldYaw = yaw
    lockTimeOrigin = origin
    lastDriftLogTime = 0                                            
    lastAnchorV2LogTime = 0
    logAnchorObservationV2(
      frameTimestamp: frame.timestamp,
      trackingStateName: "normal",
      currentTransform: lockTimeAnchorTransform,
      anchorState: "locked"
    )

    NSLog("[OfficialAetherARKit] lockOrigin: SUCCESS via \(positionSource) at "
      + "(\(origin.x), \(origin.y), \(origin.z))")

    if #available(iOS 16.0, *) {
      restoreContinuousExposureFocus(reason: "subject lock")
    }

    return [
      "originX": origin.x,
      "originY": origin.y,
      "originZ": origin.z,
      "worldYaw": yaw,
      "anchorTransform": Self.floatArray(
        lockTimeAnchorTransform ?? matrix_identity_float4x4
      ),
    ]
  }

                                                
    
                                                                              
                                                                          
                                                                              
                                                                                
                                                                     
    
                                                                             
                                                                            
                                        
    
                                                                 
                                                                
  @available(iOS 16.0, *)
  private func restoreContinuousExposureFocus(reason: String) {
                                                                         
                                                                         
                                                                 
                                                                    
    guard let device = ARWorldTrackingConfiguration.configurableCaptureDeviceForPrimaryCamera else {
      NSLog("[OfficialAetherARKit] camera auto restore skipped (\(reason)): configurableCaptureDeviceForPrimaryCamera is nil")
      return
    }
    do {
      try device.lockForConfiguration()
      defer { device.unlockForConfiguration() }
      if device.isSmoothAutoFocusSupported {
        device.isSmoothAutoFocusEnabled = true
      }
      if device.isExposureModeSupported(.continuousAutoExposure) {
        device.exposureMode = .continuousAutoExposure
      }
      if device.isFocusModeSupported(.continuousAutoFocus) {
        device.focusMode = .continuousAutoFocus
      }
      NSLog("[OfficialAetherARKit] camera auto restored (\(reason)): continuous exposure+focus")
    } catch {
      NSLog("[OfficialAetherARKit] camera auto restore failed (\(reason)): \(error)")
    }
  }

                                                                     
    
                                                                     
                                                                   
                                                                  
                                                                      
    
                                                                    
                                                                      
                                                                     
                                                              
                                                           
    
                                                               
                                                              
                                                                       
                                                  
  private func selectFrameSnapshot(
    targetTimestamp: TimeInterval?,
    maxTimestampDelta: TimeInterval
  ) -> (
    snapshot: LatestFrameSnapshot?,
    delta: TimeInterval?,
    errorMessage: String?
  ) {
    guard let targetTimestamp else {
      return (lastFrameSnapshot, nil, nil)
    }
    guard !recentFrameSnapshots.isEmpty else {
      return (nil, nil, "saveCurrentFrameAsJpeg: no ARFrame snapshots buffered")
    }
    var best: LatestFrameSnapshot?
    var bestDelta = TimeInterval.greatestFiniteMagnitude
    for snap in recentFrameSnapshots {
      let delta = abs(snap.timestamp - targetTimestamp)
      if delta < bestDelta {
        best = snap
        bestDelta = delta
      }
    }
    if let best, bestDelta <= maxTimestampDelta {
      return (best, bestDelta, nil)
    }
    return (
      nil,
      bestDelta,
      String(
        format: "saveCurrentFrameAsJpeg: nearest ARFrame is %.3fs from target %.6f, over max %.3fs",
        bestDelta,
        targetTimestamp,
        maxTimestampDelta
      )
    )
  }

  private func saveCurrentFrameAsJpeg(
    jpegPath: String,
    metadataPath: String,
    targetTimestamp: TimeInterval?,
    maxTimestampDelta: TimeInterval,
    quality: Float,
    metadataSchemaVersion: Int = 1,
    dartSaveContract: [String: Any]? = nil,
    includeSfmFeed: Bool = true,
    completion: @escaping ([String: Any]?, Error?) -> Void
  ) {
    let selection = selectFrameSnapshot(
      targetTimestamp: targetTimestamp,
      maxTimestampDelta: maxTimestampDelta
    )
    guard let snap = selection.snapshot else {
      completion(nil, NSError(
        domain: "OfficialAetherARKit", code: 200,
        userInfo: [NSLocalizedDescriptionKey:
          selection.errorMessage ?? "saveCurrentFrameAsJpeg: no ARFrame yet — call after lockOrigin"]
      ))
      return
    }
    jpegEncodeQueue.async { [snap, ciContext = self.ciContext] in
      do {
                                                                   
        let parent = (jpegPath as NSString).deletingLastPathComponent
        try FileManager.default.createDirectory(
          atPath: parent, withIntermediateDirectories: true
        )
        try Self.encodeCVPixelBufferAsJpeg(
          snap.pixelBuffer,
          to: URL(fileURLWithPath: jpegPath),
          quality: CGFloat(quality),
          ciContext: ciContext
        )
                                                                      
                                                                       
                                                                      
        var metadata: [String: Any] = [
          "version": metadataSchemaVersion,
          "native_role": "thin_arkit_frame_executor",
          "t": snap.timestamp,
          "image_w": snap.imageW,
          "image_h": snap.imageH,
          "extrinsic": snap.extrinsic,
          "intrinsics_fxfycxcy": snap.intrinsicsFxFyCxCy,
          "trackingStateName": snap.trackingStateName,
          "tracking_state": snap.trackingStateName,
          "is_tracking": snap.isTracking,
          "anchors_world": snap.anchorsWorld,
          "anchor_ids": snap.anchorIds.map { NSNumber(value: $0) },
          "scale_align_premetrics": [
            "anchor_depth_count": snap.scaleAlignPremetrics.anchorDepthCount,
            "anchor_depth_min_m": snap.scaleAlignPremetrics.anchorDepthMinM,
            "anchor_depth_max_m": snap.scaleAlignPremetrics.anchorDepthMaxM,
            "anchor_depth_span_m": snap.scaleAlignPremetrics.anchorDepthSpanM,
            "reliability_prior": snap.scaleAlignPremetrics.reliabilityPrior,
          ],
          "save_dt": selection.delta ?? 0.0,
        ]
        if let dartSaveContract {
          metadata["dart_save_contract"] = dartSaveContract
        }
        if let targetTimestamp {
          metadata["save_target_t"] = targetTimestamp
        }
        let json = try PWJSONSafety.data(withJSONObject: metadata)
        try json.write(to: URL(fileURLWithPath: metadataPath))
                                                                           
                                                                            
                                                                            
                                                                    
                                          
        var payload: [String: Any] = [
          "t": snap.timestamp,
          "image_w": snap.imageW,
          "image_h": snap.imageH,
          "intrinsics_fxfycxcy": snap.intrinsicsFxFyCxCy,
          "extrinsic": snap.extrinsic,
        ]
        if includeSfmFeed, let g = Self.extractGrayAspect(
          snap.pixelBuffer, maxSide: Self.sfmFeedMaxSide
        ) {
          payload["sfm_gray"] = FlutterStandardTypedData(bytes: g.data)
          payload["sfm_gray_w"] = g.width
          payload["sfm_gray_h"] = g.height
        }
        DispatchQueue.main.async { completion(payload, nil) }
      } catch {
        DispatchQueue.main.async { completion(nil, error) }
      }
    }
  }

                                                                                             
                                                                                    
                                                                       
  static func physFootprintMB() -> Double {
    var info = task_vm_info_data_t()
    var count = mach_msg_type_number_t(
      MemoryLayout<task_vm_info_data_t>.size / MemoryLayout<integer_t>.size)
    let kr = withUnsafeMutablePointer(to: &info) {
      $0.withMemoryRebound(to: integer_t.self, capacity: Int(count)) { reb in
        task_info(mach_task_self_, task_flavor_t(TASK_VM_INFO), reb, &count)
      }
    }
    return (kr == KERN_SUCCESS) ? Double(info.phys_footprint) / 1_048_576.0 : -1.0
  }

                                                                                              
                                                                                                       
                                                                      
     
                                                                               
                                                                          
  @available(iOS 26.0, *)
  private static func resolveHighResPhotoSettings(
    session: ARSession
  ) -> AVCapturePhotoSettings? {
    guard let format = session.configuration?.videoFormat else { return nil }
    let settings = format.defaultPhotoSettings
                                                                                     
                                                                                                
    var arm = "default"
    if let raw = getenv("OFFICIAL_AETHER_PHOTO_QUALITY"),
       let v = String(validatingUTF8: raw)?.lowercased(), !v.isEmpty {
      switch v {
      case "speed":
        settings.photoQualityPrioritization = .speed; arm = "speed"
      case "balanced":
        settings.photoQualityPrioritization = .balanced; arm = "balanced"
      case "quality":
        settings.photoQualityPrioritization = .quality; arm = "quality"
      default:
        arm = "default(bad env \(v))"
      }
    }
    let dims = settings.maxPhotoDimensions
                                                                                  
                                                                                               
                                                                     
    OfficialPwNativeTelemetry.shared.log("hires_settings", [
      "arm": arm,
      "quality_prioritization": settings.photoQualityPrioritization.rawValue,
      "max_photo_w": Int(dims.width),
      "max_photo_h": Int(dims.height),
    ])
    return settings
  }

  private func captureHighResolutionStill(
    highresPath: String,
    previewPath: String,
    quality: Float,
    metadataPath: String? = nil,
    targetTimestamp: TimeInterval? = nil,
    metadataSchemaVersion: Int = 1,
    dartSaveContract: [String: Any]? = nil,
    feedSfm: Bool = false,
    deriveAuxiliary: Bool = true,
    completion: @escaping ([String: Any]?, Error?) -> Void
  ) {
    guard let session = arSession else {
      completion(nil, NSError(
        domain: "OfficialAetherARKit", code: 210,
        userInfo: [NSLocalizedDescriptionKey:
          "captureHighResolutionStill: ARSession is not running"]
      ))
      return
    }
                                                                              
                                                                               
                                                                           
                             
    guard let requestFrameTimestamp = session.currentFrame?.timestamp else {
      completion(nil, NSError(
        domain: "OfficialAetherARKit", code: 214,
        userInfo: [NSLocalizedDescriptionKey:
          "captureHighResolutionStill: no ARFrame at shutter request"]
      ))
      return
    }
                                                                                      
                                                                                                
                                                                                                   
                                                                                        
                                                                                 
                                                                                           
    let kMemCeilingMB = 4000.0
    let kStillSpikeMB = 900.0
    let footprintNow = Self.physFootprintMB()
    if footprintNow + kStillSpikeMB > kMemCeilingMB {
      NSLog("[OfficialAetherARKit] [内存刹车] footprint=%.0fMB +900 尖峰将过 4000MB 天花板, 跳过本帧12MP", footprintNow)
      completion(nil, NSError(
        domain: "OfficialAetherARKit", code: 213,
        userInfo: [NSLocalizedDescriptionKey:
          "captureHighResolutionStill: skipped under memory pressure"]
      ))
      return
    }
    if #available(iOS 16.0, *) {
      let onHighResFrame: (ARFrame?, Error?) -> Void = { [weak self] frame, error in
        guard let self else { return }
        if let error {
          completion(nil, error)
          return
        }
        guard let frame else {
          completion(nil, NSError(
            domain: "OfficialAetherARKit", code: 211,
            userInfo: [NSLocalizedDescriptionKey:
              "captureHighResolutionStill: ARKit returned no frame"]
          ))
          return
        }

        let pixelBuffer = frame.capturedImage
        let timestamp = frame.timestamp
        let requestToCaptureDelta = timestamp - requestFrameTimestamp

                                                                                          
          
                                                                                  
                                                                                       
                                                                                           
                                                                                      
                                                                                        
                                                                                   
                                                                                  
                                                                       
                                                                              
                                                                                   
                                                             
          
                                                                              
                                                                                                       
                                                                                                
                                                                                                         
                                                                                    
          
                                                                                           
                                                                                                              
                                                                                                  
                                                                                       
          
                                                                                                   
                                                                                                        
                                                                                                
                                                                                         
                                                                                       
        OfficialAetherARKitPlugin.photoBasisByEvidencePath[highresPath] = (
          invView: frame.camera.viewMatrix(for: .portrait).inverse,
          transform: frame.camera.transform
        )
                                                                                              
                                                                                                       
                                                                                                     
        if let thumb = Self.makePhotoCardThumb(
          pixelBuffer, ciContext: self.ciContext
        ) {
          OfficialAetherARKitPlugin.photoCardThumbByEvidencePath[highresPath] = thumb
        }
        let capturedAtHostMs = Date().timeIntervalSince1970 * 1000.0
        DispatchQueue.main.async {
          self.methodChannel.invokeMethod(
            "highResFrameCaptured",
            arguments: [
              "evidenceJpegPath": highresPath,
              "previewJpegPath": previewPath,
              "captureTimestamp": timestamp,
              "capturedAtHostMs": capturedAtHostMs,
            ]
          )
        }

                                                                                                     
                                                                                             
                                                                                                        
                                                                                                  
                                                                                                        
                                                                                                                
          
                                                                                              
                                                                               
          
                                                                                   
                                                                                        
                                                                                   
                                                                
          
                                                                                                     
                                                                                                 
        if #available(iOS 16.0, *) {
          let exif = frame.exifData
          func num(_ key: CFString) -> Double? {
            (exif[key as String] as? NSNumber)?.doubleValue
          }
          var row: [String: Any] = [
                                                                                                  
            "jpeg": URL(fileURLWithPath: highresPath).lastPathComponent,
            "frame_t": timestamp,
          ]
          if let v = num(kCGImagePropertyExifExposureTime) { row["exposure_sec"] = v }
          if let v = num(kCGImagePropertyExifBrightnessValue) { row["brightness_ev"] = v }
          if let arr = exif[kCGImagePropertyExifISOSpeedRatings as String] as? [NSNumber],
             let first = arr.first {
            row["iso"] = first.doubleValue
          }
          if let v = num(kCGImagePropertyExifFNumber) { row["f_number"] = v }
                                                                                                           
          row["fields"] = row.count - 2
          OfficialPwNativeTelemetry.shared.log("highres_exif", row)
        }
                                                                          
                                                                                
                                                                             
                                                                               
                                                                               
        let imageWidth = CVPixelBufferGetWidth(pixelBuffer)
        let imageHeight = CVPixelBufferGetHeight(pixelBuffer)
        guard imageWidth == 4032, imageHeight == 3024 else {
          completion(nil, NSError(
            domain: "OfficialAetherARKit", code: 216,
            userInfo: [NSLocalizedDescriptionKey:
              "captureHighResolutionStill: expected 4032x3024, got \(imageWidth)x\(imageHeight)"]
          ))
          return
        }
        let transform = frame.camera.transform
        let intrinsics = frame.camera.intrinsics
        let trackingStateName = Self.trackingStateString(frame.camera.trackingState)
        let isTracking: Bool
        switch frame.camera.trackingState {
        case .normal: isTracking = true
        default: isTracking = false
        }
        let cameraTransform: [Float] = [
          transform.columns.0.x, transform.columns.0.y,
          transform.columns.0.z, transform.columns.0.w,
          transform.columns.1.x, transform.columns.1.y,
          transform.columns.1.z, transform.columns.1.w,
          transform.columns.2.x, transform.columns.2.y,
          transform.columns.2.z, transform.columns.2.w,
          transform.columns.3.x, transform.columns.3.y,
          transform.columns.3.z, transform.columns.3.w,
        ]
        let intrinsicFxFyCxCy: [Float] = [
          intrinsics[0, 0],
          intrinsics[1, 1],
          intrinsics[2, 0],
          intrinsics[2, 1],
        ]
        var anchorsWorld: [[Float]] = []
        var anchorIds: [UInt64] = []
        if let raw = frame.rawFeaturePoints {
          let n = raw.points.count
          anchorsWorld.reserveCapacity(n)
          anchorIds.reserveCapacity(n)
          for i in 0..<n {
            let p = raw.points[i]
            anchorsWorld.append([p.x, p.y, p.z])
            anchorIds.append(raw.identifiers[i])
          }
        }
        let finiteAnchors = PWJSONSafety.finitePointPairs(
          anchorsWorld,
          identifiers: anchorIds
        )
        anchorsWorld = finiteAnchors.points
        anchorIds = finiteAnchors.identifiers
        do {
          try PWJSONSafety.requireFinite(
            cameraTransform,
            field: "extrinsic"
          )
          try PWJSONSafety.requireFinite(
            intrinsicFxFyCxCy,
            field: "intrinsics_fxfycxcy"
          )
        } catch {
          completion(nil, error)
          return
        }
        let scaleAlignPremetrics = Self.computeScaleAlignPremetrics(
          cameraTransform: transform,
          anchorsWorld: anchorsWorld
        )
        let ciImage = CIImage(cvPixelBuffer: pixelBuffer)
        let ciContext = self.ciContext

        self.jpegEncodeQueue.async {
                                                                                                    
                                                                                                               
                                                                                                    
          let tStart = CACurrentMediaTime()
          var tGray = tStart, tDirs = tStart, tJpeg = tStart, tSfmGray = tStart
          do {
            let gray1024 = deriveAuxiliary
              ? Self.extractGray(
                  pixelBuffer,
                  targetSide: Self.highResQualityDownsampleSide
                )
              : nil
            let gray128 = deriveAuxiliary
              ? Self.extractGray128(pixelBuffer)
              : nil
            tGray = CACurrentMediaTime()
            try FileManager.default.createDirectory(
              atPath: (highresPath as NSString).deletingLastPathComponent,
              withIntermediateDirectories: true
            )
            if deriveAuxiliary {
              try FileManager.default.createDirectory(
                atPath: (previewPath as NSString).deletingLastPathComponent,
                withIntermediateDirectories: true
              )
            }
            if let metadataPath {
              try FileManager.default.createDirectory(
                atPath: (metadataPath as NSString).deletingLastPathComponent,
                withIntermediateDirectories: true
              )
            }
            tDirs = CACurrentMediaTime()
            try Self.encodeCVPixelBufferAsJpeg(
              pixelBuffer,
              to: URL(fileURLWithPath: highresPath),
              quality: CGFloat(quality),
              ciContext: ciContext
            )
            tJpeg = CACurrentMediaTime()

            var payload: [String: Any] = [
              "highresPath": highresPath,
              "previewPath": previewPath,
              "requestTimestamp": requestFrameTimestamp,
              "timestamp": timestamp,
              "timestampDelta": requestToCaptureDelta,
              "imageWidth": imageWidth,
              "imageHeight": imageHeight,
              "cameraTransform": cameraTransform,
              "intrinsics": intrinsicFxFyCxCy,
              "trackingStateName": trackingStateName,
              "isTracking": isTracking,
              "scaleAlignAnchorCount": scaleAlignPremetrics.anchorDepthCount,
              "scaleAlignDepthSpanM": scaleAlignPremetrics.anchorDepthSpanM,
              "scaleAlignReliabilityPrior": scaleAlignPremetrics.reliabilityPrior,
              "captureKind": "arkit_high_res_still",
              "poseSyncQuality": "ar_session_high_res_frame",
              "nativeRole": "thin_arkit_high_res_still_executor",
            ]
            if let dartSaveContract {
              payload["dartSaveContract"] = dartSaveContract
            }
            if let gray1024 {
              payload["q_gray1024"] = FlutterStandardTypedData(bytes: gray1024)
              payload["q_gray1024W"] = Self.highResQualityDownsampleSide
              payload["q_gray1024H"] = Self.highResQualityDownsampleSide
            }
            if let gray128 {
              payload["q_gray128"] = FlutterStandardTypedData(bytes: gray128)
            }
                                                                                      
                                                                                            
                                                                           
                                       
            if feedSfm, let g = Self.extractGrayAspect(
              pixelBuffer, maxSide: Self.sfmFeedMaxSide
            ) {
              payload["sfm_gray"] = FlutterStandardTypedData(bytes: g.data)
              payload["sfm_gray_w"] = g.width
              payload["sfm_gray_h"] = g.height
            }
            tSfmGray = CACurrentMediaTime()
                                                               
            DispatchQueue.main.async { completion(payload, nil) }
            let ms = { (a: CFTimeInterval, b: CFTimeInterval) in Int((b - a) * 1000.0) }
            OfficialPwNativeTelemetry.shared.log("hires_critical_path", [
              "gray_ms": ms(tStart, tGray),
              "mkdir_ms": ms(tGray, tDirs),
              "jpeg12mp_ms": ms(tDirs, tJpeg),
              "sfmgray_ms": ms(tJpeg, tSfmGray),
              "total_ms": ms(tStart, tSfmGray),
            ])
                                                                          
                                                                                    
                                                                                      
                                                                                                       
                                                                                                       
                                                                                          
                                                                                                 
                                                                                                  
            let tAfter = CACurrentMediaTime()
            do {
            if deriveAuxiliary {
              try Self.encodeCIImageAsJpeg(
                Self.makePreviewImage(from: ciImage),
                to: URL(fileURLWithPath: previewPath),
                quality: CGFloat(quality),
                ciContext: ciContext
              )
            }
            if let metadataPath {
              var metadata: [String: Any] = [
                "version": metadataSchemaVersion,
                "native_role": "thin_arkit_high_res_still_executor",
                "t": timestamp,
                "request_frame_timestamp": requestFrameTimestamp,
                "request_to_capture_dt": requestToCaptureDelta,
                "image_w": imageWidth,
                "image_h": imageHeight,
                "extrinsic": cameraTransform,
                "intrinsics_fxfycxcy": intrinsicFxFyCxCy,
                "trackingStateName": trackingStateName,
                "tracking_state": trackingStateName,
                "is_tracking": isTracking,
                "anchors_world": anchorsWorld,
                "anchor_ids": anchorIds.map { NSNumber(value: $0) },
                "scale_align_premetrics": [
                  "anchor_depth_count": scaleAlignPremetrics.anchorDepthCount,
                  "anchor_depth_min_m": scaleAlignPremetrics.anchorDepthMinM,
                  "anchor_depth_max_m": scaleAlignPremetrics.anchorDepthMaxM,
                  "anchor_depth_span_m": scaleAlignPremetrics.anchorDepthSpanM,
                  "reliability_prior": scaleAlignPremetrics.reliabilityPrior,
                ],
              ]
              if let dartSaveContract {
                metadata["dart_save_contract"] = dartSaveContract
              }
              if let targetTimestamp {
                metadata["save_target_t"] = targetTimestamp
                metadata["save_dt"] = abs(timestamp - targetTimestamp)
              } else {
                metadata["save_dt"] = 0.0
              }
              let json = try PWJSONSafety.data(withJSONObject: metadata)
              try json.write(to: URL(fileURLWithPath: metadataPath))
            }
              OfficialPwNativeTelemetry.shared.log("hires_deferred_writes", [
                "ms": Int((CACurrentMediaTime() - tAfter) * 1000.0),
              ])
            } catch {
              OfficialPwNativeTelemetry.shared.log("hires_deferred_writes", [
                "ms": Int((CACurrentMediaTime() - tAfter) * 1000.0),
                "error": String(describing: error),
              ])
            }
          } catch {
            DispatchQueue.main.async { completion(nil, error) }
          }
        }
      }
                                                                                     
        
                                                                                         
                                                                                                    
                                                                                                           
                                                                                     
                                                                                 
                                                                                          
                                                                                                
        
                                                                         
                                                                            
                                                                                                       
                                                      
        
                                                                                   
                                                                                                
                                                                                                   
                                                                     
                                                                                        
                                          
                                                                                                    
                                           
      if #available(iOS 26.0, *),
         let settings = Self.resolveHighResPhotoSettings(session: session) {
        session.captureHighResolutionFrame(using: settings,
                                           completion: onHighResFrame)
      } else {
        session.captureHighResolutionFrame(completion: onHighResFrame)
      }
    } else {
      completion(nil, NSError(
        domain: "OfficialAetherARKit", code: 212,
        userInfo: [NSLocalizedDescriptionKey:
          "captureHighResolutionStill requires iOS 16 or newer"]
      ))
    }
  }

                                                                         
                                                                     
                                                                      
                                                                            
  private static func encodeCVPixelBufferAsJpeg(
    _ buffer: CVPixelBuffer,
    to url: URL,
    quality: CGFloat,
    ciContext: CIContext
  ) throws {
    let ciImage = CIImage(cvPixelBuffer: buffer)
    guard let cgImage = ciContext.createCGImage(ciImage, from: ciImage.extent) else {
      throw NSError(
        domain: "OfficialAetherARKit", code: 201,
        userInfo: [NSLocalizedDescriptionKey:
          "encodeCVPixelBufferAsJpeg: CIContext.createCGImage failed"]
      )
    }
    guard let dest = CGImageDestinationCreateWithURL(
      url as CFURL, "public.jpeg" as CFString, 1, nil
    ) else {
      throw NSError(
        domain: "OfficialAetherARKit", code: 202,
        userInfo: [NSLocalizedDescriptionKey:
          "encodeCVPixelBufferAsJpeg: CGImageDestinationCreateWithURL failed"]
      )
    }
                                                                           
                                                                            
                                                                              
                                                                           
                                                                          
                                                                             
    let opts: [CFString: Any] = [
      kCGImageDestinationLossyCompressionQuality: quality,
      kCGImagePropertyOrientation: CGImagePropertyOrientation.right.rawValue,
    ]
    CGImageDestinationAddImage(dest, cgImage, opts as CFDictionary)
    if !CGImageDestinationFinalize(dest) {
      throw NSError(
        domain: "OfficialAetherARKit", code: 203,
        userInfo: [NSLocalizedDescriptionKey:
          "encodeCVPixelBufferAsJpeg: CGImageDestinationFinalize failed"]
      )
    }
  }

  private static func encodeCIImageAsJpeg(
    _ image: CIImage,
    to url: URL,
    quality: CGFloat,
    ciContext: CIContext
  ) throws {
    guard let cgImage = ciContext.createCGImage(image, from: image.extent) else {
      throw NSError(
        domain: "OfficialAetherARKit", code: 204,
        userInfo: [NSLocalizedDescriptionKey:
          "encodeCIImageAsJpeg: CIContext.createCGImage failed"]
      )
    }
    guard let dest = CGImageDestinationCreateWithURL(
      url as CFURL, "public.jpeg" as CFString, 1, nil
    ) else {
      throw NSError(
        domain: "OfficialAetherARKit", code: 205,
        userInfo: [NSLocalizedDescriptionKey:
          "encodeCIImageAsJpeg: CGImageDestinationCreateWithURL failed"]
      )
    }
                                                                                                    
                                                                                        
                                                                                         
                                                                                                        
                                                 
      
                                                                                           
                                                                                     
                                                                                              
                                                                                        
      
                                                                                                  
                                                                                                 
                                                                                                        
                                                                   
    CGImageDestinationAddImage(
      dest,
      cgImage,
      [
        kCGImageDestinationLossyCompressionQuality: quality,
        kCGImagePropertyOrientation: CGImagePropertyOrientation.right.rawValue,
      ] as CFDictionary
    )
    if !CGImageDestinationFinalize(dest) {
      throw NSError(
        domain: "OfficialAetherARKit", code: 206,
        userInfo: [NSLocalizedDescriptionKey:
          "encodeCIImageAsJpeg: CGImageDestinationFinalize failed"]
      )
    }
  }

                                                                                                  
                                                                                       
                                                                                                            
                                            
  private static func makePhotoCardThumb(
    _ buffer: CVPixelBuffer, ciContext: CIContext
  ) -> CGImage? {
                                                                                                   
                                                                                         
      
                                                                       
                                                                           
                                                                                                 
                                                                       
                                                                                   
                                                                                        
                                                                                                 
                                          
                                                                                            
                                                                                                    
      
                                                                                                          
                                                                                                
                                                                                                    
                                                                                    
    let ci = CIImage(cvPixelBuffer: buffer).oriented(.right)
    let maxEdge = max(ci.extent.width, ci.extent.height)
    guard maxEdge > 0 else { return nil }
    let target = CGFloat(photoCardThumbMaxPx)
    let scaled = maxEdge > target
      ? ci.transformed(by: CGAffineTransform(scaleX: target / maxEdge,
                                             y: target / maxEdge))
      : ci
    return ciContext.createCGImage(scaled, from: scaled.extent)
  }

  private static func makePreviewImage(from image: CIImage) -> CIImage {
    let maxEdge = max(image.extent.width, image.extent.height)
    guard maxEdge > 1024 else { return image }
    let scale = 1024 / maxEdge
    return image.transformed(by: CGAffineTransform(scaleX: scale, y: scale))
  }

  private static func computeScaleAlignPremetrics(
    cameraTransform: simd_float4x4,
    anchorsWorld: [[Float]]
  ) -> ScaleAlignPremetrics {
    let cameraPosition = SIMD3<Float>(
      cameraTransform.columns.3.x,
      cameraTransform.columns.3.y,
      cameraTransform.columns.3.z
    )
    let cameraZAxisWorld = SIMD3<Float>(
      cameraTransform.columns.2.x,
      cameraTransform.columns.2.y,
      cameraTransform.columns.2.z
    )

    var count = 0
    var minDepth = Float.greatestFiniteMagnitude
    var maxDepth = -Float.greatestFiniteMagnitude
    for p in anchorsWorld {
      if p.count < 3 { continue }
      let worldPoint = SIMD3<Float>(p[0], p[1], p[2])
      let delta = worldPoint - cameraPosition
                                                                          
      let depth = -simd_dot(delta, cameraZAxisWorld)
      if depth.isFinite && depth >= 0.10 && depth <= 6.0 {
        count += 1
        minDepth = min(minDepth, depth)
        maxDepth = max(maxDepth, depth)
      }
    }

    if count == 0 {
      return ScaleAlignPremetrics(
        anchorDepthCount: 0,
        anchorDepthMinM: 0,
        anchorDepthMaxM: 0,
        anchorDepthSpanM: 0,
        reliabilityPrior: 0
      )
    }

    let span = max(0, maxDepth - minDepth)
    let countScore = clamp01((Float(count) - 12.0) / 48.0)
    let spanScore = clamp01((span - 0.08) / 0.42)
    let reliability = clamp01(countScore * 0.45 + spanScore * 0.55)
    return ScaleAlignPremetrics(
      anchorDepthCount: count,
      anchorDepthMinM: minDepth,
      anchorDepthMaxM: maxDepth,
      anchorDepthSpanM: span,
      reliabilityPrior: reliability
    )
  }

  private static func clamp01(_ x: Float) -> Float {
    return min(1.0, max(0.0, x))
  }

  private static func cameraControlPayload() -> [String: Any] {
    guard #available(iOS 16.0, *),
          let device = ARWorldTrackingConfiguration.configurableCaptureDeviceForPrimaryCamera else {
      return [:]
    }
    return [
      "isAdjustingFocus": device.isAdjustingFocus,
      "isAdjustingExposure": device.isAdjustingExposure,
      "lensPosition": device.lensPosition,
      "exposureTargetOffset": device.exposureTargetOffset,
      "iso": device.iso,
      "exposureDurationSec": CMTimeGetSeconds(device.exposureDuration),
      "focusMode": "\(device.focusMode.rawValue)",
      "exposureMode": "\(device.exposureMode.rawValue)",
    ]
  }

                                                                          
                                                                             
                                              
  private func logAnchorObservationV2(
    frameTimestamp: TimeInterval,
    trackingStateName: String,
    currentTransform: simd_float4x4?,
    anchorState: String? = nil
  ) {
    guard let lockTransform = lockTimeAnchorTransform else {
      OfficialPwNativeTelemetry.shared.log("arkit_anchor_delta_v2", [
        "contract": "PW_LIVE_CLOUD_DIAG_RUNTIME_V2_STREAM",
        "frame_t": frameTimestamp,
        "anchor_state": anchorState ?? "missing_lock",
        "tracking": trackingStateName,
        "observation_only": true,
      ])
      lastAnchorV2LogTime = frameTimestamp
      return
    }
    guard let currentTransform else {
      OfficialPwNativeTelemetry.shared.log("arkit_anchor_delta_v2", [
        "contract": "PW_LIVE_CLOUD_DIAG_RUNTIME_V2_STREAM",
        "frame_t": frameTimestamp,
        "anchor_state": anchorState ?? "missing_current_anchor",
        "tracking": trackingStateName,
        "observation_only": true,
      ])
      lastAnchorV2LogTime = frameTimestamp
      return
    }

    let absolute = LiveCloudAnchorDiagnostics.delta(
      lock: lockTransform,
      current: currentTransform
    )
    let step = LiveCloudAnchorDiagnostics.delta(
      lock: lastAnchorV2Transform ?? lockTransform,
      current: currentTransform
    )
    OfficialPwNativeTelemetry.shared.log("arkit_anchor_delta_v2", [
      "contract": "PW_LIVE_CLOUD_DIAG_RUNTIME_V2_STREAM",
      "frame_t": frameTimestamp,
      "anchor_state": anchorState ?? "tracked",
      "dx_m": Double(absolute.translation.x),
      "dy_m": Double(absolute.translation.y),
      "dz_m": Double(absolute.translation.z),
      "translation_m": absolute.translationMeters,
      "qx": Double(absolute.rotation.imag.x),
      "qy": Double(absolute.rotation.imag.y),
      "qz": Double(absolute.rotation.imag.z),
      "qw": Double(absolute.rotation.real),
      "rotation_deg": absolute.rotationDegrees,
      "step_translation_m": step.translationMeters,
      "step_rotation_deg": step.rotationDegrees,
      "severity": LiveCloudAnchorDiagnostics.severity(
        translationMeters: absolute.translationMeters
      ),
      "tracking": trackingStateName,
      "observation_only": true,
    ])
    lastAnchorV2Transform = currentTransform
    lastAnchorV2LogTime = frameTimestamp
  }

                              

                                                                                      
                                                                                              
                                                                         
  private func centerRayDepthM(cameraTransform t: simd_float4x4) -> Float? {
    guard let session = arSession else { return nil }
    let camPos = simd_float3(t.columns.3.x, t.columns.3.y, t.columns.3.z)
    let forward = simd_normalize(
      -simd_float3(t.columns.2.x, t.columns.2.y, t.columns.2.z)
    )
    var hits: [ARRaycastResult] = []
    if #available(iOS 13.0, *) {
      hits = session.raycast(ARRaycastQuery(
        origin: camPos, direction: forward,
        allowing: .estimatedPlane, alignment: .any
      ))
      if hits.isEmpty {
        hits = session.raycast(ARRaycastQuery(
          origin: camPos, direction: forward,
          allowing: .existingPlaneInfinite, alignment: .horizontal
        ))
      }
    }
    guard let hit = hits.first else { return nil }
    let d = simd_distance(
      camPos,
      simd_float3(
        hit.worldTransform.columns.3.x,
        hit.worldTransform.columns.3.y,
        hit.worldTransform.columns.3.z
      )
    )
    return (d > 0.05 && d <= 2.5) ? d : nil
  }

  private func broadcast(frame: ARFrame) {
                                                                        
                                                                        
                                                                      
                                                                          
                                        
      
                                                                         
                                                                         
                                                                       
                                                                        
                                                    
    let cameraTransform = frame.camera.transform
    let cameraIntrinsics = frame.camera.intrinsics
    let extrinsicArr: [Float] = [
      cameraTransform.columns.0.x, cameraTransform.columns.0.y,
      cameraTransform.columns.0.z, cameraTransform.columns.0.w,
      cameraTransform.columns.1.x, cameraTransform.columns.1.y,
      cameraTransform.columns.1.z, cameraTransform.columns.1.w,
      cameraTransform.columns.2.x, cameraTransform.columns.2.y,
      cameraTransform.columns.2.z, cameraTransform.columns.2.w,
      cameraTransform.columns.3.x, cameraTransform.columns.3.y,
      cameraTransform.columns.3.z, cameraTransform.columns.3.w,
    ]
    let intrinsicArr: [Float] = [
      cameraIntrinsics.columns.0.x,      
      cameraIntrinsics.columns.1.y,      
      cameraIntrinsics.columns.2.x,      
      cameraIntrinsics.columns.2.y,      
    ]
    let trackingStateName = Self.trackingStateString(frame.camera.trackingState)
    let isTracking: Bool
    switch frame.camera.trackingState {
    case .normal: isTracking = true
    default: isTracking = false
    }
    let pixelBuf = frame.capturedImage
    let imgW = CVPixelBufferGetWidth(pixelBuf)
    let imgH = CVPixelBufferGetHeight(pixelBuf)
    var anchorsW: [[Float]] = []
    var anchorIds: [UInt64] = []
    if let raw = frame.rawFeaturePoints {
      let n = raw.points.count
      anchorsW.reserveCapacity(n)
      anchorIds.reserveCapacity(n)
      for i in 0..<n {
        let p = raw.points[i]
        anchorsW.append([p.x, p.y, p.z])
        anchorIds.append(raw.identifiers[i])
      }
    }
    let finiteAnchors = PWJSONSafety.finitePointPairs(
      anchorsW,
      identifiers: anchorIds
    )
    anchorsW = finiteAnchors.points
    anchorIds = finiteAnchors.identifiers
    let scaleAlignPremetrics = Self.computeScaleAlignPremetrics(
      cameraTransform: cameraTransform,
      anchorsWorld: anchorsW
    )
    let snapshot = LatestFrameSnapshot(
      pixelBuffer: pixelBuf,
      timestamp: frame.timestamp,
      extrinsic: extrinsicArr,
      intrinsicsFxFyCxCy: intrinsicArr,
      imageW: imgW,
      imageH: imgH,
      trackingStateName: trackingStateName,
      isTracking: isTracking,
      anchorsWorld: anchorsW,
      anchorIds: anchorIds,
      scaleAlignPremetrics: scaleAlignPremetrics
    )
    lastFrameSnapshot = snapshot
    recentFrameSnapshots.append(snapshot)
    if recentFrameSnapshots.count > Self.maxRecentFrameSnapshots {
      recentFrameSnapshots.removeFirst(
        recentFrameSnapshots.count - Self.maxRecentFrameSnapshots
      )
    }

                                                                             
                                                                     
                                                                       
                                                                      
                                                                         
                                                                   
                                                                  
                                                                     
                                                                       
                                                                                      
    if let myAnchor = worldSubjectAnchor,
       let updatedAnchor = frame.anchors.first(
         where: { $0.identifier == myAnchor.identifier }
       ) {
      worldSubjectAnchor = updatedAnchor
      worldOrigin = simd_float3(
        updatedAnchor.transform.columns.3.x,
        updatedAnchor.transform.columns.3.y,
        updatedAnchor.transform.columns.3.z
      )
    }

    if frame.timestamp - lastAnchorV2LogTime > 1.0 {
      logAnchorObservationV2(
        frameTimestamp: frame.timestamp,
        trackingStateName: trackingStateName,
        currentTransform: worldSubjectAnchor?.transform
      )
    }

                                                                       
                                                                          
                                      
    if let lockTransform = lockTimeAnchorTransform,
       let currentTransform = worldSubjectAnchor?.transform {
      let absolute = LiveCloudAnchorDiagnostics.delta(
        lock: lockTransform,
        current: currentTransform
      )
      let severity = LiveCloudAnchorDiagnostics.severity(
        translationMeters: absolute.translationMeters
      )
      let severityChanged = severity != lastAnchorSeverity
      if frame.timestamp - lastDriftLogTime > 1.0 || severityChanged {
        let step = LiveCloudAnchorDiagnostics.delta(
          lock: lastLoggedAnchorTransform ?? lockTransform,
          current: currentTransform
        )
        OfficialPwNativeTelemetry.shared.log("arkit_anchor_delta_v1", [
          "contract": "PW_LIVE_CLOUD_DIAG_V1_STREAM",
          "frame_t": frame.timestamp,
          "dx_m": Double(absolute.translation.x),
          "dy_m": Double(absolute.translation.y),
          "dz_m": Double(absolute.translation.z),
          "translation_m": absolute.translationMeters,
          "qx": Double(absolute.rotation.imag.x),
          "qy": Double(absolute.rotation.imag.y),
          "qz": Double(absolute.rotation.imag.z),
          "qw": Double(absolute.rotation.real),
          "rotation_deg": absolute.rotationDegrees,
          "step_translation_m": step.translationMeters,
          "step_rotation_deg": step.rotationDegrees,
          "severity": severity,
          "tracking": trackingStateName,
          "observation_only": true,
        ])
        NSLog(String(
          format: "[OfficialAetherARKit] anchor drift: %.3f m / %.2f deg (%@)",
          absolute.translationMeters,
          absolute.rotationDegrees,
          severity
        ))
        lastDriftLogTime = frame.timestamp
        lastLoggedAnchorTransform = currentTransform
        lastAnchorSeverity = severity
      }
    }

                                                       
    let q = simd_quaternion(cameraTransform)

                                                                                        
                                                                                                
                                      
    if frame.timestamp - lastCenterRayAt >= Self.centerRayInterval {
      lastCenterRayAt = frame.timestamp
      lastCenterRayDepthM = centerRayDepthM(cameraTransform: cameraTransform) ?? -1
    }

    var payload: [String: Any] = [
      "centerRayDepthM": lastCenterRayDepthM,
      "tx": cameraTransform.columns.3.x,
      "ty": cameraTransform.columns.3.y,
      "tz": cameraTransform.columns.3.z,
      "qx": q.imag.x,
      "qy": q.imag.y,
      "qz": q.imag.z,
      "qw": q.real,
      "extrinsic": extrinsicArr,
      "intrinsicFxFyCxCy": intrinsicArr,
      "isTracking": isTracking,
      "trackingStateName": trackingStateName,
      "t": frame.timestamp,
      "imageWidth": imgW,
      "imageHeight": imgH,
      "scaleAlignAnchorCount": scaleAlignPremetrics.anchorDepthCount,
      "scaleAlignDepthSpanM": scaleAlignPremetrics.anchorDepthSpanM,
      "scaleAlignReliabilityPrior": scaleAlignPremetrics.reliabilityPrior,
    ]
    if frame.timestamp - lastPreviewPointPayloadTime >= Self.previewPointInterval {
      let previewPayload = Self.makePreviewPointPayload(
        frame: frame,
        maxPoints: Self.previewPointMaxCount
      )
      if !previewPayload.isEmpty {
        payload.merge(previewPayload) { _, new in new }
      }
      lastPreviewPointPayloadTime = frame.timestamp
    }
    payload.merge(Self.cameraControlPayload()) { _, new in new }

                                                                      
                                                                   
                                                                
                                                                   
                                                               
                                                                      
                                                      
      
                                                                  
                                                                      
                                                                   
                                                                                 
                                                               
    qDiagPoseEvents += 1
    if qDiagWindowStart == 0 { qDiagWindowStart = frame.timestamp }
    if frame.timestamp - lastQualityComputeTime >= qualityInterval {
      if qualityComputeInFlight {
                                                                           
                                                                              
        qDiagSkips += 1
      } else {
        lastQualityComputeTime = frame.timestamp
        qualityComputeInFlight = true
                                                                       
                                                                    
                                                                      
        let pixelBuffer = frame.capturedImage
        let graySourceTimestamp = frame.timestamp
        let graySourceWidth = Double(CVPixelBufferGetWidth(pixelBuffer))
        let graySourceHeight = Double(CVPixelBufferGetHeight(pixelBuffer))
        let graySourceFocalX = Double(cameraIntrinsics.columns.0.x)
          * Double(Self.downsampleSide) / graySourceWidth
        let graySourceFocalY = Double(cameraIntrinsics.columns.1.y)
          * Double(Self.downsampleSide) / graySourceHeight
        let computeStart = CACurrentMediaTime()
        qualityQueue.async { [weak self] in
          let g = OfficialAetherARKitPlugin.extractGray128(pixelBuffer)
          let elapsedMs = (CACurrentMediaTime() - computeStart) * 1000
          DispatchQueue.main.async {
            guard let self = self else { return }
            self.pendingGray128 = g
            self.pendingGraySourceTimestamp = graySourceTimestamp
            self.pendingGraySourceFocalX = graySourceFocalX
            self.pendingGraySourceFocalY = graySourceFocalY
            self.qualityComputeInFlight = false
            self.qDiagFires += 1
            self.qDiagElapsedMsSum += elapsedMs
          }
        }
      }
    }
                                                                       
                                                                    
                                                                        
                                                                       
    if let g = pendingGray128 {
      payload["q_grayW"] = OfficialAetherARKitPlugin.downsampleSide
      payload["q_grayH"] = OfficialAetherARKitPlugin.downsampleSide
      payload["q_gray128"] = FlutterStandardTypedData(bytes: g)
      if let sourceTimestamp = pendingGraySourceTimestamp {
        payload["q_graySourceTimestamp"] = sourceTimestamp
      }
      if let sourceFocalX = pendingGraySourceFocalX {
        payload["q_graySourceFocalX"] = sourceFocalX
      }
      if let sourceFocalY = pendingGraySourceFocalY {
        payload["q_graySourceFocalY"] = sourceFocalY
      }
      pendingGray128 = nil
      pendingGraySourceTimestamp = nil
      pendingGraySourceFocalX = nil
      pendingGraySourceFocalY = nil
      qDiagAttached += 1
    }
                                                      
                                            
                                                                        
                                                                       
                                                                                 
    if frame.timestamp - qDiagWindowStart >= 5.0 {
      let avgMs = qDiagFires > 0 ? qDiagElapsedMsSum / Double(qDiagFires) : 0
                                                                                      
                                                                                                
                                                                          
      OfficialPwNativeTelemetry.shared.log("quality_window", [
        "target_hz": 1.0 / qualityInterval,
        "arm_index": qualityArmIndex % Self.qualityHzArms.count,
        "arms": Self.qualityHzArms.count,
        "fires": qDiagFires,
        "skips": qDiagSkips,
        "avg_compute_ms": avgMs,
        "attached": qDiagAttached,
        "pose_events": qDiagPoseEvents,
      ])
      NSLog(String(
        format: "[OfficialAetherARKit] 5s quality: fires=%d skips=%d avgMs=%.1f attached=%d/%d",
        qDiagFires, qDiagSkips, avgMs, qDiagAttached, qDiagPoseEvents
      ))
                                                                                       
      qualityArmIndex += 1
      qDiagWindowStart = frame.timestamp
      qDiagFires = 0
      qDiagSkips = 0
      qDiagElapsedMsSum = 0
      qDiagAttached = 0
      qDiagPoseEvents = 0
    }
                                                                 
                                                                  
                                                                     
    if let origin = worldOrigin {
      payload["worldOriginX"] = origin.x
      payload["worldOriginY"] = origin.y
      payload["worldOriginZ"] = origin.z
      payload["worldYaw"] = worldYaw
      payload["hasOrigin"] = true
      payload["anchorTransform"] = Self.floatArray(
        worldSubjectAnchor?.transform ?? matrix_identity_float4x4
      )
    } else {
      payload["worldOriginX"] = Float(0)
      payload["worldOriginY"] = Float(0)
      payload["worldOriginZ"] = Float(0)
      payload["worldYaw"] = Float(0)
      payload["hasOrigin"] = false
      payload["anchorTransform"] = [Float]()
    }

    poseStreamHandler.send(payload)
  }
}

                                                                       

@available(iOS 11.0, *)
extension OfficialAetherARKitPlugin {
                                                        
                                                             
  static let downsampleSide = 128
  static let highResQualityDownsampleSide = 1024

                                                                           
                                                                          
                                                                        
                                                                           
                                  
     
                                                                       
                                                                       
                                                                          
                                                                          
                                                                        
                                                                              
                                                                                
                                                                              
     
                                                                             
                                                                              
                                                                      
                                                                                
                                                                         
                                                                              
                                                                              
                                                                             
                                                                            
                                                                            
  static let sfmFeedMaxSide = 4224

                                                                                
                                                                                   
                                                                                                
                                                                            
  static let arSplatMaxScreenRadius: CGFloat = {
    if let raw = ProcessInfo.processInfo.environment["OFFICIAL_AETHER_AR_SPLAT_MAX_PX"],
       let v = Double(raw), v >= 2, v <= 200 {
      return CGFloat(v)
    }
    return 6
  }()

                                                                
                                                                      
                                                                       
                                                                 
     
                                                           
                                                                      
                                                               
                                                        
  static func trackingStateString(_ state: ARCamera.TrackingState) -> String {
    switch state {
    case .normal:
      return "normal"
    case .notAvailable:
      return "not_available"
    case .limited(let reason):
      switch reason {
      case .initializing: return "limited_initializing"
      case .relocalizing: return "limited_relocalizing"
      case .excessiveMotion: return "limited_excessive_motion"
      case .insufficientFeatures: return "limited_insufficient_features"
      @unknown default: return "limited_unknown"
      }
    }
  }

                                                                     
                                                            
     
                                                                       
                                                                   
                                                                     
                                                                  
                                                                    
                                                                     
                  
     
                                                                    
                                                                     
                                                                  
                                                                 
     
                                                                   
                                                                   
                                    
  static func extractGray128(_ pixelBuffer: CVPixelBuffer) -> Data? {
    return extractGray(pixelBuffer, targetSide: downsampleSide)
  }

  static func extractGray(
    _ pixelBuffer: CVPixelBuffer,
    targetSide: Int
  ) -> Data? {
    let format = CVPixelBufferGetPixelFormatType(pixelBuffer)
    let isYUV =
      format == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange ||
      format == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
    guard isYUV, targetSide > 0 else { return nil }

    CVPixelBufferLockBaseAddress(pixelBuffer, .readOnly)
    defer { CVPixelBufferUnlockBaseAddress(pixelBuffer, .readOnly) }

    let width = CVPixelBufferGetWidthOfPlane(pixelBuffer, 0)
    let height = CVPixelBufferGetHeightOfPlane(pixelBuffer, 0)
    let rowStride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0)
    guard let baseAddr = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0)
    else { return nil }
    let src = baseAddr.assumingMemoryBound(to: UInt8.self)

    let tw = targetSide
    let th = targetSide
    var data = Data(count: tw * th)
    data.withUnsafeMutableBytes { (raw: UnsafeMutableRawBufferPointer) in
      let dst = raw.bindMemory(to: UInt8.self).baseAddress!
                                                                      
                                                                    
                                                                   
                                     
      let sxFixed = (width << 16) / tw
      let syFixed = (height << 16) / th
      for dy in 0..<th {
        let srcY = (dy * syFixed) >> 16
        let srcRowOffset = srcY * rowStride
        let dstRowOffset = dy * tw
        for dx in 0..<tw {
          let srcX = (dx * sxFixed) >> 16
          dst[dstRowOffset + dx] = src[srcRowOffset + srcX]
        }
      }
    }
    return data
  }

                                                                            
                                                                           
                                                                              
                                                                             
                                                                 
                                                                           
                                           
  static func extractGrayAspect(
    _ pixelBuffer: CVPixelBuffer,
    maxSide: Int
  ) -> (data: Data, width: Int, height: Int)? {
    let format = CVPixelBufferGetPixelFormatType(pixelBuffer)
    let isYUV =
      format == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange ||
      format == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
    guard isYUV, maxSide > 0 else { return nil }

    CVPixelBufferLockBaseAddress(pixelBuffer, .readOnly)
    defer { CVPixelBufferUnlockBaseAddress(pixelBuffer, .readOnly) }

    let width = CVPixelBufferGetWidthOfPlane(pixelBuffer, 0)
    let height = CVPixelBufferGetHeightOfPlane(pixelBuffer, 0)
    guard width > 0, height > 0 else { return nil }
    let rowStride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0)
    guard let baseAddr = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0)
    else { return nil }
    let src = baseAddr.assumingMemoryBound(to: UInt8.self)

    let longEdge = max(width, height)
                                          
    let scaleNum = min(maxSide, longEdge)
    let tw = max(1, width * scaleNum / longEdge)
    let th = max(1, height * scaleNum / longEdge)
    var data = Data(count: tw * th)
    data.withUnsafeMutableBytes { (raw: UnsafeMutableRawBufferPointer) in
      let dst = raw.bindMemory(to: UInt8.self).baseAddress!
                                                                            
                                                                               
                                                                               
                                                                               
                                                                                
                                                                               
                                                                                 
                                                                              
                                                                                
                                                                                  
                                                                         
                                                                              
                                                    
      for dy in 0..<th {
        let sy0 = dy * height / th
        var sy1 = (dy + 1) * height / th
        if sy1 <= sy0 { sy1 = sy0 + 1 }
        let dstRowOffset = dy * tw
        for dx in 0..<tw {
          let sx0 = dx * width / tw
          var sx1 = (dx + 1) * width / tw
          if sx1 <= sx0 { sx1 = sx0 + 1 }
          var sum = 0
          var cnt = 0
          var sy = sy0
          while sy < sy1 {
            let rowOff = sy * rowStride
            var sx = sx0
            while sx < sx1 {
              sum += Int(src[rowOff + sx])
              cnt += 1
              sx += 1
            }
            sy += 1
          }
          dst[dstRowOffset + dx] = UInt8(sum / cnt)
        }
      }

                                                                             
                                                                             
                                                                       
                                                                        
                                                                          
                                                                        
                                                                            
                                                             
      let n = tw * th
      var hist = [Int](repeating: 0, count: 256)
      for i in 0..<n { hist[Int(dst[i])] += 1 }
      let lowCount = n / 50             
      let highCount = n - n / 50         
      var acc = 0
      var p2 = 0
      var p98 = 255
      for v in 0..<256 {
        acc += hist[v]
        if acc <= lowCount { p2 = v }
        if acc < highCount { p98 = v }
      }
      let span = max(32, p98 - p2)                     
      if p2 > 0 || span < 250 {
        var lut = [UInt8](repeating: 0, count: 256)
        for v in 0..<256 {
          let stretched = (v - p2) * 255 / span
          lut[v] = UInt8(min(255, max(0, stretched)))
        }
        for i in 0..<n { dst[i] = lut[Int(dst[i])] }
      }
    }
    return (data, tw, th)
  }

                                                                     
                                                                      
                                                                     
                                                                     
                                  
  static func makePreviewPointPayload(
    frame: ARFrame,
    maxPoints: Int
  ) -> [String: Any] {
    guard maxPoints > 0, let raw = frame.rawFeaturePoints else {
      return [:]
    }
    let rawCount = raw.points.count
    guard rawCount > 0 else { return [:] }

    let pixelBuffer = frame.capturedImage
    let width = CVPixelBufferGetWidth(pixelBuffer)
    let height = CVPixelBufferGetHeight(pixelBuffer)
    guard width > 0, height > 0 else { return [:] }

    let step = max(1, rawCount / maxPoints)
    let viewport = CGSize(width: width, height: height)
    var xyz: [Float] = []
    var rgb: [Int] = []
    var confidence: [Float] = []
    xyz.reserveCapacity(min(maxPoints, rawCount) * 3)
    rgb.reserveCapacity(min(maxPoints, rawCount) * 3)
    confidence.reserveCapacity(min(maxPoints, rawCount))

    CVPixelBufferLockBaseAddress(pixelBuffer, .readOnly)
    defer { CVPixelBufferUnlockBaseAddress(pixelBuffer, .readOnly) }

    for i in Swift.stride(from: 0, to: rawCount, by: step) {
      if confidence.count >= maxPoints { break }
      let p = raw.points[i]
      let projected = frame.camera.projectPoint(
        p,
        orientation: .landscapeRight,
        viewportSize: viewport
      )
      let x = Int(projected.x.rounded())
      let y = Int(projected.y.rounded())
      guard x >= 0, y >= 0, x < width, y < height else { continue }
      guard let color = sampleYuvRgbLocked(pixelBuffer, x: x, y: y) else {
        continue
      }
      xyz.append(p.x)
      xyz.append(p.y)
      xyz.append(p.z)
      rgb.append(Int(color.r))
      rgb.append(Int(color.g))
      rgb.append(Int(color.b))
      confidence.append(1.0)
    }

    if confidence.isEmpty { return [:] }
    return [
      "previewPointXYZ": xyz,
      "previewPointRGB": rgb,
      "previewPointConfidence": confidence,
      "previewPointSource": "arkit_rawFeaturePoints_voxel_preview",
    ]
  }

  private static func sampleYuvRgbLocked(
    _ pixelBuffer: CVPixelBuffer,
    x: Int,
    y: Int
  ) -> (r: UInt8, g: UInt8, b: UInt8)? {
    let format = CVPixelBufferGetPixelFormatType(pixelBuffer)
    let isYUV =
      format == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange ||
      format == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
    guard isYUV else { return nil }

    let yWidth = CVPixelBufferGetWidthOfPlane(pixelBuffer, 0)
    let yHeight = CVPixelBufferGetHeightOfPlane(pixelBuffer, 0)
    let uvWidth = CVPixelBufferGetWidthOfPlane(pixelBuffer, 1)
    let uvHeight = CVPixelBufferGetHeightOfPlane(pixelBuffer, 1)
    guard x >= 0, y >= 0, x < yWidth, y < yHeight else { return nil }

    guard
      let yBase = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0),
      let uvBase = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1)
    else {
      return nil
    }

    let yStride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0)
    let uvStride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1)
    let yPtr = yBase.assumingMemoryBound(to: UInt8.self)
    let uvPtr = uvBase.assumingMemoryBound(to: UInt8.self)

    let uvX = min(max(x / 2, 0), max(uvWidth - 1, 0))
    let uvY = min(max(y / 2, 0), max(uvHeight - 1, 0))
    let yValue = Float(yPtr[y * yStride + x])
    let uvIndex = uvY * uvStride + uvX * 2
    let cb = Float(uvPtr[uvIndex]) - 128.0
    let cr = Float(uvPtr[uvIndex + 1]) - 128.0

    let r = yValue + 1.402 * cr
    let g = yValue - 0.344136 * cb - 0.714136 * cr
    let b = yValue + 1.772 * cb
    return (
      r: clampRgb(r),
      g: clampRgb(g),
      b: clampRgb(b)
    )
  }

  private static func clampRgb(_ value: Float) -> UInt8 {
    return UInt8(max(0, min(255, Int(value.rounded()))))
  }


                                                                               
                                                                                  
                                                                             
  private func handleDecodeJpegForColor(
    call: FlutterMethodCall,
    result: @escaping FlutterResult
  ) {
    guard let args = call.arguments as? [String: Any],
          let path = args["jpegPath"] as? String else {
      result(FlutterError(code: "ar_decode_bad_args",
                          message: "decodeJpegForColor requires {jpegPath}",
                          details: nil))
      return
    }
    let maxPx = (args["maxPx"] as? Int) ?? 1280
                                                                             
                                                         
    let opts: [CFString: Any] = [
      kCGImageSourceCreateThumbnailFromImageAlways: true,
      kCGImageSourceCreateThumbnailWithTransform: false,
      kCGImageSourceThumbnailMaxPixelSize: maxPx,
    ]
    guard let src = CGImageSourceCreateWithURL(
            URL(fileURLWithPath: path) as CFURL, nil),
          let cg = CGImageSourceCreateThumbnailAtIndex(
            src, 0, opts as CFDictionary) else {
      result(FlutterError(code: "ar_decode_failed",
                          message: "thumbnail decode failed: \(path)",
                          details: nil))
      return
    }
    let w = cg.width, h = cg.height
    guard w > 0, h > 0 else {
      result(FlutterError(code: "ar_decode_empty", message: "0-size",
                          details: nil))
      return
    }
                                                                           
    var rgba = [UInt8](repeating: 0, count: w * h * 4)
    let cs = CGColorSpaceCreateDeviceRGB()
    let ok: Bool = rgba.withUnsafeMutableBytes { buf -> Bool in
      guard let ctx = CGContext(
              data: buf.baseAddress, width: w, height: h, bitsPerComponent: 8,
              bytesPerRow: w * 4, space: cs,
              bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue) else {
        return false
      }
      ctx.draw(cg, in: CGRect(x: 0, y: 0, width: w, height: h))
      return true
    }
    guard ok else {
      result(FlutterError(code: "ar_decode_ctx", message: "CGContext failed",
                          details: nil))
      return
    }
    var rgb = Data(count: w * h * 3)
    rgb.withUnsafeMutableBytes { (d: UnsafeMutableRawBufferPointer) in
      let dst = d.bindMemory(to: UInt8.self).baseAddress!
      var si = 0, di = 0
      let px = w * h
      for _ in 0..<px {
        dst[di] = rgba[si]; dst[di + 1] = rgba[si + 1]; dst[di + 2] = rgba[si + 2]
        si += 4; di += 3
      }
    }
    result(["w": w, "h": h, "rgb": FlutterStandardTypedData(bytes: rgb)])
  }
}

                                   

@available(iOS 11.0, *)
private class OfficialPoseStreamHandler: NSObject, FlutterStreamHandler {
  private var sink: FlutterEventSink?

  func onListen(
    withArguments arguments: Any?,
    eventSink events: @escaping FlutterEventSink
  ) -> FlutterError? {
    self.sink = events
    return nil
  }

  func onCancel(withArguments arguments: Any?) -> FlutterError? {
    self.sink = nil
    return nil
  }

  func send(_ payload: [String: Any]) {
                                                                     
                                                              
                                       
    if Thread.isMainThread {
      sink?(payload)
    } else {
      DispatchQueue.main.async { [weak self] in
        self?.sink?(payload)
      }
    }
  }
}

                                      
  
                                                                      
                                                                                   
                                                                      
                           

@available(iOS 11.0, *)
private class OfficialARSessionForwarder: NSObject, ARSessionDelegate {
  var onFrame: ((ARFrame) -> Void)?
  var onSessionFailure: (() -> Void)?

                                                                   
  private var loggedFirstFrame = false
  private var lastTrackingDescription: String = ""

                                                                                                                         
                                                                                        
                                                                                           
                                                                                          
                                                                                   
                                                                               
                                                                                           
                                                                                          
  private let stallLock = NSLock()
  private var lastFrameAt: CFTimeInterval = 0
  private var stalledSince: CFTimeInterval = 0                    
  private var lastStallLogAt: CFTimeInterval = 0
  private var interrupted = false
  private var watchdog: DispatchSourceTimer?
  private let watchdogQueue = DispatchQueue(
    label: "com.worldswithin.official.arframe.watchdog",
    qos: .utility
  )

  private func startWatchdogIfNeeded() {
    watchdogQueue.async { [weak self] in
      guard let self = self, self.watchdog == nil else { return }
      let timer = DispatchSource.makeTimerSource(queue: self.watchdogQueue)
      timer.schedule(deadline: .now() + 1, repeating: 1)
      timer.setEventHandler { [weak self] in self?.watchdogTick() }
      timer.resume()
      self.watchdog = timer
    }
  }

                                                                                   
                                                                                        
                                                                                 
                                                                                    
                                                                                  
  func disarmStallWatchdog() {
    stallLock.lock()
    lastFrameAt = 0
    stalledSince = 0
    lastStallLogAt = 0
    stallLock.unlock()
    NSLog("[OfficialAetherARKit] frame-stall watchdog disarmed (session stopped)")
  }

  private func watchdogTick() {
    let now = CACurrentMediaTime()
    stallLock.lock()
    let last = lastFrameAt
    let wasStalled = stalledSince > 0
    let isInterrupted = interrupted
    var emit: (type: String, gapMs: Int)? = nil
    if last > 0, now - last > 1.0 {
      if !wasStalled {
        stalledSince = last
        lastStallLogAt = now
        emit = ("ar_frame_stall", Int((now - last) * 1000))
      } else if now - lastStallLogAt >= 10.0 {
        lastStallLogAt = now
        emit = ("ar_frame_stall", Int((now - last) * 1000))
      }
    }
    stallLock.unlock()
    if let e = emit {
      OfficialPwNativeTelemetry.shared.log(e.type, [
        "gap_ms": e.gapMs,
        "thermal": ProcessInfo.processInfo.thermalState.rawValue,
        "interrupted": isInterrupted,
      ])
    }
  }

  func session(_ session: ARSession, didUpdate frame: ARFrame) {
                                                                                                      
                                                                         
                                                                                                 
                                                                            
                                                                                            
    PwVioTimebase.shared.noteARFrame(frame)
                                                                                   
                                                                                
    _ = PwVioSlamFeeder.shared.enqueue(frame: frame)
    if !loggedFirstFrame {
      loggedFirstFrame = true
      NSLog("[OfficialAetherARKit] first ARFrame received")
      startWatchdogIfNeeded()
    }
                                                                                                 
    let now = CACurrentMediaTime()
    stallLock.lock()
    let stalledFrom = stalledSince
    lastFrameAt = now
    stalledSince = 0
    stallLock.unlock()
    if stalledFrom > 0 {
      OfficialPwNativeTelemetry.shared.log("ar_frame_resume", [
        "gap_ms": Int((now - stalledFrom) * 1000),
        "thermal": ProcessInfo.processInfo.thermalState.rawValue,
      ])
    }
    let desc: String
    switch frame.camera.trackingState {
    case .normal: desc = "normal"
    case .limited(let r): desc = "limited(\(r))"
    case .notAvailable: desc = "notAvailable"
    @unknown default: desc = "unknown"
    }
    if desc != lastTrackingDescription {
      lastTrackingDescription = desc
      NSLog("[OfficialAetherARKit] trackingState → \(desc)")
    }
    onFrame?(frame)
  }

  func session(_ session: ARSession, didFailWithError error: Error) {
    PwVioTimebase.shared.suspendShadowPipeline()
    NSLog("[OfficialAetherARKit] ARSession failed: \(error.localizedDescription)")
    OfficialPwNativeTelemetry.shared.log("ar_session_failed", [
      "error": error.localizedDescription,
      "thermal": ProcessInfo.processInfo.thermalState.rawValue,
    ])
    onSessionFailure?()
  }

  func sessionWasInterrupted(_ session: ARSession) {
    PwVioTimebase.shared.suspendShadowPipeline()
    NSLog("[OfficialAetherARKit] ARSession interrupted")
    stallLock.lock()
    interrupted = true
    stallLock.unlock()
    OfficialPwNativeTelemetry.shared.log("ar_interrupted", [
      "thermal": ProcessInfo.processInfo.thermalState.rawValue,
    ])
  }

  func sessionInterruptionEnded(_ session: ARSession) {
    NSLog("[OfficialAetherARKit] ARSession interruption ended")
    stallLock.lock()
    interrupted = false
    stallLock.unlock()
    OfficialPwNativeTelemetry.shared.log("ar_interruption_ended", [
      "thermal": ProcessInfo.processInfo.thermalState.rawValue,
    ])
    PwVioTimebase.shared.resumeShadowPipeline()
  }

  deinit {
    watchdog?.cancel()
  }
}

                                                        
                                                                                   
  
                                                                     
                                                                   
                                                                    
                                                                    
                                                                               

@available(iOS 11.0, *)
class OfficialAetherARKitPreviewFactory: NSObject, FlutterPlatformViewFactory {
  private let getSession: () -> ARSession?

  init(getSession: @escaping () -> ARSession?) {
    self.getSession = getSession
    super.init()
  }

  func create(
    withFrame frame: CGRect,
    viewIdentifier viewId: Int64,
    arguments args: Any?
  ) -> FlutterPlatformView {
    return OfficialAetherARKitPreviewView(frame: frame, getSession: getSession)
  }

  func createArgsCodec() -> FlutterMessageCodec & NSObjectProtocol {
    return FlutterStandardMessageCodec.sharedInstance()
  }
}

                                       
   
                                                                       
                                                                         
                                                                             
                                                                              
                                                                              
private final class CapturePointCloudLodController {
  private let fractions = [1.0, 0.67, 0.40, 0.25]
  private var tier = 0
  private var lastFrameTime: TimeInterval?
  private var smoothedFrameSeconds = 1.0 / 30.0
  private var lastDecisionTime: TimeInterval = 0
  private var healthySince: TimeInterval?

  func reset(totalPoints: Int) -> Int {
    lastFrameTime = nil
    healthySince = nil
                                                             
    if totalPoints <= 24_000 { tier = 0 }
    return renderCount(totalPoints: totalPoints)
  }

  func renderCount(totalPoints: Int) -> Int {
    guard totalPoints > 0 else { return 0 }
    if totalPoints <= 24_000 { return totalPoints }
    let fraction = fractions[min(tier, fractions.count - 1)]
    return min(totalPoints, max(12_000, Int((Double(totalPoints) * fraction).rounded(.up))))
  }

                                                                           
  func observeFrame(time: TimeInterval, totalPoints: Int) -> Int? {
    defer { lastFrameTime = time }
    guard totalPoints > 24_000, let previous = lastFrameTime else { return nil }
    let dt = time - previous
    guard dt > 0, dt < 0.25 else { return nil }
    smoothedFrameSeconds = smoothedFrameSeconds * 0.92 + dt * 0.08
    guard time - lastDecisionTime >= 2.0 else { return nil }
    lastDecisionTime = time

    let fps = 1.0 / max(smoothedFrameSeconds, 1.0 / 120.0)
    let thermal = ProcessInfo.processInfo.thermalState
    let oldTier = tier

    switch thermal {
    case .critical:
      tier = max(tier, 3)
      healthySince = nil
    case .serious:
      tier = max(tier, 2)
      healthySince = nil
    case .nominal, .fair:
      if fps < 24.0 {
        tier = min(tier + 1, fractions.count - 1)
        healthySince = nil
      } else if fps >= 28.5 {
        if healthySince == nil { healthySince = time }
                                                                               
        if tier > 0, time - (healthySince ?? time) >= 8.0 {
          tier -= 1
          healthySince = time
        }
      } else {
        healthySince = nil
      }
    @unknown default:
      healthySince = nil
    }

    guard tier != oldTier else { return nil }
    let count = renderCount(totalPoints: totalPoints)
    OfficialPwNativeTelemetry.shared.log("capture_point_lod", [
      "from_tier": oldTier,
      "to_tier": tier,
      "render_points": count,
      "source_points": totalPoints,
      "fps_ewma": (fps * 10).rounded() / 10,
      "thermal": thermal.rawValue,
    ])
    return count
  }
}

@available(iOS 11.0, *)
class OfficialAetherARKitPreviewView: NSObject, FlutterPlatformView, ARSCNViewDelegate {
  private let arscnView: ARSCNView
  private let getSession: () -> ARSession?
  private var pollTimer: Timer?

                                                                                            
    
                                                                  
                                                                     
                                                    
                                                                   
                                                                 
                                                         
                                                                    
                                                                     
                                                               
    
                                                                       
                                                                
                                                          
  private static let subjectMarkerRadius: CGFloat = 0.03        
  private static let subjectAnchorName = "worldswithin_official_subject_origin"

                                                                                                                                                                
                                                               
  private var photoCardNodes: [String: SCNNode] = [:]
                                                                                            
                                                                  
  private static let photoCardDistanceAnchorM: Float = 1.0
                                                                                       
                                                                                    
  private static let photoCardDistanceBeta: Float = 0.5

                                                                                   
                                                                                 
                                 
  private var photoCardStateMats: [String: [SCNMaterial]] = [:]

                                                                           
                                                                                           
  private static func photoCardStateColor(_ state: Int) -> UIColor {
    switch state {
    case 1: return .white
    case 2: return .systemRed
    case 3: return .systemYellow
    default: return .black
    }
  }

                                                                                                     
                                                                              
                                                                             
                                                             
                                                                 
  private var pointCloudNode: SCNNode?
  private var fullPointCloudXyz: [Float] = []
  private var fullPointCloudRgb: [UInt8] = []
  private var pointCloudTransform = matrix_identity_float4x4
  private let pointCloudLod = CapturePointCloudLodController()
  private var renderedPointCount = 0
  private var liveCloudRenderApplySequence = 0

                                                                           
                                         
  private func updateFeaturePointOverlay(at time: TimeInterval) {
    if !OfficialAetherARKitPlugin.featurePointsVisible {
      if pointCloudNode != nil {
        pointCloudNode?.removeFromParentNode()
        pointCloudNode = nil
      }
      renderedPointCount = 0
      if let dropped = OfficialAetherARKitPlugin.takeCoverageCloudIfDirty() {
        pointCloudTransform = dropped.transform
        let m = dropped.metadata
        OfficialPwNativeTelemetry.shared.log("live_cloud_render_drop_v1", [
          "contract": m.contract,
          "source_receive_seq": m.sourceReceiveSequence,
          "receive_seq": m.receiveSequence,
          "channel_push_seq": m.channelPushSequence,
          "source": m.source,
          "publish_version": m.publishVersion,
          "points": m.pointCount,
          "reason": "hidden",
          "observation_only": true,
        ])
      }
      return
    }
    if let cloud = OfficialAetherARKitPlugin.takeCoverageCloudIfDirty() {
      fullPointCloudXyz = cloud.xyz
      fullPointCloudRgb = cloud.rgb
      pointCloudTransform = cloud.transform
      let totalPoints = fullPointCloudXyz.count / 3
      renderedPointCount = pointCloudLod.reset(totalPoints: totalPoints)
      rebuildPointCloud(renderCount: renderedPointCount)
      liveCloudRenderApplySequence += 1
      let m = cloud.metadata
      OfficialPwNativeTelemetry.shared.log("live_cloud_render_v1", [
        "contract": m.contract,
        "source_receive_seq": m.sourceReceiveSequence,
        "receive_seq": m.receiveSequence,
        "channel_push_seq": m.channelPushSequence,
        "render_apply_seq": liveCloudRenderApplySequence,
        "source": m.source,
        "publish_version": m.publishVersion,
        "declared_points": m.pointCount,
        "received_points": totalPoints,
        "rendered_points": renderedPointCount,
        "receive_t": m.receiveEpochMs,
        "compute_done_t": m.computeDoneEpochMs,
        "observation_only": true,
      ])
      OfficialPwNativeTelemetry.shared.log("live_cloud_render_v2", [
        "contract": "PW_LIVE_CLOUD_DIAG_RUNTIME_V2_STREAM",
        "source_receive_seq": m.sourceReceiveSequence,
        "receive_seq": m.receiveSequence,
        "channel_push_seq": m.channelPushSequence,
        "render_apply_seq": liveCloudRenderApplySequence,
        "source": m.source,
        "publish_version": m.publishVersion,
        "declared_points": m.pointCount,
        "received_points": totalPoints,
        "rendered_points": renderedPointCount,
        "receive_t": m.receiveEpochMs,
        "compute_done_t": m.computeDoneEpochMs,
        "observation_only": true,
      ])
    }
    let totalPoints = fullPointCloudXyz.count / 3
    if let nextCount = pointCloudLod.observeFrame(
      time: time,
      totalPoints: totalPoints
    ), nextCount != renderedPointCount {
      renderedPointCount = nextCount
      rebuildPointCloud(renderCount: nextCount)
    }
  }

  private func rebuildPointCloud(renderCount: Int) {
    let available = min(fullPointCloudXyz.count / 3, fullPointCloudRgb.count / 3)
    let n = min(max(0, renderCount), available)
    guard n > 0 else {
      pointCloudNode?.removeFromParentNode()
      pointCloudNode = nil
      return
    }
                                                                          
    let xyz = Array(fullPointCloudXyz.prefix(renderCount * 3))
    let rgb = Array(fullPointCloudRgb.prefix(renderCount * 3))
    var verts: [SCNVector3] = []
    var colors: [SIMD4<Float>] = []
    verts.reserveCapacity(n)
    colors.reserveCapacity(n)
    for i in 0..<n {
      verts.append(SCNVector3(xyz[i * 3], xyz[i * 3 + 1], xyz[i * 3 + 2]))
      colors.append(SIMD4<Float>(
        Float(rgb[i * 3]) / 255.0,
        Float(rgb[i * 3 + 1]) / 255.0,
        Float(rgb[i * 3 + 2]) / 255.0,
        1.0))
    }
    let vSource = SCNGeometrySource(vertices: verts)
    let cData = colors.withUnsafeBytes { Data($0) }
    let cSource = SCNGeometrySource(
      data: cData, semantic: .color, vectorCount: colors.count,
      usesFloatComponents: true, componentsPerVector: 4,
      bytesPerComponent: MemoryLayout<Float>.size, dataOffset: 0,
      dataStride: MemoryLayout<SIMD4<Float>>.stride)
    let element = SCNGeometryElement(
      indices: (0..<verts.count).map { Int32($0) }, primitiveType: .point)
                                                                                      
                                                                                               
                                                                                     
                                                               
      
                                                                                                
                                                                                         
                                                                                             
      
                                                                                   
                                                                          
    element.pointSize = 6
    element.minimumPointScreenSpaceRadius = 2
    element.maximumPointScreenSpaceRadius =
      OfficialAetherARKitPlugin.arSplatMaxScreenRadius
    let geo = SCNGeometry(sources: [vSource, cSource], elements: [element])
    let mat = SCNMaterial()
    mat.lightingModel = .constant
    mat.diffuse.contents = UIColor.white                             
    mat.writesToDepthBuffer = false
    mat.readsFromDepthBuffer = false
    geo.materials = [mat]
    if let node = pointCloudNode {
      node.geometry = geo
      node.simdTransform = pointCloudTransform
    } else {
      let node = SCNNode(geometry: geo)
      node.simdTransform = pointCloudTransform
      node.renderingOrder = -10                                              
      arscnView.scene.rootNode.addChildNode(node)
      pointCloudNode = node
    }
  }

  init(frame: CGRect, getSession: @escaping () -> ARSession?) {
    self.arscnView = ARSCNView(frame: frame)
    self.getSession = getSession
    super.init()
    arscnView.automaticallyUpdatesLighting = true
    arscnView.scene = SCNScene()                                            
    arscnView.rendersContinuously = true
                                                                                             
                                                             
    if ProcessInfo.processInfo.environment["OFFICIAL_AETHER_AR_30FPS"] == "1" {
      arscnView.preferredFramesPerSecond = 30
    }
    arscnView.preferredFramesPerSecond = 30
    arscnView.antialiasingMode = .none
    arscnView.delegate = self
    attachSessionIfReady()
  }

  func view() -> UIView {
    return arscnView
  }

                                                                               
                                                                  
                                                                   
                                              
     
                                                                     
                                                            
                                                                      
                                                                  
                                                                  
                                                                     
                  
  private func attachSessionIfReady() {
    if let session = getSession() {
      arscnView.session = session
      NSLog("[OfficialAetherARKitPreview] attached to ARSession on first try")
      return
    }
    pollTimer = Timer.scheduledTimer(
      withTimeInterval: 0.05, repeats: true
    ) { [weak self] timer in
      guard let self = self else {
        timer.invalidate()
        return
      }
      if let session = self.getSession() {
        self.arscnView.session = session
        NSLog("[OfficialAetherARKitPreview] attached to ARSession after poll")
        timer.invalidate()
        self.pollTimer = nil
      }
    }
  }

                            

                                                                       
                                                                        
                                                                   
                                                           
  func renderer(_ renderer: SCNSceneRenderer, didAdd node: SCNNode, for anchor: ARAnchor) {
                                                
                                                                                  
                                                                               
                                                                               
                                                                               
                                                       
    guard let name = anchor.name, name.hasPrefix("official_photo_card_") else { return }
    NSLog("[PHOTOCARD] renderer didAdd %@", name)
    guard let spec = OfficialAetherARKitPlugin.photoCardSpecs[name] else {
      NSLog("[PHOTOCARD] renderer: spec MISSING for %@", name)
      return
    }
                                                                           
                                                                                 
                                                                                 
                                                                                 
                                                                                     
                                                                                   
    let thumbOpts: [CFString: Any] = [
      kCGImageSourceCreateThumbnailFromImageAlways: true,
      kCGImageSourceCreateThumbnailWithTransform: true,
      kCGImageSourceThumbnailMaxPixelSize: OfficialAetherARKitPlugin.photoCardThumbMaxPx,
    ]
                                                                                                
                                                                                                              
                                                                                         
    if let memImage = spec.textureCGImage {
      OfficialAetherARKitPlugin.photoCardThumbRetries.removeValue(forKey: name)
      buildPhotoCard(image: memImage, spec: spec, name: name, on: node)
      OfficialPwNativeTelemetry.shared.log("photocard_photo_in", [
        "name": name, "source": "memory_thumb",
      ])
      return
    }
    guard let imgSrc = CGImageSourceCreateWithURL(
            URL(fileURLWithPath: spec.texturePath) as CFURL, nil),
          let thumbCG = CGImageSourceCreateThumbnailAtIndex(
            imgSrc, 0, thumbOpts as CFDictionary) else {
                                                                                                  
                                                                                           
                                                                       
                                                                      
                                                                                                   
                                                                                            
                                                                                               
                                                                                                   
                                                                                                        
                                       
                                                                                                
                                                                                                  
                                            
      if photoCardNodes[name] == nil {
        buildPhotoCardShell(corners: spec.localCorners,
                            evidencePath: spec.evidencePath,
                            name: name, on: node)
      }
      let tries = OfficialAetherARKitPlugin.photoCardThumbRetries[name, default: 0]
      if tries < 30 {
        OfficialAetherARKitPlugin.photoCardThumbRetries[name] = tries + 1
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.15) { [weak self, weak node] in
          guard let self, let node else { return }
          self.renderer(renderer, didAdd: node, for: anchor)
        }
      } else {
        NSLog("[PHOTOCARD] renderer: thumbnail decode FAILED (gave up) for %@", name)
        OfficialAetherARKitPlugin.photoCardThumbRetries.removeValue(forKey: name)
      }
      return
    }
    OfficialAetherARKitPlugin.photoCardThumbRetries.removeValue(forKey: name)
                                                                                   
    buildPhotoCard(image: thumbCG, spec: spec, name: name, on: node)
    OfficialPwNativeTelemetry.shared.log("photocard_photo_in", [
      "name": name, "source": "file_decode",
    ])
  }

                                                                         
                                                                                          
                                                                                     
                                                                                           
                                                                                
                                                                                               
  private var photoCardVisibleLogged: Set<String> = []

                                                                                                   
                           
     
                                                                                                          
                                                                                                    
                                                                                                        
                                                                                                      
     
                                                                                                
                                                        
                                                                                               
                                                     
                                                                                                      
                                                                          
  private func logPhotoCardVisible(
    name: String,
    spec: OfficialAetherARKitPlugin.PhotoCardSpec,
    kind: String
  ) {
    let key = "\(name)|\(kind)"
    guard !photoCardVisibleLogged.contains(key) else { return }
    photoCardVisibleLogged.insert(key)
    let nowMs = Date().timeIntervalSince1970 * 1000.0
    var fields: [String: Any] = [
      "name": name,
      "kind": kind,
      "evidence": (spec.evidencePath as NSString).lastPathComponent,
                                                                        
      "since_add_call_ms": Int((nowMs - spec.addCalledEpochMs).rounded()),
    ]
    if let fb = spec.shutterFeedbackEpochMs {
                                                                                             
      fields["since_feedback_ms"] = Int((nowMs - fb).rounded())
    }
    OfficialPwNativeTelemetry.shared.log("photocard_visible", fields)
  }

  private func buildPhotoCard(
    image: CGImage,
    spec: OfficialAetherARKitPlugin.PhotoCardSpec,
    name: String,
    on node: SCNNode
  ) {
    NSLog("[PHOTOCARD] renderer building quad for %@ (%d corners) thumb=%dx%d",
          name, spec.localCorners.count, image.width, image.height)
    let c = spec.localCorners
    let quadW = CGFloat(simd_length(simd_float3(
      c[1].x - c[0].x, c[1].y - c[0].y, c[1].z - c[0].z)))            
    let quadH = CGFloat(simd_length(simd_float3(
      c[3].x - c[0].x, c[3].y - c[0].y, c[3].z - c[0].z)))            
                                                                                           
                                                                                             
                                                      
    let texAspect = image.height > 0
      ? CGFloat(image.width) / CGFloat(image.height) : 0.75
    let quadAspect = quadH > 0 ? quadW / quadH : 0.46
    var u0: CGFloat = 0, u1: CGFloat = 1, v0: CGFloat = 0, v1: CGFloat = 1
    if texAspect > quadAspect {                                                      
      let f = quadAspect / texAspect; u0 = (1 - f) / 2; u1 = 1 - u0
    } else {                                                                           
      let f = texAspect / quadAspect; v0 = (1 - f) / 2; v1 = 1 - v0
    }
    let texUVs = [CGPoint(x: u0, y: v0), CGPoint(x: u1, y: v0),
                  CGPoint(x: u1, y: v1), CGPoint(x: u0, y: v1)]                 
    NSLog("[PHOTOCARD] tex thumb=%dx%d texAsp=%.3f quadAsp=%.3f",
          image.width, image.height, texAspect, quadAspect)

    let positionSource = SCNGeometrySource(vertices: spec.localCorners)
    let texSource = SCNGeometrySource(textureCoordinates: texUVs)
    let element = SCNGeometryElement(indices: [Int32]([0, 1, 2, 0, 2, 3]),
                                     primitiveType: .triangles)
    let geometry = SCNGeometry(sources: [positionSource, texSource],
                               elements: [element])
    let mat = SCNMaterial()
    mat.diffuse.contents = image
    mat.isDoubleSided = false                                                    
    mat.cullMode = .front                                                         
                                                                                     
    mat.lightingModel = .constant                                              
    mat.transparency = 0.7                                                        
    mat.writesToDepthBuffer = false
    mat.diffuse.wrapS = .clamp
    mat.diffuse.wrapT = .clamp
    geometry.materials = [mat]

                                                                                               
                                                                         
    if let shell = photoCardNodes[name] {
      photoCardFrontFill[name]?.removeFromParentNode()
      photoCardFrontFill.removeValue(forKey: name)
      shell.addChildNode(SCNNode(geometry: geometry))
      OfficialPwNativeTelemetry.shared.log("photocard_photo_in", ["name": name])
      NSLog("[PHOTOCARD] renderer: photo swapped into standing shell for %@", name)
      return
    }

                                                                         
                                                                                
                                                                                
                                                                                  
    let backGeo = SCNGeometry(sources: [positionSource], elements: [element])
    let backMat = SCNMaterial()
    backMat.diffuse.contents = UIColor.black
    backMat.isDoubleSided = false
    backMat.cullMode = .back                                                       
    backMat.lightingModel = .constant
    backMat.transparency = 1.0                    
    backMat.writesToDepthBuffer = false
    backGeo.materials = [backMat]

                                                                          
                                                                               
                                                                              
                                                                             
                                                                          
                                                                           
    let inner = spec.localCorners
    let outer = inner.map { SCNVector3($0.x * 1.12, $0.y * 1.12, $0.z * 1.12) }                                                                             
    let frameVerts = inner + outer                                              
    let frameIdx: [Int32] = [4, 5, 1, 4, 1, 0,                      
                             5, 6, 2, 5, 2, 1,                        
                             6, 7, 3, 6, 3, 2,                         
                             7, 4, 0, 7, 0, 3]                       
    let frameGeo = SCNGeometry(
      sources: [SCNGeometrySource(vertices: frameVerts)],
      elements: [SCNGeometryElement(indices: frameIdx, primitiveType: .triangles)])
    let frameMat = SCNMaterial()
    frameMat.diffuse.contents = UIColor.black
    frameMat.isDoubleSided = true
    frameMat.lightingModel = .constant
    frameMat.transparency = 0.95
    frameMat.writesToDepthBuffer = false
    frameGeo.materials = [frameMat]

                                                                                    
                                                                                  
                                                                        
    let container = SCNNode()
    container.addChildNode(SCNNode(geometry: frameGeo))                          
    container.addChildNode(SCNNode(geometry: backGeo))                        
    container.addChildNode(SCNNode(geometry: geometry))                        
    node.addChildNode(container)
    photoCardNodes[name] = container
    logPhotoCardVisible(name: name, spec: spec, kind: "photo")
                                                                                         
                                                                       
    photoCardStateMats[name] = [frameMat, backMat]
    let initialState = OfficialAetherARKitPlugin.photoCardState(
      forPath: spec.evidencePath
    )
    if initialState != 0 {
      let c = Self.photoCardStateColor(initialState)
      frameMat.diffuse.contents = c
      backMat.diffuse.contents = c
    }
  }


                                                                             
                                                                 
  private var photoCardFrontFill: [String: SCNNode] = [:]

                                                                                                     
                                                                                               
                                                
     
                                                                                                       
                                                                                             
                                                                         
  private func buildPhotoCardShell(
    corners: [SCNVector3],
    evidencePath: String,
    name: String,
    on node: SCNNode
  ) {
    let positionSource = SCNGeometrySource(vertices: corners)
    let element = SCNGeometryElement(indices: [Int32]([0, 1, 2, 0, 2, 3]),
                                     primitiveType: .triangles)

                                                                                       
                                                                               
                                                             
    let fillGeo = SCNGeometry(sources: [positionSource], elements: [element])
    let fillMat = SCNMaterial()
    fillMat.diffuse.contents = UIColor.black
    fillMat.isDoubleSided = false
    fillMat.cullMode = .front
    fillMat.lightingModel = .constant
    fillMat.transparency = 0.7
    fillMat.writesToDepthBuffer = false
    fillGeo.materials = [fillMat]

    let backGeo = SCNGeometry(sources: [positionSource], elements: [element])
    let backMat = SCNMaterial()
    backMat.diffuse.contents = UIColor.black
    backMat.isDoubleSided = false
    backMat.cullMode = .back
    backMat.lightingModel = .constant
    backMat.transparency = 1.0
    backMat.writesToDepthBuffer = false
    backGeo.materials = [backMat]

    let inner = corners
    let outer = inner.map { SCNVector3($0.x * 1.12, $0.y * 1.12, $0.z * 1.12) }
    let frameVerts = inner + outer
    let frameIdx: [Int32] = [4, 5, 1, 4, 1, 0,
                             5, 6, 2, 5, 2, 1,
                             6, 7, 3, 6, 3, 2,
                             7, 4, 0, 7, 0, 3]
    let frameGeo = SCNGeometry(
      sources: [SCNGeometrySource(vertices: frameVerts)],
      elements: [SCNGeometryElement(indices: frameIdx, primitiveType: .triangles)])
    let frameMat = SCNMaterial()
    frameMat.diffuse.contents = UIColor.black
    frameMat.isDoubleSided = true
    frameMat.lightingModel = .constant
    frameMat.transparency = 0.95
    frameMat.writesToDepthBuffer = false
    frameGeo.materials = [frameMat]

    let container = SCNNode()
    container.addChildNode(SCNNode(geometry: frameGeo))
    container.addChildNode(SCNNode(geometry: backGeo))
    let fillNode = SCNNode(geometry: fillGeo)
    container.addChildNode(fillNode)
    node.addChildNode(container)
    photoCardNodes[name] = container
    photoCardFrontFill[name] = fillNode
                                                                                                       
                                                                                         
    if let spec = OfficialAetherARKitPlugin.photoCardSpecs[name] {
      logPhotoCardVisible(name: name, spec: spec, kind: "shell")
    }
    photoCardStateMats[name] = [frameMat, backMat]
    let initialState = OfficialAetherARKitPlugin.photoCardState(
      forPath: evidencePath
    )
    if initialState != 0 {
      let c = Self.photoCardStateColor(initialState)
      frameMat.diffuse.contents = c
      backMat.diffuse.contents = c
    }
                                                                                                 
                                                                                                  
    OfficialPwNativeTelemetry.shared.log("photocard_shell_up", ["name": name])
    NSLog("[PHOTOCARD] shell up (black frame, photo pending) for %@", name)
  }

                                                                            
                                                                           
                                                                                     
             
                                                                               
  private var photoCardCameraHolders: [String: SCNNode] = [:]

                                                                  
                                                                                                               
                                                                                                       
                                                                                                            
                                                                                               
                                                                                                             
                           
  private func servicePendingPhotoCardAnchors(_ renderer: SCNSceneRenderer) {
    let pending = OfficialAetherARKitPlugin.photoCardPendingAnchors
    guard !pending.isEmpty else { return }
    if let pov = renderer.pointOfView {
      for (name, p) in pending where photoCardCameraHolders[name] == nil {
        let holder = SCNNode()
        holder.position = SCNVector3(0, 0, -p.z)
        pov.addChildNode(holder)
        photoCardCameraHolders[name] = holder
        let c: [SCNVector3] = [
          SCNVector3(-p.halfX,  p.halfY, 0),        
          SCNVector3( p.halfX,  p.halfY, 0),        
          SCNVector3( p.halfX, -p.halfY, 0),        
          SCNVector3(-p.halfX, -p.halfY, 0),        
        ]
        buildPhotoCardShell(corners: c, evidencePath: p.evidencePath,
                            name: name, on: holder)
      }
    }
    guard let status = arscnView.session.currentFrame?.worldMappingStatus,
          status == .extending || status == .mapped else { return }
    for (name, p) in pending {
      photoCardCameraHolders[name]?.removeFromParentNode()
      photoCardCameraHolders.removeValue(forKey: name)
      photoCardNodes.removeValue(forKey: name)
      photoCardStateMats.removeValue(forKey: name)
      photoCardFrontFill.removeValue(forKey: name)
      OfficialAetherARKitPlugin.photoCardThumbRetries.removeValue(forKey: name)
      arscnView.session.add(anchor: p.anchor)
      OfficialPwNativeTelemetry.shared.log("photocard_anchor_migrated", [
        "name": name,
        "waited_ms": Int((CACurrentMediaTime() - p.since) * 1000.0),
        "world_mapping_status": status.rawValue,
      ])
                                                                                       
                                                                                                  
                                                                                    
      OfficialAetherARKitPlugin.photoCardPendingAnchors.removeValue(forKey: name)
    }
  }

  func renderer(_ renderer: SCNSceneRenderer, updateAtTime time: TimeInterval) {
    OfficialPwNativeTelemetry.shared.noteRenderFrame()                                   
    servicePendingPhotoCardAnchors(renderer)
    updateFeaturePointOverlay(at: time)                                            
    applyPhotoCardStatesIfDirty()                                                   
    guard !photoCardNodes.isEmpty, let cam = renderer.pointOfView else { return }
    let camPos = cam.simdWorldPosition
    for card in photoCardNodes.values {
                                                                                                      
                                                                       
      card.isHidden = !OfficialAetherARKitPlugin.photoCardsVisible
                                                                          
                                                                            
                                                                                         
                                                                          
        
                                                                                   
                                                                             
                                                                               
                                                                                           
                                                                                                   
                                                                                                  
                                                                                                   
                                                                        
      let d = simd_distance(camPos, card.simdWorldPosition)
      let s = d > Self.photoCardDistanceAnchorM
        ? powf(d / Self.photoCardDistanceAnchorM, Self.photoCardDistanceBeta)
        : 1.0
      card.simdScale = simd_float3(repeating: s)
    }
  }

                                                                         
                                                                             
                                                                           
                                                                              
                                                                
  private func applyPhotoCardStatesIfDirty() {
    guard let states = OfficialAetherARKitPlugin.takePhotoCardStatesIfDirty() else {
      return
    }
                                                                                   
    let t0 = CACurrentMediaTime()
    for (name, mats) in photoCardStateMats {
      guard let path =
        OfficialAetherARKitPlugin.photoCardSpecs[name]?.evidencePath else {
        continue
      }
      let color = Self.photoCardStateColor(states[path] ?? 0)
      for m in mats { m.diffuse.contents = color }
    }
    OfficialPwNativeTelemetry.shared.logCardPushApply(
      applyMs: (CACurrentMediaTime() - t0) * 1000.0,
      cardCount: photoCardStateMats.count
    )
  }

                                                                   
                                                                         
                                                              
  func renderer(_ renderer: SCNSceneRenderer, didRemove node: SCNNode, for anchor: ARAnchor) {
    if let name = anchor.name {
      if name.hasPrefix("official_photo_card_") {
                                                                                   
                                                                                  
                                                                         
        NSLog("[PHOTOCARD] *** ANCHOR REMOVED by ARKit: %@ (card gone) ***", name)
      }
      photoCardNodes.removeValue(forKey: name)
      photoCardStateMats.removeValue(forKey: name)
      photoCardFrontFill.removeValue(forKey: name)                                       
    }
    guard anchor.name == Self.subjectAnchorName else { return }
    NSLog("[OfficialAetherARKitPreview] subject anchor removed; marker went with it")
  }

  deinit {
    pollTimer?.invalidate()
  }
}

                                                          
                                                                                          
                                                                                                 
                                                              

                                                                            
  
                                                                                       
                                                                                 
                                                                                      
                                                                                     
                                                                                        
  
          
                                                                                             
                                                          
                                                                                  
                                                                     
                                                                      
                                                                               
                                                                                         
                                          
  
                                                                                          
                                                                    
final class OfficialPwNativeTelemetry {
  static let shared = OfficialPwNativeTelemetry()

  private let queue = DispatchQueue(
    label: "com.worldswithin.official.telemetry",
    qos: .utility
  )
  private var handle: FileHandle?
  private var handleFailed = false
  private var resourceTimer: DispatchSourceTimer?

                                                                                 
  private let frameLock = NSLock()
  private var renderFrames = 0
  private var frameWindowStart = CACurrentMediaTime()

                                                                                   
                           
  private let cardPushLock = NSLock()
  private var pendingCardPushCount = 0

  private init() {}

                                                                                                                                                                   

  private func ensureHandle() -> FileHandle? {
    if let h = handle { return h }
    if handleFailed { return nil }
    guard
      let docs = FileManager.default.urls(
        for: .documentDirectory, in: .userDomainMask
      ).first
    else {
      handleFailed = true
      return nil
    }
    let url = docs.appendingPathComponent("telemetry_official_native.jsonl")
    if !FileManager.default.fileExists(atPath: url.path) {
      FileManager.default.createFile(atPath: url.path, contents: nil)
    }
    guard let h = try? FileHandle(forWritingTo: url) else {
      handleFailed = true
      return nil
    }
    h.seekToEndOfFile()
    handle = h
    return h
  }

                                                                                       
  func log(_ type: String, _ fields: [String: Any] = [:]) {
    let t = Int(Date().timeIntervalSince1970 * 1000)
    queue.async { [weak self] in
      guard let self = self, let h = self.ensureHandle() else { return }
      var obj: [String: Any] = ["t": t, "type": type]
      for (k, v) in fields { obj[k] = v }
      guard JSONSerialization.isValidJSONObject(obj),
            var data = try? JSONSerialization.data(withJSONObject: obj)
      else { return }
      data.append(0x0A)         
      h.write(data)
    }
  }

                                                                                                                                                                                    

                                                                                         
                                                        
  func logSession() {
    UIDevice.current.isBatteryMonitoringEnabled = true
    var sysinfo = utsname()
    uname(&sysinfo)
    let model = withUnsafePointer(to: &sysinfo.machine) {
      $0.withMemoryRebound(to: CChar.self, capacity: 256) {
        String(cString: $0)
      }
    }
    var buildStamp = "unknown"
    if let exe = Bundle.main.executablePath,
       let attrs = try? FileManager.default.attributesOfItem(atPath: exe),
       let mtime = attrs[.modificationDate] as? Date {
      buildStamp = ISO8601DateFormatter().string(from: mtime)
    }
    let version =
      (Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String)
      ?? "?"
    let build =
      (Bundle.main.infoDictionary?["CFBundleVersion"] as? String) ?? "?"
    let diagnosticBuildId =
      (Bundle.main.infoDictionary?["PWLiveCloudDiagnosticBuildId"] as? String)
      ?? "UNSTAMPED"
    let productManifest =
      (Bundle.main.infoDictionary?["PWProductSourceManifestSHA256"] as? String)
      ?? "UNSTAMPED"
    let dartAotSHA =
      (Bundle.main.infoDictionary?["PWDartAOTSHA256"] as? String)
      ?? "UNSTAMPED"
    let officialSfmSHA =
      (Bundle.main.infoDictionary?["PWOfficialSfmSHA256"] as? String)
      ?? "UNSTAMPED"
    let battery = UIDevice.current.batteryLevel                              
    log("session", [
      "build_stamp": buildStamp,
      "app_version": "\(version)(\(build))",
      "model": model,
      "os": UIDevice.current.systemVersion,
      "battery": Double(battery),
      "phys_mem_mb": Double(ProcessInfo.processInfo.physicalMemory)
        / 1_048_576.0,
      "cores": ProcessInfo.processInfo.processorCount,
      "thermal": ProcessInfo.processInfo.thermalState.rawValue,
      "diagnostic_build_id": diagnosticBuildId,
      "product_manifest": productManifest,
      "dart_aot_sha256": dartAotSHA,
      "official_sfm_sha256": officialSfmSHA,
    ])
    log("live_cloud_diag_build_v1", [
      "contract": "PW_LIVE_CLOUD_DIAG_V1_STREAM",
      "diagnostic_build_id": diagnosticBuildId,
      "product_manifest": productManifest,
      "dart_aot_sha256": dartAotSHA,
      "official_sfm_sha256": officialSfmSHA,
      "app_version": "\(version)(\(build))",
      "observation_only": true,
    ])
  }

                                                                            
                                                                           
                                                                  
  func logCaptureIdentity() {
    let info = Bundle.main.infoDictionary
    log("live_cloud_diag_capture_v2", [
      "contract": "PW_LIVE_CLOUD_DIAG_RUNTIME_V2_STREAM",
      "diagnostic_build_id":
        (info?["PWLiveCloudDiagnosticBuildId"] as? String) ?? "UNSTAMPED",
      "product_manifest":
        (info?["PWProductSourceManifestSHA256"] as? String) ?? "UNSTAMPED",
      "dart_aot_sha256":
        (info?["PWDartAOTSHA256"] as? String) ?? "UNSTAMPED",
      "official_sfm_sha256":
        (info?["PWOfficialSfmSHA256"] as? String) ?? "UNSTAMPED",
      "app_version":
        "\((info?["CFBundleShortVersionString"] as? String) ?? "?")"
        + "(\((info?["CFBundleVersion"] as? String) ?? "?"))",
      "observation_only": true,
    ])
  }

                                                                                                                                                                                  

                                                                           
  func startResourceSampling() {
    queue.async { [weak self] in
      guard let self = self, self.resourceTimer == nil else { return }
      self.frameLock.lock()
      self.renderFrames = 0
      self.frameWindowStart = CACurrentMediaTime()
      self.frameLock.unlock()
      let timer = DispatchSource.makeTimerSource(queue: self.queue)
      timer.schedule(deadline: .now() + 10, repeating: 10)
      timer.setEventHandler { [weak self] in
        self?.sampleResourcesOnQueue()
      }
      timer.resume()
      self.resourceTimer = timer
      self.log("resource_begin")
    }
  }

                                                                                       
  func stopResourceSampling() {
    queue.async { [weak self] in
      guard let self = self, let timer = self.resourceTimer else { return }
      timer.cancel()
      self.resourceTimer = nil
      self.sampleResourcesOnQueue()
      self.log("resource_end")
    }
  }

                                                                                       
                                                               
  func noteRenderFrame() {
    frameLock.lock()
    renderFrames += 1
    frameLock.unlock()
  }

  private func sampleResourcesOnQueue() {
                                              
    frameLock.lock()
    let frames = renderFrames
    let windowS = CACurrentMediaTime() - frameWindowStart
    renderFrames = 0
    frameWindowStart = CACurrentMediaTime()
    frameLock.unlock()
    let fps = windowS > 0.1 ? Double(frames) / windowS : 0

    let footprint = Self.physFootprintMB()
    let cpu = Self.processCpuOneCorePercent()
    let thermal = ProcessInfo.processInfo.thermalState.rawValue
                                                                                         
    DispatchQueue.main.async { [weak self] in
      let battery = Double(UIDevice.current.batteryLevel)
      let appState: String
      switch UIApplication.shared.applicationState {
      case .active: appState = "active"
      case .inactive: appState = "inactive"
      case .background: appState = "background"
      @unknown default: appState = "unknown"
      }
      self?.log("resource", [
        "thermal": thermal,
        "footprint_mb": (footprint * 10).rounded() / 10,
        "battery": battery,
        "cpu_one_core_pct": (cpu * 10).rounded() / 10,
        "scn_fps": (fps * 10).rounded() / 10,
        "app_state": appState,
                                                                                                        
        "quality_hz": OfficialAetherARKitPlugin.currentQualityHz,
      ])
    }
  }

                                                                                                                                                                                  

                                                                             
  func noteCardPush(diffCount: Int) {
    cardPushLock.lock()
    pendingCardPushCount += diffCount
    cardPushLock.unlock()
  }

                                                                     
  func logCardPushApply(applyMs: Double, cardCount: Int) {
    cardPushLock.lock()
    let n = pendingCardPushCount
    pendingCardPushCount = 0
    cardPushLock.unlock()
    guard applyMs > 1.0 else { return }
    log("cardpush", [
      "n": n,
      "apply_ms": (applyMs * 100).rounded() / 100,
      "cards": cardCount,
    ])
  }

                                                                                                                                                                                                 

                                                                             
  private static func physFootprintMB() -> Double {
    var info = task_vm_info_data_t()
    var count = mach_msg_type_number_t(
      MemoryLayout<task_vm_info_data_t>.size / MemoryLayout<integer_t>.size
    )
    let kr = withUnsafeMutablePointer(to: &info) { ptr in
      ptr.withMemoryRebound(to: integer_t.self, capacity: Int(count)) {
        task_info(mach_task_self_, task_flavor_t(TASK_VM_INFO), $0, &count)
      }
    }
    guard kr == KERN_SUCCESS else { return -1 }
    return Double(info.phys_footprint) / 1_048_576.0
  }

                                                                                       
  private static func processCpuOneCorePercent() -> Double {
    var threadList: thread_act_array_t?
    var threadCount = mach_msg_type_number_t(0)
    guard task_threads(mach_task_self_, &threadList, &threadCount)
            == KERN_SUCCESS,
          let threadList
    else { return 0 }
    defer {
      vm_deallocate(
        mach_task_self_,
        vm_address_t(UInt(bitPattern: threadList)),
        vm_size_t(Int(threadCount) * MemoryLayout<thread_t>.stride)
      )
    }
    var total = 0.0
    for index in 0..<Int(threadCount) {
      var info = thread_basic_info()
      var count = mach_msg_type_number_t(THREAD_INFO_MAX)
      let result = withUnsafeMutablePointer(to: &info) { ptr in
        ptr.withMemoryRebound(to: integer_t.self, capacity: Int(count)) {
          thread_info(
            threadList[index],
            thread_flavor_t(THREAD_BASIC_INFO),
            $0,
            &count
          )
        }
      }
      guard result == KERN_SUCCESS else { continue }
      if (info.flags & TH_FLAGS_IDLE) == 0 {
        total += Double(info.cpu_usage) / Double(TH_USAGE_SCALE) * 100.0
      }
    }
    return total
  }
}
