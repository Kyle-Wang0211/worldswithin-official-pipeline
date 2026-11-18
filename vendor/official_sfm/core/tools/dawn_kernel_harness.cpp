// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Aether3D. All rights reserved.

#include "dawn_kernel_harness.h"

#include "dawn/native/DawnNative.h"                                         
#include "dawn/platform/DawnPlatform.h"                                             
#include "dawn_histogram_sink.h"                                                                 
#include "official_gpu_timestamp_diagnostics_v1.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>                                                          
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#if !defined(AETHER_GPU_TIMESTAMP_DAWN_REVISION)
#error "AETHER_GPU_TIMESTAMP_DAWN_REVISION must pin Dawn's 40-hex revision"
#endif
#if !defined(AETHER_GPU_TIMESTAMP_DAWN_ARTIFACT_SHA256)
#error "AETHER_GPU_TIMESTAMP_DAWN_ARTIFACT_SHA256 must pin the linked Dawn archive"
#endif

namespace aether {
namespace tools {

                                                                            
  
                                                                                               
                                                                                                        
                                                                                                        
  
                                               
                                                                               
                                                                        
                                                                          
                                                                                  
  
                                                                                             
                                                   
namespace {
std::mutex g_msl_mu;
std::vector<std::string> g_msl_dumps;
constexpr char kMslDumpPrefix[] = "/* Dumped generated MSL */";

                                                                                                        
const char* msl_dump_dir() {
    static const char* dir = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_DUMP_MSL");
        return (v != nullptr && v[0] != '\0') ? v : nullptr;
    }();
    return dir;
}
}              

                                                                     
                                                                                                       
                                                                                                   
                                                           
int DawnKernelHarness::dump_msl(const char* dir) const {
    if (dir == nullptr) dir = msl_dump_dir();
    if (dir == nullptr) return -1;                                                       
    std::lock_guard<std::mutex> lk(g_msl_mu);
    int n = 0;
    for (const std::string& m : g_msl_dumps) {
        uint64_t h = 1469598103934665603ull;
        for (unsigned char c : m) { h ^= c; h *= 1099511628211ull; }
        char path[1024];
        std::snprintf(path, sizeof(path), "%s/msl_%016llx.msl", dir,
                      static_cast<unsigned long long>(h));
        FILE* f = std::fopen(path, "wb");
        if (f == nullptr) continue;
        std::fwrite(m.data(), 1, m.size(), f);
        std::fclose(f);
        ++n;
    }
    return n;
}

void DawnKernelHarness::dawn_histogram(const char* name, double* out_ms,
                                       uint32_t* out_n) const {
    dawn_histogram_get(name, out_ms, out_n);                                     
}


namespace {

                                                                                                
                                                                                                     
                                                                       
                                                                                                                                                               
                                                             
bool gpu_ts_diag_enabled() {
    static const bool kOn = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_GPU_TS_DIAG");
        return v != nullptr && v[0] == '1' && v[1] == '\0';
    }();
    return kOn;
}

                                                                                
std::ostream& operator<<(std::ostream& os, const wgpu::StringView& s) {
    if (s.data == nullptr) {
        return os;
    }
    if (s.length == WGPU_STRLEN) {
        return os << s.data;
    }
    return os.write(s.data, static_cast<std::streamsize>(s.length));
}

                                                                                                
                                                                                
                                                                                               
                                                                                                      
                                                                                        
                                                                  
                                                               
                                                          
                                                                                   
constexpr uint64_t kRuntimeWaitNs = 20000000000ull;        
constexpr uint64_t kInitWaitNs = 15000000000ull;           

uint64_t env_wait_ns_or(const char* key, uint64_t fallback_ns) {
    const char* e = std::getenv(key);
    if (e != nullptr) {
        const long long ms = std::atoll(e);
        if (ms > 0) {
            return static_cast<uint64_t>(ms) * 1000000ull;
        }
    }
    return fallback_ns;
}

uint64_t runtime_wait_ns() {
    static const uint64_t v =
        env_wait_ns_or("OFFICIAL_AETHER_GPU_WAIT_MS", kRuntimeWaitNs);
    return v;
}

uint64_t init_wait_ns() {
    static const uint64_t v =
        env_wait_ns_or("OFFICIAL_AETHER_GPU_INIT_WAIT_MS", kInitWaitNs);
    return v;
}

const char* error_type_name(wgpu::ErrorType t) {
    switch (t) {
        case wgpu::ErrorType::NoError:     return "NoError";
        case wgpu::ErrorType::Validation:  return "Validation";
        case wgpu::ErrorType::OutOfMemory: return "OutOfMemory";
        case wgpu::ErrorType::Internal:    return "Internal";
        case wgpu::ErrorType::Unknown:     return "Unknown";
    }
    return "<?>";
}

                                                                                                                                                                        
                                                                                           
                                                                                     
                                                                                      
std::atomic<bool> g_device_error{false};
std::mutex g_device_error_mu;
std::string g_device_error_msg;

                           
  
                                                                                 
                                                                                
                                                                                            
                                                                                           
                                                                                           
                                                                              
  
                                                                                        
                                                                                           
                                                                                    
                                                                                                             
void on_uncaptured_error(const wgpu::Device&           ,
                          wgpu::ErrorType type,
                          wgpu::StringView msg) {
    std::ostringstream oss;
    oss << error_type_name(type) << " (" << static_cast<unsigned>(type)
        << "): " << msg;
    const std::string text = oss.str();
    {
        std::lock_guard<std::mutex> lock(g_device_error_mu);
        g_device_error_msg = text;
    }
                                                                               
    g_device_error.store(true, std::memory_order_release);
    std::cerr << "\n[Dawn UNCAPTURED ERROR] " << text << '\n'
              << "  Recorded — caller must fall back (no abort in shipping "
                 "builds)\n";
}

struct TimestampErrorScopeResult {
    bool pop_status_success = true;
    bool clean = true;
    std::string message;
};

void push_timestamp_error_scopes(const wgpu::Device& device) {
    device.PushErrorScope(wgpu::ErrorFilter::Validation);
    device.PushErrorScope(wgpu::ErrorFilter::OutOfMemory);
    device.PushErrorScope(wgpu::ErrorFilter::Internal);
}

TimestampErrorScopeResult pop_timestamp_error_scopes(
        const wgpu::Instance& instance, const wgpu::Device& device) {
    TimestampErrorScopeResult result;
    for (uint32_t i = 0; i < 3; ++i) {
        bool callback_completed = false;
        bool callback_status_success = false;
        wgpu::ErrorType callback_type = wgpu::ErrorType::Unknown;
        std::string callback_message;
        const wgpu::Future future = device.PopErrorScope(
            wgpu::CallbackMode::WaitAnyOnly,
            [&](wgpu::PopErrorScopeStatus status, wgpu::ErrorType type,
                wgpu::StringView message) {
                callback_completed = true;
                callback_status_success =
                    status == wgpu::PopErrorScopeStatus::Success;
                callback_type = type;
                std::ostringstream text;
                text << message;
                callback_message = text.str();
            });
                                                                         
                                                                                    
        const wgpu::WaitStatus wait_status =
            instance.WaitAny(future, runtime_wait_ns());
        if (wait_status != wgpu::WaitStatus::Success ||
            !callback_completed || !callback_status_success) {
            result.pop_status_success = false;
        }
        if (callback_type != wgpu::ErrorType::NoError) {
            result.clean = false;
            if (!result.message.empty()) {
                result.message.append(" | ");
            }
            result.message.append(error_type_name(callback_type));
            if (!callback_message.empty()) {
                result.message.append(": ");
                result.message.append(callback_message);
            }
        }
    }
    return result;
}

}              

namespace gpu_timestamp_internal {
namespace {

struct SnapshotStoreV1 {
    std::mutex mutex;
    uint64_t probe_sequence = 0;
    uint64_t frame_sequence = 0;
    uint64_t last_frame_ordinal = 0;
    bool probe_ready = false;
    bool frame_ready = false;
    AetherGpuTimestampProbeV1 probe{};
    AetherGpuTimestampFrameV1 frame{};
};

SnapshotStoreV1 g_timestamp_snapshots;
std::atomic<uint64_t> g_next_probe_instance_id{1};
thread_local bool g_caller_thread_frame_ready = false;
thread_local AetherGpuTimestampFrameV1 g_caller_thread_frame{};

template <size_t N>
void copy_inline_text(char (&destination)[N], const char* source) noexcept {
    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }
    const size_t length = std::min(std::strlen(source), N - 1);
    std::memcpy(destination, source, length);
    destination[length] = '\0';
    if (length + 1 < N) {
        std::memset(destination + length + 1, 0, N - length - 1);
    }
}

void normalize_probe(AetherGpuTimestampProbeV1* probe) noexcept {
    probe->struct_size = sizeof(*probe);
    probe->schema_version = AETHER_GPU_TIMESTAMP_SCHEMA_VERSION_V1;
    probe->selected_env_key[AETHER_GPU_TIMESTAMP_ENV_KEY_CAPACITY_V1 - 1] =
        '\0';
    probe->reason[AETHER_GPU_TIMESTAMP_REASON_CAPACITY_V1 - 1] = '\0';
    probe->adapter_identity[AETHER_GPU_TIMESTAMP_ADAPTER_CAPACITY_V1 - 1] =
        '\0';
    probe->dawn_version[AETHER_GPU_TIMESTAMP_DAWN_VERSION_CAPACITY_V1 - 1] =
        '\0';
    probe->dawn_build_hash[AETHER_GPU_TIMESTAMP_DAWN_HASH_CAPACITY_V1 - 1] =
        '\0';
    if (probe->status != AETHER_GPU_TIMESTAMP_STATUS_ENABLED_V1 ||
        probe->ts_enabled == 0) {
        probe->presence_flags &=
            ~static_cast<uint32_t>(AETHER_GPU_TIMESTAMP_PRESENCE_PERIOD_V1);
        probe->timestamp_period_ns = 0;
        probe->ts_enabled = 0;
    }
}

void normalize_frame(AetherGpuTimestampFrameV1* frame) noexcept {
    frame->struct_size = sizeof(*frame);
    frame->schema_version = AETHER_GPU_TIMESTAMP_SCHEMA_VERSION_V1;
    frame->reason[AETHER_GPU_TIMESTAMP_REASON_CAPACITY_V1 - 1] = '\0';
    if (frame->status != AETHER_GPU_TIMESTAMP_STATUS_ENABLED_V1 ||
        frame->valid == 0) {
        frame->valid = 0;
        frame->presence_flags &=
            ~static_cast<uint32_t>(
                AETHER_GPU_TIMESTAMP_PRESENCE_PERIOD_V1 |
                AETHER_GPU_TIMESTAMP_PRESENCE_DURATIONS_V1 |
                AETHER_GPU_TIMESTAMP_PRESENCE_RAW_PAIRS_V1);
        frame->stage_count = 0;
        frame->raw_pair_count = 0;
        frame->timestamp_period_ns = 0;
        std::fill(std::begin(frame->stage_duration_ns),
                  std::end(frame->stage_duration_ns), 0);
        std::memset(frame->raw_pairs, 0, sizeof(frame->raw_pairs));
    }
}

void publish_probe_locked(SnapshotStoreV1* store,
                          AetherGpuTimestampProbeV1 probe) noexcept {
    normalize_probe(&probe);
    ++store->probe_sequence;                                         
    store->probe = probe;
    store->probe_ready = true;
    ++store->probe_sequence;
}

void publish_frame_locked(SnapshotStoreV1* store,
                          AetherGpuTimestampFrameV1 frame) noexcept {
    normalize_frame(&frame);
    ++store->frame_sequence;                                         
    store->frame = frame;
    store->frame_ready = true;
    ++store->frame_sequence;
}

}              

