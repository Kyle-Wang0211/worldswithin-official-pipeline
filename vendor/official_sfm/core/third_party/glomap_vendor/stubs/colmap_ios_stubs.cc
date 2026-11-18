                                                                                  
                                                                    
  
                                                                                   
                                                                                
                                                                               
                                                                                 
                                                                             
                                                                                 
                                                                             
                                                                    
                                                                                 
                                                                                  

#include "colmap/sensor/bitmap.h"
#include "colmap/feature/index.h"
#include "colmap/feature/aliked.h"                                                 
#include "colmap/feature/onnx_matchers.h"                                            

#include <cstring>
#include <memory>
#include <ostream>

namespace colmap {

namespace {
inline int ChannelsForRGB(bool as_rgb) { return as_rgb ? 3 : 1; }
}              

                                                                               
Bitmap::Bitmap()
    : width_(0), height_(0), channels_(0), linear_colorspace_(false) {}

Bitmap::Bitmap(int width, int height, bool as_rgb, bool linear_colorspace)
    : width_(width),
      height_(height),
      channels_(ChannelsForRGB(as_rgb)),
      linear_colorspace_(linear_colorspace),
      data_(static_cast<size_t>(width) * height * ChannelsForRGB(as_rgb), 0) {}

Bitmap::Bitmap(const Bitmap& other)
    : width_(other.width_),
      height_(other.height_),
      channels_(other.channels_),
      linear_colorspace_(other.linear_colorspace_),
      data_(other.data_) {}

Bitmap::Bitmap(Bitmap&& other) noexcept
    : width_(other.width_),
      height_(other.height_),
      channels_(other.channels_),
      linear_colorspace_(other.linear_colorspace_),
      data_(std::move(other.data_)) {
  other.width_ = 0;
  other.height_ = 0;
  other.channels_ = 0;
}

Bitmap& Bitmap::operator=(const Bitmap& other) {
  if (this != &other) {
    width_ = other.width_;
    height_ = other.height_;
    channels_ = other.channels_;
    linear_colorspace_ = other.linear_colorspace_;
    data_ = other.data_;
  }
  return *this;
}

Bitmap& Bitmap::operator=(Bitmap&& other) noexcept {
  if (this != &other) {
    width_ = other.width_;
    height_ = other.height_;
    channels_ = other.channels_;
    linear_colorspace_ = other.linear_colorspace_;
    data_ = std::move(other.data_);
    other.width_ = 0;
    other.height_ = 0;
    other.channels_ = 0;
  }
  return *this;
}

                                                                               
void Bitmap::Fill(const BitmapColor<uint8_t>& color) {
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      SetPixel(x, y, color);
    }
  }
}

                                                                             
                                                                        
std::optional<BitmapColor<uint8_t>> Bitmap::InterpolateNearestNeighbor(
    double x, double y) const {
  return GetPixel(static_cast<int>(std::lround(x)),
                  static_cast<int>(std::lround(y)));
}

std::optional<BitmapColor<float>> Bitmap::InterpolateBilinear(
    double      , double      ) const {
  return std::nullopt;
}

                                                                               
std::optional<int> Bitmap::ExifOrientation() const { return std::nullopt; }
std::optional<std::string> Bitmap::ExifCameraModel() const {
  return std::nullopt;
}
std::optional<double> Bitmap::ExifFocalLength() const { return std::nullopt; }
std::optional<double> Bitmap::ExifLatitude() const { return std::nullopt; }
std::optional<double> Bitmap::ExifLongitude() const { return std::nullopt; }
std::optional<double> Bitmap::ExifAltitude() const { return std::nullopt; }

                                                                               
bool Bitmap::Read(const std::filesystem::path&         ,
                  bool           ,
                  bool                         ) {
  return false;
}

bool Bitmap::Write(const std::filesystem::path&         ,
                   bool                           ) const {
  return false;
}

                                                                               
void Bitmap::Rescale(int              ,
                     int               ,
                     RescaleFilter           ) {}

                                                                                  
                                                                               
double Bitmap::Thumbnail(int                   , RescaleFilter           ) {
  return 1.0;
}

void Bitmap::Rot90(int      ) {}

Bitmap Bitmap::Clone() const { return *this; }

Bitmap Bitmap::CloneAsGrey() const {
  Bitmap grey;
  grey.width_ = width_;
  grey.height_ = height_;
  grey.channels_ = 1;
  grey.linear_colorspace_ = linear_colorspace_;
  grey.data_.assign(static_cast<size_t>(width_) * height_, 0);
  return grey;
}

Bitmap Bitmap::CloneAsRGB() const {
  Bitmap rgb;
  rgb.width_ = width_;
  rgb.height_ = height_;
  rgb.channels_ = 3;
  rgb.linear_colorspace_ = linear_colorspace_;
  rgb.data_.assign(static_cast<size_t>(width_) * height_ * 3, 0);
  return rgb;
}

void Bitmap::SetJpegQuality(int            ) {}

                                                                               
void Bitmap::SetMetaData(const std::string_view&         ,
                         const std::string_view&         ,
                         const void*          ) {}

void Bitmap::SetMetaData(const std::string_view&         ,
                         const std::string_view&          ) {}

bool Bitmap::GetMetaData(const std::string_view&         ,
                         const std::string_view&         ,
                         void*          ) const {
  return false;
}

std::optional<std::string> Bitmap::GetMetaData(
    const std::string_view&         ) const {
  return std::nullopt;
}

void Bitmap::CloneMetadata(Bitmap*           ) const {}

std::ostream& operator<<(std::ostream& stream, const Bitmap& bitmap) {
  stream << "Bitmap(width=" << bitmap.Width() << ", height=" << bitmap.Height()
         << ", channels=" << bitmap.Channels() << ")";
  return stream;
}

                                                                              
                                                                                 
                                                                 
std::unique_ptr<FeatureDescriptorIndex> FeatureDescriptorIndex::Create(
    FeatureDescriptorIndex::Type         , int                ) {
  return nullptr;
}

                                                                                
                                                                               
                                                                            
                                                                                  
                                                                               
                                                                                   
                                                                                 
                                                                                  
std::unique_ptr<FeatureExtractor> CreateAlikedFeatureExtractor(
    const FeatureExtractionOptions&            ) {
  return nullptr;
}

std::unique_ptr<FeatureMatcher> CreateAlikedFeatureMatcher(
    const FeatureMatchingOptions&            ) {
  return nullptr;
}

std::unique_ptr<FeatureMatcher> CreateLightGlueONNXFeatureMatcher(
    const FeatureMatchingOptions&            ,
    const LightGlueONNXMatchingOptions&                      ) {
  return nullptr;
}

bool AlikedExtractionOptions::Check() const { return true; }
bool AlikedMatchingOptions::Check() const { return true; }
bool LightGlueONNXMatchingOptions::Check() const { return true; }

}                     
