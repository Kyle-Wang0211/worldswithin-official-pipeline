                                                                                                 
#include "dense_session.h"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace aether::dense {

                                                                               
double numpy_pairwise_sum(const double* a, long n) {
    if (n < 8) {
        double res = 0.;
        for (long i = 0; i < n; i++) res += a[i];
        return res;
    } else if (n <= 128) {
        double r[8], res;
        for (int j = 0; j < 8; ++j) r[j] = a[j];
        long i;
        for (i = 8; i < n - (n % 8); i += 8)
            for (int j = 0; j < 8; ++j) r[j] += a[i + j];
        res = ((r[0] + r[1]) + (r[2] + r[3])) + ((r[4] + r[5]) + (r[6] + r[7]));
        for (; i < n; i++) res += a[i];
        return res;
    } else {
        long n2 = n / 2;
        n2 -= n2 % 8;
        return numpy_pairwise_sum(a, n2) + numpy_pairwise_sum(a + n2, n - n2);
    }
}

                                                                                                
double numpy_percentile_linear(const std::vector<double>& s, double q_percent) {
    const long n = (long)s.size();
    const double q = q_percent / 100.0;                                                
    const double virt = (double)(n - 1) * q;                                                           
    long prev = (long)std::floor(virt), next = prev + 1;                     
    if (virt >= (double)(n - 1)) { prev = n - 1; next = n - 1; }                               
    if (virt < 0) { prev = 0; next = 0; }
    const double a = s[prev], b = s[next];
    const double gamma = virt - (double)prev;                                                                
    const double diff = b - a;                                        
    double r = a + diff * gamma;
    if (gamma >= 0.5) r = b - diff * (1.0 - gamma);
    return r;
}

namespace {
                                                                                                                   
inline double dot3(double a0, double a1, double a2, double b0, double b1, double b2) {
    double acc = a0 * b0; acc = std::fma(a1, b1, acc); acc = std::fma(a2, b2, acc); return acc;
}
inline double norm3(double x, double y, double z) { return std::sqrt((x * x + y * y) + z * z); }                           
}              