uint64_t next_probe_instance_id_v1() noexcept {
    return g_next_probe_instance_id.fetch_add(1, std::memory_order_relaxed);
}

bool publish_probe_v1(const AetherGpuTimestampProbeV1& probe) noexcept {
    std::lock_guard<std::mutex> lock(g_timestamp_snapshots.mutex);
    if (g_timestamp_snapshots.probe_ready &&
        probe.probe_instance_id <
            g_timestamp_snapshots.probe.probe_instance_id) {
        return false;
    }
    publish_probe_locked(&g_timestamp_snapshots, probe);
    ++g_timestamp_snapshots.frame_sequence;
    g_timestamp_snapshots.last_frame_ordinal = 0;
    g_timestamp_snapshots.frame_ready = false;
    ++g_timestamp_snapshots.frame_sequence;
    return true;
}

bool publish_frame_v1(const AetherGpuTimestampFrameV1& frame) noexcept {
    std::lock_guard<std::mutex> lock(g_timestamp_snapshots.mutex);
    if (!g_timestamp_snapshots.probe_ready ||
        g_timestamp_snapshots.probe.probe_instance_id !=
            frame.probe_instance_id ||
        frame.extraction_ordinal == 0 ||
        frame.extraction_ordinal <=
            g_timestamp_snapshots.last_frame_ordinal) {
        return false;
    }
    publish_frame_locked(&g_timestamp_snapshots, frame);
    g_timestamp_snapshots.last_frame_ordinal = frame.extraction_ordinal;
    return true;
}

bool mark_frame_not_ready_v1(
        uint64_t expected_probe_instance_id) noexcept {
    std::lock_guard<std::mutex> lock(g_timestamp_snapshots.mutex);
    if (!g_timestamp_snapshots.probe_ready ||
        g_timestamp_snapshots.probe.probe_instance_id !=
            expected_probe_instance_id) {
        return false;
    }
    ++g_timestamp_snapshots.frame_sequence;
    g_timestamp_snapshots.frame_ready = false;
    ++g_timestamp_snapshots.frame_sequence;
    return true;
}

void clear_caller_thread_frame_v1() noexcept {
    g_caller_thread_frame_ready = false;
    g_caller_thread_frame = {};
}

bool stash_current_frame_for_caller_thread_v1(
        uint64_t expected_probe_instance_id,
        uint64_t expected_extraction_ordinal) noexcept {
    std::lock_guard<std::mutex> lock(g_timestamp_snapshots.mutex);
    if (!g_timestamp_snapshots.frame_ready ||
        g_timestamp_snapshots.frame.probe_instance_id !=
            expected_probe_instance_id ||
        g_timestamp_snapshots.frame.extraction_ordinal !=
            expected_extraction_ordinal) {
        clear_caller_thread_frame_v1();
        return false;
    }
    g_caller_thread_frame = g_timestamp_snapshots.frame;
    g_caller_thread_frame_ready = true;
    return true;
}

int take_caller_thread_frame_v1(
        AetherGpuTimestampFrameV1* out_frame) noexcept {
    if (out_frame == nullptr) {
        return AETHER_GPU_TIMESTAMP_PULL_INVALID_ARGUMENT_V1;
    }
    if (out_frame->struct_size != sizeof(*out_frame)) {
        return AETHER_GPU_TIMESTAMP_PULL_SIZE_MISMATCH_V1;
    }
    if (!g_caller_thread_frame_ready) {
        return AETHER_GPU_TIMESTAMP_PULL_NOT_READY_V1;
    }
    const AetherGpuTimestampFrameV1 local = g_caller_thread_frame;
    clear_caller_thread_frame_v1();
    *out_frame = local;
    return AETHER_GPU_TIMESTAMP_PULL_OK_V1;
}

bool finalize_frame_v1(AetherGpuTimestampFrameV1* frame,
                       uint64_t expected_probe_instance_id,
                       uint64_t expected_extraction_ordinal,
                       const uint32_t* cumulative_pass_counts,
                       uint32_t stage_count) noexcept {
    if (frame == nullptr) {
        return false;
    }
    const auto reject = [frame](uint32_t reason_code,
                                const char* reason) noexcept {
        const uint32_t rejected_pairs = frame->raw_pair_count;
        frame->status = AETHER_GPU_TIMESTAMP_STATUS_RESOLVE_FAILED_V1;
        frame->valid = 0;
        frame->reason_code = reason_code;
        frame->resolve_success_count = 0;
        frame->drop_count =
            add_drop_count_v1(frame->drop_count, rejected_pairs);
        copy_inline_text(frame->reason, reason);
        normalize_frame(frame);
        return false;
    };
    if (frame->probe_instance_id != expected_probe_instance_id ||
        frame->extraction_ordinal != expected_extraction_ordinal ||
        cumulative_pass_counts == nullptr ||
        stage_count != AETHER_GPU_TIMESTAMP_STAGE_CAPACITY_V1) {
        return reject(AETHER_GPU_TIMESTAMP_REASON_PROVENANCE_FAILED_V1,
                      "missing, foreign, or invalid nine-stage provenance");
    }
    const uint32_t pair_count = frame->raw_pair_count;
    if (pair_count > AETHER_GPU_TIMESTAMP_RAW_PAIR_CAPACITY_V1) {
        return reject(AETHER_GPU_TIMESTAMP_REASON_SLOT_OVERFLOW_V1,
                      "raw timestamp pair capacity exceeded");
    }
    uint32_t previous = 0;
    for (uint32_t stage = 0; stage < stage_count; ++stage) {
        const uint32_t boundary = cumulative_pass_counts[stage];
        if (boundary < previous || boundary > pair_count) {
            return reject(AETHER_GPU_TIMESTAMP_REASON_PROVENANCE_FAILED_V1,
                          "nonmonotonic or out-of-range stage boundary");
        }
        previous = boundary;
    }
    if (previous != pair_count) {
        return reject(AETHER_GPU_TIMESTAMP_REASON_PROVENANCE_FAILED_V1,
                      "final stage boundary does not cover every timestamp pair");
    }

    AetherGpuTimestampFrameV1 complete = *frame;
    complete.stage_count = stage_count;
    complete.raw_pair_count = pair_count;
    std::fill(std::begin(complete.stage_duration_ns),
              std::end(complete.stage_duration_ns), 0);
    uint32_t stage = 0;
    for (uint32_t pair = 0; pair < pair_count; ++pair) {
        while (stage < stage_count &&
               pair >= cumulative_pass_counts[stage]) {
            ++stage;
        }
                                                                                       
                                                                                          
        if (stage >= stage_count ||
            complete.raw_pairs[pair].end_tick <
                complete.raw_pairs[pair].begin_tick) {
            return reject(AETHER_GPU_TIMESTAMP_REASON_NONMONOTONIC_PAIR_V1,
                          "timestamp pair is nonmonotonic");
        }
        complete.raw_pairs[pair].stage_index = stage;
        const uint64_t ticks = complete.raw_pairs[pair].end_tick -
                               complete.raw_pairs[pair].begin_tick;
        if (complete.timestamp_period_ns == 0 ||
            ticks > std::numeric_limits<uint64_t>::max() /
                        complete.timestamp_period_ns) {
            return reject(AETHER_GPU_TIMESTAMP_REASON_RANGE_FAILED_V1,
                          "timestamp period or duration is invalid");
        }
        const uint64_t duration = ticks * complete.timestamp_period_ns;
        if (complete.stage_duration_ns[stage] >
            std::numeric_limits<uint64_t>::max() - duration) {
            return reject(AETHER_GPU_TIMESTAMP_REASON_RANGE_FAILED_V1,
                          "stage duration overflow");
        }
        complete.stage_duration_ns[stage] += duration;
    }
    complete.status = AETHER_GPU_TIMESTAMP_STATUS_ENABLED_V1;
    complete.valid = 1;
    complete.reason_code = AETHER_GPU_TIMESTAMP_REASON_NONE_V1;
    complete.drop_count = 0;
    complete.presence_flags =
        AETHER_GPU_TIMESTAMP_PRESENCE_PERIOD_V1 |
        AETHER_GPU_TIMESTAMP_PRESENCE_DURATIONS_V1 |
        AETHER_GPU_TIMESTAMP_PRESENCE_RAW_PAIRS_V1 |
        AETHER_GPU_TIMESTAMP_PRESENCE_PROBE_REF_V1;
    copy_inline_text(complete.reason, "");
    *frame = complete;
    return true;
}

void reset_snapshots_for_test_v1() noexcept {
    std::lock_guard<std::mutex> lock(g_timestamp_snapshots.mutex);
    g_timestamp_snapshots.probe_sequence = 0;
    g_timestamp_snapshots.frame_sequence = 0;
    g_timestamp_snapshots.last_frame_ordinal = 0;
    g_timestamp_snapshots.probe_ready = false;
    g_timestamp_snapshots.frame_ready = false;
    g_timestamp_snapshots.probe = {};
    g_timestamp_snapshots.frame = {};
    clear_caller_thread_frame_v1();
}

int pull_probe_v1(AetherGpuTimestampProbeV1* out_probe) noexcept {
    if (out_probe == nullptr) {
        return AETHER_GPU_TIMESTAMP_PULL_INVALID_ARGUMENT_V1;
    }
    if (out_probe->struct_size != sizeof(*out_probe)) {
        return AETHER_GPU_TIMESTAMP_PULL_SIZE_MISMATCH_V1;
    }
    std::lock_guard<std::mutex> lock(g_timestamp_snapshots.mutex);
    if (!g_timestamp_snapshots.probe_ready) {
        return AETHER_GPU_TIMESTAMP_PULL_NOT_READY_V1;
    }
    const uint64_t before = g_timestamp_snapshots.probe_sequence;
    const AetherGpuTimestampProbeV1 local = g_timestamp_snapshots.probe;
    const uint64_t after = g_timestamp_snapshots.probe_sequence;
    if ((before & 1u) != 0 || before != after) {
        return AETHER_GPU_TIMESTAMP_PULL_SNAPSHOT_INCONSISTENT_V1;
    }
    *out_probe = local;
    return AETHER_GPU_TIMESTAMP_PULL_OK_V1;
}

