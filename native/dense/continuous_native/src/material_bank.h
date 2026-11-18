#include "actual_frame_count.h"
#pragma once
#include <set>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <array>
#include <vector>
#include <string>
#include <unistd.h>
#include "portable_sha.h"
#include <filesystem>
#include <fcntl.h>
#include <sys/stat.h>
#include <cctype>
namespace reconstruction::material {
std::string sha_bytes(const void* data,size_t n){unsigned char h[32];CC_SHA256_CTX ctx;CC_SHA256_Init(&ctx);CC_SHA256_Update(&ctx,data,CC_LONG(n));CC_SHA256_Final(h,&ctx);std::string s;const char* d="0123456789abcdef";for(auto v:h){s+=d[v>>4];s+=d[v&15];}return s;}
bool is_sha(const std::string& s){return s.size()==64&&std::all_of(s.begin(),s.end(),[](unsigned char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
struct Blob {std::string path,sha;size_t bytes=0,offset=0,total_bytes=0;};
struct BankRow {int id=-1;std::string photo,key,gray,producer;std::array<Blob,3> levels;};
struct BankIndex {int width=0;std::string model,preprocess,manifest;std::vector<BankRow> rows;std::string format,layout;};
std::vector<std::string> fields(const std::string& s){std::vector<std::string> v;std::istringstream in(s);std::string x;while(std::getline(in,x,'\t'))v.push_back(x);return v;}
std::string expected_feature_model(int w){if(w==768)return "ac6bc8f7822c735191e50038e6b0ac9c4b57cbce82faf1f778e217d361027593";if(w==1536)return "376e6c36db0064adae367db87f35b3ef9c6a187e0e36c18a818e3ab7942a0683";throw std::runtime_error("bank width");}
size_t parse_size(const std::string& s){if(s.empty()||!std::all_of(s.begin(),s.end(),[](unsigned char c){return c>='0'&&c<='9';}))throw std::runtime_error("strict decimal bytes");size_t used=0;auto x=std::stoull(s,&used);if(used!=s.size()||x>64*1024*1024ULL)throw std::runtime_error("bounded blobbytes");return size_t(x);}
void check_readonly_stat(const Blob& b){struct stat st{};if(std::filesystem::canonical(b.path).string()!=b.path)throw std::runtime_error("material path alias");if(b.path.empty()||b.path[0]!='/'||::lstat(b.path.c_str(),&st)||!S_ISREG(st.st_mode)||st.st_size<0||size_t(st.st_size)!=(b.total_bytes?b.total_bytes:b.bytes)||(st.st_mode&0222)||!is_sha(b.sha)||b.bytes%2||b.offset%2||b.offset>(b.total_bytes?b.total_bytes:b.bytes)||b.bytes>(b.total_bytes?b.total_bytes:b.bytes)-b.offset)throw std::runtime_error("unsealed/wrongsize/dtype/blobpath");}
BankIndex read_bank_index(const std::string& path,int w,const std::string& preprocess,const std::string& manifest,int frames,const std::string& model,const std::string& root){std::ifstream in(path);std::string line;if(!std::getline(in,line))throw std::runtime_error("index open/header");auto h=fields(line);if(!reconstruction::actual_frame_count_supported(frames)||(w!=768&&w!=1536)||root.empty()||root[0]!='/'||std::filesystem::canonical(root).string()!=root)throw std::runtime_error("index scene/root identity");
 std::string format,layout;
 if(h.size()==7&&h[0]=="BANKV3"){format="BANKV3";layout=h[6];if(layout!="objects_key_v2"&&layout!="legacy_frame_v1")throw std::runtime_error("unknown explicit material layout");}
 else if(h.size()==6&&h[0]=="BANKV2"){format="BANKV2_compat";layout=frames==20?"objects_key_v2":"legacy_frame_v1";}
 else throw std::runtime_error("material format discriminator");
 struct stat indexStat{};if(std::filesystem::canonical(path).string()!=path||lstat(path.c_str(),&indexStat)||!S_ISREG(indexStat.st_mode)||(indexStat.st_mode&0222)||indexStat.st_size<=0||indexStat.st_size>256*1024)throw std::runtime_error("material index sealed/canonical/bounded");
 if(h[1]!=std::to_string(w)||h[2]!=model||h[5]!=std::to_string(frames)||h[3]!=preprocess||h[4]!=manifest||!is_sha(h[3])||!is_sha(h[4]))throw std::runtime_error("index model/preprocess/manifest identity");BankIndex b;b.width=w;b.model=h[2];b.preprocess=h[3];b.manifest=h[4];b.format=format;b.layout=layout;b.rows.resize(frames);std::set<int> ids;std::set<std::string> keys;size_t count=0;const size_t lowB[3]={663552,1769472,3538944};
while(std::getline(in,line)){auto v=fields(line);if(v.size()!=19)throw std::runtime_error("index columns");int id=int(parse_size(v[0]));if(id<0||id>=frames||!ids.insert(id).second||id!=int(count)||!is_sha(v[1]))throw std::runtime_error("index original97 frame/photo order");auto& r=b.rows[id];r.id=id;r.photo=v[1];r.gray=v[18];r.producer=v[17];if(!is_sha(v[17])||!is_sha(v[18]))throw std::runtime_error("producer cachekey/gray identity");r.key=r.photo+":"+b.model+":"+b.preprocess+":"+v[18]+":"+v[17];std::string canonicalIdentity="{\"model_sha256\":\""+b.model+"\",\"photo_sha256\":\""+r.photo+"\",\"prepared_gray_sha256\":\""+r.gray+"\",\"preprocess_sha256\":\""+b.preprocess+"\"}";if(sha_bytes(canonicalIdentity.data(),canonicalIdentity.size())!=r.producer)throw std::runtime_error("producer cachekey photo/model/preprocess/gray identity mismatch");if(!keys.insert(r.key).second)throw std::runtime_error("cache key collision");for(int l=0;l<3;++l){r.levels[l]={v[2+l*5],v[3+l*5],parse_size(v[5+l*5]),parse_size(v[4+l*5]),parse_size(v[6+l*5])};if(r.levels[l].bytes!=lowB[l]*(w==1536?4:1))throw std::runtime_error("bank levelshape/bytes");size_t expectedOffset=0;for(int k=0;k<l;++k)expectedOffset+=lowB[k]*(w==1536?4:1);if(r.levels[l].offset!=expectedOffset||r.levels[l].total_bytes!=(lowB[0]+lowB[1]+lowB[2])*(w==1536?4:1))throw std::runtime_error("packed offset/totalbytes");const auto expectedPath=layout=="objects_key_v2"?root+"/objects/"+r.producer+"/features.f16":root+"/frame_"+std::to_string(id)+".f16";if(r.levels[l].path!=expectedPath)throw std::runtime_error("material explicit layout/path confinement/partial");check_readonly_stat(r.levels[l]);}++count;}
if(count!=size_t(frames))throw std::runtime_error("reference_capture index required");return b;}
std::vector<uint16_t> read_half_blob(const Blob& b){check_readonly_stat(b);int fd=::open(b.path.c_str(),O_RDONLY|O_NOFOLLOW);if(fd<0)throw std::runtime_error("readonly blob open");struct Close{int fd;~Close(){::close(fd);}}close{fd};struct stat before{},after{};if(::fstat(fd,&before)||(fcntl(fd,F_GETFL)&O_ACCMODE)!=O_RDONLY)throw std::runtime_error("blob FD");std::vector<uint16_t> bits(b.bytes/2);size_t at=0;while(at<b.bytes){ssize_t n=::pread(fd,reinterpret_cast<unsigned char*>(bits.data())+at,b.bytes-at,b.offset+at);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("blob truncated/read");at+=size_t(n);}if(::fstat(fd,&after)||before.st_ino!=after.st_ino||before.st_dev!=after.st_dev||before.st_size!=after.st_size||before.st_mtime!=after.st_mtime)throw std::runtime_error("blob changed during read");if(sha_bytes(bits.data(),b.bytes)!=b.sha)throw std::runtime_error("blob SHA mismatch");for(uint16_t v:bits)if((v&0x7c00u)==0x7c00u)throw std::runtime_error("blob nonfinite F16");return bits;}

std::string file_sha(const std::string&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("SHA open");CC_SHA256_CTX ctx;CC_SHA256_Init(&ctx);char b[65536];while(f){f.read(b,sizeof b);if(f.gcount())CC_SHA256_Update(&ctx,b,CC_LONG(f.gcount()));}if(!f.eof())throw std::runtime_error("SHA read");unsigned char h[32];CC_SHA256_Final(h,&ctx);std::string out;for(auto v:h){out+="0123456789abcdef"[v>>4];out+="0123456789abcdef"[v&15];}return out;}
}
