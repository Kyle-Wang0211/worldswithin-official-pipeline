                                             
  
                                               
                                         
                                           
                                               
  
                        
                                                 
                      
                                      
import 'selection_box.dart';

                  
   
                                                       
                                            
                          
class DenseStageRequest {
  const DenseStageRequest({
    required this.captureDir,
    required this.sparsePlyPath,
    required this.pointCount,
    this.selection,
    this.recordId,
  });

                                  
  final String captureDir;

                     
  final String sparsePlyPath;

                                   
  final int pointCount;

                            
  final SelectionBox? selection;

                                                       
  final String? recordId;
}

enum DenseStageStatus {
                 
  started,

                   
  unavailable,

                                                        
  failed,
}

class DenseStageResult {
  const DenseStageResult(this.status, {this.message});

  const DenseStageResult.started()
    : status = DenseStageStatus.started,
      message = null;

  final DenseStageStatus status;
  final String? message;
}

             
abstract interface class DenseStageLauncher {
                                             
                              
  bool get isAvailable;

                                            
                                         
  Future<DenseStageResult> start(DenseStageRequest request);
}

                   
   
                                                
                    
class UnavailableDenseStageLauncher implements DenseStageLauncher {
  const UnavailableDenseStageLauncher();

  @override
  bool get isAvailable => false;

  @override
  Future<DenseStageResult> start(DenseStageRequest request) async =>
      const DenseStageResult(DenseStageStatus.unavailable);
}

                                        
DenseStageLauncher denseStageLauncher = const UnavailableDenseStageLauncher();

                                                                            
                                                                            
enum DenseStageRecoveryAction { restartDense }

enum DenseStageRecoveryStatus {
  available,
  busy,
  completed,
  unavailable,
  invalid,
}

final class DenseStageRecoveryPlan {
  const DenseStageRecoveryPlan(
    this.status, {
    required this.message,
    this.action,
    this.priorRunPath,
  });
  final DenseStageRecoveryStatus status;
  final DenseStageRecoveryAction? action;
  final String message;
  final String? priorRunPath;
  bool get canRecover => status == DenseStageRecoveryStatus.available;
}

abstract interface class DenseStageRecoveryLauncher
    implements DenseStageLauncher {
  Future<DenseStageRecoveryPlan> inspectRecovery(DenseStageRequest request);
  Future<DenseStageResult> recover(DenseStageRequest request);
}
