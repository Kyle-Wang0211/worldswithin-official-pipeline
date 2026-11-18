                                                                                  
  
                                                                                                     
                                                                                                      
                                                                                   
  
                                                                                                                                                                                                                           
                                                                                                           
                                                                                                                                                                                                                           
  
                                                               
                                                
                                      
                                                                                 
                                                                                     
                                                                                                   
                                                                             
  
                                             
                                                                                      
                                                         
                                                                          
                                                                              
                                                                     
                                             
                                                         
                                                                              
                                                                                                   
                               
                                                                                 
                                                                            
                                                                                   
                                                                                      
  
                          
                                                                                    
                                                                                          
                                                                                            
                                                                            
  
                                                        
                                                                           
                                                                             
                                                                               
                                                                
                                                                       
                                                                
                                                                            
                                                                                
                                                                   
  
                                                                                                                                                                                                                           
                                                                                                  
                                                                                                                                                                                                                           
                                                                                     
                                                                               
                                                            
  
                                                                                    
                                                                                         
                                                                                                         
                                                                                               
  
                                                                     
                                                 
                                          
                                               
                                                                                                            
  
                                                                                                                                                                                                                           
                                                                                                 
                                                                                                                                                                                                                           
                                                                 
                                                                              
                                                                                                       
                                                 
                                                                          
                                                                              
                                                                               
                                                                                                    
  
                                                                                                                                                                                                                           
                            
                                                                                                                                                                                                                           
                                                                                   
                                                                                 
                                                                               
                                          
                                                                                            
                               

import AVFoundation
import CoreMedia
import CoreMotion
import Darwin
import Foundation
import ImageIO
import simd

#if canImport(ARKit)
  import ARKit
#endif

#if canImport(Flutter)
  import Flutter
#endif

                                                                      
                                                                          
                                                                          
                                                                         
                                                           
public enum PwVioSensorIngress {
  public static let dispatchQueue = DispatchQueue(
    label: "pw.vio.sensor-ingress",
    qos: .userInteractive
  )
  public static let operationQueue: OperationQueue = {
    let queue = OperationQueue()
    queue.name = "pw.vio.sensor-ingress"
    queue.maxConcurrentOperationCount = 1
    queue.qualityOfService = .userInteractive
    queue.underlyingQueue = dispatchQueue
    return queue
  }()
}

                                              

public enum PwVioTimebaseIdentifiers {
  public static let methodChannel = "worldswithin_vio_timebase"
}

                                

                                                                                              
                                
@inline(__always)
public func pwVioTimebaseUptimeRawSeconds() -> Double {
  return Double(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) * 1e-9
}

                                                                 
                                                                                                         
@inline(__always)
public func pwVioTimebaseMonotonicSeconds() -> Double {
  return Double(clock_gettime_nsec_np(CLOCK_MONOTONIC)) * 1e-9
}

                                                                      
                                                          
public struct PwVioClockSandwich {
  public let uptimeRawBeforeSeconds: Double
  public let monotonicSeconds: Double
  public let uptimeRawAfterSeconds: Double
}

                                                                                                         
@inline(__always)
public func pwVioTimebaseSampleClockPair() -> PwVioClockSandwich {
  return PwVioClockSandwich(
    uptimeRawBeforeSeconds: pwVioTimebaseUptimeRawSeconds(),
    monotonicSeconds: pwVioTimebaseMonotonicSeconds(),
    uptimeRawAfterSeconds: pwVioTimebaseUptimeRawSeconds()
  )
}

                                                                    

private struct PwVioRawTimestampSample {
  let sequence: Int
  let sourceSeconds: Double
  let uptimeRawBeforeSeconds: Double
  let monotonicSeconds: Double
  let uptimeRawAfterSeconds: Double

  var wire: [String: Any] {
    [
      "seq": sequence,
      "sourceSeconds": sourceSeconds,
      "uptimeRawBeforeSeconds": uptimeRawBeforeSeconds,
      "monotonicSeconds": monotonicSeconds,
      "uptimeRawAfterSeconds": uptimeRawAfterSeconds,
    ]
  }
}

                                                                           
                                                                     
private final class PwVioRawSampleWindow {
  private static let capacity = 512
  private var storage = Array<PwVioRawTimestampSample?>(
    repeating: nil,
    count: capacity
  )
  private var head = 0
  private var count = 0
  private(set) var totalAdded = 0
  private(set) var overwritten = 0
  private(set) var totalDelivered = 0

  func clear() {
    for index in storage.indices { storage[index] = nil }
    head = 0
    count = 0
    totalAdded = 0
    overwritten = 0
    totalDelivered = 0
  }

  func add(sourceSeconds: Double, pair: PwVioClockSandwich) {
    totalAdded += 1
    let sample = PwVioRawTimestampSample(
      sequence: totalAdded,
      sourceSeconds: sourceSeconds,
      uptimeRawBeforeSeconds: pair.uptimeRawBeforeSeconds,
      monotonicSeconds: pair.monotonicSeconds,
      uptimeRawAfterSeconds: pair.uptimeRawAfterSeconds
    )
    if count == Self.capacity {
      storage[head] = sample
      head = (head + 1) % Self.capacity
      overwritten += 1
      return
    }
    storage[(head + count) % Self.capacity] = sample
    count += 1
  }

  func drain() -> [PwVioRawTimestampSample] {
    var out: [PwVioRawTimestampSample] = []
    out.reserveCapacity(count)
    for offset in 0..<count {
      if let sample = storage[(head + offset) % Self.capacity] {
        out.append(sample)
      }
    }
    for index in storage.indices { storage[index] = nil }
    head = 0
    count = 0
    totalDelivered += out.count
    return out
  }
}

                                                                             
                                                                    
private final class PwVioGenerationCounter {
  private static let countMask: UInt64 = 0xffff_ffff
  private var packed: Int64 = 0

  private func load() -> UInt64 {
    UInt64(bitPattern: OSAtomicAdd64Barrier(0, &packed))
  }

