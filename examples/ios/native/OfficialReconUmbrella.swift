import BackgroundTasks
import UIKit

                                                                               
                                                                               
                                                                              
                                                                                
@available(iOS 26.0, *)
final class OfficialReconUmbrella {
  static let shared = OfficialReconUmbrella()
  static let taskIdentifier: String = {
    guard let bundleIdentifier = Bundle.main.bundleIdentifier else {
      preconditionFailure("Configure the host CFBundleIdentifier")
    }
    return "\(bundleIdentifier).official.recon"
  }()

  private struct FullPipelineJob {
    var status: String
    var requiresGPU: Bool
    var reason: String?
    var holdsLease = true
    var completedStages = 0
    var phase = "准备完整重建"
    var done = 0
    var total = 0
  }

                                                                              
  private var legacyJobs = Set<String>()
  private var full_pipelineJobs: [String: FullPipelineJob] = [:]
  private var grantJobIDs = Set<String>()
  private var endedJobIDs = [String]()
  private let retainedEndedJobLimit = 64
  private var retiredGrantTotalStages: Int64 = 0
  private var retiredGrantCompletedStages: Int64 = 0
  private var currentTask: BGContinuedProcessingTask?
  private var requestPending = false
  private var pendingRequiresGPU = false
  private var pendingGenerationTitle: String?
  private var activeRequiresGPU = false
  private var registered = false
  private var grantFailed = false
  private var grantExpired = false
  private var realProgressUnits: Int64 = 0
  private var realSubtitle: String?

  private func onMain<T>(_ body: () -> T) -> T {
    if Thread.isMainThread { return body() }
    return DispatchQueue.main.sync(execute: body)
  }

  private var hasLeases: Bool {
    !legacyJobs.isEmpty || full_pipelineJobs.values.contains { $0.holdsLease }
  }

  func register() {
    onMain {
      guard !registered else { return }
                                                                           
      BGTaskScheduler.shared.cancel(taskRequestWithIdentifier: Self.taskIdentifier)
      registered = BGTaskScheduler.shared.register(
        forTaskWithIdentifier: Self.taskIdentifier, using: .main
      ) { [weak self] task in
        guard let self = self, let cp = task as? BGContinuedProcessingTask else {
          task.setTaskCompleted(success: false)
          return
        }
        self.runHandler(cp)
      }
    }
  }

                                                                              
  func begin(jobID: String, requiresGPU: Bool = true) {
    onMain {
      let key = jobID.isEmpty ? "legacy" : jobID
      guard !legacyJobs.contains(key), !grantExpired else { return }
      let wasIdle = !hasLeases
      legacyJobs.insert(key)
      if wasIdle { resetGrant() }
                                                                            
                                                                              
      let requestGPU = requiresGPU && BGTaskScheduler.supportedResources.contains(.gpu)
      if currentTask == nil && !requestPending, submit(requiresGPU: requestGPU) != nil {
        legacyJobs.remove(key)
      }
      publishProgress()
    }
  }

  func setRealProgress(fraction: Double, subtitle: String?) {
    onMain {
      let fraction = fraction.isFinite ? min(max(fraction, 0), 1) : 0
      realProgressUnits = max(realProgressUnits, Int64(fraction * 10000))
      if let subtitle = subtitle, !subtitle.isEmpty { realSubtitle = subtitle }
      publishProgress()
    }
  }

  func end(jobID: String) {
    onMain {
      legacyJobs.remove(jobID.isEmpty ? "legacy" : jobID)
      finishIfIdle()
    }
  }

