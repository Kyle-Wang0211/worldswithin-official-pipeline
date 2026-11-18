                                                                           
                                              
enum ReconstructionPipeline { selfDeveloped, official }

                                                                             
                                                                               
                          
final class ReconstructionLeaseBusyException implements Exception {
  const ReconstructionLeaseBusyException({
    required this.activePipeline,
    required this.requestedPipeline,
  });

  final ReconstructionPipeline activePipeline;
  final ReconstructionPipeline requestedPipeline;

  @override
  String toString() =>
      'ReconstructionLeaseBusyException: '
      '${activePipeline.name} reconstruction is still active; '
      'cannot start ${requestedPipeline.name}';
}

                                                                       
   
                                                                              
                                                                               
                                                                                
final class ReconstructionLease {
  Object? _owner;
  ReconstructionPipeline? _pipeline;

  ReconstructionPipeline? get activePipeline => _pipeline;

  void acquire({
    required Object owner,
    required ReconstructionPipeline pipeline,
  }) {
    final activeOwner = _owner;
    if (activeOwner == null) {
      _owner = owner;
      _pipeline = pipeline;
      return;
    }
    if (identical(activeOwner, owner)) {
      if (_pipeline != pipeline) {
        throw StateError('A reconstruction lease owner cannot change pipeline');
      }
      return;
    }
    throw ReconstructionLeaseBusyException(
      activePipeline: _pipeline!,
      requestedPipeline: pipeline,
    );
  }

                                                                           
  bool release(Object owner) {
    if (!identical(_owner, owner)) return false;
    _owner = null;
    _pipeline = null;
    return true;
  }
}

                                                                         
                                    
final reconstructionLease = ReconstructionLease();
