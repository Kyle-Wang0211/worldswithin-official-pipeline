                                                                             
#pragma once

#include <onnxruntime_cxx_api.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace aether::dense {

struct DenseViewSegmentOptions {
    bool webgpu = false;
    bool final_outputs_cpu = true;
                                                                        
                                                                         
                                                                             
                                                                          
    int intra_op_threads = 0;
                                                                            
                                                                                
                                                             
    bool native_output_locations = false;
                                                                         
                                                                            
                                                                               
    bool disable_cpu_mem_arena = false;
                                                                              
                                                                             
                                                                              
    bool disable_graph_optimizations = false;
                                                                             
                                                                          
                                                                              
                                                                                   
    std::string storage_buffer_cache_mode;
};

struct DenseViewInputRank {
    std::string name;
                                                                              
                                                                               
    std::optional<size_t> rank;
};

                                                                               
                                                                              
struct DenseViewSegmentPlan {
    std::string model_path;
    std::vector<std::string> consumed_input_names;
                                                                             
                                                                     
    std::vector<DenseViewInputRank> input_ranks;
                                                                              
    size_t ordinal = static_cast<size_t>(-1);
};

struct DenseViewNamedInput {
    std::string name;
    const Ort::Value* value = nullptr;
};

struct DenseViewNamedOutput {
    std::string name;
    const Ort::Value* value = nullptr;
};

struct DenseViewSegmentStats {
    size_t segments_run = 0;
    size_t produced_outputs = 0;
    size_t passthrough_outputs = 0;
    size_t skipped_passthrough_inputs = 0;
    size_t native_output_locations_queried = 0;
    size_t native_output_bindings = 0;
};

                                                                                
                                                                              
                                
                                                                                 
                                                                                 
                                                                              
                                                                       
class DenseViewSegments {
public:
    DenseViewSegments();
    ~DenseViewSegments();
    DenseViewSegments(const DenseViewSegments&) = delete;
    DenseViewSegments& operator=(const DenseViewSegments&) = delete;

    bool init(const std::vector<DenseViewSegmentPlan>& ordered_models,
              DenseViewSegmentOptions options, std::string* error);
    bool run(const std::vector<DenseViewNamedInput>& inputs, std::string* error);
    std::vector<DenseViewNamedOutput> outputs() const;
    DenseViewSegmentStats stats() const;
    void release() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}                            
