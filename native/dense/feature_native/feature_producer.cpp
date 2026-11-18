#include "feature_producer.h"
#ifndef PW_FEATURE_BRIDGE_SOURCE
#error "Define PW_FEATURE_BRIDGE_SOURCE to the frozen existing typed-half bridge .cpp"
#endif
#include PW_FEATURE_BRIDGE_SOURCE
#include "dense_inputs.h"
#include "sha256.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <array>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
namespace fs=std::filesystem;
using Hash=model_package::Sha256;
namespace {PwFeatureProducer* failed_producer_slot=nullptr;}
extern "C" int ReconstructionRegisterExperimentalSchema() noexcept;
namespace {
struct Layer {std::string name;std::array<int64_t,4> shape;size_t bytes=0,offset=0;std::string sha;};
struct Identity {int index;std::string photo,gray;};
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
void copytext(const char*s,char*p,size_t n) noexcept {fixedcopy(p,n,s);}
void copytext(const std::string&s,char*p,size_t n){if(p&&n){size_t k=std::min(s.size(),n-1);std::memcpy(p,s.data(),k);p[k]=0;}}
bool hex64(const std::string&s){return s.size()==64&&s.find_first_not_of("0123456789abcdef")==std::string::npos;}
void freeze(const fs::path&dir){fs::permissions(dir/"features.f16",fs::perms::owner_read|fs::perms::group_read|fs::perms::others_read);fs::permissions(dir/"seal.txt",fs::perms::owner_read|fs::perms::group_read|fs::perms::others_read);fs::permissions(dir,fs::perms::owner_read|fs::perms::owner_exec|fs::perms::group_read|fs::perms::group_exec|fs::perms::others_read|fs::perms::others_exec);}
void durable(const fs::path&p){int fd=::open(p.c_str(),O_RDONLY);if(fd<0)throw std::runtime_error("durable open");int rc=::fsync(fd);::close(fd);if(rc)throw std::runtime_error("durable fsync");}
struct Lock {int fd;Lock(const fs::path&p){fd=::open(p.c_str(),O_CREAT|O_RDWR,0600);if(fd<0||::flock(fd,LOCK_EX))throw std::runtime_error("cache writer lock");}~Lock(){::flock(fd,LOCK_UN);::close(fd);}};
}
struct PwFeatureProducer {
 std::string model,modelsha,pre,root;int w,h;size_t frameB,cacheB;std::array<Layer,3> layers;std::map<int,Identity> allowed;PwFeatureProducerStats stats{};pw_ort_model* feature=nullptr;bool request_failed=false;std::vector<float> rgb_backing;std::vector<uint16_t> half_backing;
 ~PwFeatureProducer(){if(feature)pw_ort_destroy(feature);}
 std::string key(const Identity&i){return Hash::HashStringHex("{\"model_sha256\":"+quote(modelsha)+",\"photo_sha256\":"+quote(i.photo)+",\"prepared_gray_sha256\":"+quote(i.gray)+",\"preprocess_sha256\":"+quote(pre)+"}");}
 void clear(){if(!feature||feature->failure.uncertain)return;for(auto&v:feature->gpu_outputs)v.reset();for(size_t i=0;i<feature->values.size();++i){if(feature->values[i]&&!feature->gpu_inputs[i])api->ReleaseValue(feature->values[i]);feature->values[i]=nullptr;feature->gpu_inputs[i].reset();std::vector<float>().swap(feature->inputs[i]);}stats.resident_feature_frames=0;}
 void load(){if(feature)return;need(Hash::HashFileHex(model)==modelsha,"model content SHA mismatch");api=OrtGetApiBase()->GetApi(ORT_API_VERSION);need(api&&ReconstructionRegisterExperimentalSchema()==0,"fixed ORT schema/API");version=OrtGetApiBase()->GetVersionString();char e[4096];feature=pw_ort_load(model.c_str(),1,1,0,e,sizeof(e));if(!feature)throw std::runtime_error(e);need(feature->ins.size()==1&&feature->outs.size()==3&&feature->ins[0].dtype==ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,"encoder arity/input dtype");}
 size_t used(){size_t bytes=0;for(auto&e:fs::recursive_directory_iterator(root))if(e.is_regular_file()&&e.path().filename()=="features.f16")bytes+=e.file_size();return bytes;}
 std::array<Layer,3> validate(const fs::path&dir,const Identity&i){std::ifstream seal(dir/"seal.txt");std::string k,p,g,m,pr,filehash;int wi,hi;seal>>k>>p>>g>>m>>pr>>wi>>hi;need(bool(seal)&&k==key(i)&&p==i.photo&&g==i.gray&&m==modelsha&&pr==pre&&wi==w&&hi==h,"immutable cache identity mismatch");auto actual=layers;
 for(auto&l:actual){std::string name;size_t bytes,offset;std::array<int64_t,4> shape;seal>>name;for(auto&x:shape)seal>>x;seal>>bytes>>offset>>l.sha;need(bool(seal)&&name==l.name&&shape==l.shape&&bytes==l.bytes&&offset==l.offset&&hex64(l.sha),"cache seal layer mismatch");}
 seal>>filehash;need(bool(seal)&&hex64(filehash)&&fs::file_size(dir/"features.f16")==frameB,"cache payload size");need(Hash::HashFileHex((dir/"features.f16").string())==filehash,"cache file SHA mismatch");std::ifstream f(dir/"features.f16",std::ios::binary);
 for(auto&l:actual){std::vector<uint16_t> b(l.bytes/2);f.read(reinterpret_cast<char*>(b.data()),l.bytes);need(size_t(f.gcount())==l.bytes,"cache layer count");for(auto v:b)need((v&0x7c00u)!=0x7c00u,"cache nonfinite");need(Hash::HashBytesHex(b.data(),l.bytes)==l.sha,"cache layer SHA mismatch");}return actual;
 }
 std::string receipt(const Identity&i,const std::array<Layer,3>&ls,bool hit,bool recovered,const fs::path&dir){std::ostringstream o;o<<"{\"pack_index\":"<<i.index<<",\"photo_sha256\":"<<quote(i.photo)<<",\"prepared_gray_sha256\":"<<quote(i.gray)<<",\"cache_key\":"<<quote(key(i))<<",\"file\":"<<quote(std::string("objects/")+key(i)+"/features.f16")<<",\"bytes\":"<<frameB<<",\"sha256\":"<<quote(Hash::HashFileHex((dir/"features.f16").string()))<<",\"cache_hit\":"<<(hit?"true":"false")<<",\"partial_recovered\":"<<(recovered?"true":"false")<<",\"native_runs_process\":"<<stats.native_runs<<",\"layers\":[";
 for(int j=0;j<3;++j){if(j)o<<",";auto&l=ls[j];o<<"{\"level\":"<<j<<",\"name\":"<<quote(l.name)<<",\"shape\":[";for(int z=0;z<4;++z){if(z)o<<",";o<<l.shape[z];}o<<"],\"dtype\":10,\"offset\":"<<l.offset<<",\"bytes\":"<<l.bytes<<",\"sha256\":"<<quote(l.sha)<<",\"finite\":true}";}o<<"]}";return o.str();}
 int request(const PwFeatureRequest&r,std::string&json){auto it=allowed.find(r.pack_index);need(it!=allowed.end(),"unregistered photo");const auto&i=it->second;need(r.photo_sha256&&i.photo==r.photo_sha256,"wrong photo identity");need(r.model_sha256&&modelsha==r.model_sha256,"wrong model identity");need(r.preprocess_sha256&&pre==r.preprocess_sha256,"wrong preprocess identity");need(r.gray&&r.gray_elements==size_t(w)*h,"wrong gray dimensions");need(Hash::HashBytesHex(r.gray,r.gray_elements*2)==i.gray,"wrong prepared photo content");need(Hash::HashFileHex(model)==modelsha,"model file changed");Lock lock(fs::path(root)/"producer.lock");auto objects=fs::path(root)/"objects";auto final=objects/key(i),partial=objects/("."+key(i)+".partial");
 if(fs::exists(final)){auto ls=validate(final,i);++stats.cache_hits;json=receipt(i,ls,true,false,final);return 0;}
 if(fs::exists(partial)&&fs::exists(partial/"seal.txt")){auto ls=validate(partial,i);fs::permissions(partial,fs::perms::owner_write,fs::perm_options::add);validate(partial,i);fs::rename(partial,final);freeze(final);durable(objects);++stats.partial_recoveries;++stats.atomic_commits;json=receipt(i,ls,false,true,final);return 0;}
 if(fs::exists(partial)){auto failed=objects/("."+key(i)+".incomplete."+std::to_string(::getpid()));fs::rename(partial,failed);}
 need(used()+frameB<=cacheB,"bounded immutable disk cache budget");fs::create_directory(partial);load();auto actual=layers;
 try{auto&rgb=rgb_backing;rgb.resize(r.gray_elements*3);for(size_t n=0;n<r.gray_elements;++n){float v=aether::dense::fp16_to_fp32(r.gray[n]);for(int c=0;c<3;++c)rgb[size_t(c)*r.gray_elements+n]=v;}int32_t dims[4]={1,3,h,w};need(pw_ort_set_input_gpu(feature,feature->ins[0].name.c_str(),rgb.data(),dims,4,rgb.size())==0,"encoder RGB upload");double ignored;int rc=pw_ort_run_gpu(feature,&ignored);if(rc)throw std::runtime_error(pw_ort_last_error(feature));++stats.native_runs;stats.resident_feature_frames=1;stats.peak_frame_payload_B=std::max(stats.peak_frame_payload_B,frameB);std::ofstream f(partial/"features.f16",std::ios::binary);
 for(int j=0;j<3;++j){auto v=feature->gpu_outputs[j];need(bool(v),"GPU output missing");require_gpu(v->value);ONNXTensorElementDataType dt;auto sh=tensor_shape(v->value,&dt);auto&l=actual[j];need(dt==ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16&&std::vector<int64_t>(l.shape.begin(),l.shape.end())==sh&&tensor_bytes(sh,dt)==l.bytes,"actual GPU F16 shape/dtype mismatch");auto&half=half_backing;half.resize(l.bytes/2);OrtValue*&host=feature->failure.host_alias;check(api->CreateTensorWithDataAsOrtValue(feature->mem,half.data(),l.bytes,sh.data(),sh.size(),dt,&host));const OrtValue* src[]={v->value};OrtValue* dst[]={host};try{feature->failure.uncertain=true;check(api->CopyTensors(feature->env,src,dst,nullptr,1));feature->failure.uncertain=false;api->ReleaseValue(host);host=nullptr;}catch(...){fail_model(feature,"feature download completion unconfirmed");throw;}for(auto x:half)need((x&0x7c00u)!=0x7c00u,"nonfinite half feature");l.sha=Hash::HashBytesHex(half.data(),l.bytes);f.write(reinterpret_cast<const char*>(half.data()),l.bytes);need(bool(f),"partial payload write");feature->gpu_outputs[j].reset();v.reset();}
 f.flush();f.close();clear();std::vector<float>().swap(rgb_backing);std::vector<uint16_t>().swap(half_backing);durable(partial/"features.f16");std::ofstream seal(partial/"seal.txt");seal<<key(i)<<" "<<i.photo<<" "<<i.gray<<" "<<modelsha<<" "<<pre<<" "<<w<<" "<<h<<"\n";for(auto&l:actual){seal<<l.name;for(auto x:l.shape)seal<<" "<<x;seal<<" "<<l.bytes<<" "<<l.offset<<" "<<l.sha<<"\n";}seal<<Hash::HashFileHex((partial/"features.f16").string())<<"\n";seal.flush();seal.close();durable(partial/"seal.txt");durable(partial);actual=validate(partial,i);
 if(r.interrupt_after_complete_partial){json=receipt(i,actual,false,false,partial);return 1;}
 fs::rename(partial,final);freeze(final);durable(objects);++stats.atomic_commits;json=receipt(i,actual,false,false,final);return 0;
 }catch(...){clear();throw;}
 }
};
extern "C" PwFeatureProducer* pw_feature_producer_create(const PwFeatureProducerConfig*c,char*e,size_t n){if(failed_producer_slot||failed_model_slot){copytext("failed process forbids new producer",e,n);return nullptr;}try{need(c&&c->model_path&&c->model_sha256&&c->preprocess_sha256&&c->cache_root,"null config");auto p=std::make_unique<PwFeatureProducer>();p->model=c->model_path;p->modelsha=c->model_sha256;p->pre=c->preprocess_sha256;p->root=c->cache_root;p->w=c->width;p->h=c->height;p->cacheB=c->max_cache_payload_B;p->frameB=0;need(hex64(p->modelsha)&&hex64(p->pre)&&p->w>0&&p->h>0,"config identity/shape");for(int j=0;j<3;++j){auto&s=c->layers[j];auto&l=p->layers[j];need(s.name&&s.bytes>0&&s.bytes%2==0,"layer config");l.name=s.name;l.bytes=s.bytes;l.offset=p->frameB;size_t count=1;for(int k=0;k<4;++k){need(s.shape[k]>0&&count<=SIZE_MAX/size_t(s.shape[k]),"shape overflow");l.shape[k]=s.shape[k];count*=s.shape[k];}need(count<=SIZE_MAX/2&&count*2==l.bytes,"shape byte mismatch");p->frameB+=l.bytes;}need(p->frameB==c->max_frame_payload_B&&p->frameB<=p->cacheB,"bounded frame config");need(Hash::HashFileHex(p->model)==p->modelsha,"configured model SHA mismatch");fs::create_directories(fs::path(p->root)/"objects");return p.release();}catch(const std::exception&x){copytext(x.what(),e,n);return nullptr;}}
extern "C" int pw_feature_producer_register(PwFeatureProducer*p,const PwFeatureMaterialIdentity*i){try{if(!p||!i||!i->photo_sha256||!i->prepared_gray_sha256)return -1;Identity v{i->pack_index,i->photo_sha256,i->prepared_gray_sha256};if(!hex64(v.photo)||!hex64(v.gray)||p->allowed.count(v.index))return -1;p->allowed.emplace(v.index,v);return 0;}catch(...){if(p){p->request_failed=true;failed_producer_slot=p;}return -1;}}
extern "C" int pw_feature_producer_request(PwFeatureProducer*p,const PwFeatureRequest*r,char*out,size_t n,char*e,size_t en){if(!p||!r)return -1;try{std::string json;int rc=p->request(*r,json);need(json.size()+1<=n,"receipt buffer too small");copytext(json,out,n);return rc;}catch(const std::exception&x){++p->stats.rejections;copytext(x.what(),e,en);return -1;}catch(...){++p->stats.rejections;copytext("unknown feature request exception",e,en);return -1;}}
extern "C" void pw_feature_producer_stats(const PwFeatureProducer*p,PwFeatureProducerStats*s){if(p&&s)*s=p->stats;}
extern "C" void pw_feature_producer_destroy(PwFeatureProducer*p){delete p;}
