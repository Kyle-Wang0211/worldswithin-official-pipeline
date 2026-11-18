
// weld identical positions (meshopt_generateVertexRemap), simplify with position update (options 0 = "safe default"), compact
// (meshopt_optimizeVertexFetch). No attributes, no vertex lock, no Prune (small objects must survive), no error cap (face target rules).
// I/O: raw little-endian files -- in: <pos.f32> <idx.u32> ; out: <pos.f32> <idx.u32>. Usage: mo_simplify in_pos in_idx target_tris out_pos out_idx
#include "meshoptimizer.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
template <class T> static std::vector<T> readAll(const char* p) { FILE* f = fopen(p, "rb"); fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  std::vector<T> v(n / sizeof(T)); fread(v.data(), sizeof(T), v.size(), f); fclose(f); return v; }
template <class T> static void writeAll(const char* p, const T* d, size_t n) { FILE* f = fopen(p, "wb"); fwrite(d, sizeof(T), n, f); fclose(f); }
int main(int argc, char** argv) {
  if (argc != 6) { fprintf(stderr, "usage\n"); return 1; }
  auto t0 = std::chrono::steady_clock::now();
  std::vector<float> pos = readAll<float>(argv[1]); std::vector<unsigned int> idx = readAll<unsigned int>(argv[2]);
  size_t nv = pos.size() / 3, target = (size_t)atol(argv[3]) * 3;
  std::vector<unsigned int> remap(nv);
  size_t uv = meshopt_generateVertexRemap(remap.data(), idx.data(), idx.size(), pos.data(), nv, sizeof(float) * 3);
  std::vector<float> wpos(uv * 3); std::vector<unsigned int> widx(idx.size());
  meshopt_remapVertexBuffer(wpos.data(), pos.data(), nv, sizeof(float) * 3, remap.data());
  meshopt_remapIndexBuffer(widx.data(), idx.data(), idx.size(), remap.data());
  float err = 0.f;
  size_t ni = meshopt_simplifyWithUpdate(widx.data(), widx.size(), wpos.data(), uv, sizeof(float) * 3, nullptr, 0, nullptr, 0, nullptr, target, 1.0f, 0, &err);
  widx.resize(ni);
  std::vector<float> opos(uv * 3);
  size_t ov = meshopt_optimizeVertexFetch(opos.data(), widx.data(), ni, wpos.data(), uv, sizeof(float) * 3);
  writeAll(argv[4], opos.data(), ov * 3); writeAll(argv[5], widx.data(), ni);
  double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  printf("in %zu v / %zu tris, welded %zu v -> out %zu v / %zu tris, relative error %.6f, %.2f s\n", nv, idx.size() / 3, uv, ov, ni / 3, err, s);
  return 0;
}
