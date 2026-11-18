#ifndef AETHER_PAIR_POLICY_V2_C_H_
#define AETHER_PAIR_POLICY_V2_C_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_PAIR_POLICY_CONFIG_V2_VERSION 2U

typedef enum aether_pair_policy_mode_v2 {
                                                                    
  AETHER_PAIR_POLICY_MODE_SHIPPING_LEGACY = 0,
                                                                           
  AETHER_PAIR_POLICY_MODE_S20_T2_SHADOW = 1,
                                                                              
                                                                           
  AETHER_PAIR_POLICY_MODE_S20_T2_CANDIDATE = 2,
} aether_pair_policy_mode_v2;

typedef enum aether_pair_policy_result_v2 {
  AETHER_PAIR_POLICY_V2_OK = 0,
  AETHER_PAIR_POLICY_V2_ERR_NULL = -1,
  AETHER_PAIR_POLICY_V2_ERR_SIZE = -2,
  AETHER_PAIR_POLICY_V2_ERR_VERSION = -3,
  AETHER_PAIR_POLICY_V2_ERR_MODE = -4,
  AETHER_PAIR_POLICY_V2_ERR_RANGE = -5,
  AETHER_PAIR_POLICY_V2_ERR_RESERVED = -6,
} aether_pair_policy_result_v2;

                                                     
                                                                           
                                                                               
                                                    
typedef struct aether_pair_policy_config_v2 {
  uint32_t struct_size;
  uint32_t version;
  uint32_t mode;

  int32_t spatial_k;
  int32_t temporal_lookback;
  int32_t spatial_recent_exclusion;

  uint32_t visual_loop_enabled;
  int32_t visual_loop_period;
  int32_t visual_loop_topup;
  int32_t visual_loop_retrieve_cap;
  int32_t visual_loop_recent_exclusion;

  uint32_t flags;
  uint32_t reserved[8];
} aether_pair_policy_config_v2;

void aether_pair_policy_config_v2_default(
    aether_pair_policy_config_v2* out_config);

int32_t aether_pair_policy_config_v2_validate(
    const aether_pair_policy_config_v2* config);

#ifdef __cplusplus
}               
#endif

#endif                               
