                                                                                   
  
                                                                                                 
                                                                                      
                                                                                                           
  
                                                                      
                                                                                 
                                                                       
                                                        
                                                                                      
                                                                         
                                                                
                                                                               
                                                                                 
                                                                
                                                                                    
                                                                                                   
                                                                                        
                                                                 
                                                           
                                                                        
                                       
  
                                                             
                                                             
                                                                                               
                                              

import AVFoundation
import Foundation

#if canImport(Flutter)
import Flutter
#endif

                                                                               

public enum PwVioThermalIdentifiers {
  public static let methodChannel = "worldswithin_vio_thermal"
  public static let eventChannel = "worldswithin_vio_thermal/events"
}

                                           

                                                                                                        
@inline(__always)
public func pwVioMonotonicMicros() -> Int64 {
  return Int64(clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1_000)
}

                                                                           
   
                                                                                                   
                                                                                                    
                                                 
public func pwVioThreadCpuMillis() -> Double {
  var info = thread_basic_info()
  var count = mach_msg_type_number_t(
    MemoryLayout<thread_basic_info_data_t>.size / MemoryLayout<integer_t>.size
  )
  let thread = mach_thread_self()
  defer { mach_port_deallocate(mach_task_self_, thread) }

  let kr: kern_return_t = withUnsafeMutablePointer(to: &info) { ptr in
    ptr.withMemoryRebound(to: integer_t.self, capacity: Int(count)) { intPtr in
      thread_info(thread, thread_flavor_t(THREAD_BASIC_INFO), intPtr, &count)
    }
  }
  guard kr == KERN_SUCCESS else { return -1 }

  let userMs =
    Double(info.user_time.seconds) * 1000.0
    + Double(info.user_time.microseconds) / 1000.0
  let sysMs =
    Double(info.system_time.seconds) * 1000.0
    + Double(info.system_time.microseconds) / 1000.0
  return userMs + sysMs
}

                 

public enum PwVioCameraStreamState: String {
  case running
  case interrupted
  case stopped
  case unknown
}

                                                                                       
public struct PwVioThermalSnapshot {
  public var tsUs: Int64
  public var rawStatus: Int
  public var statusReadable: Bool
  public var lowPowerMode: Bool
  public var cameraStream: PwVioCameraStreamState
  public var interruptionReason: Int?
  public var systemPressureLevel: String?
  public var systemPressureFactors: Int?
  public var activeProcessorCount: Int

  public func asDictionary() -> [String: Any] {
    var d: [String: Any] = [
      "schema": 1,
      "platform": "ios",
      "tsUs": NSNumber(value: tsUs),
      "rawStatus": rawStatus,
      "statusReadable": statusReadable,
      "lowPowerMode": lowPowerMode,
      "cameraStream": cameraStream.rawValue,
      "activeProcessorCount": activeProcessorCount,
    ]
                                                                                     
                                                                            
    if let r = interruptionReason { d["interruptionReason"] = r }
    if let l = systemPressureLevel { d["systemPressureLevel"] = l }
    if let f = systemPressureFactors { d["systemPressureFactors"] = f }
    return d
  }
}

                                                    

                                                                   
public final class PwVioThermalMonitor: NSObject {

                                                     
  public var onSnapshot: ((PwVioThermalSnapshot) -> Void)?

  private var cameraStream: PwVioCameraStreamState = .unknown
  private var lastInterruptionReason: Int?
  private var lastPressureLevel: String?
  private var lastPressureFactors: Int?
  private var observing = false

                                                                           
                                                                                                     
                                                       
  private weak var session: AVCaptureSession?
  private weak var videoDevice: AVCaptureDevice?

  public func attach(session: AVCaptureSession?, videoDevice: AVCaptureDevice?) {
    self.session = session
    self.videoDevice = videoDevice
    if let s = session {
      cameraStream = s.isRunning ? .running : .stopped
    }
  }

  public func start() {
    guard !observing else { return }
    observing = true
    let nc = NotificationCenter.default
    nc.addObserver(
      self,
      selector: #selector(thermalStateChanged),
      name: ProcessInfo.thermalStateDidChangeNotification,
      object: nil)
    nc.addObserver(
      self,
      selector: #selector(sessionWasInterrupted(_:)),
      name: AVCaptureSession.wasInterruptedNotification,
      object: nil)
    nc.addObserver(
      self,
      selector: #selector(sessionInterruptionEnded(_:)),
      name: AVCaptureSession.interruptionEndedNotification,
      object: nil)
    emit()
  }

