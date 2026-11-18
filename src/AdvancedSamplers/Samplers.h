#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace sampling {
using Point = std::array<double, 3>;
using Cloud = std::vector<Point>;

inline double radicalInverse(unsigned n, unsigned base) {
    double place = 1.0 / base, result = 0;
    while (n) {
        result += (n % base) * place;
        n /= base;
        place /= base;
    }
    return result;
}

inline Point halton(unsigned n) {
    return {radicalInverse(n, 2), radicalInverse(n, 3), radicalInverse(n, 5)};
}

inline Point sobol(uint32_t n) {
    // Standard first three Sobol dimensions: dimension 1 powers of two;
    // dimension 2 polynomial x+1, m=(1); dimension 3 x²+x+1, m=(1,3).
    static const auto directions = [] {
        std::array<std::array<uint32_t, 32>, 3> v{};
        v[1][0] = v[2][0] = 1u << 31;
        v[2][1] = 3u << 30;
        for (int j = 0; j < 32; ++j) {
            v[0][j] = 1u << (31-j);
            if (j >= 1) v[1][j] = v[1][j-1] ^ (v[1][j-1] >> 1);
            if (j >= 2) v[2][j] = v[2][j-2] ^ (v[2][j-2] >> 2) ^ v[2][j-1];
        }
        return v;
    }();
    Point p{};
    const uint32_t gray = n ^ (n >> 1);
    for (int d = 0; d < 3; ++d) {
        uint32_t bits = 0;
        for (uint32_t g = gray, j = 0; g; g >>= 1, ++j)
            if (g & 1u) bits ^= directions[d][j];
        p[d] = std::ldexp(double(bits), -32);
    }
    return p;
}

inline std::array<Cloud, 3> generate(unsigned count, uint32_t seed) {
    std::array<Cloud, 3> clouds;
    for (auto& cloud : clouds) cloud.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> unit(0, 1);
    for (unsigned i = 0; i < count; ++i) {
        clouds[0].push_back(halton(i+1));
        clouds[1].push_back(sobol(i+1));
        clouds[2].push_back({unit(rng), unit(rng), unit(rng)});
    }
    return clouds;
}

struct Occupancy { int empty, minimum, maximum; double rms; };
inline Occupancy occupancy(const Cloud& cloud, int first=0, int second=1, int third=2) {
    constexpr int bins = 8;
    std::array<int, bins*bins*bins> counts{};
    const int cells = third < 0 ? bins*bins : bins*bins*bins;
    for (const auto& p : cloud) {
        int index = int(p[first]*bins) + bins*int(p[second]*bins);
        if (third >= 0) index += bins*bins*int(p[third]*bins);
        ++counts[index];
    }
    Occupancy result{0, int(cloud.size()), 0, 0};
    const double mean = double(cloud.size()) / cells;
    for (int i=0; i<cells; ++i) {
        const int count = counts[i];
        if (!count) ++result.empty;
        if (count < result.minimum) result.minimum = count;
        if (count > result.maximum) result.maximum = count;
        result.rms += (count-mean)*(count-mean);
    }
    result.rms = std::sqrt(result.rms/cells);
    return result;
}
}
