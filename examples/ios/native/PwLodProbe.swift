                                                                                         
                                                                                                 
                                                                                                
  
                                                                                            
                                                                                               
                                                                                              
       
  
                                                                                              
                                                                                            
                                                                                            
                                       
                                                                                           
                                                                                                
                                                                                              
                                                                                         
import Darwin
import Foundation
import os

                                                                                           
                                                                                       
let pwLodProbe: PwLodProbeFn = { out, _ in
    guard let out = out else { return }
    out.pointee.thermal_state = Int32(ProcessInfo.processInfo.thermalState.rawValue)
    var info = task_vm_info_data_t()
    var count = mach_msg_type_number_t(MemoryLayout<task_vm_info_data_t>.size / MemoryLayout<natural_t>.size)
    let kr = withUnsafeMutablePointer(to: &info) {
        $0.withMemoryRebound(to: integer_t.self, capacity: Int(count)) {
            task_info(mach_task_self_, task_flavor_t(TASK_VM_INFO), $0, &count)
        }
    }
    out.pointee.footprint_mb = kr == KERN_SUCCESS ? Double(info.phys_footprint) / 1048576.0 : -1
    out.pointee.avail_mb = Double(os_proc_available_memory()) / 1048576.0
}

enum PwLodProbe {
    static func sample() -> PwLodProbeSample {
        var s = PwLodProbeSample(thermal_state: -1, footprint_mb: -1, avail_mb: -1)
        pwLodProbe(&s, nil)
        return s
    }

    static func toMap(_ s: PwLodProbeSample) -> [String: Any] {
        ["thermal_state": Int(s.thermal_state), "footprint_mb": s.footprint_mb, "avail_mb": s.avail_mb]
    }

                                                                                                
    static func footprintAndLedgerPeakMb() -> (Double, Double) {
        var info = task_vm_info_data_t()
        var count = mach_msg_type_number_t(MemoryLayout<task_vm_info_data_t>.size / MemoryLayout<natural_t>.size)
        let kr = withUnsafeMutablePointer(to: &info) {
            $0.withMemoryRebound(to: integer_t.self, capacity: Int(count)) {
                task_info(mach_task_self_, task_flavor_t(TASK_VM_INFO), $0, &count)
            }
        }
        guard kr == KERN_SUCCESS else { return (-1, -1) }
        let now = Double(info.phys_footprint) / 1048576.0
                                                                                              
        let peakEnd = (MemoryLayout<task_vm_info_data_t>.offset(of: \.ledger_phys_footprint_peak) ?? Int.max)
            + MemoryLayout<Int64>.size
        let filled = Int(count) * MemoryLayout<natural_t>.size
        let peak = filled >= peakEnd ? Double(info.ledger_phys_footprint_peak) / 1048576.0 : -1
        return (now, peak)
    }
}

                                                                     
final class PwLodFootprintPeak {
    private let queue = DispatchQueue(label: "io.worldswithin.lod.footprint", qos: .utility)
    private let timer: DispatchSourceTimer
    private let lock = NSLock()
    private var sampledPeakMb: Double = -1
    private var samples = 0
    private(set) var baselineMb: Double = -1
    private(set) var ledgerPeakBeforeMb: Double = -1

    init(intervalMs: Int = 10) {
        timer = DispatchSource.makeTimerSource(queue: queue)
        timer.schedule(deadline: .now(), repeating: .milliseconds(intervalMs))
        timer.setEventHandler { [weak self] in self?.take() }
    }

    private func take() {
        let (now, _) = PwLodProbe.footprintAndLedgerPeakMb()
        lock.lock()
        if now > sampledPeakMb { sampledPeakMb = now }
        samples += 1
        lock.unlock()
    }

    func start() {
        (baselineMb, ledgerPeakBeforeMb) = PwLodProbe.footprintAndLedgerPeakMb()
        timer.resume()
    }

                                                                                 
    func stop() -> [String: Any] {
        timer.cancel()
        queue.sync {}                               
        take()
        let (_, ledgerAfter) = PwLodProbe.footprintAndLedgerPeakMb()
        lock.lock()
        defer { lock.unlock() }
        return [
            "peak_footprint_mb": sampledPeakMb,
            "baseline_footprint_mb": baselineMb,
            "footprint_samples": samples,
            "ledger_peak_before_mb": ledgerPeakBeforeMb,
            "ledger_peak_after_mb": ledgerAfter,
        ]
    }
}
