#include "reconstruction_scene_images_original.h"
#include <opencv2/core.hpp>
#include <opencv2/core/version.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <CommonCrypto/CommonDigest.h>
#include <pthread.h>
#include <sys/stat.h>
#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

static_assert(CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR == 11 &&
    CV_VERSION_REVISION == 0, "Pinned original OpenCV4.11 headers required");
namespace {
thread_local char error_text[2048] = {};
bool digest_text(const char *s) {
  if (!s || std::strlen(s) != 64) return false;
  for (unsigned i = 0; i < 64; ++i)
    if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) return false;
  return true;
}
bool absolute(const char *s) { return s && s[0] == '/' && s[1]; }
void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
std::string file_sha(const char *path, bool jpeg = false) {
  struct stat st;
  require(lstat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0,
      "Expected nonempty regular immutable input file");
  FILE *f = std::fopen(path, "rb");
  require(f != nullptr, "Input file open failed");
  CC_SHA256_CTX ctx; CC_SHA256_Init(&ctx);
  unsigned char buffer[65536], digest[CC_SHA256_DIGEST_LENGTH];
  bool first = true, valid_jpeg = !jpeg;
  size_t n;
  while ((n = std::fread(buffer, 1, sizeof(buffer), f)) != 0) {
    if (first) { valid_jpeg = !jpeg || (n >= 2 && buffer[0] == 0xff && buffer[1] == 0xd8); first = false; }
    CC_SHA256_Update(&ctx, buffer, static_cast<CC_LONG>(n));
  }
  const bool ok = !std::ferror(f);
  const int close_rc = std::fclose(f);
  require(ok && close_rc == 0 && valid_jpeg, "Actual JPEG read/signature failed");
  CC_SHA256_Final(digest, &ctx);
  char out[65];
  for (unsigned i = 0; i < sizeof(digest); ++i) std::snprintf(out + i * 2, 3, "%02x", digest[i]);
  return std::string(out, 64);
}
int fail(int rc, const char *message) {
  std::snprintf(error_text, sizeof(error_text), "%s", message); return rc;
}
}  // namespace

extern "C" int reconstruction_scene_image_abi_version(void) { return RECONSTRUCTION_SCENE_IMAGE_ABI; }
extern "C" const char *reconstruction_scene_images_opencv_version(void) { return CV_VERSION; }
extern "C" const char *reconstruction_scene_images_build_information(void) {
  return cv::getBuildInformation().c_str();
}
extern "C" const char *reconstruction_scene_images_last_error(void) { return error_text; }
extern "C" int reconstruction_scene_images_prepare(const char *const *paths,
    const char *const *hashes, uint64_t count, const char *manifest,
    const char *manifest_hash, const char *output, uint64_t *completed) {
  error_text[0] = '\0';
  if (completed) *completed = 0;
  if (!paths || !hashes || count == 0 || count > SIZE_MAX / sizeof(*paths) || !completed || !absolute(manifest) ||
      !digest_text(manifest_hash) || !absolute(output) || pthread_main_np())
    return fail(22, "Representable positive actual-N frozen inputs, fresh output and worker thread required");
  for (uint64_t i = 0; i < count; ++i) {
    if (!absolute(paths[i]) || !digest_text(hashes[i])) return fail(22, "Invalid ordered JPEG path/hash");
    for (uint64_t j = 0; j < i; ++j)
      if (std::strcmp(paths[i], paths[j]) == 0) return fail(22, "Distinct ordered JPEG paths required");
  }
  try {
    require(std::strcmp(cv::getVersionString().c_str(), "4.11.0") == 0,
        "Linked original OpenCV4.11 runtime mismatch");
    require(file_sha(manifest) == manifest_hash, "Frozen source manifest changed");
    for (uint64_t i = 0; i < count; ++i)
      require(file_sha(paths[i], true) == hashes[i], "Frozen source JPEG changed");
    const std::string dir(output);
    require(dir.back() != '/' && dir.find("/../") == std::string::npos &&
        dir.find("/./") == std::string::npos, "Canonical fresh output path required");
    const std::string parent = dir.substr(0, dir.find_last_of('/'));
    struct stat st;
    require(!parent.empty() && lstat(parent.c_str(), &st) == 0 && S_ISDIR(st.st_mode),
        "Existing real output parent required");
    require(mkdir(output, 0700) == 0, "Exclusive fresh output directory creation failed");
    for (uint64_t i = 0; i < count; ++i) {
      // These three calls and flags are the recovered original1920 recipe.
      cv::Mat im = cv::imread(paths[i], cv::IMREAD_COLOR | cv::IMREAD_IGNORE_ORIENTATION);
      require(!im.empty() && im.type() == CV_8UC3, "Original JPEG decode failed");
      cv::Mat resized;
      cv::resize(im, resized, cv::Size(1920, 1440), 0.0, 0.0, cv::INTER_AREA);
      char name[32]; std::snprintf(name, sizeof(name), "%08" PRIu64 ".jpg", i);
      const std::string target = dir + "/" + name;
      require(cv::imwrite(target, resized, {cv::IMWRITE_JPEG_QUALITY, 95}),
          "Original JPEG95 encode/write failed");
      require(lstat(target.c_str(), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0,
          "Actual JPEG output missing");
      ++*completed;
      require(file_sha(paths[i], true) == hashes[i], "Source JPEG changed during native call");
    }
    require(file_sha(manifest) == manifest_hash, "Source manifest changed during native call");
    return 0;
  } catch (const std::exception &e) { return fail(1, e.what()); }
  catch (...) { return fail(1, "Unknown original OpenCV image producer failure"); }
}
