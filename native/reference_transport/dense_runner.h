                                                                                               
  
                                                                                                              
                                                                                                       
                                                                                                        
                                                                                                         
                                        
  
                                                                                                                      
                                                                                                               
                                                                                                              
                                                                 
                                                                                             
                                                                                                               
                                                                                                            
                                            
                                                                                                                  
                                                                                                          
                                                                                                                
                                                                                                            
                                                                                                             
                                                                                                               
                                                                                                        
                                                                                                            
                                                                                                            
// none), so the only thing that licenses this is the end-to-end byte gate: pack + PLY sha vs the fused arm,
                                            
#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "dense_inputs.h"
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
#include "dense_view_segments.h"
#endif

namespace Ort { struct Env; struct Session; }

namespace aether::dense {

                                                                                                                
                                                                                                     
                                                                                                                 
                                                                                                        
                                                                                            
struct FeatureSet {
    std::vector<float> f[3];
    bool empty() const { return f[0].empty() || f[1].empty() || f[2].empty(); }
    size_t bytes() const { return (f[0].size() + f[1].size() + f[2].size()) * sizeof(float); }
};

class DenseRunner {
public:
    DenseRunner();
    ~DenseRunner();
                                                                                                    
    bool init(const std::string& model, bool webgpu, int W, int H, int n_view, std::string* err, double* session_ms);
                                                                                                            
                                                                
    bool run(const float* imgs, const FrameInputs& in, const float* noise2, const float* noise3,
             float* depth, float* conf0, float* conf1, float* conf2, double* ms, std::string* err);
#if defined(PWDENSE_ENABLE_MODEL_SNAPSHOT)
                                                                                         
                                                                                            
                                                                                          
                                                                                      
                                                                                    
                                                                                         
                                                                                        
                                                                                      
    bool snapshot_optimized_model(const std::string& model, const std::string& destination,
                                  bool webgpu, std::string* err, double* session_ms);
#endif
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
                                                                                
                                                                                
    bool init_view_segments(const std::vector<DenseViewSegmentPlan>& plan,
                            DenseViewSegmentOptions options, int W, int H, int n_view,
                            std::string* err, double* session_ms);
#endif

                                           
                                                                                                                  
                                                                                                               
                                                                                       
    bool init_split(const std::string& feat_model, const std::string& rest_model, bool webgpu, int W, int H,
                    int n_view, std::string* err, double* session_ms);
                                                                                                             
                                                                       
    bool run_features(const float* img, FeatureSet& out, double* ms, std::string* err);
                                                                                                                
                                                                                                                
    bool run_rest(const FeatureSet* const* views, const float* ref_img, const FrameInputs& in,
                  const float* noise2, const float* noise3,
                  float* depth, float* conf0, float* conf1, float* conf2, double* ms, std::string* err);
                                                                                                            
    void feature_sizes(size_t n[3]) const;

    void release();
    bool ready() const { return sess_ != nullptr; }
    bool split() const { return split_; }
private:
    struct Impl; std::unique_ptr<Impl> impl_;
    void* sess_ = nullptr;
    int W_ = 0, H_ = 0, nview_ = 0;
    bool split_ = false;
};

                                                                                                                
                                                                                                           
                                                                                                  
void make_noise(uint64_t seed, int frame_id, size_t n2, size_t n3, std::vector<float>& noise2, std::vector<float>& noise3);

}                            
