                         
                                                       
  
                                                                             
                                               
                        
                                            
                                    
                                 
                                           
                                                                  
  
                                                                             
                                                    
  
                                                                   
                                                                                     
                                                                                   
                                                     
                                           
                                                                 
                                                             
                                                                                   
                                          
  
                                
                                                                              
                                                                                     
                                                    
                                               
  
                                                              
                                                          
                                                                    
                                                          
                                                                   
                                                                   
  
                                                
                                              
                                                                                    
                                                                  
                                                           
                                                                         
                                                      
  
                                                             
                                                                  
                             
  
                                                                             
                                       
                                                          

import AVFoundation
import CoreMedia
import CoreMotion
import Foundation
import simd

#if canImport(ARKit)
import ARKit
#endif

               
  
                                                               
                                                   
                                                   
                                                                
                                            
                                                     
                              
  
                                                           
                                   
@inline(__always)
public func pwVioCapabilityHostUptimeSeconds() -> Double {
    return Double(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) * 1e-9
}

                           
  
                                                                       
                                   
public enum PwVioWire {
    public enum IntrinsicsSource: String {
        case none
        case fieldOfViewFallback
        case staticCharacteristics
        case platformTracker
        case perFrameAttachment
    }
}

             

public struct PwVioIntrinsics {
    public let source: PwVioWire.IntrinsicsSource
    public let fx: Double
    public let fy: Double
    public let cx: Double
    public let cy: Double
    public let skew: Double
    public let referenceWidth: Int
    public let referenceHeight: Int

    public static let absent = PwVioIntrinsics(
        source: .none, fx: 0, fy: 0, cx: 0, cy: 0, skew: 0,
        referenceWidth: 0, referenceHeight: 0)

    public var wire: [String: Any] {
        return [
            "source": source.rawValue,
            "fx": fx, "fy": fy, "cx": cx, "cy": cy, "skew": skew,
            "referenceWidth": referenceWidth,
            "referenceHeight": referenceHeight,
        ]
    }

                           
                                                            
                                              
                                                 
    init(matrix m: matrix_float3x3,
         referenceWidth w: Int,
         referenceHeight h: Int,
         source: PwVioWire.IntrinsicsSource) {
        self.source = source
        self.fx = Double(m.columns.0.x)
        self.fy = Double(m.columns.1.y)
        self.skew = Double(m.columns.1.x)
        self.cx = Double(m.columns.2.x)
        self.cy = Double(m.columns.2.y)
        self.referenceWidth = w
        self.referenceHeight = h
    }

    init(source: PwVioWire.IntrinsicsSource, fx: Double, fy: Double,
         cx: Double, cy: Double, skew: Double,
         referenceWidth: Int, referenceHeight: Int) {
        self.source = source
        self.fx = fx; self.fy = fy; self.cx = cx; self.cy = cy
        self.skew = skew
        self.referenceWidth = referenceWidth
        self.referenceHeight = referenceHeight
    }
}

public enum PwVioIntrinsicsReader {

                                        
                                             
    @discardableResult
    public static func enablePerFrameDelivery(on connection: AVCaptureConnection) -> Bool {
        guard connection.isCameraIntrinsicMatrixDeliverySupported else { return false }
        connection.isCameraIntrinsicMatrixDeliveryEnabled = true
        return connection.isCameraIntrinsicMatrixDeliveryEnabled
    }

                       
       
                                                             
                                                                        
                                              
    public static func fromSampleBuffer(_ sampleBuffer: CMSampleBuffer) -> PwVioIntrinsics? {
        guard let raw = CMGetAttachment(
            sampleBuffer,
            key: kCMSampleBufferAttachmentKey_CameraIntrinsicMatrix,
            attachmentModeOut: nil) as? Data,
            raw.count >= MemoryLayout<matrix_float3x3>.size
        else { return nil }

        let m: matrix_float3x3 = raw.withUnsafeBytes { buf in
            buf.loadUnaligned(as: matrix_float3x3.self)
        }
        guard let fd = CMSampleBufferGetFormatDescription(sampleBuffer) else { return nil }
        let dims = CMVideoFormatDescriptionGetDimensions(fd)
        return PwVioIntrinsics(matrix: m,
                               referenceWidth: Int(dims.width),
                               referenceHeight: Int(dims.height),
                               source: .perFrameAttachment)
    }

    #if canImport(ARKit)
                                                                      
