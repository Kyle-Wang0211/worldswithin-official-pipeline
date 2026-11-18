                                                                            
                                                                           
                                  
                                                                            
                                                                            
                                                                           
                                                                                  
                                                                  
                                                                       
                                                                               
                                                                                
                                                                             
                                                                           
                                                                              
                                                                         
  
                                                                         
                                                                      
                                              
  
                                                                        
                                                                           
                                                                           
                                  

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {

                                                                                                                                                                
int pwdawn_gpu_match_gemm_pairs(const uint8_t* dA, int nA, const uint8_t* dB,
                                int nB, double max_ratio, uint32_t* out_pairs,
                                int max_pairs, int* out_num_matches);
int pwdawn_gpu_match_gemm_pairs_resident(
    uint64_t session_nonce, uint32_t frame_a, uint32_t generation_a,
    const uint8_t* dA, int nA, uint32_t frame_b, uint32_t generation_b,
    const uint8_t* dB, int nB, double max_ratio, uint32_t* out_pairs,
    int max_pairs, int* out_num_matches);
void pwdawn_gpu_match_descriptor_residency_invalidate(uint64_t session_nonce,
                                                      uint32_t frame_ordinal);
void pwdawn_gpu_match_descriptor_residency_clear_session(uint64_t session_nonce);
int pwdawn_gpu_match_descriptor_residency_stats(
    uint64_t session_nonce, uint64_t* hits, uint64_t* misses,
    uint64_t* evictions, uint64_t* stale_replacements, uint64_t* upload_bytes,
    uint64_t* resident_bytes, uint64_t* resident_entries,
    uint64_t* allocation_failures, uint64_t* device_resets);
int pwdawn_gpu_match_probe_batch(const uint8_t* dA, int nA,
                                 const uint8_t* const* dBs, const int* nBs,
                                 int n_cands, double max_ratio, int* out_counts);
int pwdawn_gpu_match_gemm_pairs_guided(
    const uint8_t* dA, int nA, const float* xyA, const uint8_t* dB, int nB,
    const float* xyB, double max_ratio, const float* matrixAB,
    const float* matrixBA, int guide_mode, float max_residual,
    uint32_t* out_pairs, int max_pairs, int* out_num_matches);
int pwdawn_gpu_match_last_error(char* buf, int cap);
void pwdawn_match_set_ab_phase(int phase);
void pwdawn_gpu_match_set_capture_active(int active);
void pwdawn_gpu_match_set_preview_fps30(int on);
void pwdawn_gpu_match_set_thermal_state(int state);
int pwdawn_gpu_match_backend_info(char* buf, int cap);

#if !defined(PWOFFICIAL_MATCH_NO_METAL)
                                                                                                      
int pwmetal_gpu_match_gemm_pairs(const uint8_t* dA, int nA, const uint8_t* dB,
                                 int nB, double max_ratio, uint32_t* out_pairs,
                                 int max_pairs, int* out_num_matches);
int pwmetal_gpu_match_gemm_pairs_resident(
    uint64_t session_nonce, uint32_t frame_a, uint32_t generation_a,
    const uint8_t* dA, int nA, uint32_t frame_b, uint32_t generation_b,
    const uint8_t* dB, int nB, double max_ratio, uint32_t* out_pairs,
    int max_pairs, int* out_num_matches);
void pwmetal_gpu_match_descriptor_residency_invalidate(uint64_t session_nonce,
                                                       uint32_t frame_ordinal);
void pwmetal_gpu_match_descriptor_residency_clear_session(uint64_t session_nonce);
int pwmetal_gpu_match_descriptor_residency_stats(
    uint64_t session_nonce, uint64_t* hits, uint64_t* misses,
    uint64_t* evictions, uint64_t* stale_replacements, uint64_t* upload_bytes,
    uint64_t* resident_bytes, uint64_t* resident_entries,
    uint64_t* allocation_failures, uint64_t* device_resets);
int pwmetal_gpu_match_probe_batch(const uint8_t* dA, int nA,
                                  const uint8_t* const* dBs, const int* nBs,
                                  int n_cands, double max_ratio, int* out_counts);
int pwmetal_gpu_match_gemm_pairs_guided(
    const uint8_t* dA, int nA, const float* xyA, const uint8_t* dB, int nB,
    const float* xyB, double max_ratio, const float* matrixAB,
    const float* matrixBA, int guide_mode, float max_residual,
    uint32_t* out_pairs, int max_pairs, int* out_num_matches);
int pwmetal_gpu_match_last_error(char* buf, int cap);
void pwmetal_match_set_ab_phase(int phase);
void pwmetal_gpu_match_set_capture_active(int active);
void pwmetal_gpu_match_set_preview_fps30(int on);
#endif

}               

