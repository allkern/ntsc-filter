#pragma once

#ifdef _WIN32
#include "SDL.h"
#endif

#ifdef __linux__
#include "SDL2/SDL.h"
#endif

#define LGW_FORMAT_ARGB8888

#include "lgw/framebuffer.hpp"

#include <functional>
#include <chrono>

#define PPU_WIDTH  800
#define PPU_HEIGHT 600

namespace frame {
    namespace sdl {
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        SDL_Texture* texture = nullptr;
    }

    typedef std::function<void(SDL_Keycode)> key_event_callback_t;

    key_event_callback_t keydown_cb, keyup_cb;

    // FPS tracking stuff
    auto start = std::chrono::high_resolution_clock::now();
    size_t frames_rendered = 0, fps = 0;

    typedef lgw::framebuffer <PPU_WIDTH, PPU_HEIGHT> framebuffer_t;

    framebuffer_t* frame = nullptr;

    void init(framebuffer_t* f_ptr) {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);

        frame = f_ptr;

        sdl::window = SDL_CreateWindow(
            "NTSC Frame",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            PPU_WIDTH, PPU_HEIGHT,
            SDL_WINDOW_VULKAN
        );

        sdl::renderer = SDL_CreateRenderer(
            sdl::window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );

        sdl::texture = SDL_CreateTexture(
            sdl::renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            PPU_WIDTH, PPU_HEIGHT
        );
    }

    bool open = true;

    inline bool is_open() {
        return open;
    }

    inline size_t get_fps() {
        return fps;
    }

    inline void register_keydown_cb(const key_event_callback_t& kd) {
        keydown_cb = kd;
    }

    inline void register_keyup_cb(const key_event_callback_t& ku) {
        keyup_cb = ku;
    }

    void close() {
        open = false;
        
        SDL_DestroyTexture(sdl::texture);
        SDL_DestroyRenderer(sdl::renderer);
        SDL_DestroyWindow(sdl::window);

        SDL_Quit();
    }

    void update() {
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration <double> d = end - start;

        if (std::chrono::duration_cast<std::chrono::seconds>(d).count() >= 1) {
            fps = frames_rendered;
            frames_rendered = 0;
            start = std::chrono::high_resolution_clock::now();
        }

        SDL_RenderClear(sdl::renderer);

        SDL_UpdateTexture(
            sdl::texture,
            NULL,
            frame->get_buffer(),
            PPU_WIDTH * sizeof(uint32_t)
        );

        SDL_RenderCopy(sdl::renderer, sdl::texture, NULL, NULL);

        SDL_RenderPresent(sdl::renderer);

        frames_rendered++;

        SDL_Event event;

        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: { close(); } break;
                case SDL_KEYDOWN: { keydown_cb(event.key.keysym.sym); } break;
                case SDL_KEYUP: { keyup_cb(event.key.keysym.sym); } break;
            }
        }
    }
}