  public func stop() {
    guard observing else { return }
    observing = false
    NotificationCenter.default.removeObserver(self)
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  @objc private func thermalStateChanged() {
    emit()
  }

  @objc private func sessionWasInterrupted(_ note: Notification) {
    cameraStream = .interrupted
    if let n = note.userInfo?[AVCaptureSessionInterruptionReasonKey] as? NSNumber {
      lastInterruptionReason = n.intValue
    } else {
      lastInterruptionReason = nil
    }
                                                                                
                                                                         
    if #available(iOS 11.1, *),
      let sp = note.userInfo?[AVCaptureSessionInterruptionSystemPressureStateKey]
        as? AVCaptureDevice.SystemPressureState
    {
      lastPressureLevel = sp.level.rawValue
      lastPressureFactors = Int(sp.factors.rawValue)
    }
    emit()
  }

  @objc private func sessionInterruptionEnded(_ note: Notification) {
                                                                               
                                                                                                
                                                                                                  
                                                                                                 
                                                                             
                                                                                 
    cameraStream = session.map { $0.isRunning ? PwVioCameraStreamState.running
                                              : PwVioCameraStreamState.stopped }
      ?? .unknown
    lastInterruptionReason = nil
    emit()
  }

                                                                     
  public func snapshot() -> PwVioThermalSnapshot {
    let pi = ProcessInfo.processInfo
    var level = lastPressureLevel
    var factors = lastPressureFactors
    if #available(iOS 11.1, *), let dev = videoDevice {
      let sp = dev.systemPressureState
      level = sp.level.rawValue
      factors = Int(sp.factors.rawValue)
    }
                                                                                               
                                                                           
    var stream = cameraStream
    if let s = session, stream != .interrupted {
      stream = s.isRunning ? .running : .stopped
    } else if session == nil && stream != .interrupted {
      stream = .unknown
    }
    return PwVioThermalSnapshot(
      tsUs: pwVioMonotonicMicros(),
      rawStatus: pi.thermalState.rawValue,
                                                                                    
                                                                               
      statusReadable: true,
      lowPowerMode: pi.isLowPowerModeEnabled,
      cameraStream: stream,
      interruptionReason: lastInterruptionReason,
      systemPressureLevel: level,
      systemPressureFactors: factors,
      activeProcessorCount: pi.activeProcessorCount
    )
  }

  private func emit() {
    let snap = snapshot()
    if Thread.isMainThread {
      onSnapshot?(snap)
    } else {
      DispatchQueue.main.async { [weak self] in self?.onSnapshot?(snap) }
    }
  }
}

                                                 

                                                 
   
                                                                                                
                                                                                            
public final class PwVioUpdateTimer {
  private var wallStartUs: Int64 = 0
  private var cpuStartMs: Double = -1
  private var startThread: ObjectIdentifier?

  public init() {}

  public func begin() {
    wallStartUs = pwVioMonotonicMicros()
    cpuStartMs = pwVioThreadCpuMillis()
    startThread = ObjectIdentifier(Thread.current)
  }

                                                                       
  public func end() -> (wallMs: Double, cpuMs: Double, sameThread: Bool) {
    let wallMs = Double(pwVioMonotonicMicros() - wallStartUs) / 1000.0
    let cpuNow = pwVioThreadCpuMillis()
    let cpuMs = (cpuStartMs >= 0 && cpuNow >= 0) ? (cpuNow - cpuStartMs) : -1
    let same = startThread == ObjectIdentifier(Thread.current)
    return (wallMs, cpuMs, same)
  }
}

                         

#if canImport(Flutter)

public final class PwVioThermalPlugin: NSObject, FlutterPlugin, FlutterStreamHandler {

  private let monitor = PwVioThermalMonitor()
  private var sink: FlutterEventSink?
  private let timer = PwVioUpdateTimer()

  public static func register(with registrar: FlutterPluginRegistrar) {
    let instance = PwVioThermalPlugin()
    let method = FlutterMethodChannel(
      name: PwVioThermalIdentifiers.methodChannel,
      binaryMessenger: registrar.messenger())
    let events = FlutterEventChannel(
      name: PwVioThermalIdentifiers.eventChannel,
      binaryMessenger: registrar.messenger())
    registrar.addMethodCallDelegate(instance, channel: method)
    events.setStreamHandler(instance)
  }

                                                                               
  public func attach(session: AVCaptureSession?, videoDevice: AVCaptureDevice?) {
    monitor.attach(session: session, videoDevice: videoDevice)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "start":
      monitor.onSnapshot = { [weak self] snap in
        self?.sink?(snap.asDictionary())
      }
      monitor.start()
      result(nil)
    case "stop":
      monitor.stop()
      result(nil)
    case "snapshot":
      result(monitor.snapshot().asDictionary())
    case "beginVisualUpdate":
      timer.begin()
      result(nil)
    case "endVisualUpdate":
      let m = timer.end()
      result([
        "wallMs": m.wallMs,
        "cpuMs": m.cpuMs,
        "sameThread": m.sameThread,
      ])
    default:
      result(FlutterMethodNotImplemented)
    }
  }

  public func onListen(
    withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink
  ) -> FlutterError? {
    sink = events
    monitor.onSnapshot = { [weak self] snap in
      self?.sink?(snap.asDictionary())
    }
    monitor.start()
    return nil
  }

  public func onCancel(withArguments arguments: Any?) -> FlutterError? {
    sink = nil
    monitor.stop()
    return nil
  }
}

#endif