namespace {

bool UseDawn() {
#if defined(PWOFFICIAL_MATCH_NO_METAL)
  return true;
#else
                                                                          
                                                                            
                                                                           
                                                                              
                                                                            
  static const bool v = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_MATCH_BACKEND");
    const bool dawn = !(e != nullptr && std::strcmp(e, "metal") == 0);
                                                                              
                                                                              
    if (const char* home = std::getenv("HOME")) {
      std::string path = std::string(home) + "/Documents/matcher_backend.jsonl";
      if (FILE* f = std::fopen(path.c_str(), "a")) {
        std::fprintf(f, "{\"backend\":\"%s\",\"env\":\"%s\"}\n",
                     dawn ? "dawn" : "metal", e ? e : "");
        std::fclose(f);
      }
    }
    return dawn;
  }();
  return v;
#endif
}

}              

                                                                          
                                                                              
extern "C" const char* aether_gpu_match_backend_name(void) {
  return UseDawn() ? "dawn" : "metal";
}

extern "C" int aether_gpu_match_gemm_pairs(const uint8_t* dA, int nA,
                                           const uint8_t* dB, int nB,
                                           double max_ratio,
                                           uint32_t* out_pairs, int max_pairs,
                                           int* out_num_matches) {
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  if (!UseDawn()) {
    return pwmetal_gpu_match_gemm_pairs(dA, nA, dB, nB, max_ratio, out_pairs,
                                        max_pairs, out_num_matches);
  }
#endif
  return pwdawn_gpu_match_gemm_pairs(dA, nA, dB, nB, max_ratio, out_pairs,
                                     max_pairs, out_num_matches);
}

extern "C" int aether_gpu_match_gemm_pairs_resident(
    uint64_t session_nonce, uint32_t frame_a, uint32_t generation_a,
    const uint8_t* dA, int nA, uint32_t frame_b, uint32_t generation_b,
    const uint8_t* dB, int nB, double max_ratio, uint32_t* out_pairs,
    int max_pairs, int* out_num_matches) {
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  if (!UseDawn()) {
    return pwmetal_gpu_match_gemm_pairs_resident(
        session_nonce, frame_a, generation_a, dA, nA, frame_b, generation_b,
        dB, nB, max_ratio, out_pairs, max_pairs, out_num_matches);
  }
#endif
  return pwdawn_gpu_match_gemm_pairs_resident(
      session_nonce, frame_a, generation_a, dA, nA, frame_b, generation_b, dB,
      nB, max_ratio, out_pairs, max_pairs, out_num_matches);
}

extern "C" void aether_gpu_match_descriptor_residency_invalidate(
    uint64_t session_nonce, uint32_t frame_ordinal) {
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  if (!UseDawn()) {
    pwmetal_gpu_match_descriptor_residency_invalidate(session_nonce,
                                                      frame_ordinal);
    return;
  }
#endif
  pwdawn_gpu_match_descriptor_residency_invalidate(session_nonce, frame_ordinal);
}

extern "C" void aether_gpu_match_descriptor_residency_clear_session(
    uint64_t session_nonce) {
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  if (!UseDawn()) {
    pwmetal_gpu_match_descriptor_residency_clear_session(session_nonce);
    return;
  }
#endif
  pwdawn_gpu_match_descriptor_residency_clear_session(session_nonce);
}

