#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct PwFeatureProducer PwFeatureProducer;
typedef struct { const char* name;int64_t shape[4];size_t bytes; } PwFeatureLayerSpec;
typedef struct { const char* model_path;const char* model_sha256;const char* preprocess_sha256;const char* cache_root;int width,height;PwFeatureLayerSpec layers[3];size_t max_frame_payload_B,max_cache_payload_B; } PwFeatureProducerConfig;
typedef struct { int pack_index;const char* photo_sha256;const char* prepared_gray_sha256; } PwFeatureMaterialIdentity;
typedef struct { int pack_index;const char* photo_sha256;const char* model_sha256;const char* preprocess_sha256;const uint16_t* gray;size_t gray_elements;int interrupt_after_complete_partial; } PwFeatureRequest;
typedef struct { uint64_t native_runs,cache_hits,partial_recoveries,atomic_commits,rejections;size_t resident_feature_frames,peak_frame_payload_B; } PwFeatureProducerStats;
PwFeatureProducer* pw_feature_producer_create(const PwFeatureProducerConfig*,char* error,size_t error_bytes);
int pw_feature_producer_register(PwFeatureProducer*,const PwFeatureMaterialIdentity*);
/* 0=immutable committed receipt, 1=deliberate precommit interruption, -1=reject/error.
   Request and registration are serial. Host owns gray for the duration of this call.
   No output F32 shadow. The JSON receipt describes packed F16 bytes and every layer.
   Exact existing committed bytes are verified on hits; complete interrupted partials
   are verified and atomically committed without another encoder forward. */
int pw_feature_producer_request(PwFeatureProducer*,const PwFeatureRequest*,char* receipt,size_t receipt_bytes,char* error,size_t error_bytes);
void pw_feature_producer_stats(const PwFeatureProducer*,PwFeatureProducerStats*);
void pw_feature_producer_destroy(PwFeatureProducer*);
#ifdef __cplusplus
}
#endif