int pull_frame_v1(AetherGpuTimestampFrameV1* out_frame) noexcept {
    if (out_frame == nullptr) {
        return AETHER_GPU_TIMESTAMP_PULL_INVALID_ARGUMENT_V1;
    }
    if (out_frame->struct_size != sizeof(*out_frame)) {
        return AETHER_GPU_TIMESTAMP_PULL_SIZE_MISMATCH_V1;
    }
    std::lock_guard<std::mutex> lock(g_timestamp_snapshots.mutex);
    if (!g_timestamp_snapshots.frame_ready) {
        return AETHER_GPU_TIMESTAMP_PULL_NOT_READY_V1;
    }
    const uint64_t before = g_timestamp_snapshots.frame_sequence;
    const AetherGpuTimestampFrameV1 local = g_timestamp_snapshots.frame;
    const uint64_t after = g_timestamp_snapshots.frame_sequence;
    if ((before & 1u) != 0 || before != after) {
        return AETHER_GPU_TIMESTAMP_PULL_SNAPSHOT_INCONSISTENT_V1;
    }
    *out_frame = local;
    return AETHER_GPU_TIMESTAMP_PULL_OK_V1;
}

}                                     

bool DawnKernelHarness::take_device_error(std::string* msg) {
    if (!g_device_error.exchange(false, std::memory_order_acq_rel)) {
        return false;
    }
    if (msg != nullptr) {
        std::lock_guard<std::mutex> lock(g_device_error_mu);
        *msg = g_device_error_msg;
    }
    return true;
}

DawnKernelHarness::DawnKernelHarness() = default;

DawnKernelHarness::~DawnKernelHarness() {
                                                                      
                                                   
}