  func begin(_ generation: Int) {
    let next = UInt64(UInt32(truncatingIfNeeded: generation)) << 32
    while true {
      let old = OSAtomicAdd64Barrier(0, &packed)
      if OSAtomicCompareAndSwap64Barrier(
        old, Int64(bitPattern: next), &packed
      ) { return }
    }
  }

  @discardableResult
  func increment(generation: Int) -> Bool {
    let expected = UInt32(truncatingIfNeeded: generation)
    while true {
      let old = load()
      guard UInt32(truncatingIfNeeded: old >> 32) == expected else {
        return false
      }
      let count = old & Self.countMask
      guard count < Self.countMask else { return false }
      let next = (UInt64(expected) << 32) | (count + 1)
      if OSAtomicCompareAndSwap64Barrier(
        Int64(bitPattern: old), Int64(bitPattern: next), &packed
      ) { return true }
    }
  }

  func value(generation: Int) -> Int {
    let bits = load()
    guard UInt32(truncatingIfNeeded: bits >> 32) ==
            UInt32(truncatingIfNeeded: generation) else { return 0 }
    return Int(bits & Self.countMask)
  }
}

private final class PwVioAtomicCounter {
  private var storage: Int64 = 0
  func increment() { _ = OSAtomicIncrement64Barrier(&storage) }
  func reset() {
    while true {
      let old = OSAtomicAdd64Barrier(0, &storage)
      if old == 0 || OSAtomicCompareAndSwap64Barrier(old, 0, &storage) {
        return
      }
    }
  }
  var value: Int { Int(OSAtomicAdd64Barrier(0, &storage)) }
}

                                                                            
                                                                         
                                                                         
private final class PwVioTimebaseIngressGate {
  enum Phase: UInt64 { case open = 0, rejecting = 1, sealed = 2 }
  struct Entry { let generation: UInt32 }

  private static let activeMask: UInt64 = 0x3fff_ffff
  private static let phaseShift: UInt64 = 30
  private var packed: Int64
  private let sealLock = NSLock()
  private var sealGeneration: Int?
  private var sealCompletions: [() -> Void] = []

  init() {
    packed = Int64(bitPattern: Phase.sealed.rawValue << Self.phaseShift)
  }

  private func load() -> UInt64 {
    UInt64(bitPattern: OSAtomicAdd64Barrier(0, &packed))
  }

  func prepare(generation: Int) -> Bool {
    let expectedGeneration = UInt32(truncatingIfNeeded: generation)
    while true {
      let old = load()
      let phase = (old >> Self.phaseShift) & 0x3
      guard phase == Phase.sealed.rawValue,
            old & Self.activeMask == 0 else { return false }
      let next = (UInt64(expectedGeneration) << 32) |
        (Phase.sealed.rawValue << Self.phaseShift)
      if OSAtomicCompareAndSwap64Barrier(
        Int64(bitPattern: old), Int64(bitPattern: next), &packed
      ) {
        sealLock.lock()
        sealGeneration = nil
        sealCompletions.removeAll(keepingCapacity: true)
        sealLock.unlock()
        return true
      }
    }
  }

  func open(generation: Int) -> Bool {
    let expectedGeneration = UInt32(truncatingIfNeeded: generation)
    while true {
      let old = load()
      guard UInt32(truncatingIfNeeded: old >> 32) == expectedGeneration,
            old & Self.activeMask == 0,
            (old >> Self.phaseShift) & 0x3 == Phase.sealed.rawValue else {
        return false
      }
      let next = old & ~(UInt64(0x3) << Self.phaseShift)
      if OSAtomicCompareAndSwap64Barrier(
        Int64(bitPattern: old), Int64(bitPattern: next), &packed
      ) { return true }
    }
  }

  func enter() -> Entry? {
    while true {
      let old = load()
      guard (old >> Self.phaseShift) & 0x3 == Phase.open.rawValue else {
        return nil
      }
      let active = old & Self.activeMask
      guard active < Self.activeMask else { return nil }
      if OSAtomicCompareAndSwap64Barrier(
        Int64(bitPattern: old), Int64(bitPattern: old + 1), &packed
      ) {
        return Entry(generation: UInt32(truncatingIfNeeded: old >> 32))
      }
    }
  }

  func leave(_ entry: Entry) {
    while true {
      let old = load()
      guard UInt32(truncatingIfNeeded: old >> 32) == entry.generation,
            old & Self.activeMask > 0 else { return }
      let next = old - 1
      if OSAtomicCompareAndSwap64Barrier(
        Int64(bitPattern: old), Int64(bitPattern: next), &packed
      ) {
        if next & Self.activeMask == 0 { completeSealIfReady() }
        return
      }
    }
  }

  func closeWhenQuiescent(
    generation: Int,
    completion: @escaping () -> Void
  ) {
    let expectedGeneration = UInt32(truncatingIfNeeded: generation)
    while true {
      let old = load()
      guard UInt32(truncatingIfNeeded: old >> 32) == expectedGeneration else {
        completion()
        return
      }
      let phase = (old >> Self.phaseShift) & 0x3
      if phase == Phase.sealed.rawValue {
        completion()
        return
      }
      if phase == Phase.open.rawValue {
        let next = old | (Phase.rejecting.rawValue << Self.phaseShift)
        if !OSAtomicCompareAndSwap64Barrier(
          Int64(bitPattern: old), Int64(bitPattern: next), &packed
        ) { continue }
      }
      break
    }

    sealLock.lock()
    if sealCompletions.isEmpty {
      sealGeneration = generation
    }
    if sealGeneration == generation { sealCompletions.append(completion) }
    sealLock.unlock()
    completeSealIfReady()
  }

