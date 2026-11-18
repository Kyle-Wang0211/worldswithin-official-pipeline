#pragma once
#include <onnxruntime_c_api.h>
#include <array>
#include <memory>
#include <string>
#include <vector>
namespace reconstruction {
// Source integration contract, not an additional publicApp/AOT ABI.
// The lender owns the existing37fixed inputs+4outputs in the SAME rest session/Env/allocator.
// Capture must bind these exact OrtValues/backings. It must not allocate/copy a second37-input set.
struct FixedMaterialTensorWitness{std::string name;const OrtValue*value=nullptr;const void*backing=nullptr;std::vector<int64_t>shape;int dtype=0;size_t bytes=0;int canonical_view_slot=-1,feature_level=-1;};
struct FixedMaterialInputContractV1{uint32_t abi=1;std::shared_ptr<void>lifetime_owner;const OrtEnv*environment=nullptr;const OrtAllocator*allocator=nullptr;std::vector<FixedMaterialTensorWitness>inputs;size_t feature_slot_count=30,total_input_count=37,output_count=4;bool output_lease_live=false;std::array<int,10>canonical_photo_ids{};uint64_t fixed_feature_payload_B=0,fixed_input_payload_B=0,scratch_tensor_payload_B=0,extra_feature_bank_B=0,d2d_copies=0,d2d_payload_B=0;};
// Witness validity lasts only while lifetime_owner is retained; no value/backing replacement is allowed.
// Before any update, previous replay+4outputreadbacks must complete and its lease release.
// Prepare stages hits via samequeue officialD2D and <=onelevel scratch; misses upload readonlyF16 levels.
// No GPU->CPU feature transfer, no F32feature shadow, no encoder, no extra featurebank.
// On partial update failure latchfailed and abortscene; never replay partiallyupdated inputs.
}