bool DawnKernelHarness::init() {
    using namespace gpu_timestamp_internal;

    if (init_called_) {
        std::cerr << "[DawnKernelHarness] init is one-shot\n";
        return false;
    }
    init_called_ = true;
    ts_requested_ = process_env_requests_v1();
    ts_feature_request_to_device_ = false;
    ts_feature_granted_ = false;
    ts_enabled_ = false;
                                                                                     
                                                            
    hb_enabled_ = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_HOST_BD");
        return v != nullptr && !(v[0] == '0' && v[1] == '\0');
    }();
    ts_capability_ = AETHER_GPU_TIMESTAMP_CAPABILITY_NOT_PROBED_V1;
    ts_status_ = AETHER_GPU_TIMESTAMP_STATUS_OFF_V1;
    ts_reason_code_ = AETHER_GPU_TIMESTAMP_REASON_NOT_REQUESTED_V1;
    ts_probe_instance_id_ = next_probe_instance_id_v1();
    std::string adapter_identity;

    const auto publish_probe =
        [this, &adapter_identity](uint32_t capability, uint32_t status,
                                  uint32_t reason_code, const char* reason,
                                  uint32_t query_capacity,
                                  uint64_t period_ns) {
            AetherGpuTimestampProbeV1 probe{};
            probe.struct_size = sizeof(probe);
            probe.schema_version = AETHER_GPU_TIMESTAMP_SCHEMA_VERSION_V1;
            probe.requested = ts_requested_ ? 1u : 0u;
            probe.probe_instance_id = ts_probe_instance_id_;
            probe.capability = capability;
            probe.status = status;
            probe.ts_enabled =
                status == AETHER_GPU_TIMESTAMP_STATUS_ENABLED_V1 ? 1u : 0u;
            probe.reason_code = reason_code;
            probe.query_capacity = query_capacity;
            probe.timestamp_period_ns = period_ns;
            if (probe.ts_enabled != 0 && period_ns != 0) {
                probe.presence_flags =
                    AETHER_GPU_TIMESTAMP_PRESENCE_PERIOD_V1;
            }
            const auto put = [](auto* destination, size_t capacity,
                                const char* source) {
                if (capacity == 0) return;
                const char* value = source == nullptr ? "" : source;
                const size_t length =
                    std::min(std::strlen(value), capacity - 1);
                std::memcpy(destination, value, length);
                destination[length] = '\0';
            };
            put(probe.selected_env_key, sizeof(probe.selected_env_key),
                AETHER_GPU_TIMESTAMP_ENV_KEY_V1);
            put(probe.reason, sizeof(probe.reason), reason);
            put(probe.adapter_identity, sizeof(probe.adapter_identity),
                adapter_identity.c_str());
            put(probe.dawn_version, sizeof(probe.dawn_version),
                "git:" AETHER_GPU_TIMESTAMP_DAWN_REVISION);
            put(probe.dawn_build_hash, sizeof(probe.dawn_build_hash),
                AETHER_GPU_TIMESTAMP_DAWN_ARTIFACT_SHA256);
            ts_capability_ = capability;
            ts_status_ = status;
            ts_reason_code_ = reason_code;
            publish_probe_v1(probe);
        };

                                                                           
                                                                         
                                                                             
                                                                               
    if (!ts_requested_) {
        publish_probe(AETHER_GPU_TIMESTAMP_CAPABILITY_NOT_PROBED_V1,
                      AETHER_GPU_TIMESTAMP_STATUS_OFF_V1,
                      AETHER_GPU_TIMESTAMP_REASON_NOT_REQUESTED_V1,
                      "timestamp query not requested", 0, 0);
    }

                                                                                                          
    hist_sink_ = dawn_histogram_sink_platform();
    dawn::native::DawnInstanceDescriptor dawn_instance_desc{};
    dawn_instance_desc.platform =
        static_cast<dawn::platform::Platform*>(hist_sink_);
                                                                                               
                                                                                                   
                                                                                       
                                                                              
                                                                                                   
                                                                                         
                                                                             

                                                                           
                                                                   
    static constexpr auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor instance_desc{
        .requiredFeatureCount = 1,
        .requiredFeatures = &kTimedWaitAny,
    };
                                                                                       
    instance_desc.nextInChain =
        reinterpret_cast<wgpu::ChainedStruct*>(&dawn_instance_desc);
    instance_ = wgpu::CreateInstance(&instance_desc);
    if (instance_ == nullptr) {
        if (ts_requested_) {
            publish_probe(AETHER_GPU_TIMESTAMP_CAPABILITY_PROBE_ERROR_V1,
                          AETHER_GPU_TIMESTAMP_STATUS_RESOURCE_FAILED_V1,
                          AETHER_GPU_TIMESTAMP_REASON_INSTANCE_FAILED_V1,
                          "Dawn instance creation failed", 0, 0);
        }
        std::cerr << "[DawnKernelHarness] wgpu::CreateInstance failed\n";
        return false;
    }

                                                                   
                                                                                   
                                                                                  
    {
        wgpu::RequestAdapterOptions options{};
        instance_.WaitAny(
            instance_.RequestAdapter(
                &options,
                wgpu::CallbackMode::WaitAnyOnly,
                [this](wgpu::RequestAdapterStatus status,
                       wgpu::Adapter adapter,
                       wgpu::StringView message) {
                    if (status != wgpu::RequestAdapterStatus::Success) {
                        std::cerr << "[DawnKernelHarness] RequestAdapter failed: "
                                  << message << '\n';
                        return;
                    }
                    adapter_ = std::move(adapter);
                }),
            init_wait_ns());
        if (adapter_ == nullptr) {
            if (ts_requested_) {
                publish_probe(AETHER_GPU_TIMESTAMP_CAPABILITY_PROBE_ERROR_V1,
                              AETHER_GPU_TIMESTAMP_STATUS_RESOURCE_FAILED_V1,
                              AETHER_GPU_TIMESTAMP_REASON_ADAPTER_FAILED_V1,
                              "Dawn adapter request failed", 0, 0);
            }
            std::cerr << "[DawnKernelHarness] adapter is null\n";
            return false;
        }
    }

    bool timestamp_feature_requested = false;
    if (ts_requested_) {
        wgpu::AdapterInfo info{};
        if (!adapter_.GetInfo(&info)) {
            publish_probe(AETHER_GPU_TIMESTAMP_CAPABILITY_PROBE_ERROR_V1,
                          AETHER_GPU_TIMESTAMP_STATUS_RESOURCE_FAILED_V1,
                          AETHER_GPU_TIMESTAMP_REASON_ADAPTER_FAILED_V1,
                          "adapter identity query failed", 0, 0);
        } else {
            std::ostringstream identity;
            identity
                     << "timestamp_domain=webgpu_queue"
                     << "|resolved_unit=nanoseconds"
                     << "|disable_timestamp_query_conversion=forced_disabled|"
                     << info.vendor << '|' << info.architecture << '|'
                     << info.device << '|' << info.description << "|vendor=0x"
                     << std::hex << info.vendorID << "|device=0x"
                     << info.deviceID << std::dec << "|backend="
                     << static_cast<uint32_t>(info.backendType);
            adapter_identity = identity.str();
            if (adapter_.HasFeature(wgpu::FeatureName::TimestampQuery)) {
                timestamp_feature_requested = true;
            } else {
                publish_probe(AETHER_GPU_TIMESTAMP_CAPABILITY_UNSUPPORTED_V1,
                              AETHER_GPU_TIMESTAMP_STATUS_UNSUPPORTED_V1,
                              AETHER_GPU_TIMESTAMP_REASON_ADAPTER_UNSUPPORTED_V1,
                              "adapter does not advertise TimestampQuery", 0,
                              0);
            }
        }
    }

                                                                  
                                                                                
                                              
    {
        wgpu::DeviceDescriptor device_desc{};
        wgpu::DawnTogglesDescriptor timestamp_toggles{};
        const char* disabled_timestamp_toggle =
            "disable_timestamp_query_conversion";
                                                                       
                                                                   
                                                                        
                                                                       
                                                                        
                                                                        
                                                                          
                                                                         
        device_desc.SetUncapturedErrorCallback(on_uncaptured_error);

                                                                 
        // @workgroup_size(512). WebGPU's default cap is 256; bump
                                                                    
                                                                   
                                                                  
                                          
        wgpu::Limits required_limits{};
                                                                                           
                                                                                
                                                                                            
                                                                                             
        wgpu::Limits adapter_limits{};
        const bool have_adapter_limits =
            adapter_.GetLimits(&adapter_limits) == wgpu::Status::Success;
        auto clamp_to_adapter = [&](uint32_t want, uint32_t have) -> uint32_t {
            return have_adapter_limits && have < want ? have : want;
        };
        auto clamp_to_adapter64 = [&](uint64_t want, uint64_t have) -> uint64_t {
            return have_adapter_limits && have < want ? have : want;
        };
        required_limits.maxComputeInvocationsPerWorkgroup =
            clamp_to_adapter(512, adapter_limits.maxComputeInvocationsPerWorkgroup);
        required_limits.maxComputeWorkgroupSizeX =
            clamp_to_adapter(512, adapter_limits.maxComputeWorkgroupSizeX);
                                                                        
                                                                       
                                                                       
                                                                          
                                                                      
        required_limits.maxStorageBuffersPerShaderStage =
            clamp_to_adapter(10, adapter_limits.maxStorageBuffersPerShaderStage);
                                                                                 
                                                                           
                                                                              
                                                                                 
        required_limits.maxBufferSize =
            clamp_to_adapter64(1024ull * 1024ull * 1024ull, adapter_limits.maxBufferSize);
        required_limits.maxStorageBufferBindingSize =
            clamp_to_adapter64(1024ull * 1024ull * 1024ull, adapter_limits.maxStorageBufferBindingSize);
        std::fprintf(stderr,
                     "[DawnKernelHarness] limits: invocations/wg=%u wgSizeX=%u storageBufs=%u maxBuffer=%llu maxStorageBinding=%llu (adapter: %u %u %u %llu %llu)\n",
                     required_limits.maxComputeInvocationsPerWorkgroup, required_limits.maxComputeWorkgroupSizeX,
                     required_limits.maxStorageBuffersPerShaderStage,
                     (unsigned long long)required_limits.maxBufferSize, (unsigned long long)required_limits.maxStorageBufferBindingSize,
                     adapter_limits.maxComputeInvocationsPerWorkgroup, adapter_limits.maxComputeWorkgroupSizeX,
                     adapter_limits.maxStorageBuffersPerShaderStage,
                     (unsigned long long)adapter_limits.maxBufferSize, (unsigned long long)adapter_limits.maxStorageBufferBindingSize);
        device_desc.requiredLimits = &required_limits;

                                                                 
                                                                   
                                                                     
                                                                   
                                                                    
                                                                      
                                                                  
                           
                                                                               
                                                                               
                                                                                   
                                                                                 
                               
        std::vector<wgpu::FeatureName> feats;
                                                                                   
                                                                                   
                                                                                                   
                                                                                             
                                                                                                  
        if (adapter_.HasFeature(wgpu::FeatureName::Subgroups)) {
            feats.push_back(wgpu::FeatureName::Subgroups);
        }
        if (adapter_.HasFeature(wgpu::FeatureName::ShaderModuleCompilationOptions)) {
            feats.push_back(wgpu::FeatureName::ShaderModuleCompilationOptions);
            has_strict_math_ = true;
        }
                                                                                             
                                                                                           
                                                                                                      
                                                                                        
#if defined(AETHER_GPU_TIMESTAMPS_ENV_OFFICIAL)
        if (const char* sm = std::getenv("OFFICIAL_AETHER_STRICT_MATH"))
            strict_math_ = (sm[0] == '1');
#else
        if (const char* sm = std::getenv("AETHER_STRICT_MATH")) strict_math_ = (sm[0] == '1');
#endif
        if (adapter_.HasFeature(wgpu::FeatureName::ShaderF16)) {
            feats.push_back(wgpu::FeatureName::ShaderF16);
            has_f16_ = true;
        }
                                                                              
                                                                      
                                                                                 
                                                                                       
                                                                                             
                                                                                      
                                                                                           
                                                               
        static const bool no_lazy_clear = [] {
            const char* v = std::getenv("OFFICIAL_AETHER_DAWN_LAZY_CLEAR");
            return v != nullptr && v[0] == '0' && v[1] == '\0';
        }();
        std::vector<const char*> disabled_toggles;
        if (timestamp_feature_requested) {
            ts_feature_request_to_device_ = true;
            feats.push_back(wgpu::FeatureName::TimestampQuery);
            disabled_toggles.push_back(disabled_timestamp_toggle);
        }
        if (no_lazy_clear) disabled_toggles.push_back("lazy_clear_resource_on_first_use");
                                                                                     
                                                                                          
        std::vector<const char*> enabled_toggles;
        if (msl_dump_dir() != nullptr) enabled_toggles.push_back("dump_shaders");
        if (!disabled_toggles.empty() || !enabled_toggles.empty()) {
            timestamp_toggles.disabledToggleCount = disabled_toggles.size();
            timestamp_toggles.disabledToggles = disabled_toggles.data();
            timestamp_toggles.enabledToggleCount = enabled_toggles.size();
            timestamp_toggles.enabledToggles = enabled_toggles.data();
            device_desc.nextInChain = &timestamp_toggles;
        }
        device_desc.requiredFeatureCount = feats.size();
        device_desc.requiredFeatures = feats.data();
        instance_.WaitAny(
            adapter_.RequestDevice(
                &device_desc,
                wgpu::CallbackMode::WaitAnyOnly,
                [this](wgpu::RequestDeviceStatus status,
                       wgpu::Device device,
                       wgpu::StringView message) {
                    if (status != wgpu::RequestDeviceStatus::Success) {
                        std::cerr << "[DawnKernelHarness] RequestDevice failed: "
                                  << message << '\n';
                        return;
                    }
                    device_ = std::move(device);
                }),
            init_wait_ns());                               
        if (device_ != nullptr && timestamp_feature_requested) {
            ts_feature_granted_ = true;
        }
        if (device_ == nullptr && timestamp_feature_requested) {
                                                                           
                                                                                
                                                     
            publish_probe(AETHER_GPU_TIMESTAMP_CAPABILITY_SUPPORTED_V1,
                          AETHER_GPU_TIMESTAMP_STATUS_RESOURCE_FAILED_V1,
                          AETHER_GPU_TIMESTAMP_REASON_DEVICE_FAILED_V1,
                          "device rejected TimestampQuery enablement", 0, 0);
            feats.pop_back();
            timestamp_feature_requested = false;
            device_desc.nextInChain = nullptr;
            device_desc.requiredFeatureCount = feats.size();
            device_desc.requiredFeatures = feats.data();
            instance_.WaitAny(
                adapter_.RequestDevice(
                    &device_desc, wgpu::CallbackMode::WaitAnyOnly,
                    [this](wgpu::RequestDeviceStatus status,
                           wgpu::Device device, wgpu::StringView message) {
                        if (status != wgpu::RequestDeviceStatus::Success) {
                            std::cerr
                                << "[DawnKernelHarness] fallback RequestDevice "
                                   "failed: "
                                << message << '\n';
                            return;
                        }
                        device_ = std::move(device);
                    }),
                init_wait_ns());                               
        }
        if (device_ == nullptr) {
            if (ts_requested_ &&
                ts_status_ != AETHER_GPU_TIMESTAMP_STATUS_RESOURCE_FAILED_V1) {
                publish_probe(
                    ts_capability_,
                    AETHER_GPU_TIMESTAMP_STATUS_RESOURCE_FAILED_V1,
                    AETHER_GPU_TIMESTAMP_REASON_DEVICE_FAILED_V1,
                    "Dawn device creation failed", 0, 0);
            }
            std::cerr << "[DawnKernelHarness] device is null\n";
            return false;
        }
    }

                                               
                                                                                           
                                                                                            
                                                                                             
    if (msl_dump_dir() != nullptr) {
        device_.SetLoggingCallback(
            +[](wgpu::LoggingType, wgpu::StringView message) {
                std::string m(message.data, message.length);
                if (m.rfind(kMslDumpPrefix, 0) != 0) return;
                std::lock_guard<std::mutex> lk(g_msl_mu);
                g_msl_dumps.push_back(std::move(m));
            });
    }
    queue_ = device_.GetQueue();

    if (queue_ == nullptr) {
        if (ts_requested_) {
            publish_probe(ts_capability_,
                          AETHER_GPU_TIMESTAMP_STATUS_RESOURCE_FAILED_V1,
                          AETHER_GPU_TIMESTAMP_REASON_DEVICE_FAILED_V1,
                          "device queue acquisition failed", 0, 0);
        }
        std::cerr << "[DawnKernelHarness] device.GetQueue returned null\n";
        return false;
    }

                                                                             
                                                                           
    if (timestamp_feature_requested) {
        push_timestamp_error_scopes(device_);
        wgpu::QuerySetDescriptor qd{};
        qd.type = wgpu::QueryType::Timestamp;
        qd.count = kTsCapacity;
#if defined(AETHER_GPU_TIMESTAMPS_ENV_SELFTEST)
        if (env_value_requests_v1(std::getenv(
                "AETHER_GPU_TIMESTAMP_SELFTEST_FORCE_RESOURCE_SCOPE_ERROR"))) {
                                                                           
                                                                              
                                                   
            qd.count = std::numeric_limits<uint32_t>::max();
        }
#endif
        ts_qset_ = device_.CreateQuerySet(&qd);
        wgpu::BufferDescriptor rd{};
        rd.usage = wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc;
        rd.size = kTsCapacity * sizeof(uint64_t);
        ts_resolve_ = device_.CreateBuffer(&rd);
        wgpu::BufferDescriptor bd{};
        bd.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
        bd.size = kTsCapacity * sizeof(uint64_t);
        ts_readback_ = device_.CreateBuffer(&bd);
        const TimestampErrorScopeResult resource_scopes =
            pop_timestamp_error_scopes(instance_, device_);
        const bool handles_nonnull =
            ts_qset_ != nullptr && ts_resolve_ != nullptr &&
            ts_readback_ != nullptr;
        const auto resource_decision =
            classify_scoped_resource_result_v1(
                handles_nonnull, resource_scopes.pop_status_success,
                resource_scopes.clean);
        if (!resource_decision.accepted) {
            ts_enabled_ = false;
            uint32_t reason_code =
                AETHER_GPU_TIMESTAMP_REASON_QUERY_SET_FAILED_V1;
            const char* reason = "timestamp query-set allocation failed";
            if (!resource_scopes.pop_status_success ||
                !resource_scopes.clean) {
                reason = "timestamp resource error scope rejected creation";
            } else if (ts_qset_ != nullptr && ts_resolve_ == nullptr) {
                reason_code =
                    AETHER_GPU_TIMESTAMP_REASON_RESOLVE_BUFFER_FAILED_V1;
                reason = "timestamp resolve-buffer allocation failed";
            } else if (ts_qset_ != nullptr && ts_resolve_ != nullptr &&
                       ts_readback_ == nullptr) {
                reason_code =
                    AETHER_GPU_TIMESTAMP_REASON_READBACK_BUFFER_FAILED_V1;
                reason = "timestamp readback-buffer allocation failed";
            }
            publish_probe(AETHER_GPU_TIMESTAMP_CAPABILITY_SUPPORTED_V1,
                          AETHER_GPU_TIMESTAMP_STATUS_RESOURCE_FAILED_V1,
                          reason_code, reason, 0, 0);
        } else {
            ts_enabled_ = true;
            hb_enabled_ = true;                                                          
            publish_probe(AETHER_GPU_TIMESTAMP_CAPABILITY_SUPPORTED_V1,
                          AETHER_GPU_TIMESTAMP_STATUS_ENABLED_V1,
                          AETHER_GPU_TIMESTAMP_REASON_NONE_V1, "",
                          kTsCapacity, 1);
        }
    }
    return true;
}

                                                                             
                                                                          