  private func completeSealIfReady() {
    let bits = load()
    guard bits & Self.activeMask == 0,
          (bits >> Self.phaseShift) & 0x3 == Phase.rejecting.rawValue else {
      return
    }
    let generation = Int(UInt32(truncatingIfNeeded: bits >> 32))
    guard sealLock.try() else { return }
    var completions: [() -> Void] = []
    if sealGeneration == generation, !sealCompletions.isEmpty {
      let clearedPhase = bits & ~(UInt64(0x3) << Self.phaseShift)
      let sealed = clearedPhase | (Phase.sealed.rawValue << Self.phaseShift)
      if OSAtomicCompareAndSwap64Barrier(
        Int64(bitPattern: bits), Int64(bitPattern: sealed), &packed
      ) {
        completions = sealCompletions
        sealCompletions.removeAll(keepingCapacity: true)
        sealGeneration = nil
      }
    }
    sealLock.unlock()
    for completion in completions { completion() }
  }
}

private final class PwVioShadowStopJoin {
  private let lock = NSLock()
  private let completion: ([String: Any]) -> Void
  private var receipt: [String: Any]?
  private var timebaseClosed = false
  private var delivered = false

  init(completion: @escaping ([String: Any]) -> Void) {
    self.completion = completion
  }

  func receive(receipt: [String: Any]) {
    finish(receipt: receipt, closed: nil)
  }

  func markTimebaseClosed() {
    finish(receipt: nil, closed: true)
  }

  private func finish(receipt newReceipt: [String: Any]?, closed: Bool?) {
    lock.lock()
    if let newReceipt { receipt = newReceipt }
    if closed == true { timebaseClosed = true }
    let ready = !delivered && timebaseClosed && receipt != nil
    let finalReceipt = ready ? receipt : nil
    if ready { delivered = true }
    lock.unlock()
    if let finalReceipt {
      DispatchQueue.main.async { self.completion(finalReceipt) }
    }
  }
}

private final class PwVioRawSourceLedger {
  let invalid = PwVioGenerationCounter()
  let lockContention = PwVioGenerationCounter()
  let staleGeneration = PwVioGenerationCounter()

  func begin(_ generation: Int) {
    invalid.begin(generation)
    lockContention.begin(generation)
    staleGeneration.begin(generation)
  }
}

private struct PwVioRawIntrinsics {
  let sessionId: String
  let sessionEpoch: Int
  let sessionGeneration: Int
  let intrinsicMatrix: matrix_float3x3
  let imageResolutionWidth: Double
  let imageResolutionHeight: Double
  let referenceTrackingState: String
  let referenceTrackingReason: String

  var wire: [String: Any] {
    [
      "schema": "pw.vio.ios.intrinsics-raw/1",
      "sessionId": sessionId,
      "sessionEpoch": sessionEpoch,
      "sessionGeneration": sessionGeneration,
                                                                          
                                                                              
      "intrinsicMatrixColumnMajor": [
        Double(intrinsicMatrix.columns.0.x),
        Double(intrinsicMatrix.columns.0.y),
        Double(intrinsicMatrix.columns.0.z),
        Double(intrinsicMatrix.columns.1.x),
        Double(intrinsicMatrix.columns.1.y),
        Double(intrinsicMatrix.columns.1.z),
        Double(intrinsicMatrix.columns.2.x),
        Double(intrinsicMatrix.columns.2.y),
        Double(intrinsicMatrix.columns.2.z),
      ],
      "imageResolutionWidth": imageResolutionWidth,
      "imageResolutionHeight": imageResolutionHeight,
      "source": "ARCamera.intrinsics",
      "referenceTrackingState": referenceTrackingState,
      "referenceTrackingReason": referenceTrackingReason,
    ]
  }
}

                          

                                          
   
                                                                                                   
                                                                            
public final class PwVioTimebase {

  public static let shared = PwVioTimebase()

                                                                           
                                                                                            
  private var latestIntrinsics: PwVioRawIntrinsics?

                                                                                          
                                                                                   
  public func snapshotIntrinsics() -> [String: Any]? {
    lock.lock(); defer { lock.unlock() }
    return latestIntrinsics?.wire
  }

  private let lock = NSLock()

                                                                             
                                                                         
  private var rawSources: [String: PwVioRawSampleWindow]
  private var sourceLedgers: [String: PwVioRawSourceLedger]
  private var timebaseSessionGeneration = 0
  private let timebaseIngress = PwVioTimebaseIngressGate()
  private var timebaseSessionId = ""
  private var timebaseSessionEpoch = 0
  private let outOfSessionStaleObservations = PwVioAtomicCounter()
  private let syncClockUnavailable = PwVioGenerationCounter()
  private let intrinsicsLockContention = PwVioGenerationCounter()
  private let intrinsicsStaleGeneration = PwVioGenerationCounter()
  private var intrinsicsAccepted = 0
  private var intrinsicsOverwritten = 0

                                                                  
  private var sessionStartPair: PwVioClockSandwich?

  public static let sourceCoreMotionAccelerometer = "coreMotionAccelerometer"
  public static let sourceCoreMotionGyroscope = "coreMotionGyroscope"
  public static let sourceArFrame = "arFrame"
  public static let sourceCaptureRawPts = "capturePtsRaw"
  public static let sourceCaptureHostPts = "capturePtsHost"

