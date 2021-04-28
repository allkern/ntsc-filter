#include "ntsc.hpp"
#include "frame.hpp"

#include "omp.h"

#include <iostream>
#include <vector>

#define W 800
#define H 600
#define PI 3.14159265359
#define PI180 3.14159265359 / 180
#define D2R(t) ((t) * PI180)

bool render_decoded = false;

double color_phase = 0.0, color_ire = 0.0;

void keydown_cb(SDL_Keycode k) {
    if (k == SDLK_RETURN) render_decoded = !render_decoded;
    if (k == SDLK_RIGHT) color_phase--;
    if (k == SDLK_LEFT) color_phase++;
    if (k == SDLK_UP) color_ire++;
    if (k == SDLK_DOWN) color_ire--;
}

void keyup_cb(SDL_Keycode k) {}

static ntsc_frame_t<W, H> ntsc_frame;

double x_freq = 0.0;

void render_morphing_circle() {
    #pragma omp parallel for
    for (int t = 0; t < 90; t++) {
        #pragma omp parallel for
        for (size_t b = 0; b < 75; b++) {
            ntsc_frame[(H/2) + sin(D2R(t * 4)) * (200 - b)][(W/2) + cos(D2R(t * x_freq)) * (200 - b)] = 255.0;
        }
    }
}

#define TOP_SIZE  400
#define MID_START 400
#define MID_END   450
#define BOT_SIZE  150

void render_smpte_bars() {
    static double top_bars[7] = {
        192,
        170,
        135,
        113,
        79,
        57,
        21
    };
    static double mid_bars[7] = {
        21,
        19,
        79,
        19,
        135,
        19,
        192
    };
    static double bot_bars[8] = {
        28,
        255,
        27,
        19,
        9,
        19,
        29,
        19
    };

    size_t counter = 0;

    double* array = &top_bars[0];

    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            if (y >= MID_START && y < MID_END) {
                array = &mid_bars[0];
            } else {
                if (y > MID_END) array = &bot_bars[0];
            }
            
            if (y > MID_END) {
                if (x <= 571) {
                    if (!((x + 1) % 142)) counter++;
                } else {
                    if ((x > 570) && (x <= 685)) {
                        if (!((x + 1) % 38)) counter++;
                    } else {
                        if (x > 685) counter = counter;
                    }
                }
            } else {
                if (!((x + 1) % (W / 7))) counter++;
            }

            ntsc_frame[y][x] = array[counter];
        }

        counter = 0;
    }
}

#define NTSC_ENCODER_FREQUENCY 90.0

void render_solid_color(double phase, double sat, double ire) {
    #pragma omp parallel for
    for (size_t y = 0; y < H; y++) {
        #pragma omp parallel for
        for (size_t x = 4; x < W - 3; x++) {
            double signal = ire + (std::sin((phase + (x * NTSC_ENCODER_FREQUENCY)) * (PI / 180)) * sat);

            ntsc_frame[y][x] = signal;
        }
    }
}

#undef main

int main() {
    frame::init(&ntsc_frame.render_frame);

    frame::register_keydown_cb(&keydown_cb);
    frame::register_keyup_cb(&keyup_cb);
    
    while (frame::is_open()) {
        //render_solid_color(color_phase, 128, color_ire);
        
        //render_smpte_bars();

        render_morphing_circle();

        if (render_decoded) {
            ntsc_frame.decode();
        } else {
            ntsc_frame.render_encoded_frame();
        }

        ntsc_frame.apply_simple_lpf(4);

        frame::update();

        ntsc_frame.reset();

        x_freq += 0.0075;
    }

    return 0;
}