    @available(iOS 11.0, *)
    public static func fromARCamera(_ camera: ARCamera) -> PwVioIntrinsics {
        return PwVioIntrinsics(matrix: camera.intrinsics,
                               referenceWidth: Int(camera.imageResolution.width),
                               referenceHeight: Int(camera.imageResolution.height),
                               source: .platformTracker)
    }
    #endif

                                                     
                                              
                                               
    public static func fieldOfViewWire(device: AVCaptureDevice,
                                       width: Int,
                                       height: Int) -> [String: Any] {
        let format = device.activeFormat
        var corrected: Any = NSNull()
        if #available(iOS 13.0, *) {
            corrected = Double(
                format.geometricDistortionCorrectedVideoFieldOfView)
        }
        return [
            "videoFieldOfViewDegrees": Double(format.videoFieldOfView),
            "geometricDistortionCorrectedVideoFieldOfViewDegrees": corrected,
            "referenceWidth": width,
            "referenceHeight": height,
        ]
    }
}

             

public struct PwVioStabilizationRawReport {
    public let videoStabilizationSupported: Bool
    public let requestedPreferredModeRawValue: Int
    public let requestedPreferredModeRecognized: Bool
    public let preferredModeAssignmentPerformed: Bool
    public let activeVideoStabilizationModeRawValue: Int
    public let geometricDistortionCorrectionSupported: Bool
    public let geometricDistortionCorrectionEnabled: Bool
    public let opticalImageStabilizationPublicApiAvailable: Bool

    public var wire: [String: Any] {
        return [
            "schema": "pw.vio.ios.stabilization-raw/1",
            "videoStabilizationSupported": videoStabilizationSupported,
            "requestedPreferredModeRawValue": requestedPreferredModeRawValue,
            "requestedPreferredModeRecognized": requestedPreferredModeRecognized,
            "preferredModeAssignmentPerformed": preferredModeAssignmentPerformed,
            "activeVideoStabilizationModeRawValue": activeVideoStabilizationModeRawValue,
            "geometricDistortionCorrectionSupported": geometricDistortionCorrectionSupported,
            "geometricDistortionCorrectionEnabled": geometricDistortionCorrectionEnabled,
            "opticalImageStabilizationPublicApiAvailable": opticalImageStabilizationPublicApiAvailable,
        ]
    }
}

public enum PwVioStabilization {

                                      
                                                           
                                                   
                                             
    @discardableResult
    public static func configureAndReadRaw(
        connection: AVCaptureConnection,
        device: AVCaptureDevice,
        requestedPreferredModeRawValue: Int
    ) -> PwVioStabilizationRawReport {
        let supported = connection.isVideoStabilizationSupported
        let requestedMode = AVCaptureVideoStabilizationMode(
            rawValue: requestedPreferredModeRawValue)
        let recognized = requestedMode != nil
        let assignmentPerformed = supported && recognized
        if assignmentPerformed, let mode = requestedMode {
            connection.preferredVideoStabilizationMode = mode
        }

        var gdcSupported = false
        var gdcEnabled = false
        if #available(iOS 13.0, *) {
            gdcSupported = device.isGeometricDistortionCorrectionSupported
            gdcEnabled = device.isGeometricDistortionCorrectionEnabled
        }

        return PwVioStabilizationRawReport(
            videoStabilizationSupported: supported,
            requestedPreferredModeRawValue: requestedPreferredModeRawValue,
            requestedPreferredModeRecognized: recognized,
            preferredModeAssignmentPerformed: assignmentPerformed,
            activeVideoStabilizationModeRawValue:
                connection.activeVideoStabilizationMode.rawValue,
            geometricDistortionCorrectionSupported: gdcSupported,
            geometricDistortionCorrectionEnabled: gdcEnabled,
            opticalImageStabilizationPublicApiAvailable: false)
    }
}

                   

private struct PwVioImuArrival {
    let sampleTsNs: Int64
    let deliveryTsNs: Int64
}

                                      
                                 
   
         
                                                                               
                                                                      
                                                                          
                                                                
                                                            
                                           
public final class PwVioImuProbe {

    private static let capacity = 4096
    private let motion = CMMotionManager()
    private let queue: OperationQueue
    private let lock = NSLock()
    private var imuArrivalRing = [PwVioImuArrival?](
        repeating: nil,
        count: PwVioImuProbe.capacity)
    private var writeIndex = 0
    private var retainedCount = 0
    private var attemptedCount = 0
    private var overwrittenCount = 0

    public init() {
        queue = OperationQueue()
                                       
        queue.maxConcurrentOperationCount = 1
        queue.qualityOfService = .userInitiated
    }

    public var isAvailable: Bool { return motion.isGyroAvailable }

                                                        
                                             