  public init() {
    let sources = [
      Self.sourceCoreMotionAccelerometer,
      Self.sourceCoreMotionGyroscope,
      Self.sourceArFrame,
      Self.sourceCaptureRawPts,
      Self.sourceCaptureHostPts,
    ]
    rawSources = Dictionary(uniqueKeysWithValues: sources.map {
      ($0, PwVioRawSampleWindow())
    })
    sourceLedgers = Dictionary(uniqueKeysWithValues: sources.map {
      ($0, PwVioRawSourceLedger())
    })
  }

                             

                                                                
  @discardableResult
  public func beginSession(sessionId: String, sessionEpoch: Int) -> Bool {
    lock.lock()
    let generation = timebaseSessionGeneration + 1
    guard timebaseIngress.prepare(generation: generation) else {
      lock.unlock()
      return false
    }
    outOfSessionStaleObservations.reset()
    let pair = pwVioTimebaseSampleClockPair()
    timebaseSessionGeneration = generation
    timebaseSessionId = sessionId
    timebaseSessionEpoch = sessionEpoch
    for source in rawSources.keys {
      rawSources[source]?.clear()
      sourceLedgers[source]?.begin(generation)
    }
    syncClockUnavailable.begin(generation)
    intrinsicsLockContention.begin(generation)
    intrinsicsStaleGeneration.begin(generation)
    intrinsicsAccepted = 0
    intrinsicsOverwritten = 0
    latestIntrinsics = nil
    sessionStartPair = pair
    let opened = timebaseIngress.open(generation: generation)
    lock.unlock()
    return opened
  }

                                                                              
  @discardableResult
  public func remeasure() -> [String: Any] {
    lock.lock()
    let pair = pwVioTimebaseSampleClockPair()
    let start = sessionStartPair
    let sessionId = timebaseSessionId
    let sessionEpoch = timebaseSessionEpoch
    let generation = timebaseSessionGeneration
    lock.unlock()
    let startBefore: Any = start.map { $0.uptimeRawBeforeSeconds } ?? NSNull()
    let startMonotonic: Any = start.map { $0.monotonicSeconds } ?? NSNull()
    let startAfter: Any = start.map { $0.uptimeRawAfterSeconds } ?? NSNull()
    return [
      "schema": "pw.vio.timebase-remeasure-raw/1",
      "sessionId": sessionId,
      "sessionEpoch": sessionEpoch,
      "sessionGeneration": generation,
      "uptimeRawBeforeSeconds": pair.uptimeRawBeforeSeconds,
      "monotonicSeconds": pair.monotonicSeconds,
      "uptimeRawAfterSeconds": pair.uptimeRawAfterSeconds,
      "sessionStartUptimeRawBeforeSeconds": startBefore,
      "sessionStartMonotonicSeconds": startMonotonic,
      "sessionStartUptimeRawAfterSeconds": startAfter,
    ]
  }

                                                                             

                                                                                                        
                                                                                 
  public func noteCoreMotionAccelerometer(timestamp: TimeInterval) {
    note(
      source: PwVioTimebase.sourceCoreMotionAccelerometer,
      rawSeconds: timestamp
    )
  }

  public func noteCoreMotionGyroscope(timestamp: TimeInterval) {
    note(source: PwVioTimebase.sourceCoreMotionGyroscope, rawSeconds: timestamp)
  }

                                                   
    
                                                                                         
                                                                                             
                                                                                                        
                                                                                            
                                                             
    
                                                                                     
  private let motion = CMMotionManager()
                                                                            
                                                                              
                                                                           
  private let shadowLifecycleLock = NSLock()
  private var shadowLifecycleGeneration = 0
  private var shadowMotionDesired = false
  private var shadowResumeAuthorized = false
  private var shadowAccelerometerHz: Double?
  private var shadowGyroscopeHz: Double?
                                                                              
                                                                         
                                                                       
  private var rawMotionFailureScheduledGeneration = -1
  private let motionFailureQueue = DispatchQueue(
    label: "pw.vio.timebase.motion-failure",
    qos: .utility
  )
                                                                              
                                                                   
  private let motionQueue = PwVioSensorIngress.operationQueue

                                                                                    
                                                                      
  @discardableResult
  public func startRawCoreMotionFeed(accelerometerHz: Double, gyroscopeHz: Double) -> Bool {
    shadowLifecycleLock.lock()
    guard accelerometerHz.isFinite, accelerometerHz > 0,
          gyroscopeHz.isFinite, gyroscopeHz > 0 else {
      shadowLifecycleLock.unlock()
      return false
    }
    shadowLifecycleGeneration += 1
    let generation = shadowLifecycleGeneration
    shadowMotionDesired = true
    shadowAccelerometerHz = accelerometerHz
    shadowGyroscopeHz = gyroscopeHz
    let result = startRawCoreMotionFeedLocked(
      accelerometerHz: accelerometerHz,
      gyroscopeHz: gyroscopeHz,
      lifecycleGeneration: generation
    )
    if !result {
      rollbackShadowStartIntentLocked()
    }
    shadowLifecycleLock.unlock()
    return result
  }

