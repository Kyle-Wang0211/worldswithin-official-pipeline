// Compile this TU once. The accepted producer includes the frozen typed-half
// bridge; never also compile vendor/pw_ort_c.cpp into this target.
#include "feature_producer.h"
extern "C" __attribute__((visibility("hidden"))) int
pw_feature_producer_request_original(PwFeatureProducer*,const PwFeatureRequest*,
    char*,size_t,char*,size_t);
extern "C" __attribute__((visibility("hidden"))) void
pw_feature_producer_destroy_original(PwFeatureProducer*);
#define PW_FEATURE_BRIDGE_SOURCE "vendor/pw_ort_c.cpp"
#define pw_feature_producer_request pw_feature_producer_request_original
#define pw_feature_producer_destroy pw_feature_producer_destroy_original
#include "feature_producer.cpp"
#undef pw_feature_producer_request
#undef pw_feature_producer_destroy
#include "production_feature_phase.h"

extern "C" int pw_feature_runtime_bind_v1(const char* path,char* error,size_t n) {
 try {
  if (!path || path[0] != '/' || n > INT_MAX) {
    copytext("absolute sole runtime path/error capacity required",error,n);
    return -1;
  }
  void* handle=dlopen(path,RTLD_NOW|RTLD_LOCAL);
  if (!handle) { copytext(dlerror(),error,n); return -1; }
  void* entry=dlsym(handle,"OrtGetApiBase");
  const bool same=entry==reinterpret_cast<void*>(&OrtGetApiBase);
  dlclose(handle); // Linked dependency keeps the sole runtime loaded.
  if (!same) {
    copytext("runtime differs from the linked sole ORT API image",error,n);
    return -1;
  }
  return pw_ort_set_runtime(path,error,static_cast<int>(n));
 }catch(const std::exception&e){copytext(e.what(),error,n);return -1;}
 catch(...){copytext("unknown runtime bind failure",error,n);return -1;}
}
extern "C" int pw_feature_producer_request(PwFeatureProducer* p,
    const PwFeatureRequest* r, char* receipt, size_t n, char* error, size_t en) {
  if (!p) {copytext("null producer",error,en);return -1;}
  if (failed_producer_slot || failed_model_slot || p->request_failed) {
    copytext("failed process retains actual native resources; retry forbidden",error,en);
    return -1;
  }
  try {
    int rc=pw_feature_producer_request_original(p,r,receipt,n,error,en);
    if(rc!=0){p->request_failed=true;failed_producer_slot=p;}
    return rc;
  } catch(const std::exception& e) {
    p->request_failed=true;failed_producer_slot=p;copytext(e.what(),error,en);
  } catch(...) {
    p->request_failed=true;failed_producer_slot=p;copytext("unknown feature C ABI failure",error,en);
  }
  return -1;
}

extern "C" void pw_feature_producer_destroy(PwFeatureProducer* p) {
  if(p&&(p->request_failed||(p->feature&&(p->feature->failed||p->feature->failure.uncertain))))return;
  pw_feature_producer_destroy_original(p);
}

extern "C" int pw_feature_producer_release_for_whole_v1(
    PwFeatureProducer** producer,char* error,size_t error_bytes) {
  if(!producer){copytext("null producer slot",error,error_bytes);return -1;}
  if(failed_producer_slot||failed_model_slot||(*producer&&(*producer)->request_failed)){
    copytext("failed process retains actual native resources; whole owner forbidden",error,error_bytes);return -1;
  }
  try {
    pw_feature_producer_destroy_original(*producer);*producer=nullptr;
    std::lock_guard<std::mutex> guard(init_mu);
    if(!shared_context.expired()){copytext("feature runtime context still retained; whole owner forbidden",error,error_bytes);return -1;}
    copytext("",error,error_bytes);return 0;
  }catch(const std::exception& e){copytext(e.what(),error,error_bytes);return -1;}
   catch(...){copytext("unknown feature phase failure",error,error_bytes);return -1;}
}