extern "C" int aether_gpu_match_descriptor_residency_stats(
    uint64_t session_nonce, uint64_t* hits, uint64_t* misses,
    uint64_t* evictions, uint64_t* stale_replacements, uint64_t* upload_bytes,
    uint64_t* resident_bytes, uint64_t* resident_entries,
    uint64_t* allocation_failures, uint64_t* device_resets) {
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  if (!UseDawn()) {
    return pwmetal_gpu_match_descriptor_residency_stats(
        session_nonce, hits, misses, evictions, stale_replacements,
        upload_bytes, resident_bytes, resident_entries, allocation_failures,
        device_resets);
  }
#endif
  return pwdawn_gpu_match_descriptor_residency_stats(
      session_nonce, hits, misses, evictions, stale_replacements, upload_bytes,
      resident_bytes, resident_entries, allocation_failures, device_resets);
}

extern "C" int aether_gpu_match_probe_batch(const uint8_t* dA, int nA,
                                            const uint8_t* const* dBs,
                                            const int* nBs, int n_cands,
                                            double max_ratio,
                                            int* out_counts) {
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  if (!UseDawn()) {
    return pwmetal_gpu_match_probe_batch(dA, nA, dBs, nBs, n_cands, max_ratio,
                                         out_counts);
  }
#endif
  return pwdawn_gpu_match_probe_batch(dA, nA, dBs, nBs, n_cands, max_ratio,
                                      out_counts);
}

extern "C" int aether_gpu_match_gemm_pairs_guided(
    const uint8_t* dA, int nA, const float* xyA, const uint8_t* dB, int nB,
    const float* xyB, double max_ratio, const float* matrixAB,
    const float* matrixBA, int guide_mode, float max_residual,
    uint32_t* out_pairs, int max_pairs, int* out_num_matches) {
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  if (!UseDawn()) {
    return pwmetal_gpu_match_gemm_pairs_guided(
        dA, nA, xyA, dB, nB, xyB, max_ratio, matrixAB, matrixBA, guide_mode,
        max_residual, out_pairs, max_pairs, out_num_matches);
  }
#endif
  return pwdawn_gpu_match_gemm_pairs_guided(
      dA, nA, xyA, dB, nB, xyB, max_ratio, matrixAB, matrixBA, guide_mode,
      max_residual, out_pairs, max_pairs, out_num_matches);
}

extern "C" int aether_gpu_match_last_error(char* buf, int cap) {
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  if (!UseDawn()) return pwmetal_gpu_match_last_error(buf, cap);
#endif
  return pwdawn_gpu_match_last_error(buf, cap);
}

                                                                       
                                              
extern "C" void aether_match_set_ab_phase(int phase) {
  pwdawn_match_set_ab_phase(phase);
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  pwmetal_match_set_ab_phase(phase);
#endif
}

extern "C" void aether_gpu_match_set_capture_active(int active) {
  pwdawn_gpu_match_set_capture_active(active);
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  pwmetal_gpu_match_set_capture_active(active);
#endif
}

extern "C" void aether_gpu_match_set_preview_fps30(int on) {
  pwdawn_gpu_match_set_preview_fps30(on);
#if !defined(PWOFFICIAL_MATCH_NO_METAL)
  pwmetal_gpu_match_set_preview_fps30(on);
#endif
}

                                                                             
                                                                             
                                                                       
extern "C" void aether_gpu_match_set_thermal_state(int state) {
  pwdawn_gpu_match_set_thermal_state(state);
}

extern "C" int aether_gpu_match_backend_info(char* buf, int cap) {
  if (!buf || cap <= 0) return 0;
  if (!UseDawn()) {
    const int n = std::snprintf(buf, (size_t)cap, "backend=metal(pwofficial_gpu_match.mm)");
    return n < 0 ? 0 : (n < cap ? n : cap - 1);
  }
  return pwdawn_gpu_match_backend_info(buf, cap);
}