    @discardableResult
    public func start(requestedHz: Double) -> Bool {
        guard requestedHz.isFinite, requestedHz > 0 else { return false }
        let requestedInterval = 1.0 / requestedHz
        guard requestedInterval.isFinite, requestedInterval > 0 else { return false }
        guard motion.isGyroAvailable, !motion.isGyroActive else {
            return false
        }
        motion.gyroUpdateInterval = requestedInterval
        motion.startGyroUpdates(to: queue) { [weak self] gyroSample, _ in
            guard let self = self, let sample = gyroSample else { return }
                                                            
                                                                           
                                                                      
                                                          
            PwVioTimebase.shared.noteCoreMotionGyroscope(
                timestamp: sample.timestamp)
            let delivery = pwVioCapabilityHostUptimeSeconds()
            let arrival = PwVioImuArrival(
                sampleTsNs: Int64((sample.timestamp * 1_000_000_000.0).rounded()),
                deliveryTsNs: Int64((delivery * 1_000_000_000.0).rounded()))
            self.lock.lock()
            self.attemptedCount += 1
            self.imuArrivalRing[self.writeIndex] = arrival
            self.writeIndex = (self.writeIndex + 1) % Self.capacity
            if self.retainedCount < Self.capacity {
                self.retainedCount += 1
            } else {
                self.overwrittenCount += 1
            }
            self.lock.unlock()
        }
        return motion.isGyroActive
    }

    public func stop() {
        if motion.isGyroActive { motion.stopGyroUpdates() }
    }

                                                        
                                         
    public func drainWire() -> [String: Any] {
        lock.lock()
        let n = retainedCount
        let attempted = attemptedCount
        let overwritten = overwrittenCount
        let oldest = n == Self.capacity ? writeIndex : 0
        var arrivals = [PwVioImuArrival]()
        arrivals.reserveCapacity(n)
        if n > 0 {
            for offset in 0..<n {
                let index = (oldest + offset) % Self.capacity
                if let arrival = imuArrivalRing[index] { arrivals.append(arrival) }
            }
        }
        lock.unlock()
        return [
            "schema": "pw.vio.imu-arrivals.raw.v1",
            "available": motion.isGyroAvailable,
            "sampleTsNs": arrivals.map { NSNumber(value: $0.sampleTsNs) },
            "deliveryTsNs": arrivals.map { NSNumber(value: $0.deliveryTsNs) },
            "attemptedCount": NSNumber(value: attempted),
            "retainedCount": n,
            "overwrittenCount": NSNumber(value: overwritten),
            "capacity": Self.capacity,
        ]
    }

    public func reset() {
        lock.lock()
        imuArrivalRing = [PwVioImuArrival?](repeating: nil, count: Self.capacity)
        writeIndex = 0
        retainedCount = 0
        attemptedCount = 0
        overwrittenCount = 0
        lock.unlock()
    }
}

                

                               
                                          
                                     
public final class PwVioFrameTimingProbe {
    private static let capacity = 512
    private let lock = NSLock()
    private var arrivalHostTsNs = [Int64](
        repeating: 0,
        count: PwVioFrameTimingProbe.capacity)
    private var writeIndex = 0
    private var retainedCount = 0
    private var attemptedCount = 0
    private var overwrittenCount = 0

    public init() {}

                                                  
    public func mark(hostUptimeSeconds: TimeInterval = pwVioCapabilityHostUptimeSeconds()) {
        let ns = Int64((hostUptimeSeconds * 1_000_000_000.0).rounded())
        lock.lock()
        attemptedCount += 1
        arrivalHostTsNs[writeIndex] = ns
        writeIndex = (writeIndex + 1) % Self.capacity
        if retainedCount < Self.capacity {
            retainedCount += 1
        } else {
            overwrittenCount += 1
        }
        lock.unlock()
    }

    public func wire() -> [String: Any] {
        lock.lock()
        let n = retainedCount
        let attempted = attemptedCount
        let overwritten = overwrittenCount
        let oldest = n == Self.capacity ? writeIndex : 0
        var arrivals = [Int64]()
        arrivals.reserveCapacity(n)
        if n > 0 {
            for offset in 0..<n {
                arrivals.append(arrivalHostTsNs[(oldest + offset) % Self.capacity])
            }
        }
        lock.unlock()
        return [
            "schema": "pw.vio.frame-arrivals.raw.v1",
            "arrivalHostTsNs": arrivals.map { NSNumber(value: $0) },
            "attemptedCount": NSNumber(value: attempted),
            "retainedCount": n,
            "overwrittenCount": NSNumber(value: overwritten),
            "capacity": Self.capacity,
        ]
    }

    public func reset() {
        lock.lock()
        arrivalHostTsNs = [Int64](repeating: 0, count: Self.capacity)
        writeIndex = 0
        retainedCount = 0
        attemptedCount = 0
        overwrittenCount = 0
        lock.unlock()
    }
}
