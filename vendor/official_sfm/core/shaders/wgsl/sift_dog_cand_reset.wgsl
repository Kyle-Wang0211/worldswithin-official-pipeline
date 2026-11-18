                                                                                                  
                                                                                            
                                                                                                      
                                                   
@group(0) @binding(0) var<storage, read_write> cand_counter : atomic<u32>;

@compute @workgroup_size(1, 1, 1)
fn main() { atomicStore(&cand_counter, 0u); }
