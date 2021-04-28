#pragma once

#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <array>
#include <cmath>

#include "omp.h"

#define LGW_FORMAT_ARGB8888
#include "lgw/framebuffer.hpp"

#define TAU 6.28318530717

template <size_t D, class T> struct vec {
    T v[D] = { 0 };

    T* begin() { return &v[0]; }
    T* end() { return &v[D]; }

    const T* cbegin() { return &v[0]; }
    const T* cend() { return &v[D]; }

    T& operator[](size_t idx) { return v[idx]; }

    vec<D, T>& operator*(vec<D, T> rhs) { for (size_t idx = 0; idx < D; idx++) v[idx] *= rhs[idx]; return *this; }
    vec<D, T>& operator*(vec<D, T>& rhs) { for (size_t idx = 0; idx < D; idx++) v[idx] *= rhs[idx]; return *this; }
    vec<D, T>& operator+=(vec<D, T> rhs) { for (size_t idx = 0; idx < D; idx++) v[idx] += rhs[idx]; return *this; }
    vec<D, T>& operator/=(T s) { for (size_t idx = 0; idx < D; idx++) v[idx] /= s; return *this; }
};

typedef vec<3, int>                vec3i;
typedef vec<3, long>               vec3l;
typedef vec<3, float>              vec3f;
typedef vec<3, double>             vec3d;
typedef vec<3, long long>          vec3ll;
typedef vec<3, unsigned int>       vec3ui;
typedef vec<3, unsigned long>      vec3ul;
typedef vec<3, unsigned long long> vec3ull;
typedef vec<2, int>                vec2i;
typedef vec<2, long>               vec2l;
typedef vec<2, float>              vec2f;
typedef vec<2, double>             vec2d;
typedef vec<2, long long>          vec2ll;
typedef vec<2, unsigned int>       vec2ui;
typedef vec<2, unsigned long>      vec2ul;
typedef vec<2, unsigned long long> vec2ull;

template <size_t R, class T> struct mat {
    typedef vec<R, T> vec_t;

    vec_t m[R] = { 0 };

    vec_t operator*(vec_t& v) {
        vec_t vec;

        for (size_t r = 0; r < R; r++)
            for (size_t c = 0; c < R; c++)
                vec[r] += (m[r][c] * v[c]);
            
        return vec;
    }

    vec<R, T>* begin() { return &m[0]; }
    vec<R, T>* end() { return &m[R]; }

    const vec<R, T>* cbegin() { return &m[0]; }
    const vec<R, T>* cend() { return &m[R]; }
};

typedef mat<3, int>                mat3x3i;
typedef mat<3, long>               mat3x3l;
typedef mat<3, float>              mat3x3f;
typedef mat<3, double>             mat3x3d;
typedef mat<3, long long>          mat3x3ll;
typedef mat<3, unsigned int>       mat3x3ui;
typedef mat<3, unsigned long>      mat3x3ul;
typedef mat<3, unsigned long long> mat3x3ull;
typedef mat<2, int>                mat2x2i;
typedef mat<2, long>               mat2x2l;
typedef mat<2, float>              mat2x2f;
typedef mat<2, double>             mat2x2d;
typedef mat<2, long long>          mat2x2ll;
typedef mat<2, unsigned int>       mat2x2ui;
typedef mat<2, unsigned long>      mat2x2ul;
typedef mat<2, unsigned long long> mat2x2ull;

// YIQ to RGB conversion matrix
static mat3x3d yiq_to_rgb = {
    1.000,  0.956,  0.621,
    1.000, -0.272, -0.647,
    1.000, -1.107,  1.704
};

template <size_t W> struct ntsc_line_t {
    std::array <double, W> line;

    double* begin() { return &line[0]; }
    double* end() { return &line[W]; }

    const double* cbegin() { return &line[0]; }
    const double* cend() { return &line[W]; }

    double& operator[](size_t idx) { return line[idx]; }
};

template <size_t W, size_t H> struct ntsc_frame_t {
    std::array <ntsc_line_t <W>, H> frame;
    lgw::framebuffer <W, H> render_frame;

    bool decoded = false;

    // Iteration related methods
    ntsc_line_t<W>* begin() { return &frame[0]; }
    ntsc_line_t<W>* end() { return &frame[H]; }
    
    const ntsc_line_t<W>* cbegin() { return &frame[0]; }
    const ntsc_line_t<W>* cend() { return &frame[H]; }

    ntsc_line_t<W>& operator[](size_t idx) { return frame[idx]; }

    // Clamping helpers
    inline size_t clamp_index(int idx, size_t max) { return (idx > max) ? max : ((idx < 0) ? 0 : idx); }
    inline size_t clamp(double value, double max, double min = 0.0) { return (value > max) ? max : ((value < min) ? 0 : value); }

    void apply_simple_lpf(size_t rd) {
        for (size_t y = 0; y < H; y++) {
            for (size_t x = 0; x < W; x++) {
                uint32_t c = render_frame.read(x, y);

                int r = (c & 0xff0000) >> 16,
                    g = (c & 0xff00  ) >> 8,
                    b = (c & 0xff    );

                for (int d = 1; d < rd; d++) {
                    uint32_t c = render_frame.read(clamp_index(x + d, W - 1), y);
                    
                    r += (c & 0xff0000) >> 16;
                    g += (c & 0xff00  ) >> 8;
                    b += (c & 0xff    );
                }

                r /= rd;
                g /= rd;
                b /= rd;

                render_frame.draw(x, y, lgw::rgb(r, g, b));
            }
        }
    }

    void render_encoded_frame() {
        for (size_t y = 0; y < H; y++) {
            for (size_t x = 0; x < W; x++) {
                render_frame.draw(x, y, lgw::rgb(frame[y][x]));
            }
        }
    }

    void reset() {
        decoded = false;

        for (ntsc_line_t<W>& l : frame) {
            l.line.fill(0);
        }

        render_frame.clear(lgw::color::black);
    }

    void decode() {
        #pragma omp parallel for
        for (size_t y = 0; y < H; y++) {
            #pragma omp parallel for
            for (size_t x = 0; x < W - 1; x++) {
                vec3d yiq = { 0.0 }, rgb = { 0.0 };

                // Not O(n) :(
                for (int d = -2; d < 2; d++) {
                    double phase = (x + double(d)) * TAU / 4.0,
                           s = clamp(frame[y][clamp_index(x + d, W)], 255);

                    vec3d sample = { s, s, s };


                    yiq += (sample * vec3d{1.0, std::cos(phase), std::sin(phase)});
                }
                
                yiq /= 4.0;

                rgb = yiq_to_rgb * yiq;

                render_frame.draw(x, y, lgw::rgb(clamp(rgb[0], 255.0), clamp(rgb[1], 255.0), clamp(rgb[2], 255.0)));
            }
        }

        decoded = true;
    }
};
