                                                                
  
                                                             
                                                                           
                                                                              
                                                                               
                                                                             
                                                                            
                                             

#include "aether_threaded_extract.h"

#include "colmap/feature/utils.h"

#include "thirdparty/VLFeat/covdet.h"
#include "thirdparty/VLFeat/imopv.h"
#include "thirdparty/VLFeat/sift.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <thread>
#include <vector>

namespace aether {
namespace {

                                                                            
                                                                        
                                                                               
                                                              
colmap::FeatureDescriptorsData TransformVLFeatToUBCFeatureDescriptors(
    const colmap::FeatureDescriptorsData& vlfeat_descriptors) {
  colmap::FeatureDescriptorsData ubc_descriptors(vlfeat_descriptors.rows(),
                                                 vlfeat_descriptors.cols());
  const std::array<int, 8> q{{0, 7, 6, 5, 4, 3, 2, 1}};
  for (Eigen::Index n = 0; n < vlfeat_descriptors.rows(); ++n) {
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        for (int k = 0; k < 8; ++k) {
          ubc_descriptors(n, 8 * (j + 4 * i) + q[k]) =
              vlfeat_descriptors(n, 8 * (j + 4 * i) + k);
        }
      }
    }
  }
  return ubc_descriptors;
}

}              

bool ExtractCovariantSiftThreaded(const colmap::SiftExtractionOptions& sift_opts,
                                  const colmap::Bitmap& bitmap,
                                  colmap::FeatureKeypoints* keypoints,
                                  colmap::FeatureDescriptors* descriptors,
                                  int num_threads) {
  // ---- serial: detector setup + detect + affine + orient (identical to upstream) ----
  std::unique_ptr<VlCovDet, void (*)(VlCovDet*)> covdet(
      vl_covdet_new(VL_COVDET_METHOD_DOG), &vl_covdet_delete);
  if (!covdet) {
    return false;
  }

  const int kMaxOctaveResolution = 1000;

  vl_covdet_set_first_octave(covdet.get(), sift_opts.first_octave);
  vl_covdet_set_octave_resolution(covdet.get(), sift_opts.octave_resolution);
  vl_covdet_set_peak_threshold(covdet.get(), sift_opts.peak_threshold);
  vl_covdet_set_edge_threshold(covdet.get(), sift_opts.edge_threshold);

  {
                                                                              
                                                                            
    const std::vector<uint8_t>& data_uint8 = bitmap.RowMajorData();
    std::vector<float> data_float(data_uint8.size());
    for (size_t i = 0; i < data_uint8.size(); ++i) {
      data_float[i] = static_cast<float>(data_uint8[i]) / 255.0f;
    }
    vl_covdet_put_image(
        covdet.get(), data_float.data(), bitmap.Width(), bitmap.Height());
  }

  vl_covdet_detect(covdet.get(), sift_opts.max_num_features);

                                                                                  
                                                                                  
                                                                                  
                                                                                   
  // exact upstream output (same drops, same order) -> bit-identical.
  if (sift_opts.estimate_affine_shape) {
    const int nf = vl_covdet_get_num_features(covdet.get());
    VlCovDetFeature* feats = vl_covdet_get_features(covdet.get());
    VlScaleSpace* aff_gss = vl_covdet_get_gss(covdet.get());
    std::vector<VlFrameOrientedEllipse> adapted(nf > 0 ? nf : 0);
    std::vector<char> ok(nf > 0 ? nf : 0, 0);

    const int hwa = static_cast<int>(std::thread::hardware_concurrency());
    int athreads = num_threads > 0 ? num_threads : (hwa > 0 ? hwa : 1);
    if (athreads > nf) athreads = std::max<int>(1, nf);

    auto aff_worker = [&](int lo, int hi) {
      VlCovDet* wd = vl_covdet_new(VL_COVDET_METHOD_DOG);
      if (!wd) return;
      vl_covdet_set_gss(wd, aff_gss);                         
      for (int i = lo; i < hi; ++i) {
        ok[i] = (vl_covdet_extract_affine_shape_for_frame(
                     wd, &adapted[i], feats[i].frame) == VL_ERR_OK)
                    ? 1
                    : 0;
      }
      vl_covdet_set_gss(wd, nullptr);
      vl_covdet_delete(wd);
    };

    if (nf > 0) {
      if (athreads <= 1) {
        aff_worker(0, nf);
      } else {
        std::vector<std::thread> pool;
        pool.reserve(athreads);
        const int chunk = (nf + athreads - 1) / athreads;
        for (int t = 0; t < athreads; ++t) {
          const int lo = std::min(t * chunk, nf);
          const int hi = std::min(lo + chunk, nf);
          if (lo >= hi) break;
          pool.emplace_back(aff_worker, lo, hi);
        }
        for (auto& th : pool) th.join();
      }
    }

                                                                                
                                                                             
    int j = 0;
    for (int i = 0; i < nf; ++i) {
      if (ok[i]) {
        feats[j] = feats[i];
        feats[j].frame = adapted[i];
        ++j;
      }
    }
    vl_covdet_set_num_features(covdet.get(), static_cast<vl_size>(j));
  }

  if (!sift_opts.upright) {
    vl_covdet_extract_orientations(covdet.get());
  }

  const int num_features = vl_covdet_get_num_features(covdet.get());
  VlCovDetFeature* features = vl_covdet_get_features(covdet.get());

  std::sort(
      features,
      features + num_features,
      [](const VlCovDetFeature& feature1, const VlCovDetFeature& feature2) {
        if (feature1.o == feature2.o) {
          return feature1.s > feature2.s;
        } else {
          return feature1.o > feature2.o;
        }
      });

  const size_t max_num_features =
      static_cast<size_t>(sift_opts.max_num_features);

  int prev_octave_scale_idx = std::numeric_limits<int>::max();
  for (int i = 0; i < num_features; ++i) {
    colmap::FeatureKeypoint keypoint;
    keypoint.x = features[i].frame.x + 0.5;
    keypoint.y = features[i].frame.y + 0.5;
    keypoint.a11 = features[i].frame.a11;
    keypoint.a12 = features[i].frame.a12;
    keypoint.a21 = features[i].frame.a21;
    keypoint.a22 = features[i].frame.a22;
    keypoints->push_back(keypoint);

    const int octave_scale_idx =
        features[i].o * kMaxOctaveResolution + features[i].s;

    if (octave_scale_idx != prev_octave_scale_idx &&
        keypoints->size() >= max_num_features) {
      break;
    }
    prev_octave_scale_idx = octave_scale_idx;
  }

  if (descriptors == nullptr) {
    return true;
  }

                                                                         
  const size_t num_kp = keypoints->size();
  descriptors->data.resize(num_kp, 128);                                      

                                                                               
                                                                          
  VlScaleSpace* shared_gss = vl_covdet_get_gss(covdet.get());

  const int hw = static_cast<int>(std::thread::hardware_concurrency());
  int nthreads = num_threads > 0 ? num_threads : (hw > 0 ? hw : 1);
  if (nthreads > static_cast<int>(num_kp)) {
    nthreads = std::max<int>(1, static_cast<int>(num_kp));
  }

                                                                              
                                                                           
  auto worker = [&](size_t lo, size_t hi) {
    VlCovDet* wd = vl_covdet_new(VL_COVDET_METHOD_DOG);
    if (!wd) {
      return;
    }
    vl_covdet_set_gss(wd, shared_gss);                          

    std::unique_ptr<VlSiftFilt, void (*)(VlSiftFilt*)> sift(
        vl_sift_new(16, 16, 1, 3, 0), &vl_sift_delete);
    if (!sift) {
      vl_covdet_set_gss(wd, nullptr);
      vl_covdet_delete(wd);
      return;
    }
    vl_sift_set_magnif(sift.get(), 3.0);

    const size_t kPatchResolution = 15;
    const size_t kPatchSide = 2 * kPatchResolution + 1;
    const double kPatchRelativeExtent = 7.5;
    const double kPatchRelativeSmoothing = 1;
    const double kPatchStep = kPatchRelativeExtent / kPatchResolution;
    const double kSigma =
        kPatchRelativeExtent / (3.0 * (4 + 1) / 2) / kPatchStep;

    std::vector<float> patch(kPatchSide * kPatchSide);
    std::vector<float> patchXY(2 * kPatchSide * kPatchSide);

    float dsp_min_scale = 1;
    float dsp_scale_step = 0;
    int dsp_num_scales = 1;
    if (sift_opts.domain_size_pooling) {
      dsp_min_scale = sift_opts.dsp_min_scale;
      dsp_scale_step =
          (sift_opts.dsp_max_scale - sift_opts.dsp_min_scale) /
          sift_opts.dsp_num_scales;
      dsp_num_scales = sift_opts.dsp_num_scales;
    }

                                                                               
                                                                                      
    colmap::FeatureDescriptorsFloatData descriptor(1, 128);
    colmap::FeatureDescriptorsFloatData scaled_descriptors(dsp_num_scales, 128);

    for (size_t i = lo; i < hi; ++i) {
      for (int s = 0; s < dsp_num_scales; ++s) {
        const double dsp_scale = dsp_min_scale + s * dsp_scale_step;

        VlFrameOrientedEllipse scaled_frame = features[i].frame;
        scaled_frame.a11 *= dsp_scale;
        scaled_frame.a12 *= dsp_scale;
        scaled_frame.a21 *= dsp_scale;
        scaled_frame.a22 *= dsp_scale;

        vl_covdet_extract_patch_for_frame(wd,
                                          patch.data(),
                                          kPatchResolution,
                                          kPatchRelativeExtent,
                                          kPatchRelativeSmoothing,
                                          scaled_frame);

        vl_imgradient_polar_f(patchXY.data(),
                              patchXY.data() + 1,
                              2,
                              2 * kPatchSide,
                              patch.data(),
                              kPatchSide,
                              kPatchSide,
                              kPatchSide);

        vl_sift_calc_raw_descriptor(sift.get(),
                                    patchXY.data(),
                                    scaled_descriptors.row(s).data(),
                                    kPatchSide,
                                    kPatchSide,
                                    kPatchResolution,
                                    kPatchResolution,
                                    kSigma,
                                    0);
      }

      if (sift_opts.domain_size_pooling) {
        descriptor = scaled_descriptors.colwise().mean();
      } else {
        descriptor = scaled_descriptors;
      }

      if (sift_opts.normalization ==
          colmap::SiftExtractionOptions::Normalization::L2) {
        colmap::L2NormalizeFeatureDescriptors(&descriptor);
      } else if (sift_opts.normalization ==
                 colmap::SiftExtractionOptions::Normalization::L1_ROOT) {
        colmap::L1RootNormalizeFeatureDescriptors(&descriptor);
      }

      descriptors->data.row(i) =
          colmap::FeatureDescriptorsToUnsignedByte(descriptor);
    }

    vl_covdet_set_gss(wd, nullptr);                                    
    vl_covdet_delete(wd);
  };

  if (nthreads <= 1) {
    worker(0, num_kp);
  } else {
    std::vector<std::thread> pool;
    pool.reserve(nthreads);
    const size_t chunk = (num_kp + nthreads - 1) / nthreads;
    for (int t = 0; t < nthreads; ++t) {
      const size_t lo = std::min<size_t>(static_cast<size_t>(t) * chunk, num_kp);
      const size_t hi = std::min<size_t>(lo + chunk, num_kp);
      if (lo >= hi) {
        break;
      }
      pool.emplace_back(worker, lo, hi);
    }
    for (auto& th : pool) {
      th.join();
    }
  }

                                                                                
                                                                         
  descriptors->data = TransformVLFeatToUBCFeatureDescriptors(descriptors->data);
  descriptors->type = colmap::FeatureExtractorType::SIFT;
  return true;
}

}                     