  private func startRawCoreMotionFeedLocked(
    accelerometerHz: Double,
    gyroscopeHz: Double,
    lifecycleGeneration: Int
  ) -> Bool {
    guard accelerometerHz.isFinite, accelerometerHz > 0,
          gyroscopeHz.isFinite, gyroscopeHz > 0,
          motion.isAccelerometerAvailable,
          motion.isGyroAvailable else { return false }
    stopRawCoreMotionFeedLocked()
    motion.accelerometerUpdateInterval = 1.0 / accelerometerHz
    motion.gyroUpdateInterval = 1.0 / gyroscopeHz
                                                                               
    motion.startGyroUpdates(to: motionQueue) { [weak self] sample, error in
      if error != nil {
        self?.scheduleRawMotionFailure(generation: lifecycleGeneration)
        return
      }
      guard let self, let sample else { return }
      self.noteCoreMotionGyroscope(timestamp: sample.timestamp)
      if #available(iOS 11.0, *) {
        _ = PwVioSlamFeeder.shared.enqueue(gyroscope: sample)
      }
    }
    motion.startAccelerometerUpdates(to: motionQueue) { [weak self] sample, error in
      if error != nil {
        self?.scheduleRawMotionFailure(generation: lifecycleGeneration)
        return
      }
      guard let self, let sample else { return }
      self.noteCoreMotionAccelerometer(timestamp: sample.timestamp)
      if #available(iOS 11.0, *) {
        _ = PwVioSlamFeeder.shared.enqueue(acceleration: sample)
      }
    }
                                                                          
                                                                                
                                                                               
                                                                               
                                                                             
                                                         
    return true
  }

  private func scheduleRawMotionFailure(generation: Int) {
    motionFailureQueue.async { [weak self] in
      guard let self else { return }
      self.shadowLifecycleLock.lock()
      let current = generation == self.shadowLifecycleGeneration &&
        self.shadowMotionDesired &&
        self.rawMotionFailureScheduledGeneration != generation
      if current {
        self.rawMotionFailureScheduledGeneration = generation
        self.rollbackShadowStartIntentLocked()
      }
      self.shadowLifecycleLock.unlock()
      if current, #available(iOS 11.0, *) {
        PwVioSlamFeeder.shared.stop { _ in }
      }
    }
  }

  public func stopRawCoreMotionFeed() {
    shadowLifecycleLock.lock()
    shadowLifecycleGeneration += 1
    shadowMotionDesired = false
    stopRawCoreMotionFeedLocked()
    shadowLifecycleLock.unlock()
  }

  private func stopRawCoreMotionFeedLocked() {
                                                                           
                                                                             
    motion.stopAccelerometerUpdates()
    motion.stopGyroUpdates()
  }

                                                                            
                                                                         
  public func suspendShadowPipeline() {
    shadowLifecycleLock.lock()
    shadowLifecycleGeneration += 1
    shadowMotionDesired = false
    stopRawCoreMotionFeedLocked()
    shadowLifecycleLock.unlock()
  }

  private func revokeShadowAuthorization() {
    shadowLifecycleLock.lock()
    shadowLifecycleGeneration += 1
    rollbackShadowStartIntentLocked()
    shadowLifecycleLock.unlock()
  }

                                                                                
                                                                              
  private func rollbackShadowStartIntentLocked() {
    shadowMotionDesired = false
    shadowResumeAuthorized = false
    shadowAccelerometerHz = nil
    shadowGyroscopeHz = nil
    stopRawCoreMotionFeedLocked()
  }

  public func shutdownShadowPipeline(
    completion: @escaping ([String: Any]) -> Void
  ) {
    revokeShadowAuthorization()
    lock.lock()
    let timebaseGeneration = timebaseSessionGeneration
    lock.unlock()
    let join = PwVioShadowStopJoin(completion: completion)
    if #available(iOS 11.0, *) {
                                                                             
                                                                            
                                                     
      PwVioSlamFeeder.shared.stop { receipt in
        join.receive(receipt: receipt)
      }
    } else {
      join.receive(receipt: [
          "schema": "pw.vio.shadow-terminal-unavailable/1",
          "receiptAvailable": false,
      ])
    }
    timebaseIngress.closeWhenQuiescent(generation: timebaseGeneration) {
      join.markTimebaseClosed()
    }
  }

  public func resumeShadowPipeline() {
    guard #available(iOS 11.0, *) else { return }
    shadowLifecycleLock.lock()
    guard shadowResumeAuthorized else {
      shadowLifecycleLock.unlock()
      return
    }
    guard PwVioSlamFeeder.shared.isRunning else {
      shadowLifecycleGeneration += 1
      rollbackShadowStartIntentLocked()
      shadowLifecycleLock.unlock()
      return
    }
    shadowLifecycleGeneration += 1
    shadowMotionDesired = true
    if let accelerometerHz = shadowAccelerometerHz,
       let gyroscopeHz = shadowGyroscopeHz {
      let started = startRawCoreMotionFeedLocked(
        accelerometerHz: accelerometerHz,
        gyroscopeHz: gyroscopeHz,
        lifecycleGeneration: shadowLifecycleGeneration
      )
      if !started { rollbackShadowStartIntentLocked() }
    } else {
      rollbackShadowStartIntentLocked()
    }
    shadowLifecycleLock.unlock()
  }

  private func beginShadowStartIntent(
    accelerometerHz: Double,
    gyroscopeHz: Double
  ) -> Int {
    shadowLifecycleLock.lock()
    shadowLifecycleGeneration += 1
    shadowResumeAuthorized = true
    shadowMotionDesired = true
    shadowAccelerometerHz = accelerometerHz
    shadowGyroscopeHz = gyroscopeHz
    let generation = shadowLifecycleGeneration
    shadowLifecycleLock.unlock()
    return generation
  }

  private struct PwVioShadowStartIntentResult {
    let accepted: Bool
    let failureReason: String?
  }

                                                                          
                                                                           
                                                                          
                                                                           
                                                           
  private func completeShadowStartIntent(
    generation: Int,
    rc: Int32
  ) -> PwVioShadowStartIntentResult {
    shadowLifecycleLock.lock()
    let intentCurrent = generation == shadowLifecycleGeneration
    guard intentCurrent && shadowMotionDesired && shadowResumeAuthorized else {
      shadowLifecycleLock.unlock()
      return PwVioShadowStartIntentResult(
        accepted: false,
        failureReason: "stale-start-receipt"
      )
    }
    guard rc == 1 && PwVioSlamFeeder.shared.isRunning else {
      rollbackShadowStartIntentLocked()
      shadowLifecycleLock.unlock()
      return PwVioShadowStartIntentResult(
        accepted: false,
        failureReason: nil
      )
    }
    guard let accelerometerHz = shadowAccelerometerHz,
          let gyroscopeHz = shadowGyroscopeHz else {
      rollbackShadowStartIntentLocked()
      shadowLifecycleLock.unlock()
      return PwVioShadowStartIntentResult(
        accepted: false,
        failureReason: "raw-imu-start-failed"
      )
    }
    let rawFeedStarted = startRawCoreMotionFeedLocked(
        accelerometerHz: accelerometerHz,
        gyroscopeHz: gyroscopeHz,
        lifecycleGeneration: generation
    )
    if !rawFeedStarted {
      rollbackShadowStartIntentLocked()
      shadowLifecycleLock.unlock()
      return PwVioShadowStartIntentResult(
        accepted: false,
        failureReason: "raw-imu-start-failed"
      )
    }
    shadowLifecycleLock.unlock()
    return PwVioShadowStartIntentResult(
      accepted: true,
      failureReason: nil
    )
  }

  public func startShadowPipeline(
    slamConfigPath: String,
    deviceConfigPath: String,
    sessionId: String,
    sessionEpoch: Int,
    effectiveConfigSha256: String,
    inputIdentitySha256: String,
    downsampleFactor: Int,
    downsampleFormula: String,
    requestedCameraHz: Double,
    cameraTimeOffsetSeconds: Double,
    accelerationScale: Double,
    requestedAccelerometerHz: Double,
    requestedGyroscopeHz: Double,
    completion: @escaping ([String: Any]) -> Void
  ) {
    guard #available(iOS 11.0, *) else {
      DispatchQueue.main.async {
        completion([
          "schema": "pw.vio.shadow-start-receipt/1",
          "rc": 0,
          "generation": 0,
          "failureReason": "unsupported-platform",
          "snapshot": NSNull(),
        ])
      }
      return
    }
    guard requestedCameraHz.isFinite,
          requestedCameraHz > 0,
          cameraTimeOffsetSeconds.isFinite,
          requestedAccelerometerHz.isFinite,
          requestedAccelerometerHz > 0,
          requestedGyroscopeHz.isFinite,
          requestedGyroscopeHz > 0 else {
      DispatchQueue.main.async {
        completion([
          "schema": "pw.vio.shadow-start-receipt/1",
          "rc": 0,
          "generation": 0,
          "failureReason": "invalid-raw-imu-request",
          "snapshot": NSNull(),
        ])
      }
      return
    }
    let generation = beginShadowStartIntent(
      accelerometerHz: requestedAccelerometerHz,
      gyroscopeHz: requestedGyroscopeHz
    )
    PwVioSlamFeeder.shared.start(
      slamConfigPath: slamConfigPath,
      deviceConfigPath: deviceConfigPath,
      sessionId: sessionId,
      sessionEpoch: sessionEpoch,
      effectiveConfigSha256: effectiveConfigSha256,
      inputIdentitySha256: inputIdentitySha256,
      downsampleFactor: downsampleFactor,
      downsampleFormula: downsampleFormula,
      requestedCameraHz: requestedCameraHz,
      cameraTimeOffsetSeconds: cameraTimeOffsetSeconds,
      accelerationScale: accelerationScale,
      requestedAccelerometerHz: requestedAccelerometerHz,
      requestedGyroscopeHz: requestedGyroscopeHz
    ) { [weak self] rc, feederGeneration in
      guard let self else {
        completion([
          "schema": "pw.vio.shadow-start-receipt/1",
          "rc": 0,
          "generation": feederGeneration,
          "failureReason": "owner-released",
          "snapshot": NSNull(),
        ])
        return
      }
      let intentResult = self.completeShadowStartIntent(
        generation: generation,
        rc: rc
      )
      let directSnapshot = intentResult.accepted
        ? PwVioSlamFeeder.shared.directRunningReceipt(
          expectedGeneration: feederGeneration
        ) : nil
      let success = directSnapshot != nil
      let failureReason: String
      if success {
        failureReason = "none"
      } else if let exactFailure = intentResult.failureReason {
        failureReason = exactFailure
      } else if rc == 0 && PwVioSlamFeeder.shared.isRunning {
        failureReason = "configuration-conflict"
      } else if rc == 0 {
        failureReason = "create-failed-or-canceled"
      } else {
        failureReason = "direct-running-receipt-unavailable"
      }
      completion([
        "schema": "pw.vio.shadow-start-receipt/1",
        "rc": success ? 1 : 0,
        "generation": feederGeneration,
        "failureReason": failureReason,
        "snapshot": directSnapshot ?? NSNull(),
      ])
    }
  }

  #if canImport(ARKit)
                                                      
    @available(iOS 11.0, *)
    public func noteARFrame(_ frame: ARFrame) {
      guard let entry = timebaseIngress.enter() else { return }
      defer { timebaseIngress.leave(entry) }
      let generation = Int(entry.generation)
      note(
        source: PwVioTimebase.sourceArFrame,
        rawSeconds: frame.timestamp,
        generation: generation
      )
      noteIntrinsics(frame.camera, generation: generation)
    }

                                               
       
                                                                           
                                                                                  
                                                                                  
                                                                                      
       
                                                                                                          
                                                                                                        
    @available(iOS 11.0, *)
    private func noteIntrinsics(_ camera: ARCamera, generation: Int) {
      let res = camera.imageResolution
      let tracking: (state: String, reason: String)
      switch camera.trackingState {
      case .normal:
        tracking = ("normal", "none")
      case .notAvailable:
        tracking = ("notAvailable", "none")
      case .limited(let reason):
        switch reason {
        case .initializing:
          tracking = ("limited", "initializing")
        case .excessiveMotion:
          tracking = ("limited", "excessiveMotion")
        case .insufficientFeatures:
          tracking = ("limited", "insufficientFeatures")
        case .relocalizing:
          tracking = ("limited", "relocalizing")
        @unknown default:
          tracking = ("limited", "unknown")
        }
      @unknown default:
        tracking = ("unknown", "unknown")
      }
      guard generation > 0 else {
        outOfSessionStaleObservations.increment()
        return
      }
      guard lock.try() else {
        if !intrinsicsLockContention.increment(generation: generation) {
          outOfSessionStaleObservations.increment()
        }
        return
      }
      guard generation == timebaseSessionGeneration else {
        lock.unlock()
        if !intrinsicsStaleGeneration.increment(generation: generation) {
          outOfSessionStaleObservations.increment()
        }
        return
      }
      if latestIntrinsics != nil { intrinsicsOverwritten += 1 }
      intrinsicsAccepted += 1
      latestIntrinsics = PwVioRawIntrinsics(
        sessionId: timebaseSessionId,
        sessionEpoch: timebaseSessionEpoch,
        sessionGeneration: generation,
        intrinsicMatrix: camera.intrinsics,
        imageResolutionWidth: Double(res.width),
        imageResolutionHeight: Double(res.height),
        referenceTrackingState: tracking.state,
        referenceTrackingReason: tracking.reason
      )
      lock.unlock()
    }

                                                                   
                                                                                              
                                                                   
    @available(iOS 11.0, *)
    public func exposureDurationSeconds(of frame: ARFrame) -> Double? {
      guard #available(iOS 16.0, *) else { return nil }
      let key = kCGImagePropertyExifExposureTime as String
      guard let v = frame.exifData[key] as? NSNumber else { return nil }
      let d = v.doubleValue
      return d.isFinite && d > 0 ? d : nil
    }
  #endif

                                      
                                                                             
                                                                                          
                                                                                                                  
  public func noteSampleBuffer(_ sampleBuffer: CMSampleBuffer, session: AVCaptureSession) {
    guard let entry = timebaseIngress.enter() else { return }
    defer { timebaseIngress.leave(entry) }
    let generation = Int(entry.generation)
    let pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer)
    guard pts.isValid && !pts.isIndefinite else { return }
    note(
      source: PwVioTimebase.sourceCaptureRawPts,
      rawSeconds: CMTimeGetSeconds(pts),
      generation: generation
    )

    guard let clock = captureClock(of: session) else {
      recordSyncClockUnavailable(generation: generation)
      return
    }
    let host = CMSyncConvertTime(pts, from: clock, to: CMClockGetHostTimeClock())
    guard host.isValid && !host.isIndefinite else {
      recordSyncClockUnavailable(generation: generation)
      return
    }
    note(
      source: PwVioTimebase.sourceCaptureHostPts,
      rawSeconds: CMTimeGetSeconds(host),
      generation: generation
    )
  }

                                                                               
                                                                                   
  private func captureClock(of session: AVCaptureSession) -> CMClock? {
    if #available(iOS 15.4, *) {
      return session.synchronizationClock
    } else {
      return session.masterClock
    }
  }

                                                                       
  public func exposureDurationSeconds(of device: AVCaptureDevice) -> Double? {
    let d = device.exposureDuration
    guard d.isValid && !d.isIndefinite else { return nil }
    let s = CMTimeGetSeconds(d)
    return s.isFinite && s > 0 ? s : nil
  }

                       

  private func recordSyncClockUnavailable(generation: Int) {
    if !syncClockUnavailable.increment(generation: generation) {
      outOfSessionStaleObservations.increment()
    }
  }

  private func note(source: String, rawSeconds: Double) {
    guard let entry = timebaseIngress.enter() else { return }
    defer { timebaseIngress.leave(entry) }
    note(
      source: source,
      rawSeconds: rawSeconds,
      generation: Int(entry.generation)
    )
  }

  private func note(
    source: String,
    rawSeconds: Double,
    generation: Int
  ) {
    guard let ledger = sourceLedgers[source] else {
      outOfSessionStaleObservations.increment()
      return
    }
    guard generation > 0, rawSeconds.isFinite else {
      if !ledger.invalid.increment(generation: generation) {
        outOfSessionStaleObservations.increment()
      }
      return
    }
                                                                               
                                                              
    let pair = pwVioTimebaseSampleClockPair()
    guard lock.try() else {
      if !ledger.lockContention.increment(generation: generation) {
        outOfSessionStaleObservations.increment()
      }
      return
    }
    guard generation == timebaseSessionGeneration,
          let window = rawSources[source] else {
      lock.unlock()
      if !ledger.staleGeneration.increment(generation: generation) {
        outOfSessionStaleObservations.increment()
      }
      return
    }
    window.add(sourceSeconds: rawSeconds, pair: pair)
    lock.unlock()
  }

                 

                                                                                                   
                                             
  public func snapshot() -> [String: Any] {
    lock.lock()
    let pair = pwVioTimebaseSampleClockPair()
    let start = sessionStartPair
    let generation = timebaseSessionGeneration
    let sessionId = timebaseSessionId
    let sessionEpoch = timebaseSessionEpoch
    let syncFail = syncClockUnavailable.value(generation: generation)
    var per: [String: Any] = [:]
    for (source, window) in rawSources {
      let samples = window.drain()
      let ledger = sourceLedgers[source]!
      let invalid = ledger.invalid.value(generation: generation)
      let contention = ledger.lockContention.value(generation: generation)
      let stale = ledger.staleGeneration.value(generation: generation)
      let accepted = window.totalAdded
      let rejected = invalid + contention + stale
      per[source] = [
        "sampleCount": window.totalAdded,
        "rawSamplesDropped": window.overwritten,
        "rawSamplesAttempted": accepted + rejected,
        "rawSamplesAccepted": accepted,
        "rawSamplesRejected": rejected,
        "rawSamplesDelivered": window.totalDelivered,
        "rawSamplesBatchCount": samples.count,
        "rejectionReasons": [
          "invalid_input": invalid,
          "lock_contention": contention,
          "stale_generation": stale,
        ],
        "rawSamples": samples.map { $0.wire },
      ]
    }
    let intrinsicsContentionCount = intrinsicsLockContention.value(
      generation: generation
    )
    let intrinsicsStaleCount = intrinsicsStaleGeneration.value(
      generation: generation
    )
    let intrinsicsRejected = intrinsicsContentionCount + intrinsicsStaleCount
    let intrinsicsWire: [String: Any] = [
      "rawSamplesAttempted": intrinsicsAccepted + intrinsicsRejected,
      "rawSamplesAccepted": intrinsicsAccepted,
      "rawSamplesRejected": intrinsicsRejected,
      "intrinsicsOverwritten": intrinsicsOverwritten,
      "rejectionReasons": [
                                                                            
                                                                         
        "invalid_input": 0,
        "lock_contention": intrinsicsContentionCount,
        "stale_generation": intrinsicsStaleCount,
      ],
    ]
    lock.unlock()
    let startBefore: Any = start.map { $0.uptimeRawBeforeSeconds } ?? NSNull()
    let startMonotonic: Any = start.map { $0.monotonicSeconds } ?? NSNull()
    let startAfter: Any = start.map { $0.uptimeRawAfterSeconds } ?? NSNull()

    var out: [String: Any] = [
      "schema": "pw.vio.timebase-raw/5",
      "sessionId": sessionId,
      "sessionEpoch": sessionEpoch,
      "sessionGeneration": generation,
      "uptimeRawBeforeSeconds": pair.uptimeRawBeforeSeconds,
      "monotonicSeconds": pair.monotonicSeconds,
      "uptimeRawAfterSeconds": pair.uptimeRawAfterSeconds,
      "sessionStartUptimeRawBeforeSeconds": startBefore,
      "sessionStartMonotonicSeconds": startMonotonic,
      "sessionStartUptimeRawAfterSeconds": startAfter,
      "syncClockUnavailableCount": syncFail,
      "synchronizationClockAvailable": {
        if #available(iOS 15.4, *) { return true } else { return false }
      }(),
      "arFrameExifAvailable": {
        if #available(iOS 16.0, *) { return true } else { return false }
      }(),
      "outOfSessionStaleObservations": outOfSessionStaleObservations.value,
      "intrinsicsAccounting": intrinsicsWire,
    ]

    out["sources"] = per
    return out
  }
}

                         