namespace {
inline double hb_now_ms(bool armed) {
    if (!armed) return 0.0;
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
}              

wgpu::Buffer DawnKernelHarness::upload(const void* data, size_t size,
                                        wgpu::BufferUsage usage) {
    const double t0 = hb_now_ms(hb_enabled_);
    wgpu::BufferDescriptor desc{
        .usage = usage | wgpu::BufferUsage::CopyDst,
        .size = size,
    };
    wgpu::Buffer buf = device_.CreateBuffer(&desc);
    queue_.WriteBuffer(buf,            0, data, size);
    if (hb_enabled_) {
        hb_.upload_ms += hb_now_ms(true) - t0;
        ++hb_.n_upload;
    }
    return buf;
}

wgpu::Buffer DawnKernelHarness::alloc(size_t size, wgpu::BufferUsage usage) {
    wgpu::BufferDescriptor desc{
        .usage = usage,
        .size = size,
    };
    return device_.CreateBuffer(&desc);
}

wgpu::Buffer DawnKernelHarness::alloc_staging_for_readback(size_t size) {
    wgpu::BufferDescriptor desc{
        .usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst,
        .size = size,
    };
    return device_.CreateBuffer(&desc);
}

wgpu::ComputePipeline
DawnKernelHarness::load_compute(std::string_view wgsl_source,
                                const char* entry_point) {
                                                                             
                                                                           
                                                                             
                                                                            
                                                                             
    std::string key(strict_math() ? "S:" : "F:");
    key.append(entry_point);
    key.push_back('\0');
    key.append(wgsl_source.data(), wgsl_source.size());
    auto it = pipeline_cache_.find(key);
    if (it != pipeline_cache_.end()) {
        return it->second;
    }
    const double hb_t0 = hb_now_ms(hb_enabled_);                               

    wgpu::ShaderSourceWGSL wgsl_desc{};
                                                                           
    wgsl_desc.code = wgpu::StringView{
        wgsl_source.data(),
        wgsl_source.size(),
    };
    wgpu::ShaderModuleCompilationOptions compile_opts{};
    compile_opts.strictMath = true;
    if (strict_math()) wgsl_desc.nextInChain = &compile_opts;                                                 
    wgpu::ShaderModuleDescriptor shader_desc{};
    shader_desc.nextInChain = &wgsl_desc;
    wgpu::ShaderModule shader = device_.CreateShaderModule(&shader_desc);

    wgpu::ComputePipelineDescriptor pipeline_desc{};
    pipeline_desc.compute.module = shader;
    pipeline_desc.compute.entryPoint = wgpu::StringView{entry_point, WGPU_STRLEN};
    wgpu::ComputePipeline pipeline = device_.CreateComputePipeline(&pipeline_desc);
    if (hb_enabled_) {                                          
        hb_.create_ms += hb_now_ms(true) - hb_t0;
        ++hb_.n_create;
    }
    pipeline_cache_.emplace(std::move(key), pipeline);
    return pipeline;
}

void DawnKernelHarness::dispatch(const wgpu::ComputePipeline& pipeline,
                                  const std::vector<wgpu::Buffer>& bindings,
                                  uint32_t wg_x, uint32_t wg_y, uint32_t wg_z) {
    std::vector<BufBinding> b;
    b.reserve(bindings.size());
    for (const auto& x : bindings) b.emplace_back(x);
    dispatch(pipeline, b, wg_x, wg_y, wg_z);
}

void DawnKernelHarness::dispatch(const wgpu::ComputePipeline& pipeline,
                                  const std::vector<BufBinding>& bindings,
                                  uint32_t wg_x, uint32_t wg_y, uint32_t wg_z) {
    if (!device_healthy_) return;                                                  
    const double hb_t0 = hb_now_ms(hb_enabled_);                            
                                                                            
                                                                        
                                                                       
                                                                       
                                                
    std::vector<wgpu::BindGroupEntry> bg_entries(bindings.size());
    for (size_t i = 0; i < bindings.size(); ++i) {
        bg_entries[i].binding = static_cast<uint32_t>(i);
        bg_entries[i].buffer = bindings[i].buffer;                      
        bg_entries[i].offset = bindings[i].offset;
        bg_entries[i].size = bindings[i].size;
    }
    wgpu::BindGroupDescriptor bg_desc{};
    bg_desc.layout = pipeline.GetBindGroupLayout(0);
    bg_desc.entryCount = static_cast<uint32_t>(bg_entries.size());
    bg_desc.entries = bg_entries.data();
    wgpu::BindGroup bind_group = device_.CreateBindGroup(&bg_desc);

                                                 
    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    {
        wgpu::ComputePassDescriptor pd{};                 
        wgpu::PassTimestampWrites tw{};
        wgpu::ComputePassEncoder pass =
            encoder.BeginComputePass(ts_pass_desc(&pd, &tw));
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bind_group);
        pass.DispatchWorkgroups(wg_x, wg_y, wg_z);
        pass.End();
    }
    wgpu::CommandBuffer commands = encoder.Finish();
    const double hb_t1 = hb_now_ms(hb_enabled_);                                   
    if (hb_enabled_) ++hb_.n_submit;
    queue_.Submit(1, &commands);

                                                                           
                                                                     
                                                                             
    bool done = false;
    const wgpu::WaitStatus st = instance_.WaitAny(
        queue_.OnSubmittedWorkDone(
            wgpu::CallbackMode::WaitAnyOnly,
            [&done](wgpu::QueueWorkDoneStatus, wgpu::StringView) {
                done = true;
            }),
        runtime_wait_ns());
    if (st != wgpu::WaitStatus::Success || !done) {
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] GPU wait timeout/failure (st="
                  << int(st) << ") — device marked unhealthy\n";
        return;
    }
    if (hb_enabled_) {                                         
        hb_.encode_ms += hb_t1 - hb_t0;
        hb_.submit_wait_ms += hb_now_ms(true) - hb_t1;
        ++hb_.n_dispatch;
    }
}

void DawnKernelHarness::gpu_ts_reset() {
    ts_next_ = 0;
    ts_drop_count_ = 0;
    ts_resolve_attempt_count_ = 0;
    ts_pending_ready_ = false;
    ts_pending_ticks_.clear();
    ++ts_extraction_ordinal_;

    if (ts_enabled_) {
                                                                               
                                                                             
                                           
        gpu_timestamp_internal::mark_frame_not_ready_v1(
            ts_probe_instance_id_);
        return;
    }

    AetherGpuTimestampFrameV1 frame{};
    frame.struct_size = sizeof(frame);
    frame.schema_version = AETHER_GPU_TIMESTAMP_SCHEMA_VERSION_V1;
    frame.presence_flags = AETHER_GPU_TIMESTAMP_PRESENCE_PROBE_REF_V1;
    frame.status = ts_status_;
    frame.probe_instance_id = ts_probe_instance_id_;
    frame.extraction_ordinal = ts_extraction_ordinal_;
    frame.valid = 0;
    frame.reason_code = ts_reason_code_;
    const char* reason = "timestamp query unavailable";
    if (ts_status_ == AETHER_GPU_TIMESTAMP_STATUS_OFF_V1) {
        reason = "timestamp query not requested";
    } else if (ts_status_ == AETHER_GPU_TIMESTAMP_STATUS_UNSUPPORTED_V1) {
        reason = "adapter does not advertise TimestampQuery";
    } else if (ts_status_ == AETHER_GPU_TIMESTAMP_STATUS_RESOURCE_FAILED_V1) {
        reason = "timestamp query resource initialization failed";
    }
    std::strncpy(frame.reason, reason, sizeof(frame.reason) - 1);
    frame.reason[sizeof(frame.reason) - 1] = '\0';
    gpu_timestamp_internal::publish_frame_v1(frame);
}

                                                                              
                                                                              
                                                        
const wgpu::ComputePassDescriptor* DawnKernelHarness::ts_pass_desc(
        wgpu::ComputePassDescriptor* storage,
        wgpu::PassTimestampWrites* writes) {
    if (!ts_enabled_) return nullptr;
    if (ts_next_ + 2 > kTsCapacity) {
        ts_drop_count_ =
            gpu_timestamp_internal::increment_drop_count_v1(ts_drop_count_);
        return nullptr;
    }
    writes->querySet = ts_qset_;
    writes->beginningOfPassWriteIndex = ts_next_;
    writes->endOfPassWriteIndex = ts_next_ + 1;
    ts_next_ += 2;
    storage->timestampWrites = writes;
    return storage;
}

