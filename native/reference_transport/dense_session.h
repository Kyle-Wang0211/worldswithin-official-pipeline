                                                                                                     
  
                                                         
                                                                                                                   
                                                                                                
                                                                                                 
                                                                                                
                                         
                                                                                 
                                                                   
                                                                                    
                                                                                                    
                                                                                           
                                                                                 
#pragma once
#include <cstdint>
#include <vector>

namespace aether::dense {

struct SessionFrame {
    double frame_id;
    double fx, fy, cx, cy;                                                           
    double image_w, image_h;                             
    double q[4];                                                            
    double t[3];                                   
};

struct SessionParams {
    int W = 768, H = 576;                                                          
    int nsrc = 9;                         
    double theta0 = 5.0, sig1 = 1.0, sig2 = 10.0;                        
    double min_base = 0.06;               
    int min_common = 20;                                
    double dr_lo = 0.70, dr_hi = 1.50;                             
    int min_pts = 8;                                        
    double fallback_lo = 0.3, fallback_hi = 4.0;
};

struct SessionTable {
    int NF = 0;
    std::vector<float> cams;                                                                                
    std::vector<int32_t> neighbors;           
    std::vector<double> med_angle;                                                                           
    std::vector<uint8_t> vis;                                                                                          
    long NP = 0;
};

                                                                                           
                                                                      
void build_session_table(const std::vector<SessionFrame>& frames, const std::vector<float>& points,
                         const SessionParams& p, SessionTable& out);

                                                                                                         
double numpy_percentile_linear(const std::vector<double>& sorted_vals, double q_percent);

                                                                                                        
double numpy_pairwise_sum(const double* a, long n);

}                            
