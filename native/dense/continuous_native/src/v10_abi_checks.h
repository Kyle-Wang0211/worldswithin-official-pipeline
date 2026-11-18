#pragma once
#include "reconstruction_continuous_mvs.h"
#include <cstddef>
static_assert(sizeof(reconstruction_continuous_mvs_request)==128);
static_assert(offsetof(reconstruction_continuous_mvs_request,feature_low)==16);
static_assert(offsetof(reconstruction_continuous_mvs_request,cameras_low_f32)==48);
static_assert(offsetof(reconstruction_continuous_mvs_request,output_root)==104);
static_assert(offsetof(reconstruction_continuous_mvs_request,user)==120);
