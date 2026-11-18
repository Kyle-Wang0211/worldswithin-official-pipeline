"""Explicit product policy, separate from the enforced Bench memory budget."""

from reconstruction_distribution_identity import configured_bundle_id

BENCH_CAP = 2000000000
OBSERVATION_CAP = (1 << 63) - 1
PRODUCTION_POLICY = 'production_observe_only_v1'
BENCH_POLICY = 'validation_memory_limit'

def validated_policy(profile, identity=None):
    identity = identity or {}
    production_id = configured_bundle_id('WORLDSWITHIN_PRODUCTION_BUNDLE_ID')
    bench_id = configured_bundle_id('WORLDSWITHIN_BENCH_BUNDLE_ID')
    product_copy_id = configured_bundle_id('WORLDSWITHIN_PRODUCT_COPY_BUNDLE_ID')
    production = production_id is not None and {key: value for key, value in identity.items() if key != 'build'} == dict(
        schema='RECONSTRUCTION_EXECUTION_IDENTITY_SCOPE_V1', profile='production', bundle_id=production_id)
    bench = (not identity or
        (bench_id is not None and identity.get('schema') == 'RECONSTRUCTION_EXECUTION_IDENTITY_SCOPE_V1' and
         identity.get('profile') == 'independent_product_entry_test_v1' and
         identity.get('bundle_id') == bench_id))
    product_copy = (
        product_copy_id is not None and identity.get('schema') == 'RECONSTRUCTION_EXECUTION_IDENTITY_SCOPE_V1' and
        identity.get('profile') == 'independent_product_copy_v1' and
        identity.get('bundle_id') == product_copy_id)
    if production or product_copy:
        expected = OBSERVATION_CAP
        valid = profile.get('memory_policy') == PRODUCTION_POLICY
    elif bench:
        expected = BENCH_CAP
        valid = profile.get('memory_policy') in (None, BENCH_POLICY)
    else:
        raise ValueError('unsupported memory execution identity')
    cap = profile.get('physical_limit_bytes')
    if not valid or type(cap) is not int or cap != expected:
        raise ValueError('memory policy/cap differs from bound execution identity')
    return cap

def bind_native_policy(profile, environment):
    cap = profile['physical_limit_bytes']
    policy = profile.get('memory_policy')
    if cap == OBSERVATION_CAP and policy == PRODUCTION_POLICY:
        environment['RECONSTRUCTION_MEMORY_POLICY'] = PRODUCTION_POLICY
    elif cap == BENCH_CAP and policy in (None, BENCH_POLICY):
        environment['RECONSTRUCTION_MEMORY_POLICY'] = BENCH_POLICY
    else:
        raise ValueError('invalid native memory policy binding')
