#pragma once

#include <array>

namespace aether::sfm {

enum class MandatoryArkitGravityPoseStatusV1 {
  kOk = 0,
  kMissingOutput,
  kMissingQuaternion,
  kMissingTranslation,
  kNonFinite,
  kDegenerateQuaternion,
};

                                                                          
                                                                            
                                                                      
struct MandatoryArkitGravityPoseV1 {
  std::array<double, 4> cam_from_world_qwxyz{};
  std::array<double, 3> cam_from_world_t_xyz{};
  std::array<double, 3> gravity_cam_xyz{};
};

                                                                           
                                                                         
                                                                           
MandatoryArkitGravityPoseStatusV1 BuildMandatoryArkitGravityPoseV1(
    const double* arkit_w2c_qwxyz,
    const double* arkit_w2c_t_xyz,
    MandatoryArkitGravityPoseV1* output);

}                          