void build_session_table(const std::vector<SessionFrame>& fr, const std::vector<float>& pts,
                         const SessionParams& p, SessionTable& out) {
    const int NF = (int)fr.size(); const long NP = (long)(pts.size() / 3);
    out.NF = NF; out.cams.assign((size_t)NF * 36, 0.f); out.neighbors.assign((size_t)NF * p.nsrc, 0); out.med_angle.assign(NF, 0.0);
    std::vector<double> P((size_t)NP * 3);                                               
    for (size_t i = 0; i < P.size(); ++i) P[i] = (double)pts[i];

    std::vector<double> K(NF * 9), R(NF * 9), T(NF * 3), C(NF * 3);
    std::vector<std::vector<uint8_t>> vis(NF, std::vector<uint8_t>(NP, 0));
    const double W = p.W, H = p.H;
    for (int i = 0; i < NF; ++i) {
        const SessionFrame& f = fr[i];
        const double sx = W / f.image_w, sy = H / f.image_h;            
        double* k = &K[i * 9];
        k[0] = f.fx * sx; k[1] = 0; k[2] = f.cx * sx; k[3] = 0; k[4] = f.fy * sy; k[5] = f.cy * sy; k[6] = 0; k[7] = 0; k[8] = 1;            
        const double w = f.q[0], x = f.q[1], y = f.q[2], z = f.q[3];               
        double* r = &R[i * 9];
        r[0] = 1 - 2 * (y * y + z * z); r[1] = 2 * (x * y - w * z); r[2] = 2 * (x * z + w * y);
        r[3] = 2 * (x * y + w * z);     r[4] = 1 - 2 * (x * x + z * z); r[5] = 2 * (y * z - w * x);
        r[6] = 2 * (x * z - w * y);     r[7] = 2 * (y * z + w * x);     r[8] = 1 - 2 * (x * x + y * y);
        T[i * 3] = f.t[0]; T[i * 3 + 1] = f.t[1]; T[i * 3 + 2] = f.t[2];
                                                                                
        for (int kk = 0; kk < 3; ++kk) C[i * 3 + kk] = dot3(-r[0 * 3 + kk], -r[1 * 3 + kk], -r[2 * 3 + kk], f.t[0], f.t[1], f.t[2]);
                                                                
        for (long n = 0; n < NP; ++n) {
            const double px = P[n * 3], py = P[n * 3 + 1], pz = P[n * 3 + 2];
            const double xc = dot3(r[0], r[1], r[2], px, py, pz) + f.t[0];
            const double yc = dot3(r[3], r[4], r[5], px, py, pz) + f.t[1];
            const double zc = dot3(r[6], r[7], r[8], px, py, pz) + f.t[2];
            if (!(zc > 0)) continue;
            const double u = k[0] * xc / zc + k[2], v = k[4] * yc / zc + k[5];
            vis[i][n] = (u >= 0) && (u < W) && (v >= 0) && (v < H);
        }
    }

    out.NP = NP; out.vis.assign((size_t)NF * NP, 0);
    for (int i = 0; i < NF; ++i) for (long n = 0; n < NP; ++n) out.vis[(size_t)i * NP + n] = vis[i][n];
    std::vector<double> sbuf; sbuf.reserve(NP); std::vector<double> th; th.reserve(NP);
    for (int i = 0; i < NF; ++i) {
                                                                                               
        std::vector<std::tuple<double, double, int>> sc;
        for (int j = 0; j < NF; ++j) {
            if (i == j) continue;
            const double base = norm3(C[i * 3] - C[j * 3], C[i * 3 + 1] - C[j * 3 + 1], C[i * 3 + 2] - C[j * 3 + 2]);
            if (base < p.min_base) continue;                                
            long ncom = 0;
            for (long n = 0; n < NP; ++n) ncom += (vis[i][n] & vis[j][n]);
            if (ncom < p.min_common) continue;                              
            sbuf.clear(); th.clear();
            for (long n = 0; n < NP; ++n) {
                if (!(vis[i][n] & vis[j][n])) continue;
                const double X0 = P[n * 3], X1 = P[n * 3 + 1], X2 = P[n * 3 + 2];
                double v10 = X0 - C[i * 3], v11 = X1 - C[i * 3 + 1], v12 = X2 - C[i * 3 + 2];                         
                double v20 = X0 - C[j * 3], v21 = X1 - C[j * 3 + 1], v22 = X2 - C[j * 3 + 2];
                const double n1 = norm3(v10, v11, v12) + 1e-12, n2 = norm3(v20, v21, v22) + 1e-12;                
                v10 /= n1; v11 /= n1; v12 /= n1; v20 /= n2; v21 /= n2; v22 /= n2;
                double d = (v10 * v20 + v11 * v21) + v12 * v22;                                          
                d = std::min(1.0, std::max(-1.0, d));                          
                const double ang = std::acos(d) * (180.0 / M_PI);                               
                th.push_back(ang);
                const double dd = ang - p.theta0, sig = (ang <= p.theta0) ? p.sig1 : p.sig2;                
                sbuf.push_back(std::exp(-(dd * dd) / (2 * sig * sig)));
            }
            const double s = numpy_pairwise_sum(sbuf.data(), (long)sbuf.size());            
            std::vector<double> t2 = th; std::sort(t2.begin(), t2.end());                      
            const size_t m = t2.size();
            const double med = (m % 2) ? t2[m / 2] : (t2[m / 2 - 1] + t2[m / 2]) / 2.0;
            sc.emplace_back(s, med, j);
        }
        std::sort(sc.begin(), sc.end(), [](const auto& a, const auto& b) { return a > b; });                            
        std::vector<int> picked;
        for (size_t k = 0; k < sc.size() && (int)picked.size() < p.nsrc; ++k) picked.push_back(std::get<2>(sc[k]));
        while ((int)picked.size() < p.nsrc) {                                                       
            std::vector<double> d(NF);
            for (int j = 0; j < NF; ++j) d[j] = norm3(C[j * 3] - C[i * 3], C[j * 3 + 1] - C[i * 3 + 1], C[j * 3 + 2] - C[i * 3 + 2]);
            d[i] = 1e9; for (int q : picked) d[q] = 1e9;
            int am = 0; for (int j = 1; j < NF; ++j) if (d[j] < d[am]) am = j;                              
            picked.push_back(am);
        }
        for (int k = 0; k < p.nsrc; ++k) out.neighbors[(size_t)i * p.nsrc + k] = picked[k];
        if (!sc.empty()) {                                                                  
            std::vector<double> a; for (size_t k = 0; k < sc.size() && (int)k < p.nsrc; ++k) a.push_back(std::get<1>(sc[k]));
            std::sort(a.begin(), a.end()); const size_t m = a.size();
            out.med_angle[i] = (m % 2) ? a[m / 2] : (a[m / 2 - 1] + a[m / 2]) / 2.0;
        }
                                                                                 
        std::vector<double> zc;
        const double* r = &R[i * 9];
        for (long n = 0; n < NP; ++n) {
            if (!vis[i][n]) continue;
            const double z = dot3(r[6], r[7], r[8], P[n * 3], P[n * 3 + 1], P[n * 3 + 2]) + T[i * 3 + 2];
            if (z > 0) zc.push_back(z);
        }
        double dmin, dmax;
        if ((long)zc.size() >= p.min_pts) {
            std::sort(zc.begin(), zc.end());
            dmin = numpy_percentile_linear(zc, 2.0) * p.dr_lo;
            dmax = numpy_percentile_linear(zc, 99.5) * p.dr_hi;
        } else { dmin = p.fallback_lo; dmax = p.fallback_hi; }
                                                            
        float* c = &out.cams[(size_t)i * 36];
        for (int k = 0; k < 9; ++k) { c[k] = (float)K[i * 9 + k]; c[9 + k] = (float)R[i * 9 + k]; }
        for (int k = 0; k < 3; ++k) { c[18 + k] = (float)T[i * 3 + k]; c[21 + k] = (float)C[i * 3 + k]; }
        c[24] = (float)dmin; c[25] = (float)dmax; c[26] = (float)p.W; c[27] = (float)p.H;
    }
}

}                            
