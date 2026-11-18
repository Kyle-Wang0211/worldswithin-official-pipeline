#pragma once

#include <cmath>

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace aether::sfm {

inline bool IsValidGravityBaPriorV1(const Eigen::Vector3d& gravity_cam) {
  return gravity_cam.allFinite() && gravity_cam.squaredNorm() > 1e-12;
}

                                                                             
                                                                              
                                                                              
                                                                                
                                                                    
  
                                                                   
                                                                         
                                                                                   
                                                                             
                                           
                                                                           
                                                                          
                                                                               
                                                                               
                                                 
                                                                                
                                                                             
                                                                               
                                                                                
                                                                           
                                                                                 
                                                                                 
                                                                            
                                                                     
struct GravityBaPriorCostV1 {
  GravityBaPriorCostV1(const Eigen::Vector3d& gravity_cam,
                       const double inverse_sigma)
      : gravity_cam_(gravity_cam.normalized()),
        inverse_sigma_(inverse_sigma) {}

  template <typename T>
  bool operator()(const T* const camera_from_world, T* residuals) const {
    if (!IsValidGravityBaPriorV1(gravity_cam_) ||
        !std::isfinite(inverse_sigma_) || inverse_sigma_ <= 0.0) {
      return false;
    }
    const Eigen::Map<const Eigen::Quaternion<T>> rotation(camera_from_world);
    const Eigen::Matrix<T, 3, 1> gravity_world(T(0), T(-1), T(0));
    const Eigen::Matrix<T, 3, 1> predicted = rotation * gravity_world;
    const Eigen::Matrix<T, 3, 1> measured = gravity_cam_.cast<T>();
    const Eigen::Matrix<T, 3, 1> error =
        T(inverse_sigma_) * predicted.cross(measured);
    residuals[0] = error.x();
    residuals[1] = error.y();
    residuals[2] = error.z();
    return true;
  }

  Eigen::Vector3d gravity_cam_;
  double inverse_sigma_ = 1.0;
};

}                          
