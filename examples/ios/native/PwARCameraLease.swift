import Foundation

                                                                             
   
                                                                         
                                                                             
                                                                           
final class PwARCameraLease {
  static let shared = PwARCameraLease()

  private let lock = NSLock()
  private var owner: String?

  private init() {}

                                                                        
                                                        
  func acquire(owner requestedOwner: String) -> Bool {
    guard !requestedOwner.isEmpty else { return false }
    lock.lock()
    defer { lock.unlock() }
    guard owner == nil || owner == requestedOwner else { return false }
    owner = requestedOwner
    return true
  }

                                                                            
                                                  
  func release(owner requestedOwner: String) {
    lock.lock()
    defer { lock.unlock() }
    if owner == requestedOwner { owner = nil }
  }

  func isOwned(by requestedOwner: String) -> Bool {
    lock.lock()
    defer { lock.unlock() }
    return owner == requestedOwner
  }

}
