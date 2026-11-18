                                                     
  
                                
                                  
                                        
                             
  
                                                              
                                                       
                                              
                                                
                                                   
                                                  
                                                   
  
                                            
                                     
                                                        
                                   

                                                 
                                                   
const int kSfmFeedMaxInFlight = 2;

                                   
                                             
                                                           
bool sfmFeedShouldSpool({required int inFlight, required int spoolDepth}) {
  return inFlight >= kSfmFeedMaxInFlight || spoolDepth > 0;
}

                                
                                 
bool sfmFeedCanPumpNext({required int inFlight, required int spoolDepth}) {
  return inFlight < kSfmFeedMaxInFlight && spoolDepth > 0;
}

                                       
                                              
                                         
bool sfmFeedCanSendFinalize({
  required bool finalizeRequested,
  required bool finalizeSent,
  required int spoolDepth,
  required int inFlight,
}) {
  return finalizeRequested && !finalizeSent && spoolDepth == 0 && inFlight == 0;
}