#if canImport(Flutter)
  public final class PwVioTimebasePlugin: NSObject, FlutterPlugin {
    public static func register(with registrar: FlutterPluginRegistrar) {
      let channel = FlutterMethodChannel(
        name: PwVioTimebaseIdentifiers.methodChannel,
        binaryMessenger: registrar.messenger()
      )
      registrar.addMethodCallDelegate(PwVioTimebasePlugin(), channel: channel)
    }

    public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
      switch call.method {
      case "beginSession":
        let args = call.arguments as? [String: Any] ?? [:]
        guard let sessionId = args["sessionId"] as? String,
              let sessionEpoch = args["sessionEpoch"] as? Int else {
          result(FlutterError(
            code: "PW_VIO_INVALID_SESSION",
            message: "beginSession requires sessionId/sessionEpoch",
            details: nil
          ))
          return
        }
        let began = PwVioTimebase.shared.beginSession(
          sessionId: sessionId,
          sessionEpoch: sessionEpoch
        )
        if began {
          result(nil)
        } else {
          result(FlutterError(
            code: "PW_VIO_SESSION_OVERLAP",
            message: "previous timebase session is not sealed",
            details: nil
          ))
        }
      case "remeasure":
        result(PwVioTimebase.shared.remeasure())
      case "snapshot":
        result(PwVioTimebase.shared.snapshot())
      case "startRawCoreMotionFeed":
        let args = call.arguments as? [String: Any] ?? [:]
        guard let accelerometerHz = args["accelerometerHz"] as? Double,
              let gyroscopeHz = args["gyroscopeHz"] as? Double,
              accelerometerHz.isFinite, accelerometerHz > 0,
              gyroscopeHz.isFinite, gyroscopeHz > 0 else {
          result(false)
          return
        }
        result(PwVioTimebase.shared.startRawCoreMotionFeed(
          accelerometerHz: accelerometerHz,
          gyroscopeHz: gyroscopeHz
        ))
      case "stopRawCoreMotionFeed":
        PwVioTimebase.shared.stopRawCoreMotionFeed()
        result(nil)
      case "slamStart":
        let args = call.arguments as? [String: Any] ?? [:]
        guard let slam = args["slamConfigPath"] as? String,
              let dev = args["deviceConfigPath"] as? String,
              let sessionId = args["sessionId"] as? String,
              let sessionEpoch = args["sessionEpoch"] as? Int,
              let configSha = args["effectiveConfigSha256"] as? String,
              let inputSha = args["inputIdentitySha256"] as? String,
              let downsampleFactor = args["downsampleFactor"] as? Int,
              let downsampleFormula = args["downsampleFormula"] as? String,
              let requestedCameraHz = args["requestedCameraHz"] as? Double,
              let cameraTimeOffsetSeconds = args["cameraTimeOffsetSeconds"] as? Double,
              let accelerationScale = args["accelerationScale"] as? Double,
              let requestedAccelerometerHz = args["requestedAccelerometerHz"] as? Double,
              let requestedGyroscopeHz = args["requestedGyroscopeHz"] as? Double,
              downsampleFactor > 0,
              !downsampleFormula.isEmpty,
              requestedCameraHz.isFinite,
              requestedCameraHz > 0,
              cameraTimeOffsetSeconds.isFinite,
              accelerationScale.isFinite,
              accelerationScale != 0,
              requestedAccelerometerHz.isFinite,
              requestedAccelerometerHz > 0,
              requestedGyroscopeHz.isFinite,
              requestedGyroscopeHz > 0 else {
          result([
            "schema": "pw.vio.shadow-start-receipt/1",
            "rc": 0,
            "generation": 0,
            "failureReason": "invalid-arguments",
            "snapshot": NSNull(),
          ])
          return
        }
        if #available(iOS 11.0, *) {
          PwVioTimebase.shared.startShadowPipeline(
            slamConfigPath: slam,
            deviceConfigPath: dev,
            sessionId: sessionId,
            sessionEpoch: sessionEpoch,
            effectiveConfigSha256: configSha,
            inputIdentitySha256: inputSha,
            downsampleFactor: downsampleFactor,
            downsampleFormula: downsampleFormula,
            requestedCameraHz: requestedCameraHz,
            cameraTimeOffsetSeconds: cameraTimeOffsetSeconds,
            accelerationScale: accelerationScale,
            requestedAccelerometerHz: requestedAccelerometerHz,
            requestedGyroscopeHz: requestedGyroscopeHz
          ) { receipt in
            result(receipt)
          }
        } else {
          result([
            "schema": "pw.vio.shadow-start-receipt/1",
            "rc": 0,
            "generation": 0,
            "failureReason": "unsupported-platform",
            "snapshot": NSNull(),
          ])
        }
      case "slamStop":
        PwVioTimebase.shared.shutdownShadowPipeline { receipt in
          result(receipt)
        }
      case "slamSnapshot":
        if #available(iOS 11.0, *) { result(PwVioSlamFeeder.shared.snapshot()) }
        else { result(nil) }
      case "deviceMachine":
                                                                                        
                                                                                             
        var sz = 0
        sysctlbyname("hw.machine", nil, &sz, nil, 0)
        if sz > 0 {
          var buf = [CChar](repeating: 0, count: sz)
          sysctlbyname("hw.machine", &buf, &sz, nil, 0)
          result(String(cString: buf))
        } else {
          result(nil)
        }

      case "latestIntrinsics":
        result(PwVioTimebase.shared.snapshotIntrinsics())
      default:
        result(FlutterMethodNotImplemented)
      }
    }
  }
#endif
