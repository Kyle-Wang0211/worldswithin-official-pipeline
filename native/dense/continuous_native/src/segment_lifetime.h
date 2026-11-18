#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
namespace pw_segment_v8 {
// Consumer ownership guard, not an observer. It retains the original fixed-slot
// allocator owner; actual ORT boundary values must use that owner's allocator.
struct Lifecycle {
 enum Phase{Idle,Ready,Submitted,Completed,BoundaryOwned,GraphReleased};
 Phase phase=Idle;uint64_t epoch=0,uploaded_epoch=0;unsigned next=0;
 std::shared_ptr<void> lifetime_owner;
 explicit Lifecycle(std::shared_ptr<void> owner):lifetime_owner(std::move(owner)){if(!lifetime_owner)throw std::logic_error("missing lifetime owner");}
 void begin_reference(uint64_t e){if(phase!=Idle||e==0||e==epoch)throw std::logic_error("reference transition");epoch=e;uploaded_epoch=0;next=0;}
 void fixedfeeds_upload_completed(uint64_t e){if(phase!=Idle||e!=epoch)throw std::logic_error("stale feeds");uploaded_epoch=e;}
 void begin_segment(unsigned s){if(phase!=Idle||uploaded_epoch!=epoch||s!=next)throw std::logic_error("stale/unsorted segment");phase=Ready;}
 void submitted(){if(phase!=Ready)throw std::logic_error("submit order");phase=Submitted;}
 void completion(){if(phase!=Submitted)throw std::logic_error("completion order");phase=Completed;}
 void anchor_outputs_owned(){if(phase!=Completed)throw std::logic_error("output before completion");phase=BoundaryOwned;}
 void captured_graph_released(unsigned pool_generations){if(phase!=BoundaryOwned||pool_generations!=0)throw std::logic_error("graph/pool retention");phase=GraphReleased;}
 void segment_invocation_closed(){if(phase!=GraphReleased)throw std::logic_error("destroy before handoff/release");phase=Idle;++next;}
 void segment_session_destroyed(){segment_invocation_closed();}
};
}
