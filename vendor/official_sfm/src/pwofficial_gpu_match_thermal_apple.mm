                                                                       
                                                               
                                                                            
                                                                   
                                                                                
                                                                       
                                                                     
                                                                          
                                                                      

#import <Foundation/Foundation.h>

extern "C" int pwofficial_platform_thermal_state(void) {
  if (@available(iOS 11.0, macOS 10.10.3, *)) {
    return (int)NSProcessInfo.processInfo.thermalState;
  }
  return 0;
}
