#pragma once
#include "material_bank.h"
#include <sys/mman.h>
#include <utility>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
namespace pw_host_pack {
class MappedHalfBlob {
 void* base_=MAP_FAILED;size_t map_bytes_=0,bytes_=0,prefix_=0;int fd_=-1;struct stat opened_{};std::string sha_;
 static bool SameStat(const struct stat&a,const struct stat&b){
  if(a.st_ino!=b.st_ino||a.st_dev!=b.st_dev||a.st_size!=b.st_size||a.st_mode!=b.st_mode)return false;
#if defined(__APPLE__)
  return a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec&&a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec&&a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
  return a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
 }
 public:
 MappedHalfBlob()=default;
 MappedHalfBlob(const MappedHalfBlob&)=delete;
 MappedHalfBlob&operator=(const MappedHalfBlob&)=delete;
 MappedHalfBlob(MappedHalfBlob&&x)noexcept:base_(std::exchange(x.base_,MAP_FAILED)),map_bytes_(std::exchange(x.map_bytes_,0)),bytes_(std::exchange(x.bytes_,0)),prefix_(std::exchange(x.prefix_,0)),fd_(std::exchange(x.fd_,-1)),opened_(x.opened_),sha_(std::move(x.sha_)){}
 ~MappedHalfBlob(){if(base_!=MAP_FAILED)::munmap(base_,map_bytes_);if(fd_>=0)::close(fd_);}
 static MappedHalfBlob Open(const reconstruction::material::Blob&b){
  reconstruction::material::check_readonly_stat(b);
  if(!b.bytes||b.bytes>64*1024*1024ULL)throw std::runtime_error("bounded nonempty mapped F16 blob");
  MappedHalfBlob x;x.fd_=::open(b.path.c_str(),O_RDONLY|O_NOFOLLOW);
  if(x.fd_<0)throw std::runtime_error("readonly mapped blob open");
  struct stat before{},after{};
  if(::fstat(x.fd_,&before)||(fcntl(x.fd_,F_GETFL)&O_ACCMODE)!=O_RDONLY)throw std::runtime_error("mapped blob FD");
  // The sealed bank is immutable. Validate the opened inode's shape/permissions too.
  if(!S_ISREG(before.st_mode)||before.st_size<0||size_t(before.st_size)!=(b.total_bytes?b.total_bytes:b.bytes)||(before.st_mode&0222))throw std::runtime_error("mapped opened blob identity");
  long page=::sysconf(_SC_PAGESIZE);if(page<=0)throw std::runtime_error("mapped page size");
  x.prefix_=b.offset%size_t(page);if(b.bytes>std::numeric_limits<size_t>::max()-x.prefix_)throw std::runtime_error("mapped size overflow");
  x.map_bytes_=x.prefix_+b.bytes;x.bytes_=b.bytes;x.opened_=before;x.sha_=b.sha;
  x.base_=::mmap(nullptr,x.map_bytes_,PROT_READ,MAP_PRIVATE,x.fd_,off_t(b.offset-x.prefix_));
  if(x.base_==MAP_FAILED)throw std::runtime_error("readonly material mmap failed");
  if(::fstat(x.fd_,&after)||!SameStat(before,after))throw std::runtime_error("mapped blob changed during read");
  if(reconstruction::material::sha_bytes(x.data(),b.bytes)!=b.sha)throw std::runtime_error("mapped blob SHA mismatch");
  for(uint16_t v:x)if((v&0x7c00u)==0x7c00u)throw std::runtime_error("mapped blob nonfinite F16");
  return x;
 }
 void ValidateAfterUpload()const{struct stat now{};if(::fstat(fd_,&now)||!SameStat(opened_,now)||reconstruction::material::sha_bytes(data(),bytes_)!=sha_)throw std::runtime_error("mapped blob changed before upload returned");}
 const uint16_t* data()const{return reinterpret_cast<const uint16_t*>(static_cast<const unsigned char*>(base_)+prefix_);}
 size_t size()const{return bytes_/2;}
 const uint16_t* begin()const{return data();}const uint16_t*end()const{return data()+size();}
};
class HostMaterialHalf {
 std::vector<uint16_t> copied_;std::unique_ptr<MappedHalfBlob> mapped_;
 public:
 HostMaterialHalf(const reconstruction::material::Blob&b,int frames,int width){
  const char*value=std::getenv("RECONSTRUCTION_BENCH_HOST_MATERIAL_MMAP");
  if(value&&std::strcmp(value,"0")&&std::strcmp(value,"1"))throw std::runtime_error("host material mmap must be0/1");
  const bool selected=value&&std::strcmp(value,"1")==0&&frames==36&&width==1536;
  if(selected)mapped_=std::make_unique<MappedHalfBlob>(MappedHalfBlob::Open(b));else copied_=reconstruction::material::read_half_blob(b);
 }
 const uint16_t*data()const{return mapped_?mapped_->data():copied_.data();}
 size_t size()const{return mapped_?mapped_->size():copied_.size();}
 void ValidateAfterUpload()const{if(mapped_)mapped_->ValidateAfterUpload();}
 bool mapped()const{return bool(mapped_);}
};
}
