                                                                     
                                                                                
                                                                            
                                                                              

#include "colmap/feature/extractor.h"
#include "colmap/feature/matcher.h"
#include "colmap/feature/sift.h"
#include "colmap/feature/types.h"
#include "colmap/sensor/bitmap.h"

#include "aether_bitmap_shim.h"
#include "aether_threaded_extract.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

extern "C" {

                                                                                 
                                                                         
                                                                         
  
                                                                       
                                                                                
                                                                              
                                    
                                                                                 
                                                                        
                                                                               
                                                                             
                                                   
int aether_dsp_sift_extract_v2(const uint8_t* gray,
                               int width,
                               int height,
                               int max_features,
                               float* out_xy,
                               uint8_t* out_desc,
                               float* out_scales,
                               float* out_orientations,
                               int out_cap,
                               int* out_count) {
  if (out_count) *out_count = 0;
  try {
    if (gray == nullptr || width <= 0 || height <= 0 || out_cap <= 0) return 1;

                                                                              
                                                                                
                                                    
    colmap::Bitmap bitmap(width, height,            false);
    bitmap.RowMajorData().assign(gray,
                                 gray + static_cast<size_t>(width) * height);

    colmap::FeatureExtractionOptions opts(colmap::FeatureExtractorType::SIFT);
    opts.sift = std::make_shared<colmap::SiftExtractionOptions>();
    opts.sift->max_num_features = max_features > 0 ? max_features : 8192;
    opts.sift->estimate_affine_shape = true;                               
    opts.sift->domain_size_pooling = true;
                                                                                  
                                                                              
                                                                     
                                                                 
    opts.sift->peak_threshold = 0.004;
    opts.sift->edge_threshold = 15.0;
                                                                                   

    std::unique_ptr<colmap::FeatureExtractor> extractor =
        colmap::CreateSiftFeatureExtractor(opts);
    if (!extractor) return 3;

    colmap::FeatureKeypoints kps;
    colmap::FeatureDescriptors desc;
    if (!extractor->Extract(bitmap, &kps, &desc)) return 4;

    int n = static_cast<int>(kps.size());
    if (n > out_cap) n = out_cap;
    if (out_xy != nullptr) {
      for (int i = 0; i < n; ++i) {
        out_xy[2 * i] = kps[i].x;
        out_xy[2 * i + 1] = kps[i].y;
      }
    }
    if (out_scales != nullptr) {
      for (int i = 0; i < n; ++i) out_scales[i] = kps[i].ComputeScale();
    }
    if (out_orientations != nullptr) {
      for (int i = 0; i < n; ++i)
        out_orientations[i] = kps[i].ComputeOrientation();
    }
    if (out_desc != nullptr && desc.data.cols() == 128) {
      const int dn = static_cast<int>(desc.data.rows());
      const int m = n < dn ? n : dn;
      for (int i = 0; i < m; ++i) {
        for (int d = 0; d < 128; ++d) {
          out_desc[i * 128 + d] = desc.data(i, d);
        }
      }
    }
    if (out_count) *out_count = n;
    return 0;
  } catch (...) {
    return 5;
  }
}

                                                                             
                                                         
int aether_dsp_sift_extract(const uint8_t* gray,
                            int width,
                            int height,
                            int max_features,
                            float* out_xy,
                            uint8_t* out_desc,
                            int out_cap,
                            int* out_count) {
  return aether_dsp_sift_extract_v2(gray, width, height, max_features, out_xy,
                                    out_desc,                nullptr,
                                                         nullptr, out_cap,
                                    out_count);
}

                                                                               
                                                                            
                                                                       
                                                                       
                                                
int aether_dsp_sift_extract_threaded_v2(const uint8_t* gray,
                                        int width,
                                        int height,
                                        int max_features,
                                        int num_threads,
                                        float* out_xy,
                                        uint8_t* out_desc,
                                        float* out_scales,
                                        float* out_orientations,
                                        int out_cap,
                                        int* out_count) {
  if (out_count) *out_count = 0;
  try {
    if (gray == nullptr || width <= 0 || height <= 0 || out_cap <= 0) return 1;

                                                                                   
    colmap::Bitmap bitmap(width, height,            false);
    bitmap.RowMajorData().assign(gray,
                                 gray + static_cast<size_t>(width) * height);

                                                                           
                                                                            
    colmap::SiftExtractionOptions sift;
    sift.max_num_features = max_features > 0 ? max_features : 8192;
    sift.estimate_affine_shape = true;                               
    sift.domain_size_pooling = true;
                                                                              
    sift.peak_threshold = 0.004;
    sift.edge_threshold = 15.0;

    colmap::FeatureKeypoints kps;
    colmap::FeatureDescriptors desc;
    if (!aether::ExtractCovariantSiftThreaded(
            sift, bitmap, &kps, &desc, num_threads)) {
      return 4;
    }

    int n = static_cast<int>(kps.size());
    if (n > out_cap) n = out_cap;
    if (out_xy != nullptr) {
      for (int i = 0; i < n; ++i) {
        out_xy[2 * i] = kps[i].x;
        out_xy[2 * i + 1] = kps[i].y;
      }
    }
    if (out_scales != nullptr) {
      for (int i = 0; i < n; ++i) out_scales[i] = kps[i].ComputeScale();
    }
    if (out_orientations != nullptr) {
      for (int i = 0; i < n; ++i)
        out_orientations[i] = kps[i].ComputeOrientation();
    }
    if (out_desc != nullptr && desc.data.cols() == 128) {
      const int dn = static_cast<int>(desc.data.rows());
      const int m = n < dn ? n : dn;
      for (int i = 0; i < m; ++i) {
        for (int d = 0; d < 128; ++d) {
          out_desc[i * 128 + d] = desc.data(i, d);
        }
      }
    }
    if (out_count) *out_count = n;
    return 0;
  } catch (...) {
    return 5;
  }
}

                                                            
                                                                      
int aether_dsp_sift_extract_threaded(const uint8_t* gray,
                                     int width,
                                     int height,
                                     int max_features,
                                     int num_threads,
                                     float* out_xy,
                                     uint8_t* out_desc,
                                     int out_cap,
                                     int* out_count) {
  return aether_dsp_sift_extract_threaded_v2(
      gray, width, height, max_features, num_threads, out_xy, out_desc,
                     nullptr,                      nullptr, out_cap, out_count);
}

                                                                              
                                                                             
                                                                                 
                                                                                 
                                                                                 
int aether_dsp_sift_extract_threaded_fo(const uint8_t* gray, int width,
                                        int height, int max_features,
                                        int num_threads, int first_octave,
                                        float* out_xy, uint8_t* out_desc,
                                        int out_cap, int* out_count) {
  if (out_count) *out_count = 0;
  try {
    if (gray == nullptr || width <= 0 || height <= 0 || out_cap <= 0) return 1;
    colmap::Bitmap bitmap(width, height,            false);
    bitmap.RowMajorData().assign(gray,
                                 gray + static_cast<size_t>(width) * height);
    colmap::SiftExtractionOptions sift;
    sift.max_num_features = max_features > 0 ? max_features : 8192;
    sift.first_octave = first_octave;
    sift.estimate_affine_shape = true;
    sift.domain_size_pooling = true;
    colmap::FeatureKeypoints kps;
    colmap::FeatureDescriptors desc;
    if (!aether::ExtractCovariantSiftThreaded(sift, bitmap, &kps, &desc,
                                              num_threads)) {
      return 4;
    }
    int n = static_cast<int>(kps.size());
    if (n > out_cap) n = out_cap;
    if (out_xy != nullptr)
      for (int i = 0; i < n; ++i) {
        out_xy[2 * i] = kps[i].x;
        out_xy[2 * i + 1] = kps[i].y;
      }
    if (out_desc != nullptr && desc.data.cols() == 128) {
      const int dn = static_cast<int>(desc.data.rows());
      const int m = n < dn ? n : dn;
      for (int i = 0; i < m; ++i)
        for (int d = 0; d < 128; ++d) out_desc[i * 128 + d] = desc.data(i, d);
    }
    if (out_count) *out_count = n;
    return 0;
  } catch (...) {
    return 5;
  }
}

                                                                              
                                                                             
                                                                           
                                                          
int aether_dsp_sift_selfcheck(const uint8_t* gray,
                              int width,
                              int height,
                              int max_features,
                              int num_threads,
                              int* out_n_serial,
                              int* out_n_threaded,
                              int* out_max_abs_desc_diff) {
  if (out_n_serial) *out_n_serial = 0;
  if (out_n_threaded) *out_n_threaded = 0;
  if (out_max_abs_desc_diff) *out_max_abs_desc_diff = -1;
  try {
    const int cap = (max_features > 0 ? max_features : 8192) + 8192;
                                                                           
                                                                         
    auto extract = [&](int threads, std::vector<uint8_t>& d) -> int {
      std::vector<float> xy(static_cast<size_t>(2) * cap);
      int n = 0;
      const int r =
          (threads < 0)
              ? aether_dsp_sift_extract(
                    gray, width, height, max_features, xy.data(), d.data(), cap, &n)
              : aether_dsp_sift_extract_threaded(gray, width, height, max_features,
                                                 threads, xy.data(), d.data(), cap, &n);
      return r == 0 ? n : -r;
    };
    auto maxdiff = [](const std::vector<uint8_t>& a, int na,
                      const std::vector<uint8_t>& b, int nb) -> int {
      int md = 0;
      const int m = na < nb ? na : nb;
      for (size_t i = 0; i < static_cast<size_t>(m) * 128; ++i) {
        int diff = static_cast<int>(a[i]) - static_cast<int>(b[i]);
        if (diff < 0) diff = -diff;
        if (diff > md) md = diff;
      }
      return md;
    };
    std::vector<uint8_t> d_s1(static_cast<size_t>(128) * cap);
    std::vector<uint8_t> d_s2(static_cast<size_t>(128) * cap);
    std::vector<uint8_t> d_t1(static_cast<size_t>(128) * cap);
    std::vector<uint8_t> d_tn(static_cast<size_t>(128) * cap);
    const int n_s1 = extract(-1, d_s1);
    const int n_s2 = extract(-1, d_s2);
    const int n_t1 = extract(1, d_t1);
    const int n_tn = extract(num_threads > 0 ? num_threads : 4, d_tn);
    if (n_s1 < 0) return -n_s1;
    if (n_s2 < 0) return -n_s2;
    if (n_t1 < 0) return -n_t1;
    if (n_tn < 0) return -n_tn;
    const int md_ss = maxdiff(d_s1, n_s1, d_s2, n_s2);
    const int md_st1 = maxdiff(d_s1, n_s1, d_t1, n_t1);
    const int md_stn = maxdiff(d_s1, n_s1, d_tn, n_tn);
    std::printf(
        "SELFCHECK_DIAG n_s1=%d n_s2=%d n_t1=%d n_tn=%d | "
        "maxd(s,s)=%d maxd(s,t1)=%d maxd(s,tn)=%d\n",
        n_s1, n_s2, n_t1, n_tn, md_ss, md_st1, md_stn);
    std::fflush(stdout);
    if (out_n_serial) *out_n_serial = n_s1;
    if (out_n_threaded) *out_n_threaded = n_tn;
    if (out_max_abs_desc_diff) *out_max_abs_desc_diff = md_stn;
    return 0;
  } catch (...) {
    return 9;
  }
}

                                                                               
                                                                                
                                                                           
                                                             
                                                                       
                                                                       
                                                                  
                                
  
                                                                        
                                                                        
                                                                          
                                                                      
                                                                            
  
                                                                    
                                                                        
                                                                           
                                                                         
                                                                        
int aether_sift_match_pairs(const uint8_t* desc1,
                            int n1,
                            const uint8_t* desc2,
                            int n2,
                            double max_ratio,
                            uint32_t* out_pairs,
                            int max_pairs,
                            int* out_num_matches) {
  if (out_num_matches) *out_num_matches = 0;
  try {
    if (desc1 == nullptr || desc2 == nullptr || n1 <= 0 || n2 <= 0) return 1;
    if (out_pairs != nullptr && max_pairs <= 0) return 1;

                                                                             
                                                                              
    auto d1 = std::make_shared<colmap::FeatureDescriptors>();
    d1->type = colmap::FeatureExtractorType::SIFT;
    d1->data.resize(n1, 128);
    std::memcpy(d1->data.data(), desc1, static_cast<size_t>(n1) * 128);
    auto d2 = std::make_shared<colmap::FeatureDescriptors>();
    d2->type = colmap::FeatureExtractorType::SIFT;
    d2->data.resize(n2, 128);
    std::memcpy(d2->data.data(), desc2, static_cast<size_t>(n2) * 128);

                                                                                
                                                                                 
                                                                   
    colmap::FeatureMatchingOptions opts(
        colmap::FeatureMatcherType::SIFT_BRUTEFORCE);
    opts.sift = std::make_shared<colmap::SiftMatchingOptions>();
    opts.sift->max_ratio = max_ratio > 0 ? max_ratio : 0.8;
    opts.sift->cross_check = true;                                            
    opts.sift->cpu_brute_force_matcher = true;
    opts.use_gpu = false;

    std::unique_ptr<colmap::FeatureMatcher> matcher =
        colmap::CreateSiftFeatureMatcher(opts);
    if (!matcher) return 3;

    colmap::FeatureMatcher::Image img1;
    img1.image_id = 1;
    img1.descriptors = d1;
    colmap::FeatureMatcher::Image img2;
    img2.image_id = 2;
    img2.descriptors = d2;

    colmap::FeatureMatches matches;
    matcher->Match(img1, img2, &matches);

    int n_out = static_cast<int>(matches.size());
    if (out_pairs != nullptr) {
      if (n_out > max_pairs) n_out = max_pairs;                              
      for (int i = 0; i < n_out; ++i) {
        out_pairs[2 * i] = matches[i].point2D_idx1;
        out_pairs[2 * i + 1] = matches[i].point2D_idx2;
      }
    }
    if (out_num_matches) *out_num_matches = n_out;
    return 0;
  } catch (...) {
    return 2;
  }
}

                                                                         
                                                                           
                                                                   
int aether_sift_match(const uint8_t* desc1,
                      int n1,
                      const uint8_t* desc2,
                      int n2,
                      double max_ratio,
                      int* out_num_matches) {
  return aether_sift_match_pairs(desc1, n1, desc2, n2, max_ratio,
                                               nullptr,               0,
                                 out_num_matches);
}

}               
