#include "../../memory_policy.h"
#include "actual_frame_count.h"
#include "reconstruction_continuous_mvs.h"
#include "dense_s1_adapter.h"
#include "dense_images.h"
#include "s1_memory_probe.h"
#include "portable_sha.h"
#include <array>
#include <atomic>
#include <memory>
#include <algorithm>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <sys/stat.h>
#include <unistd.h>
namespace {
using Clock=std::chrono::steady_clock; using aether::dense::DenseS1Pass;
struct RunOwner;
std::atomic<RunOwner*> retained_owner{nullptr};
std::atomic<bool> entry_active{false},failed_process{false};
static_assert(std::atomic<RunOwner*>::is_always_lock_free&&std::atomic<bool>::is_always_lock_free);
struct EntryGuard {bool owns=false;~EntryGuard()noexcept{if(owns)entry_active.store(false,std::memory_order_release);}};
void error_noexcept(char*error,size_t cap,const char*text)noexcept{if(!error||!cap)return;size_t n=0;if(text)while(n+1<cap&&text[n]){error[n]=text[n];++n;}error[n]=0;}

struct Stop{};
thread_local unsigned matched_fifo_mode=1;
unsigned parse_matched_fifo(const char*v){if(!v||std::string(v)=="1")return 1;if(std::string(v)=="4")return 4;throw std::runtime_error("RECONSTRUCTION_MATCHED_FIFO_MODE must be 1 or 4");}

std::string q(const std::string&s){std::string o="\"";for(unsigned char c:s){if(c=='\\'||c=='\"'){o+='\\';o+=char(c);}else if(c<32){char t[7];std::snprintf(t,7,"\\u%04x",c);o+=t;}else o+=char(c);}return o+'\"';}
std::string hex(const unsigned char*p,size_t n){std::ostringstream o;o<<std::hex<<std::setfill('0');for(size_t i=0;i<n;++i)o<<std::setw(2)<<unsigned(p[i]);return o.str();}
std::string hashmem(const void*p,size_t n){CC_SHA256_CTX c;CC_SHA256_Init(&c);auto*b=static_cast<const uint8_t*>(p);while(n){size_t z=std::min(n,size_t(1<<20));CC_SHA256_Update(&c,b,CC_LONG(z));b+=z;n-=z;}unsigned char out[32];CC_SHA256_Final(out,&c);return hex(out,32);}
std::string hashfile(const char*path){if(!path||*path!='/')throw std::runtime_error("absolute input path required");std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("open input: "+std::string(path));CC_SHA256_CTX c;CC_SHA256_Init(&c);char b[65536];while(f){f.read(b,sizeof b);if(f.gcount())CC_SHA256_Update(&c,b,CC_LONG(f.gcount()));}if(!f.eof())throw std::runtime_error("hash input read failed");unsigned char out[32];CC_SHA256_Final(out,&c);return hex(out,32);}
struct FD{int fd=-1;FD()=default;FD(const FD&)=delete;FD&operator=(const FD&)=delete;~FD(){if(fd>=0)::close(fd);}void open_read(const char*p,size_t bytes){if(!p||*p!='/')throw std::runtime_error("absolute bank/noise path required");fd=::open(p,O_RDONLY);struct stat s{};if(fd<0||fstat(fd,&s)||!S_ISREG(s.st_mode)||uint64_t(s.st_size)!=bytes)throw std::runtime_error("bank/noise exact byte size mismatch: "+std::string(p));}void create(const std::string&p,size_t bytes){fd=::open(p.c_str(),O_CREAT|O_EXCL|O_RDWR,0600);if(fd<0||ftruncate(fd,off_t(bytes)))throw std::runtime_error("create output bank failed");}};
struct RunOwner {
 FD lowbank,highbank,nlow,nhigh;std::array<FD,4>files;
 std::array<std::vector<float>,4>output;
 std::unique_ptr<DenseS1Pass> runtime;
};
void retain_owner_noexcept(std::unique_ptr<RunOwner>&owner)noexcept{failed_process.store(true,std::memory_order_release);if(RunOwner*p=owner.get()){retained_owner.store(p,std::memory_order_release);(void)owner.release();}}
void readrow(int fd,void*p,size_t n,size_t off){auto*b=static_cast<uint8_t*>(p);while(n){ssize_t z=pread(fd,b,n,off_t(off));if(z<=0)throw std::runtime_error("short immutable bank read");b+=z;n-=size_t(z);off+=size_t(z);}}
void writerow(int fd,const void*p,size_t n,size_t off){auto*b=static_cast<const uint8_t*>(p);while(n){ssize_t z=pwrite(fd,b,n,off_t(off));if(z<=0)throw std::runtime_error("short output bank write");b+=z;n-=size_t(z);off+=size_t(z);}}
struct Env{struct Item{std::string key,old;bool existed;};std::vector<Item>items;void set(const char*k,const char*v){const char*old=getenv(k);items.push_back({k,old?old:"",old!=nullptr});if(setenv(k,v,1))throw std::runtime_error("set model environment failed");}~Env(){for(auto it=items.rbegin();it!=items.rend();++it){if(it->existed)setenv(it->key.c_str(),it->old.c_str(),1);else unsetenv(it->key.c_str());}}};
struct SamplerEnd{~SamplerEnd(){pw_s1_memory_stop();}};
void callback(const reconstruction_continuous_mvs_request&r,int phase,int pass,int ref,double elapsed){if(!r.progress)return;int action=r.progress(r.user,phase,pass,ref,int(r.count),elapsed);while(action==2){std::this_thread::sleep_for(std::chrono::milliseconds(100));action=r.progress(r.user,4,pass,ref,int(r.count),elapsed);}if(action==1)throw Stop{};if(action!=0)throw std::runtime_error("invalid progress callback action");}
void manifest(const std::string&root,const std::string&inputs,const char*status,int completed,const std::string&error){std::ofstream f(root+"/manifest.tmp",std::ios::binary|std::ios::trunc);f<<"{\"schema\":1,\"scope\":\"MVS_ONLY_LOW_THEN_HIGH\",\"not_full13\":true,\"status\":"<<q(status)<<",\"completed_reference_runs\":"<<completed<<",\"memory_limits_bytes\":"<<reconstruction_memory_policy::limit()<<",\"sampler_ms\":100,\"max_pending_dispatches\":16,\"max_inflight_submits\":"<<matched_fifo_mode<<",\"OS_cap_guarantee\":false,\"error\":"<<q(error)<<",\"inputs\":"<<inputs<<",\"memory\":"<<pw_s1_memory_json()<<"}\n";f.close();if(!f||::rename((root+"/manifest.tmp").c_str(),(root+"/manifest.json").c_str()))throw std::runtime_error("manifest commit failed");}
}
extern "C" __attribute__((visibility("default"))) uint32_t reconstruction_continuous_mvs_abi(){return 1;}
extern "C" __attribute__((visibility("default"))) int reconstruction_continuous_mvs_run(const reconstruction_continuous_mvs_request*r,char*error,size_t cap){
 if(error&&cap)*error=0;
 EntryGuard entry;bool expected=false;
 if(!entry_active.compare_exchange_strong(expected,true,std::memory_order_acq_rel)){error_noexcept(error,cap,"continuous MVS already running");return 1;}entry.owns=true;
 if(failed_process.load(std::memory_order_acquire)||retained_owner.load(std::memory_order_acquire)){error_noexcept(error,cap,"failed process retains MVS resources; no retry before process exit");return 1;}
 std::unique_ptr<RunOwner>owner;
 try{owner=std::make_unique<RunOwner>();std::string root,inputs="{}";int done=0;bool created=false;
 matched_fifo_mode=parse_matched_fifo(std::getenv("RECONSTRUCTION_MATCHED_FIFO_MODE"));
 const char* mr=std::getenv("RECONSTRUCTION_MATCHED_MVS_ROUTE");if(mr&&std::string(mr)!="whole"&&std::string(mr)!="segmented")throw std::runtime_error("RECONSTRUCTION_MATCHED_MVS_ROUTE must be whole or segmented");
 if(!r||r->abi_version!=1||r->struct_bytes!=sizeof(*r)||r->reserved||!reconstruction::actual_frame_count_supported(r->count)||!r->cameras_low_f32||!r->neighbors_i32||!r->output_root||*r->output_root!='/')throw std::runtime_error("ABI1 requires positive actual-N count, exact struct, tables and fresh absolute output");
 const size_t count=r->count;constexpr size_t lo=768*576,hi=1536*1152;const bool banks=r->images_low_f16&&r->images_high_f16;if(bool(r->images_low_f16)!=bool(r->images_high_f16)||(!banks&&!r->original_jpeg_paths))throw std::runtime_error("supply both immutable half banks or original JPEG list");
 std::vector<float>cams(r->cameras_low_f32,r->cameras_low_f32+count*36);std::vector<int32_t>neighbors(r->neighbors_i32,r->neighbors_i32+count*9);for(float v:cams)if(!std::isfinite(v))throw std::runtime_error("nonfinite camera table");
 std::vector<int>orig(count);std::iota(orig.begin(),orig.end(),0);std::array<const char*,4>models{r->feature_low,r->rest_low,r->feature_high,r->rest_high};std::ostringstream in;in<<"{\"count\":"<<count<<",\"camera_low_sha256\":"<<q(hashmem(cams.data(),cams.size()*4))<<",\"neighbors_sha256\":"<<q(hashmem(neighbors.data(),neighbors.size()*4))<<",\"models\":[";for(int k=0;k<4;++k){if(k)in<<',';in<<"{\"path\":"<<q(models[k]?models[k]:"")<<",\"sha256\":"<<q(hashfile(models[k]))<<'}';}in<<"],\"images\":[";
 auto&lowbank=owner->lowbank;auto&highbank=owner->highbank;auto&nlow=owner->nlow;auto&nhigh=owner->nhigh;nlow.open_read(r->noise_low_f32,count*(lo/16+lo/4)*4);nhigh.open_read(r->noise_high_f32,count*(hi/16+hi/4)*4);
 if(banks){lowbank.open_read(r->images_low_f16,count*lo*2);highbank.open_read(r->images_high_f16,count*hi*2);in<<"{\"path\":"<<q(r->images_low_f16)<<",\"sha256\":"<<q(hashfile(r->images_low_f16))<<"},{\"path\":"<<q(r->images_high_f16)<<",\"sha256\":"<<q(hashfile(r->images_high_f16))<<'}';}else for(size_t i=0;i<count;++i){if(i)in<<',';in<<"{\"path\":"<<q(r->original_jpeg_paths[i]?r->original_jpeg_paths[i]:"")<<",\"sha256\":"<<q(hashfile(r->original_jpeg_paths[i]))<<'}';}
 in<<"],\"noise_low_sha256\":"<<q(hashfile(r->noise_low_f32))<<",\"noise_high_sha256\":"<<q(hashfile(r->noise_high_f32))<<",\"image_route\":"<<q(banks?"EXISTING_F16_BANKS_NO_REENCODE":"ORIGINAL_JPEG_PILLOW_CHAIN")<<'}';inputs=in.str();
 root=r->output_root;if(::mkdir(root.c_str(),0700))throw std::runtime_error("output root must be fresh");created=true;manifest(root,inputs,"PREPARING",done,"");Env env;const char*names[]={"RECONSTRUCTION_FULL13_S1_FEATURE_LOW","RECONSTRUCTION_FULL13_S1_REST_LOW","RECONSTRUCTION_FULL13_S1_FEATURE_HIGH","RECONSTRUCTION_FULL13_S1_REST_HIGH"};for(int k=0;k<4;++k)env.set(names[k],models[k]);env.set("RECONSTRUCTION_BENCH_MAX_INFLIGHT_SUBMITS",matched_fifo_mode==4?"4":"1");SamplerEnd sampler;
 std::ofstream records(root+"/refs.jsonl",std::ios::binary|std::ios::app);if(!records)throw std::runtime_error("open ref receipts failed");
 for(int pass=0;pass<2;++pass){const int width=pass?1536:768;const size_t pixels=pass?hi:lo;const std::string dir=root+(pass?"/high":"/low");if(::mkdir(dir.c_str(),0700))throw std::runtime_error("create pass output failed");auto&files=owner->files;const char*plane[]={"depth","conf0","conf1","conf2"};for(int k=0;k<4;++k)files[k].create(dir+"/"+plane[k]+".f32",count*pixels*4);
 std::vector<float>pcams=cams;if(pass)for(size_t i=0;i<count;++i){float*c=&pcams[i*36];for(int k:{0,2,4,5})c[k]*=2.0f;c[26]=1536.0f;c[27]=1152.0f;}
 auto read_half=[r,pass,pixels,bankfd=(pass?highbank.fd:lowbank.fd),banks](int view){std::vector<uint16_t>out;if(banks){out.resize(pixels);readrow(bankfd,out.data(),pixels*2,size_t(view)*pixels*2);return out;}
#if defined(__APPLE__)
aether::dense::FrameSource fs;fs.kind=aether::dense::FrameSourceKind::Jpeg;fs.path=r->original_jpeg_paths[view];aether::dense::RgbImage src,rs;std::vector<uint8_t>gray;std::string e;if(!aether::dense::load_frame_rgb(fs,src,&e))throw std::runtime_error(e);if(!pass){aether::dense::pil_resize_bilinear_rgb(src,768,576,rs);aether::dense::pil_rgb_to_l(rs,gray);aether::dense::gray_to_f16(gray,out);}else{std::vector<uint8_t>lf,lh;aether::dense::pil_rgb_to_l(src,lf);aether::dense::pil_resize_bilinear_l(lf,src.w,src.h,1536,1152,lh);aether::dense::gray_to_f16(lh,out);}return out;
#else
throw std::runtime_error("Linux ABI diagnostic requires original F16 image banks; JPEG route is Apple only");
#endif
};
 {owner->runtime=std::make_unique<DenseS1Pass>();auto&runtime=*owner->runtime;std::string e;double init=0;if(!runtime.Initialize(width,pcams,neighbors,orig,pass?nhigh.fd:nlow.fd,read_half,&e,&init))throw std::runtime_error("pass initialize: "+e);if(pw_s1_memory_start())throw std::runtime_error("100ms sampler restart failed");callback(*r,5,pass,-1,init);manifest(root,inputs,"RUNNING",done,"");char before_output[1024];if(pw_s1_memory_preflight("CONTINUOUS_OUTPUT_HOST_ALLOC",pixels*16,before_output,sizeof before_output))throw std::runtime_error(before_output);auto&output=owner->output;float*ptr[4];for(int k=0;k<4;++k){output[k].resize(pixels);ptr[k]=output[k].data();}
 for(size_t ref=0;ref<count;++ref){callback(*r,1,pass,int(ref),0);char guard[1024];if(pw_s1_memory_preflight(pass?"CONTINUOUS_HIGH_REF_ENTER":"CONTINUOUS_LOW_REF_ENTER",0,guard,sizeof guard))throw std::runtime_error(guard);double elapsed=0;const auto begin=Clock::now();if(!runtime.Run(int(ref),ptr,pixels,&elapsed,&e))throw std::runtime_error("ref "+std::to_string(ref)+": "+e);std::array<std::string,4>hashes;for(int k=0;k<4;++k){for(float value:output[k])if(!std::isfinite(value))throw std::runtime_error("nonfinite reference output");hashes[k]=hashmem(ptr[k],pixels*4);writerow(files[k].fd,ptr[k],pixels*4,ref*pixels*4);}++done;records<<"{\"pass\":"<<pass<<",\"reference\":"<<ref<<",\"finite\":true,\"elements_per_plane\":"<<pixels<<",\"infer_ms\":"<<std::setprecision(16)<<elapsed<<",\"infer_hash_write_ms\":"<<std::chrono::duration<double,std::milli>(Clock::now()-begin).count()<<",\"sha256\":[";for(int k=0;k<4;++k){if(k)records<<',';records<<q(hashes[k]);}auto c=runtime.Counters();records<<"],\"rest_runs\":"<<c.rest_runs<<",\"leases_released\":"<<c.leases_released<<",\"feature_cache\":"<<c.cached_feature_views<<",\"aux_cache\":"<<c.cached_reference_aux<<",\"completion_waits\":"<<c.completion_waits<<",\"material_hits\":"<<c.material_hits<<",\"material_misses\":"<<c.material_misses<<",\"material_evictions\":"<<c.material_evictions<<",\"material_reads\":"<<c.material_reads<<",\"material_read_B\":"<<c.material_read_B<<",\"material_uploads\":"<<c.material_uploads<<",\"material_staging_peak_B\":"<<c.material_staging_peak_B<<",\"material_gpu_peak_B\":"<<c.material_gpu_peak_B<<",\"feature_evaluations\":"<<c.feature_evaluations<<"}\n";records.flush();if(!records)throw std::runtime_error("reference receipt write failed");callback(*r,2,pass,int(ref),elapsed);}
 auto c=runtime.Counters();if(c.rest_runs!=count||c.leases_released!=count||c.cached_feature_views>10||c.cached_reference_aux)throw std::runtime_error("continuous run/lease/cache count mismatch");owner->runtime.reset();}
 for(auto&f:files){if(fsync(f.fd))throw std::runtime_error("pass bank fsync failed");if(close(f.fd))throw std::runtime_error("pass bank close failed");f.fd=-1;}char after_destroy[1024];if(pw_s1_memory_preflight(pass?"CONTINUOUS_HIGH_DESTROYED":"CONTINUOUS_LOW_DESTROYED",0,after_destroy,sizeof after_destroy))throw std::runtime_error(after_destroy);callback(*r,3,pass,-1,0);manifest(root,inputs,pass?"MVS_ONLY_COMPLETED":"LOW_COMPLETED_HIGH_PENDING",done,"");}
 return 0;
 }catch(const Stop&){retain_owner_noexcept(owner);error_noexcept(error,cap,"STOPPED_BY_CALLBACK_MVS_ONLY_PARTIAL; resources retained until process exit");return 2;}catch(const std::exception&e){retain_owner_noexcept(owner);error_noexcept(error,cap,e.what());return 1;}catch(...){retain_owner_noexcept(owner);error_noexcept(error,cap,"unknown continuous MVS failure; resources retained until process exit");return 1;}}


extern "C" int reconstruction_continuous_mvs_frame_count_supported_v1(uint32_t count){return reconstruction::actual_frame_count_supported(count)?1:0;}
