#pragma once
#include "feature_producer.h"
#ifdef __cplusplus
extern "C" {
#endif
// Bind the one linked runtime. Reject a second ORT image/API identity before
// initializing resource limits. Caller additionally verifies frozen file SHA.
int pw_feature_runtime_bind_v1(const char* runtime_path,
                               char* error, size_t error_bytes);
// Serial phase boundary after all successful feature receipts. Failure or
// incomplete request retains *producer and its actual session/context. Once
// consumed, nulls *producer. Never creates the whole owner.
// A nonzero return forbids entering whole MVS. The canonical whole create
// additionally checks the sole runtime's actual context factory.
int pw_feature_producer_release_for_whole_v1(PwFeatureProducer** producer,
                                           char* error, size_t error_bytes);
#ifdef __cplusplus
}
#endif