bool DawnKernelHarness::gpu_ts_resolve(std::vector<uint64_t>* out_ns) {
    if (!ts_enabled_) return false;
    if (!device_healthy_) return false;                                                  

    const auto attempt =
        gpu_timestamp_internal::begin_resolve_attempt_v1(
            ts_resolve_attempt_count_);
    ts_resolve_attempt_count_ = attempt.attempt_count;

    const auto publish_resolve_failure =
        [this](uint32_t reason_code, const char* reason, uint32_t drops) {
            ts_pending_ready_ = false;
            ts_pending_ticks_.clear();
            AetherGpuTimestampFrameV1 frame{};
            frame.struct_size = sizeof(frame);
            frame.schema_version = AETHER_GPU_TIMESTAMP_SCHEMA_VERSION_V1;
            frame.presence_flags =
                AETHER_GPU_TIMESTAMP_PRESENCE_PROBE_REF_V1;
            frame.status = AETHER_GPU_TIMESTAMP_STATUS_RESOLVE_FAILED_V1;
            frame.probe_instance_id = ts_probe_instance_id_;
            frame.extraction_ordinal = ts_extraction_ordinal_;
            frame.valid = 0;
            frame.reason_code = reason_code;
            frame.resolve_attempt_count = ts_resolve_attempt_count_;
            frame.resolve_success_count = 0;
            frame.drop_count = drops;
            std::strncpy(frame.reason, reason, sizeof(frame.reason) - 1);
            frame.reason[sizeof(frame.reason) - 1] = '\0';
            if (gpu_ts_diag_enabled()) {                            
                std::cerr << "[GPU-TS-DIAG] resolve FAILED reason_code="
                          << reason_code << " reason=\"" << reason
                          << "\" drops=" << drops
                          << " ts_next=" << ts_next_
                          << " ts_drop_count=" << ts_drop_count_ << "\n";
            }
            gpu_timestamp_internal::publish_frame_v1(frame);
        };

    if (!attempt.encode_allowed) {
        if (out_ns != nullptr) {
            out_ns->clear();
        }
        publish_resolve_failure(
            AETHER_GPU_TIMESTAMP_REASON_PROVENANCE_FAILED_V1,
            "more than one timestamp resolve attempted for one extraction",
            gpu_timestamp_internal::add_drop_count_v1(
                ts_drop_count_, ts_next_ / 2));
        return false;
    }
    if (out_ns == nullptr) {
        publish_resolve_failure(
            AETHER_GPU_TIMESTAMP_REASON_RANGE_FAILED_V1,
            "timestamp resolve output is null",
            gpu_timestamp_internal::add_drop_count_v1(
                ts_drop_count_, ts_next_ / 2));
        return false;
    }
    out_ns->clear();
    if (ts_next_ == 0) {
        publish_resolve_failure(
            AETHER_GPU_TIMESTAMP_REASON_NO_RECORDED_PAIRS_V1,
            "no timestamp pass pairs were recorded", ts_drop_count_);
        return false;
    }
    const uint32_t n = ts_next_;
    ts_pending_ready_ = false;
    ts_pending_ticks_.clear();
    static constexpr std::array<uint64_t, kTsCapacity> kZeroTimestamps{};
    push_timestamp_error_scopes(device_);
    queue_.WriteBuffer(ts_readback_, 0, kZeroTimestamps.data(),
                       n * sizeof(uint64_t));
    wgpu::CommandEncoder enc = device_.CreateCommandEncoder();
    uint32_t first_query = 0;
#if defined(AETHER_GPU_TIMESTAMPS_ENV_SELFTEST)
    if (gpu_timestamp_internal::env_value_requests_v1(std::getenv(
            "AETHER_GPU_TIMESTAMP_SELFTEST_FORCE_RESOLVE_SCOPE_ERROR"))) {
                                                                          
                                                                              
                                                                         
        first_query = kTsCapacity;
    }
#endif
    enc.ResolveQuerySet(ts_qset_, first_query, n, ts_resolve_, 0);
    enc.CopyBufferToBuffer(ts_resolve_, 0, ts_readback_, 0,
                           n * sizeof(uint64_t));
    wgpu::CommandBuffer cb = enc.Finish();
    if (hb_enabled_) ++hb_.n_submit;
    queue_.Submit(1, &cb);

    bool queue_callback_completed = false;
    bool queue_success = false;
    const wgpu::Future queue_future = queue_.OnSubmittedWorkDone(
        wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::QueueWorkDoneStatus status, wgpu::StringView) {
            queue_callback_completed = true;
            queue_success = status == wgpu::QueueWorkDoneStatus::Success;
        });
                                                                                      
    const wgpu::WaitStatus queue_wait_status =
        instance_.WaitAny(queue_future, runtime_wait_ns());
    if (queue_wait_status != wgpu::WaitStatus::Success ||
        !queue_callback_completed) {
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] GPU wait timeout/failure (st="
                  << int(queue_wait_status)
                  << ") — device marked unhealthy\n";
                                                                            
    }
    queue_success =
        queue_success && queue_callback_completed &&
        queue_wait_status == wgpu::WaitStatus::Success;
    const TimestampErrorScopeResult resolve_scopes =
        pop_timestamp_error_scopes(instance_, device_);
    const auto resolve_decision =
        gpu_timestamp_internal::classify_resolve_completion_v1(
            queue_success,
            resolve_scopes.pop_status_success && resolve_scopes.clean);
    if (!resolve_decision.accepted) {
        publish_resolve_failure(
            AETHER_GPU_TIMESTAMP_REASON_PROVENANCE_FAILED_V1,
            "timestamp resolve queue or error-scope completion failed",
            gpu_timestamp_internal::add_drop_count_v1(
                ts_drop_count_, n / 2));
        return false;
    }

                                                                                 
                                                                                                    
    bool mapped = false;
    bool map_completed = false;
    const wgpu::WaitStatus map_wait_status = instance_.WaitAny(
        ts_readback_.MapAsync(
            wgpu::MapMode::Read, 0, n * sizeof(uint64_t),
            wgpu::CallbackMode::WaitAnyOnly,
            [&](wgpu::MapAsyncStatus status, wgpu::StringView) {
                map_completed = true;
                mapped = (status == wgpu::MapAsyncStatus::Success);
            }),
        runtime_wait_ns());
    if (map_wait_status != wgpu::WaitStatus::Success || !map_completed) {
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] GPU wait timeout/failure (st="
                  << int(map_wait_status)
                  << ") — device marked unhealthy\n";
    }
    if (!mapped) {
        publish_resolve_failure(AETHER_GPU_TIMESTAMP_REASON_MAP_FAILED_V1,
                                "timestamp readback map failed",
                                gpu_timestamp_internal::add_drop_count_v1(
                                    ts_drop_count_, n / 2));
        return false;
    }
    const uint64_t* ts = static_cast<const uint64_t*>(
        ts_readback_.GetConstMappedRange(0, n * sizeof(uint64_t)));
    if (ts == nullptr) {
        ts_readback_.Unmap();
        publish_resolve_failure(AETHER_GPU_TIMESTAMP_REASON_RANGE_FAILED_V1,
                                "timestamp mapped range is null",
                                gpu_timestamp_internal::add_drop_count_v1(
                                    ts_drop_count_, n / 2));
        return false;
    }

    bool monotonic = true;
    out_ns->reserve(n / 2);
                                                                          
    const bool diag = gpu_ts_diag_enabled();
    uint32_t diag_zero = 0;                     
    uint32_t diag_inverted = 0;                 
    uint32_t diag_both_zero = 0;                                            
    uint32_t diag_printed = 0;
                                                                             
                                                                                        
                                                                                        
                                                                                            
                                                                                        
                                                                                
                                                                                                    
    for (uint32_t i = 0; i + 1 < n; i += 2) {
        const uint64_t begin = ts[i];
        const uint64_t end = ts[i + 1];
        if (end < begin) {
            monotonic = false;
            if (diag) {
                ++diag_inverted;
                if (begin == 0 && end == 0) ++diag_both_zero;
                if (diag_printed < 24) {
                    std::cerr << "[GPU-TS-DIAG] bad pair idx=" << (i / 2)
                              << " qidx=" << i << "/" << (i + 1)
                              << " begin=" << begin << " end=" << end
                              << " delta=" << (int64_t)(end - begin) << "\n";
                    ++diag_printed;
                }
            }
        } else {
            if (diag && end == begin) ++diag_zero;                           
            out_ns->push_back(end - begin);
        }
    }
    if (diag) {
        std::cerr << "[GPU-TS-DIAG] resolve n_queries=" << n
                  << " pairs=" << (n / 2)
                  << " good=" << out_ns->size()
                  << " zero_len=" << diag_zero
                  << " inverted=" << diag_inverted
                  << " both_zero=" << diag_both_zero
                  << " ts_drop_count=" << ts_drop_count_
                  << " monotonic=" << (monotonic ? 1 : 0) << "\n";
                                                                                                  
        const uint32_t show = n < 8u ? n : 8u;
        for (uint32_t i = 0; i < show; ++i) {
            std::cerr << "[GPU-TS-DIAG] raw[" << i << "]=" << ts[i] << "\n";
        }
    }
    ts_pending_ticks_.assign(ts, ts + n);
    ts_readback_.Unmap();

    if (ts_drop_count_ != 0) {
        out_ns->clear();
        publish_resolve_failure(
            AETHER_GPU_TIMESTAMP_REASON_SLOT_OVERFLOW_V1,
            "timestamp pass slot capacity exceeded",
            gpu_timestamp_internal::add_drop_count_v1(
                ts_drop_count_, n / 2));
        return false;
    }
    if (!monotonic || out_ns->size() != n / 2) {
        out_ns->clear();
        publish_resolve_failure(
            AETHER_GPU_TIMESTAMP_REASON_NONMONOTONIC_PAIR_V1,
            "timestamp pair is zero or nonmonotonic",
            gpu_timestamp_internal::add_drop_count_v1(
                ts_drop_count_, n / 2));
        return false;
    }

                                                                          
                                                                               
                                                                       
    ts_pending_probe_instance_id_ = ts_probe_instance_id_;
    ts_pending_extraction_ordinal_ = ts_extraction_ordinal_;
    ts_pending_resolve_attempt_count_ = ts_resolve_attempt_count_;
    ts_pending_ready_ = true;
    return true;
}

bool DawnKernelHarness::gpu_ts_finalize_frame(
        const uint32_t* cumulative_pass_counts, uint32_t stage_count) {
    if (!ts_pending_ready_ ||
        ts_pending_ticks_.size() >
            AETHER_GPU_TIMESTAMP_RAW_PAIR_CAPACITY_V1 * 2u ||
        (ts_pending_ticks_.size() & 1u) != 0) {
        if (gpu_ts_diag_enabled()) {                            
            std::cerr << "[GPU-TS-DIAG] finalize early-out pending_ready="
                      << (ts_pending_ready_ ? 1 : 0)
                      << " ticks=" << ts_pending_ticks_.size() << "\n";
        }
        return false;
    }
    AetherGpuTimestampFrameV1 frame{};
    frame.struct_size = sizeof(frame);
    frame.schema_version = AETHER_GPU_TIMESTAMP_SCHEMA_VERSION_V1;
    frame.presence_flags =
        AETHER_GPU_TIMESTAMP_PRESENCE_PERIOD_V1 |
        AETHER_GPU_TIMESTAMP_PRESENCE_PROBE_REF_V1;
    frame.status = AETHER_GPU_TIMESTAMP_STATUS_ENABLED_V1;
    frame.probe_instance_id = ts_pending_probe_instance_id_;
    frame.extraction_ordinal = ts_pending_extraction_ordinal_;
    frame.raw_pair_count =
        static_cast<uint32_t>(ts_pending_ticks_.size() / 2u);
    frame.resolve_attempt_count = ts_pending_resolve_attempt_count_;
    frame.resolve_success_count = 1;
    frame.timestamp_period_ns = 1;
    for (uint32_t pair = 0; pair < frame.raw_pair_count; ++pair) {
        frame.raw_pairs[pair].begin_tick = ts_pending_ticks_[pair * 2u];
        frame.raw_pairs[pair].end_tick = ts_pending_ticks_[pair * 2u + 1u];
        frame.raw_pairs[pair].stage_index =
            std::numeric_limits<uint32_t>::max();
    }
    ts_pending_ready_ = false;
    ts_pending_ticks_.clear();
    const bool valid = gpu_timestamp_internal::finalize_frame_v1(
        &frame, ts_probe_instance_id_, ts_extraction_ordinal_,
        cumulative_pass_counts, stage_count);
    const bool published =
        gpu_timestamp_internal::publish_frame_v1(frame);
    if (gpu_ts_diag_enabled()) {                            
        std::cerr << "[GPU-TS-DIAG] finalize valid=" << (valid ? 1 : 0)
                  << " published=" << (published ? 1 : 0)
                  << " reason_code=" << frame.reason_code
                  << " reason=\"" << frame.reason << "\" bounds=";
        for (uint32_t s = 0; s < stage_count; ++s) {
            std::cerr << cumulative_pass_counts[s]
                      << (s + 1 == stage_count ? "" : ",");
        }
        std::cerr << "\n";
    }
    return valid && published;
}

