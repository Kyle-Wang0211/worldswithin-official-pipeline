                                                                                        
  
                                                                            
                                                                              
                                                                              
  
                                                                   
                                                                                  
                                                              
                                                                       
                                                      
                                                                             
                                                                             
                                                       

#import <Foundation/Foundation.h>
#import <mach/mach.h>

extern "C" {

                                                                            
                                                   
__attribute__((visibility("default"), used))
int pwofficial_telemetry(double* phys_footprint_mb,
                         double* footprint_peak_mb,
                         int* thermal_state) {
  int rc = 0;

  if (phys_footprint_mb || footprint_peak_mb) {
    task_vm_info_data_t info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    const kern_return_t kr = task_info(mach_task_self(), TASK_VM_INFO,
                                       reinterpret_cast<task_info_t>(&info),
                                       &count);
    if (kr == KERN_SUCCESS) {
      const double footprint =
          static_cast<double>(info.phys_footprint) / (1024.0 * 1024.0);
      if (phys_footprint_mb) *phys_footprint_mb = footprint;
                                                                              
                                                                         
                                                                           
      if (footprint_peak_mb) *footprint_peak_mb = footprint;
    } else {
      rc = 1;
      if (phys_footprint_mb) *phys_footprint_mb = -1.0;
      if (footprint_peak_mb) *footprint_peak_mb = -1.0;
    }
  }

  if (thermal_state) {
    *thermal_state =
        static_cast<int>([[NSProcessInfo processInfo] thermalState]);
  }
  return rc;
}

}               