  func beginFullPipeline(jobID: String, requiresGPU: Bool = true) -> [String: Any] {
    onMain {
      if let job = full_pipelineJobs[jobID] {
        if requiresGPU && !job.requiresGPU {
          return response(jobID, status: "rejected", reason: "existing_grant_missing_gpu")
        }
        return response(jobID, job)
      }
      if requiresGPU && !BGTaskScheduler.supportedResources.contains(.gpu) {
        return response(jobID, status: "unsupported", reason: "background_gpu_not_supported")
      }
      if requiresGPU && ((currentTask != nil && !activeRequiresGPU)
                         || (requestPending && !pendingRequiresGPU)) {
        return response(jobID, status: "rejected", reason: "existing_grant_missing_gpu")
      }
      guard !grantExpired else {
        return response(jobID, status: "rejected", reason: "expired_worker_still_running")
      }
      let wasIdle = !hasLeases
      if wasIdle { resetGrant() }
      full_pipelineJobs[jobID] = FullPipelineJob(status: currentTask == nil ? "submitted" : "active",
                                      requiresGPU: requiresGPU)
      grantJobIDs.insert(jobID)
      if currentTask == nil && !requestPending, let reason = submit(requiresGPU: requiresGPU) {
                                                                            
        full_pipelineJobs.removeValue(forKey: jobID)
        grantJobIDs.remove(jobID)
        return response(jobID, status: "rejected", reason: reason)
      }
      publishProgress()
      return response(jobID, full_pipelineJobs[jobID]!)
    }
  }

  func full_pipelineStatus(jobID: String) -> [String: Any] {
    onMain {
      guard let job = full_pipelineJobs[jobID] else {
        return response(jobID, status: "rejected", reason: "unknown_job")
      }
      return response(jobID, job)
    }
  }

  func updateFullPipeline(jobID: String, completedStages: Int, phase: String,
                    done: Int, total: Int) -> [String: Any] {
    onMain {
      guard var job = full_pipelineJobs[jobID] else {
        return response(jobID, status: "rejected", reason: "unknown_job")
      }
      guard job.holdsLease else { return response(jobID, job) }
      guard (0...13).contains(completedStages), completedStages >= job.completedStages,
            done >= 0, total >= 0, done <= total else {
        return response(jobID, status: "rejected", reason: "invalid_progress")
      }
                                                                              
                                                                                
      job.completedStages = completedStages
      job.phase = phase
      job.done = done
      job.total = total
      full_pipelineJobs[jobID] = job
      publishProgress()
      return response(jobID, job)
    }
  }

  func endFullPipeline(jobID: String, success: Bool) -> [String: Any] {
    onMain {
      guard var job = full_pipelineJobs[jobID] else {
        return response(jobID, status: "rejected", reason: "unknown_job")
      }
      guard job.holdsLease else { return response(jobID, job) }
      job.holdsLease = false
      if job.status != "expired" {
        job.status = "completed"
        job.reason = success ? nil : "worker_failed"
        if success { job.completedStages = 13 }
      }
      if !success { grantFailed = true }
      full_pipelineJobs[jobID] = job
      retainEndedJob(jobID)
      publishProgress()
      finishIfIdle()
      return response(jobID, job)
    }
  }

  private func response(_ jobID: String, _ job: FullPipelineJob) -> [String: Any] {
    response(jobID, status: job.status, reason: job.reason)
  }

  private func response(_ jobID: String, status: String, reason: String? = nil) -> [String: Any] {
    var value: [String: Any] = ["schema": "RECONSTRUCTION_BACKGROUND_V1", "job_id": jobID,
                                "status": status]
    if let reason = reason { value["reason"] = reason }
    return value
  }

  private func resetGrant() {
    grantJobIDs.removeAll()
    retiredGrantTotalStages = 0
    retiredGrantCompletedStages = 0
    grantFailed = false
    grantExpired = false
    realProgressUnits = 0
    realSubtitle = nil
  }

                                                                                 
  private func submit(requiresGPU: Bool = false) -> String? {
    guard registered else { return "handler_not_registered" }
    guard UIApplication.shared.applicationState == .active else { return "foreground_required" }
                                                                            
                                                                   
    let generationTitle = "WorldsWithin 重建任务 \(UUID().uuidString)"
    let request = BGContinuedProcessingTaskRequest(
      identifier: Self.taskIdentifier,
      title: generationTitle,
      subtitle: "准备重建…")
    request.strategy = .fail
    request.requiredResources = requiresGPU ? .gpu : []
    do {
      try BGTaskScheduler.shared.submit(request)
      requestPending = true
      pendingRequiresGPU = requiresGPU
      pendingGenerationTitle = generationTitle
      return nil
    } catch {
      NSLog("[OfficialReconUmbrella] submit failed: \(error)")
      return "submit_failed"
    }
  }