void DawnKernelHarness::gpu_ts_clear_caller_thread_frame() {
    gpu_timestamp_internal::clear_caller_thread_frame_v1();
}

bool DawnKernelHarness::gpu_ts_stash_frame_for_caller_thread() {
    return gpu_timestamp_internal::stash_current_frame_for_caller_thread_v1(
        ts_probe_instance_id_, ts_extraction_ordinal_);
}

void DawnKernelHarness::begin_batch() {
    if (!device_healthy_) return;                                                  
    batch_encoder_ = device_.CreateCommandEncoder();
    batch_bind_groups_.clear();
}

void DawnKernelHarness::dispatch_batched(
        const wgpu::ComputePipeline& pipeline,
        const std::vector<wgpu::Buffer>& bindings,
        uint32_t wg_x, uint32_t wg_y, uint32_t wg_z) {
    std::vector<BufBinding> b;
    b.reserve(bindings.size());
    for (const auto& x : bindings) b.emplace_back(x);
    dispatch_batched(pipeline, b, wg_x, wg_y, wg_z);
}

void DawnKernelHarness::dispatch_batched(
        const wgpu::ComputePipeline& pipeline,
        const std::vector<BufBinding>& bindings,
        uint32_t wg_x, uint32_t wg_y, uint32_t wg_z) {
                                                                                            
    if (!device_healthy_) return;
    std::vector<wgpu::BindGroupEntry> bg_entries(bindings.size());
    for (size_t i = 0; i < bindings.size(); ++i) {
        bg_entries[i].binding = static_cast<uint32_t>(i);
        bg_entries[i].buffer = bindings[i].buffer;                      
        bg_entries[i].offset = bindings[i].offset;
        bg_entries[i].size = bindings[i].size;
    }
    wgpu::BindGroupDescriptor bg_desc{};
    bg_desc.layout = pipeline.GetBindGroupLayout(0);
    bg_desc.entryCount = static_cast<uint32_t>(bg_entries.size());
    bg_desc.entries = bg_entries.data();
    wgpu::BindGroup bind_group = device_.CreateBindGroup(&bg_desc);
                                                                  
    batch_bind_groups_.push_back(bind_group);

    wgpu::ComputePassDescriptor pd{};                     
    wgpu::PassTimestampWrites tw{};
    wgpu::ComputePassEncoder pass =
        batch_encoder_.BeginComputePass(ts_pass_desc(&pd, &tw));
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bind_group);
    pass.DispatchWorkgroups(wg_x, wg_y, wg_z);
    pass.End();
}

void DawnKernelHarness::copy_region_batched(const wgpu::Buffer& src,
                                            uint64_t src_offset,
                                            const wgpu::Buffer& dst,
                                            uint64_t dst_offset, size_t size) {
                                                                                 
    if (!device_healthy_) return;
    if (hb_enabled_) ++hb_.n_copy;
    batch_encoder_.CopyBufferToBuffer(src, src_offset, dst, dst_offset, size);
}

void DawnKernelHarness::end_batch() {
    if (!device_healthy_) return;                                                  
    const double hb_t0 = hb_now_ms(hb_enabled_);               
    wgpu::CommandBuffer commands = batch_encoder_.Finish();
    const double hb_t1 = hb_now_ms(hb_enabled_);
    if (hb_enabled_) ++hb_.n_submit;
    queue_.Submit(1, &commands);
                                                                     
                                                                                
    bool done = false;
    const wgpu::WaitStatus st = instance_.WaitAny(
        queue_.OnSubmittedWorkDone(
            wgpu::CallbackMode::WaitAnyOnly,
            [&done](wgpu::QueueWorkDoneStatus, wgpu::StringView) { done = true; }),
        runtime_wait_ns());
    if (st != wgpu::WaitStatus::Success || !done) {
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] GPU wait timeout/failure (st="
                  << int(st) << ") — device marked unhealthy\n";
        batch_encoder_ = nullptr;
        batch_bind_groups_.clear();
        return;
    }
    if (hb_enabled_) {                                         
                                                                             
                                                                          
        hb_.encode_ms += hb_t1 - hb_t0;
        hb_.submit_wait_ms += hb_now_ms(true) - hb_t1;
        ++hb_.n_dispatch;
    }
    batch_encoder_ = nullptr;
    batch_bind_groups_.clear();
}

DawnKernelHarness::AsyncBatch DawnKernelHarness::end_batch_async() {
    AsyncBatch b;
    if (!device_healthy_) return b;
    wgpu::CommandBuffer commands = batch_encoder_.Finish();
    if (hb_enabled_) ++hb_.n_submit;
    queue_.Submit(1, &commands);
    b.done = std::make_shared<bool>(false);
    auto flag = b.done;
    b.future = queue_.OnSubmittedWorkDone(wgpu::CallbackMode::WaitAnyOnly,
                                          [flag](wgpu::QueueWorkDoneStatus, wgpu::StringView) { *flag = true; });
    b.valid = true;
    b.keep = std::move(batch_bind_groups_);
    batch_encoder_ = nullptr;
    batch_bind_groups_.clear();
    return b;
}
bool DawnKernelHarness::wait_async(AsyncBatch& b) {
    if (!b.valid) return device_healthy_;
    b.valid = false;
    if (*b.done) { b.keep.clear(); return true; }
    const wgpu::WaitStatus st = instance_.WaitAny(b.future, runtime_wait_ns());
    b.keep.clear();
    if (st != wgpu::WaitStatus::Success || !*b.done) {
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] async GPU wait timeout/failure (st=" << int(st) << ") — device marked unhealthy\n";
        return false;
    }
    return true;
}
wgpu::Buffer DawnKernelHarness::alloc_indirect_args() {
    wgpu::BufferDescriptor desc{
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::Indirect |
                 wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst,
                                                                              
                                                                              
        .size = 3 * sizeof(uint32_t),
    };
    return device_.CreateBuffer(&desc);
}

void DawnKernelHarness::dispatch_indirect(
        const wgpu::ComputePipeline& pipeline,
        const std::vector<wgpu::Buffer>& bindings,
        const wgpu::Buffer& indirect_buffer,
        uint64_t indirect_offset) {
    if (!device_healthy_) return;                                                  
                                                                       
                                            
    std::vector<wgpu::BindGroupEntry> bg_entries(bindings.size());
    for (size_t i = 0; i < bindings.size(); ++i) {
        bg_entries[i].binding = static_cast<uint32_t>(i);
        bg_entries[i].buffer = bindings[i];
        bg_entries[i].offset = 0;
        bg_entries[i].size = WGPU_WHOLE_SIZE;
    }
    wgpu::BindGroupDescriptor bg_desc{};
    bg_desc.layout = pipeline.GetBindGroupLayout(0);
    bg_desc.entryCount = static_cast<uint32_t>(bg_entries.size());
    bg_desc.entries = bg_entries.data();
    wgpu::BindGroup bind_group = device_.CreateBindGroup(&bg_desc);

    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    {
        wgpu::ComputePassDescriptor pd{};                 
        wgpu::PassTimestampWrites tw{};
        wgpu::ComputePassEncoder pass =
            encoder.BeginComputePass(ts_pass_desc(&pd, &tw));
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bind_group);
        pass.DispatchWorkgroupsIndirect(indirect_buffer, indirect_offset);
        pass.End();
    }
    wgpu::CommandBuffer commands = encoder.Finish();
    if (hb_enabled_) ++hb_.n_submit;
    queue_.Submit(1, &commands);

                                                                                      
    bool done = false;
    const wgpu::WaitStatus st = instance_.WaitAny(
        queue_.OnSubmittedWorkDone(
            wgpu::CallbackMode::WaitAnyOnly,
            [&done](wgpu::QueueWorkDoneStatus, wgpu::StringView) {
                done = true;
            }),
        runtime_wait_ns());
    if (st != wgpu::WaitStatus::Success || !done) {
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] GPU wait timeout/failure (st="
                  << int(st) << ") — device marked unhealthy\n";
        return;
    }
}

void DawnKernelHarness::copy_to_staging(const wgpu::Buffer& src,
                                         const wgpu::Buffer& dst,
                                         size_t size) {
    if (!device_healthy_) return;                                                  
    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    if (hb_enabled_) ++hb_.n_copy;
    encoder.CopyBufferToBuffer(src, 0, dst, 0, size);
    wgpu::CommandBuffer commands = encoder.Finish();
    if (hb_enabled_) ++hb_.n_submit;
    queue_.Submit(1, &commands);

                                                              
                                                                                      
    bool done = false;
    const wgpu::WaitStatus st = instance_.WaitAny(
        queue_.OnSubmittedWorkDone(
            wgpu::CallbackMode::WaitAnyOnly,
            [&done](wgpu::QueueWorkDoneStatus, wgpu::StringView) {
                done = true;
            }),
        runtime_wait_ns());
    if (st != wgpu::WaitStatus::Success || !done) {
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] GPU wait timeout/failure (st="
                  << int(st) << ") — device marked unhealthy\n";
        return;
    }
}

