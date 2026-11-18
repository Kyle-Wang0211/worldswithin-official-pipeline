import 'production_runtime_binding.dart' show productionRuntimeEnvironmentValue;

import 'execution_identity_scope.dart';

/// Only complete frozen Bench graph families extend the ordinary loader table.
                                                                                
                                                                                 
Map<String, String> selectBenchKnownModelFamily(
  Map<String, dynamic> profile,
  ReconstructionExecutionIdentityScope scope,
  Map<String, String> ordinary,
) {
  final models = profile['models'];
  bool matches(Map<String, String> family) =>
      models is Map &&
      models.length == family.length &&
      family.entries.every((entry) {
        final row = models[entry.key];
        return row is Map && row['sha256'] == entry.value;
      });
  if (!scope.isIndependentTest || scope.isProductCopy) {
    final binding = profile['production_reference_closure_current'];
    final counts = binding is Map ? binding['model_graph_values'] : null;
    const family = {
      'feature_low': '6a917de8928c891274383001e48945c60dd898e6c223c90e26d512e953fed60d',
      'feature_high': '5f0fb021b12e0ca092b1259bf9828c1588e1b1b0f1f166e1b067be1fae47f2a7',
      'rest_low': '4bfb139a9fd8f726e6a296826ef41ff5f3d77e0a0e9db8220c7df4ef8c67cdf1',
      'rest_high': 'c6ef1789c0897264112ad28bf71e0e1f1c47166ed98559faa43b33b00ca1d680',
    };
    if (binding is! Map ||
        binding['schema'] != 'RECONSTRUCTION_PRODUCTION_REFERENCE_CLOSURE_V1' ||
        binding['selected_source_revision'] != 'db7b72a45ec3161cd3cb1d96e1dc7813ceeb20f4' ||
        counts is! List || counts.length != 2 ||
        counts[0] != 4119 || counts[1] != 3555 ||
        binding['fnet_graph_values'] != 65 || !matches(family)) {
      throw StateError('Production requires the complete selected REFERENCE model family');
    }
    return family;
  }
  if (matches(ordinary)) return ordinary;
  const restLow =
      '4bfb139a9fd8f726e6a296826ef41ff5f3d77e0a0e9db8220c7df4ef8c67cdf1';
                                                                            
                                                                                
  for (final (restHigh, highCount) in const [
    ('36d89ea8ec7d713518712dfba6a500ead7724bf0212d8f8aeedee50ef9559461', 3557),
    ('f8b1ad2a4f2b7912e7cb093d4edf02b936d1f9bc825ebf3da051ca5a27a08cd5', 3556),
    ('c6ef1789c0897264112ad28bf71e0e1f1c47166ed98559faa43b33b00ca1d680', 3555),
  ]) {
    final family = Map<String, String>.unmodifiable({
      ...ordinary,
      'rest_low': restLow,
      'rest_high': restHigh,
    });
    if (!matches(family)) continue;
    final binding = profile['validation_fused_graph'];
    final counts = binding is Map ? binding['model_graph_values'] : null;
    if (counts is! List ||
        counts.length != 2 ||
        counts[0] is! int ||
        counts[1] is! int ||
        counts[0] != 4119 ||
        counts[1] != highCount) {
      throw StateError('Whole Bench model family requires exact graph counts');
    }
    return family;
  }
  const mixedFnetFamily = {
    'feature_low':
        '6a917de8928c891274383001e48945c60dd898e6c223c90e26d512e953fed60d',
    'feature_high':
        '5f0fb021b12e0ca092b1259bf9828c1588e1b1b0f1f166e1b067be1fae47f2a7',
    'rest_low': restLow,
    'rest_high':
        'c6ef1789c0897264112ad28bf71e0e1f1c47166ed98559faa43b33b00ca1d680',
  };
  if (matches(mixedFnetFamily)) {
    final binding = profile['validation_fused_graph'];
    final counts = binding is Map ? binding['model_graph_values'] : null;
    if (counts is! List ||
        counts.length != 2 ||
        counts[0] is! int ||
        counts[1] is! int ||
        counts[0] != 4119 ||
        counts[1] != 3555 ||
        binding['fnet_graph_values'] is! int ||
        binding['fnet_graph_values'] != 65 ||
        productionRuntimeEnvironmentValue('PW_FNET_MIXED_CORRIDOR') !=
            '1') {
      throw StateError(
        'Whole Bench mixed FNET requires exact counts and launch flag',
      );
    }
    return mixedFnetFamily;
  }
  throw StateError('Whole Bench requires an exact complete known model family');
}