  private func runHandler(_ task: BGContinuedProcessingTask) {
    guard currentTask == nil, requestPending, hasLeases, !grantExpired else {
      task.setTaskCompleted(success: false)
      return
    }
                                                                               
                                                                                
    guard let expectedTitle = pendingGenerationTitle, task.title == expectedTitle else {
      task.setTaskCompleted(success: false)
      return
    }
    requestPending = false
    activeRequiresGPU = pendingRequiresGPU
    pendingRequiresGPU = false
    pendingGenerationTitle = nil
                                                                             
                                                                    
    if full_pipelineJobs.values.contains(where: { $0.holdsLease && $0.requiresGPU })
        && !activeRequiresGPU {
      markExpired(reason: "grant_missing_gpu")
      task.setTaskCompleted(success: false)
      return
    }
    currentTask = task
    for key in grantJobIDs {
      guard var job = full_pipelineJobs[key], job.holdsLease else { continue }
      job.status = "active"
      full_pipelineJobs[key] = job
    }
    task.expirationHandler = { [weak self, weak task] in
      DispatchQueue.main.async {
        guard let self = self, let task = task, self.currentTask === task else { return }
        self.markExpired(reason: "system_expired")
                                                                                  
        self.currentTask = nil
        self.activeRequiresGPU = false
        task.expirationHandler = nil
        task.setTaskCompleted(success: false)
      }
    }
    publishProgress()
  }

  private func markExpired(reason: String) {
    grantExpired = true
    grantFailed = true
    for key in grantJobIDs {
      guard var job = full_pipelineJobs[key], job.holdsLease else { continue }
      job.status = "expired"
      job.reason = reason
      full_pipelineJobs[key] = job
    }
  }

  private func retainEndedJob(_ jobID: String) {
    endedJobIDs.append(jobID)
    while endedJobIDs.count > retainedEndedJobLimit {
      let oldest = endedJobIDs.removeFirst()
      guard let job = full_pipelineJobs[oldest], !job.holdsLease else { continue }
                                                                            
                                                                                
      if grantJobIDs.remove(oldest) != nil {
        retiredGrantTotalStages += 13
        retiredGrantCompletedStages += Int64(job.completedStages)
      }
      full_pipelineJobs.removeValue(forKey: oldest)
    }
  }

  private func publishProgress() {
    guard let task = currentTask else { return }
    let jobs = grantJobIDs.compactMap { full_pipelineJobs[$0] }
    if !jobs.isEmpty || retiredGrantTotalStages > 0 {
                                                                            
      task.progress.totalUnitCount = retiredGrantTotalStages + Int64(jobs.count * 13)
      task.progress.completedUnitCount = retiredGrantCompletedStages
        + Int64(jobs.reduce(0) { $0 + $1.completedStages })
      let job = grantJobIDs.sorted().compactMap { full_pipelineJobs[$0] }
        .first { $0.holdsLease && $0.status == "active" }
      if let job = job {
        let count = job.total > 0 ? " · \(job.done)/\(job.total)" : ""
        task.updateTitle("WorldsWithin 完整重建", subtitle:
          "\(job.completedStages)/13 · \(job.phase)\(count)")
      }
    } else {
      task.progress.totalUnitCount = 10000
      task.progress.completedUnitCount = realProgressUnits
      if let subtitle = realSubtitle {
        task.updateTitle("WorldsWithin 官方重建", subtitle: subtitle)
      }
    }
  }

  private func finishIfIdle() {
    guard !hasLeases else { return }
    if requestPending {
      BGTaskScheduler.shared.cancel(taskRequestWithIdentifier: Self.taskIdentifier)
      requestPending = false
      pendingRequiresGPU = false
      pendingGenerationTitle = nil
    }
    if let task = currentTask {
      currentTask = nil                                                          
      task.expirationHandler = nil
      if !grantFailed { task.progress.completedUnitCount = task.progress.totalUnitCount }
      task.setTaskCompleted(success: !grantFailed)
    }
    activeRequiresGPU = false
    grantExpired = false
  }
}