void DawnKernelHarness::copy_region(const wgpu::Buffer& src,
                                     uint64_t src_offset,
                                     const wgpu::Buffer& dst,
                                     uint64_t dst_offset, size_t size) {
    if (!device_healthy_) return;                                                  
    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    if (hb_enabled_) ++hb_.n_copy;
    encoder.CopyBufferToBuffer(src, src_offset, dst, dst_offset, size);
    wgpu::CommandBuffer commands = encoder.Finish();
    if (hb_enabled_) ++hb_.n_submit;
    queue_.Submit(1, &commands);

                                                                                      
    bool done = false;
    const wgpu::WaitStatus st = instance_.WaitAny(
        queue_.OnSubmittedWorkDone(
            wgpu::CallbackMode::WaitAnyOnly,
            [&done](wgpu::QueueWorkDoneStatus, wgpu::StringView) {
                done = true;
            }),
        runtime_wait_ns());
    if (st != wgpu::WaitStatus::Success || !done) {
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] GPU wait timeout/failure (st="
                  << int(st) << ") — device marked unhealthy\n";
        return;
    }
}

std::vector<uint8_t> DawnKernelHarness::readback(const wgpu::Buffer& buf,
                                                   size_t size) {
    if (!device_healthy_) return {};                                                  
    const double hb_t0 = hb_now_ms(hb_enabled_);               
                                                                     
                                                                                       
                                                                               
    bool mapped = false;
    bool map_completed = false;
    const wgpu::WaitStatus st = instance_.WaitAny(
        buf.MapAsync(
            wgpu::MapMode::Read, 0, size,
            wgpu::CallbackMode::WaitAnyOnly,
            [&](wgpu::MapAsyncStatus status, wgpu::StringView msg) {
                map_completed = true;
                if (status != wgpu::MapAsyncStatus::Success) {
                    std::cerr << "[DawnKernelHarness] MapAsync failed: " << msg << '\n';
                    return;
                }
                mapped = true;
            }),
        runtime_wait_ns());
    if (st != wgpu::WaitStatus::Success || !map_completed) {
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] GPU wait timeout/failure (st="
                  << int(st) << ") — device marked unhealthy\n";
        return {};
    }
    if (!mapped) {
        return {};
    }
    const auto* p = static_cast<const uint8_t*>(buf.GetConstMappedRange(0, size));
    std::vector<uint8_t> out(p, p + size);
    buf.Unmap();
    if (hb_enabled_) {                                          
        hb_.map_ms += hb_now_ms(true) - hb_t0;
        ++hb_.n_readback;
    }
    return out;
}

                                                                                                                                                                                                                        
                                                          
                                                                                                                                                                                                                        

wgpu::Texture DawnKernelHarness::alloc_render_target(uint32_t w, uint32_t h,
                                                      wgpu::TextureFormat format) {
    wgpu::TextureDescriptor desc{};
    desc.size.width = w;
    desc.size.height = h;
    desc.size.depthOrArrayLayers = 1;
    desc.format = format;
    desc.mipLevelCount = 1;
    desc.sampleCount = 1;
    desc.dimension = wgpu::TextureDimension::e2D;
    desc.usage = wgpu::TextureUsage::RenderAttachment
               | wgpu::TextureUsage::CopySrc
               | wgpu::TextureUsage::TextureBinding;
    return device_.CreateTexture(&desc);
}

wgpu::RenderPipeline DawnKernelHarness::load_render_pipeline(
        std::string_view wgsl_source,
        const char* vs_entry,
        const char* fs_entry,
        wgpu::TextureFormat color_format,
        wgpu::PrimitiveTopology topology) {
    wgpu::ShaderSourceWGSL wgsl_desc{};
    wgsl_desc.code = wgpu::StringView{ wgsl_source.data(), wgsl_source.size() };
    wgpu::ShaderModuleCompilationOptions compile_opts{};
    compile_opts.strictMath = true;
    if (strict_math()) wgsl_desc.nextInChain = &compile_opts;                                                 
    wgpu::ShaderModuleDescriptor shader_desc{};
    shader_desc.nextInChain = &wgsl_desc;
    wgpu::ShaderModule shader = device_.CreateShaderModule(&shader_desc);

                                                                        
                                                                       
    wgpu::BlendState blend{};
    blend.color.srcFactor = wgpu::BlendFactor::One;
    blend.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    blend.color.operation = wgpu::BlendOperation::Add;
    blend.alpha.srcFactor = wgpu::BlendFactor::One;
    blend.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    blend.alpha.operation = wgpu::BlendOperation::Add;

    wgpu::ColorTargetState color_target{};
    color_target.format = color_format;
    color_target.blend = &blend;
    color_target.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState fragment{};
    fragment.module = shader;
    fragment.entryPoint = wgpu::StringView{ fs_entry, WGPU_STRLEN };
    fragment.targetCount = 1;
    fragment.targets = &color_target;

    wgpu::VertexState vertex{};
    vertex.module = shader;
    vertex.entryPoint = wgpu::StringView{ vs_entry, WGPU_STRLEN };
                                                                           
                                                                       
    vertex.bufferCount = 0;

    wgpu::RenderPipelineDescriptor pipeline_desc{};
    pipeline_desc.vertex = vertex;
    pipeline_desc.fragment = &fragment;
    pipeline_desc.primitive.topology = topology;
    pipeline_desc.primitive.cullMode = wgpu::CullMode::None;
    pipeline_desc.primitive.frontFace = wgpu::FrontFace::CCW;
    pipeline_desc.multisample.count = 1;
    pipeline_desc.multisample.mask = 0xFFFFFFFF;

    return device_.CreateRenderPipeline(&pipeline_desc);
}

void DawnKernelHarness::dispatch_render_pass(
        const wgpu::RenderPipeline& pipeline,
        const wgpu::Texture& target,
        const std::vector<wgpu::Buffer>& bindings,
        uint32_t vertex_count,
        uint32_t instance_count) {
    if (!device_healthy_) return;                                                  
                                                                            
                                                      
    std::vector<wgpu::BindGroupEntry> bg_entries(bindings.size());
    for (size_t i = 0; i < bindings.size(); ++i) {
        bg_entries[i].binding = static_cast<uint32_t>(i);
        bg_entries[i].buffer = bindings[i];
        bg_entries[i].offset = 0;
        bg_entries[i].size = WGPU_WHOLE_SIZE;
    }
    wgpu::BindGroupDescriptor bg_desc{};
    bg_desc.layout = pipeline.GetBindGroupLayout(0);
    bg_desc.entryCount = static_cast<uint32_t>(bg_entries.size());
    bg_desc.entries = bg_entries.data();
    wgpu::BindGroup bind_group = device_.CreateBindGroup(&bg_desc);

                                                                  
    wgpu::TextureView view = target.CreateView();
    wgpu::RenderPassColorAttachment color_attach{};
    color_attach.view = view;
    color_attach.loadOp = wgpu::LoadOp::Clear;
    color_attach.storeOp = wgpu::StoreOp::Store;
    color_attach.clearValue = {0.0, 0.0, 0.0, 0.0};

    wgpu::RenderPassDescriptor pass_desc{};
    pass_desc.colorAttachmentCount = 1;
    pass_desc.colorAttachments = &color_attach;

    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&pass_desc);
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bind_group);
        pass.Draw(vertex_count, instance_count,                 0,                   0);
        pass.End();
    }
    wgpu::CommandBuffer commands = encoder.Finish();
    if (hb_enabled_) ++hb_.n_submit;
    queue_.Submit(1, &commands);

                                                                                      
    bool done = false;
    const wgpu::WaitStatus st = instance_.WaitAny(
        queue_.OnSubmittedWorkDone(
            wgpu::CallbackMode::WaitAnyOnly,
            [&done](wgpu::QueueWorkDoneStatus, wgpu::StringView) {
                done = true;
            }),
        runtime_wait_ns());
    if (st != wgpu::WaitStatus::Success || !done) {
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] GPU wait timeout/failure (st="
                  << int(st) << ") — device marked unhealthy\n";
        return;
    }
}

std::vector<uint8_t> DawnKernelHarness::readback_texture(
        const wgpu::Texture& tex,
        uint32_t w, uint32_t h,
        uint32_t bytes_per_pixel) {
    if (!device_healthy_) return {};                                                  
                                                                      
                                                                           
    constexpr uint32_t kAlign = 256;
    const uint32_t unpadded_bpr = w * bytes_per_pixel;
    const uint32_t padded_bpr =
        (unpadded_bpr + kAlign - 1) / kAlign * kAlign;
    const uint64_t padded_total = static_cast<uint64_t>(padded_bpr) * h;

    auto staging = alloc_staging_for_readback(padded_total);

    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    wgpu::TexelCopyTextureInfo src_info{};
    src_info.texture = tex;
    src_info.mipLevel = 0;
    src_info.origin = {0, 0, 0};
    src_info.aspect = wgpu::TextureAspect::All;

    wgpu::TexelCopyBufferInfo dst_info{};
    dst_info.buffer = staging;
    dst_info.layout.offset = 0;
    dst_info.layout.bytesPerRow = padded_bpr;
    dst_info.layout.rowsPerImage = h;

    wgpu::Extent3D extent{ w, h, 1 };
    encoder.CopyTextureToBuffer(&src_info, &dst_info, &extent);
    wgpu::CommandBuffer commands = encoder.Finish();
    if (hb_enabled_) ++hb_.n_submit;
    queue_.Submit(1, &commands);

                                                                                      
    bool done = false;
    const wgpu::WaitStatus st = instance_.WaitAny(
        queue_.OnSubmittedWorkDone(
            wgpu::CallbackMode::WaitAnyOnly,
            [&done](wgpu::QueueWorkDoneStatus, wgpu::StringView) { done = true; }),
        runtime_wait_ns());
    if (st != wgpu::WaitStatus::Success || !done) {
                                                                          
                                                                         
                                                                          
                                                                   
                                                                    
                                                                                                     
        device_healthy_ = false;
        std::cerr << "[DawnKernelHarness] GPU wait timeout/failure (st="
                  << int(st) << ") — device marked unhealthy\n";
        return {};
    }

    auto padded_bytes = readback(staging, padded_total);

                                                                     
    std::vector<uint8_t> tight(static_cast<size_t>(unpadded_bpr) * h);
    for (uint32_t y = 0; y < h; ++y) {
        std::memcpy(tight.data() + y * unpadded_bpr,
                    padded_bytes.data() + y * padded_bpr,
                    unpadded_bpr);
    }
    return tight;
}

}                    
}                     

extern "C" int aether_sed_gpu_timestamp_probe_v1(
        AetherGpuTimestampProbeV1* out_probe) {
    return aether::tools::gpu_timestamp_internal::pull_probe_v1(out_probe);
}

extern "C" int aether_sed_last_gpu_timestamp_frame_v1(
        AetherGpuTimestampFrameV1* out_frame) {
    return aether::tools::gpu_timestamp_internal::pull_frame_v1(out_frame);
}

extern "C" int aether_dsp_sift_take_last_gpu_timestamp_frame_v1(
        AetherGpuTimestampFrameV1* out_frame) {
    return aether::tools::gpu_timestamp_internal::
        take_caller_thread_frame_v1(out_frame);
}
