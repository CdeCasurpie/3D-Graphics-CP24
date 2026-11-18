#pragma once
#include <array>
#include <cmath>
#include <vector>
#include "../SimplePathTracer/TracerCore.h"

namespace reconstruction {
using path_tracer::Vec;
enum Filter { Box, Triangle, Gaussian, Mitchell, Count };
constexpr std::array<double, Count> radii{0.5, 1, 2, 2};
inline double weight(Filter filter, double x) {
    x = std::abs(x);
    if (x >= radii[filter]) return 0;
    switch (filter) {
    case Box: return 1;
    case Triangle: return 1-x;
    case Gaussian: return std::exp(-2*x*x)-std::exp(-8.0);
    case Mitchell:
        // Mitchell–Netravali cubic with B=C=1/3, support [-2,2].
        return x < 1 ? (21*x*x*x-36*x*x+16)/18
                     : (-7*x*x*x+36*x*x-60*x+32)/18;
    default: return 0;
    }
}
struct Sample { float x, y; Vec rgb; };
constexpr int halo = 2;
struct Film {
    int width, height, spp;
    std::vector<Sample> data;
    Film(int w, int h, int n) : width(w), height(h), spp(n),
        data(size_t(w+2*halo)*(h+2*halo)*n) {}
    Sample* cell(int x, int y) {
        return data.data()+(size_t(y+halo)*(width+2*halo)+x+halo)*spp;
    }
    const Sample* cell(int x, int y) const {
        return data.data()+(size_t(y+halo)*(width+2*halo)+x+halo)*spp;
    }
};
inline std::array<Vec, Count> reconstruct(const Film& film, int x, int y) {
    double rgb[Count][3]{}, total[Count]{}, magnitude[Count]{};
    for (int j=y-halo; j<=y+halo; ++j) for (int i=x-halo; i<=x+halo; ++i) {
        const auto* samples=film.cell(i,j);
        for (int s=0; s<film.spp; ++s) {
            const auto& p=samples[s];
            const double dx=std::abs(p.x-(x+0.5)), dy=std::abs(p.y-(y+0.5));
            for (int f=0; f<Count; ++f) {
                const auto filter=static_cast<Filter>(f);
                const double w=weight(filter,dx)*weight(filter,dy);
                total[f]+=w; magnitude[f]+=std::abs(w);
                rgb[f][0]+=w*p.rgb.x; rgb[f][1]+=w*p.rgb.y; rgb[f][2]+=w*p.rgb.z;
            }
        }
    }
    Vec fallback{};
    const auto* local=film.cell(x,y);
    for (int i=0;i<film.spp;++i) fallback=path_tracer::add(fallback,local[i].rgb);
    fallback=path_tracer::mul(fallback,1.0f/film.spp);
    std::array<Vec, Count> result{};
    for (int f=0;f<Count;++f) {
        if (total[f]<=1e-8*magnitude[f]) result[f]=fallback;
        else result[f]={float(rgb[f][0]/total[f]),float(rgb[f][1]/total[f]),float(rgb[f][2]/total[f])};
    }
    return result;
}
}
