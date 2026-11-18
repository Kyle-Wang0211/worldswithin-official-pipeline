                                                                                                           
                                                                                                            
                                                                                                                     
                                                                                                        
  
                                                                                                  
                                                                                                            
                                                                                                            
                                                                                                                
                                                                    
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include "dense_fuse.h"
#if defined(PWDENSE_ENABLE_FUSED_DEPTH_RETENTION)
#include "dense_fused_depth_io.h"
#endif

namespace aether::dense {

typedef int (*dense_progress_fn)(const char* phase, int done, int total, void* user);                        

struct FusePackStats {
    int frames = 0;
    size_t points = 0;
    double photo_frac = 0, geo_frac = 0, final_frac = 0;
    uint64_t digest = 1469598103934665603ULL;                                                                   
    std::vector<uint64_t> frame_digest;                                                   
};

uint64_t fnv1a64(const void* data, size_t n, uint64_t h = 1469598103934665603ULL);

                                                                                                              
                                                                                                               
                                                                                     
struct BoxFilter {
    double c[3], s[3], rot[9];
    bool contains(double wx, double wy, double wz) const {
        const double px = wx - c[0], py = wy - c[1], pz = wz - c[2];
        const double lx = rot[0] * px + rot[3] * py + rot[6] * pz;
        const double ly = rot[1] * px + rot[4] * py + rot[7] * pz;
        const double lz = rot[2] * px + rot[5] * py + rot[8] * pz;
        return std::fabs(lx) <= s[0] / 2 && std::fabs(ly) <= s[1] / 2 && std::fabs(lz) <= s[2] / 2;
    }
};

                                                                                                             
                                                                      
struct FuseMap {
    const void* p = nullptr; size_t n = 0;
    FuseMap() = default;
    FuseMap(const FuseMap&) = delete;
    FuseMap& operator=(const FuseMap&) = delete;
    ~FuseMap() { close(); }
    bool open(const std::string& path, size_t expect);                                                                
    void close();
};

                                                                                                    
                                                                                     
                                                                                                            
                                                                                                            
                                                                                                      
                                              
struct FusePack {
    int NF = 0, W = 0, H = 0, NS = 0;
    size_t N = 0;                             
    FuseParams prm;
    const float* depth = nullptr;
    const float* conf[3] = {nullptr, nullptr, nullptr};
    const uint8_t* rgb = nullptr;
    const int32_t* nb = nullptr;
    std::vector<FuseCamera> cam;
    bool open(const std::string& pack_dir);
private:
    FuseMap md_, mc_[3], mr_, mk_, mn_;
};

                                                                                                            
                                                                                
                                                                                                           
                                                                                                 
struct FuseFrameResult {
    int frame = -1;
    std::vector<float> xyz;
    std::vector<uint8_t> rgb;
    uint64_t hs[5] = {0, 0, 0, 0, 0};                                          
    uint64_t digest = 0;                                                                                    
    double photo_frac = 0, geo_frac = 0, final_frac = 0;
    size_t points() const { return xyz.size() / 3; }
};

                                                                                                            
                                                                                                             
                                   
bool fuse_pack_frame(const FusePack& pk, int f, const BoxFilter* box, FuseFrameResult& out
#if defined(PWDENSE_ENABLE_FUSED_DEPTH_RETENTION)
                     , const FusedDepthRetention* retention = nullptr
#endif
                     );

                                                                                                             
                                                                                                             
                                                                                                             
                                                                                                 
                                                                                                             
class FusePlyWriter {
public:
    FusePlyWriter() = default;
    FusePlyWriter(const FusePlyWriter&) = delete;
    FusePlyWriter& operator=(const FusePlyWriter&) = delete;
    ~FusePlyWriter() { abort(); }
    bool begin(const std::string& path);                                                                       
    void append(const float* xyz, const uint8_t* rgb, size_t n);
    bool finish();                                                                
    void abort();                                                                                        
    size_t points() const { return total_; }
private:
    FILE* f_ = nullptr;
    std::string path_;
    size_t total_ = 0;
};

                                                                                                             
                                                                                                              
                                                                                                            
                                                                                                              
                                                       
class FuseScheduler {
public:
    FuseScheduler(const int32_t* neighbors, int NF, int NS, int ref_begin, int ref_end, bool all_views = false);
    void mark_inferred(int view);
    bool inferred(int view) const;
    void take_ready(std::vector<int>& out);                                                               
    int remaining() const;                                                                                
private:
    const int32_t* nb_; int NF_, NS_, r0_, r1_;
    bool all_views_ = false;                                                                          
    std::vector<uint8_t> inferred_, handed_;
};

                                                                                                             
                                                                                                        
                                                                            
                                                    
bool fuse_spill_write(const std::string& path, const std::vector<float>& xyz, const std::vector<uint8_t>& rgb);
bool fuse_spill_append(const std::string& path, FusePlyWriter& w);                                          

                                                                                                             
                                                                                                                
                             
                                                                                                
int fuse_pack(const std::string& pack_dir, int f0, int f1, const std::string& out_ply,
              dense_progress_fn progress, void* user, FusePackStats* stats, const BoxFilter* box = nullptr
#if defined(PWDENSE_ENABLE_FUSED_DEPTH_RETENTION)
              , const FusedDepthRetention* retention = nullptr
#endif
              );

}                            
