#pragma once
#include <onnxruntime_c_api.h>
#include "fixed_slot_material_bridge.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace Ort {struct Env;}
namespace reconstruction {
struct ResidentInputSource {virtual ~ResidentInputSource()=default;virtual std::vector<uint16_t> ReadHalf(int view)const=0;virtual void ReadNoise(int ref,std::vector<float>&n2,std::vector<float>&n3)const=0;virtual const std::vector<float>& Cameras()const=0;virtual const std::vector<int32_t>& Neighbors()const=0;};
struct ResidentSceneConfig {std::string feature_model,rest_model,bank_dir,noise_file;std::string material_index,material_root,material_index_sha,material_manifest_sha,material_preprocess_sha;int width=0,height=0,scene_frames=0;std::shared_ptr<ResidentInputSource> input_source;std::shared_ptr<Ort::Env> shared_env;bool rest_gpu_int64=false;std::string rest_profile_prefix;};
struct DeviceTensorView {std::string name;const OrtValue* value=nullptr;const void* provider_buffer=nullptr;std::vector<int64_t> shape;size_t elements=0;int device_id=-1;};
struct ResidentFeedView {std::string name,owner;const void* provider_buffer=nullptr;int source_view=-1,feature_level=-1;};
struct ResidentCounters {uint64_t scene_inits=0,image_uploads=0,feature_evaluations=0,auxiliary_uploads=0,rest_runs=0,output_downloads=0,leases_released=0,explicit_feature_downloads=0;uint64_t completion_waits=0,completion_payload_bytes=0,completion_staging_bytes=0;uint64_t material_hits=0,material_misses=0,material_evictions=0,material_reads=0,material_read_B=0,material_uploads=0,material_staging_peak_B=0,material_gpu_peak_B=0;size_t cached_images=0,cached_feature_views=0,cached_reference_aux=0;};
class PersistentFullMvsGpu;
struct ResidentFrameLease {
 ResidentFrameLease()=default;ResidentFrameLease(ResidentFrameLease&&)=default;ResidentFrameLease& operator=(ResidentFrameLease&&)=default;
 ResidentFrameLease(const ResidentFrameLease&)=delete;ResidentFrameLease& operator=(const ResidentFrameLease&)=delete;
 int reference=-1;uint64_t generation=0;std::array<DeviceTensorView,4> outputs;
 private:std::shared_ptr<void> owner_;friend class PersistentFullMvsGpu;
};
// One scene owns one Env, two original ORT sessions, GPU image/feature/aux caches and four GPU output buffers.
// A live output lease blocks the next reference. Consumers must finish host Download or enqueue their reads on
// the same provider device/queue before Release. Views must not be retained after Release.
// GraphCapture is intentionally disabled: normal official Run preserves terminal Squeeze/copy delivery.
// Private isolated harness options: -1 is official regular-run bypass; omitted graphid keeps existing0.
struct ResidentReferenceOptions {bool bypass_graph_capture_for_run=false;bool skip_input_update_negative_control=false;};
class PersistentFullMvsGpu {
 public:
 PersistentFullMvsGpu();~PersistentFullMvsGpu();
 bool Initialize(const ResidentSceneConfig&,std::string* error);
 bool RunReference(int reference,ResidentFrameLease* lease,std::string* error);
 bool RunReferenceWithOptions(int reference,ResidentFrameLease* lease,std::string* error,const ResidentReferenceOptions&);
 bool Download(const ResidentFrameLease&,std::array<std::vector<float>,4>* host,std::string* error);
 bool DownloadInto(const ResidentFrameLease&,float* const[4],size_t,std::string* error);
 bool Release(ResidentFrameLease* lease,std::string* error);
 ResidentCounters Counters() const;
 FixedMaterialInputContractV1 FixedInputsContract() const;
 std::vector<ResidentFeedView> FeedViews(const ResidentFrameLease&) const;
 const OrtEnv* EnvironmentHandle(const ResidentFrameLease&) const;
 private:struct State;std::shared_ptr<State> state_;
};
}
