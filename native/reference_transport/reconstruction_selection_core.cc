#include "reconstruction_selection_core.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {
// Exact source span from177 dense_fuse_pack.h. Source/span SHA256 and oracle
// include are recorded in selection_core_identity.json. No numeric edits.
struct BoxFilter {
    double c[3], s[3], rot[9];
    bool contains(double wx, double wy, double wz) const {
        const double px = wx - c[0], py = wy - c[1], pz = wz - c[2];
        const double lx = rot[0] * px + rot[3] * py + rot[6] * pz;
        const double ly = rot[1] * px + rot[4] * py + rot[7] * pz;
        const double lz = rot[2] * px + rot[5] * py + rot[8] * pz;
        return std::fabs(lx) <= s[0] / 2 && std::fabs(ly) <= s[1] / 2 && std::fabs(lz) <= s[2] / 2;
    }
};

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
    "Existing world points require IEEE754 float32");
static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559,
    "Existing box transport requires IEEE754 binary64");

struct Span { uintptr_t begin = 0, end = 0; };
bool span(const void* pointer, uint64_t bytes, Span& result) {
    const auto begin = reinterpret_cast<uintptr_t>(pointer);
    if ((bytes != 0 && pointer == nullptr) ||
        bytes > std::numeric_limits<uintptr_t>::max() - begin) return false;
    result = {begin, begin + static_cast<uintptr_t>(bytes)};
    return true;
}
bool overlaps(const Span& a, const Span& b) {
    return a.begin != a.end && b.begin != b.end &&
        a.begin < b.end && b.begin < a.end;
}
void diagnostic(char* error, uint64_t capacity, const char* message) {
    if (capacity == 0) return;
    uint64_t i = 0;
    while (i + 1 < capacity && message[i] != '\0') {
        error[i] = message[i];
        ++i;
    }
    error[i] = '\0';
}
} // namespace

extern "C" int reconstruction_selection_contains_f32(const float* xyz, uint64_t count,
    const double* box15, uint8_t* out_mask, uint64_t mask_count,
    char* error, uint64_t error_capacity) {
    // Establish all address spans without reading or writing pointed-to memory.
    // Unknown/overflowing spans cannot safely receive diagnostics either.
    constexpr uint64_t count_limit = static_cast<uint64_t>(
        std::numeric_limits<std::ptrdiff_t>::max()) / (3 * sizeof(float));
    if (count > count_limit) return 1;
    Span points_span, box_span, mask_span, error_span;
    if (!span(xyz, count * 3 * sizeof(float), points_span) ||
        !span(box15, 15 * sizeof(double), box_span) ||
        !span(out_mask, mask_count, mask_span) ||
        !span(error, error_capacity, error_span)) return 1;
    // An aliased error buffer must not alter either output or readonly inputs,
    // even while reporting another invalid argument.
    if (overlaps(error_span, points_span) || overlaps(error_span, box_span) ||
        overlaps(error_span, mask_span)) return 1;
    const auto fail = [&](const char* message) {
        diagnostic(error, error_capacity, message);
        return 1;
    };
    if (count == 0 || mask_count != count) return fail("positive count and exact mask capacity required");
    if (xyz == nullptr || box15 == nullptr || out_mask == nullptr) return fail("nonnull point, box and mask buffers required");
    if (reinterpret_cast<uintptr_t>(xyz) % alignof(float) != 0 ||
        reinterpret_cast<uintptr_t>(box15) % alignof(double) != 0) return fail("aligned float32 points and binary64 box required");
    if (overlaps(mask_span, points_span) || overlaps(mask_span, box_span)) return fail("output mask must not overlap readonly inputs");
    for (size_t i = 0; i < 15; ++i) {
        if (!std::isfinite(box15[i])) return fail("finite box values required");
    }
    for (size_t i = 3; i < 6; ++i) {
        if (box15[i] <= 0) return fail("positive full box side lengths required");
    }
    // Validate every coordinate BEFORE writing any output. A late NaN/Inf must
    // not leave a partial mask that downstream might mistake for a valid frame.
    for (uint64_t i = 0; i < count * 3; ++i) {
        if (!std::isfinite(xyz[i])) return fail("finite float32 world points required");
    }
    BoxFilter box;
    std::memcpy(box.c, box15, 3 * sizeof(double));
    std::memcpy(box.s, box15 + 3, 3 * sizeof(double));
    std::memcpy(box.rot, box15 + 6, 9 * sizeof(double));
    for (uint64_t i = 0; i < count; ++i) {
        out_mask[i] = static_cast<uint8_t>(box.contains(xyz[i*3], xyz[i*3+1], xyz[i*3+2]));
    }
    diagnostic(error, error_capacity, "");
    return 0;
}
