import 'execution_identity_scope.dart';

                                                                           
                                                                                  
const productionObservationCapBytes = 0x7fffffffffffffff;
const benchMemoryCapBytes = 2000000000;
const productionMemoryPolicy = 'production_observe_only_v1';
const benchMemoryPolicy = 'validation_memory_limit';

int wholeMemoryCapBytes(ReconstructionExecutionIdentityScope scope) =>
    (scope.isIndependentTest && !scope.isProductCopy)
    ? benchMemoryCapBytes
    : productionObservationCapBytes;

String wholeMemoryPolicy(ReconstructionExecutionIdentityScope scope) =>
    (scope.isIndependentTest && !scope.isProductCopy)
    ? benchMemoryPolicy
    : productionMemoryPolicy;

void requireWholeMemoryPolicy(
  Map<String, dynamic> profile,
  ReconstructionExecutionIdentityScope scope,
) {
  final policy = profile['memory_policy'];
  if (profile['physical_limit_bytes'] != wholeMemoryCapBytes(scope) ||
      (policy != wholeMemoryPolicy(scope) &&
          !((scope.isIndependentTest && !scope.isProductCopy) &&
              policy == null))) {
    throw StateError(
      'Whole runtime memory policy differs from execution scope',
    );
  }
}
