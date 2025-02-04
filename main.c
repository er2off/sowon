#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "./digits.h"

#ifdef PENGER
#include "./penger_walk_sheet.h"
#endif

#define FPS 60
//#define DELTA_TIME (1.0f / FPS)
#define SPRITE_CHAR_WIDTH (300 / 2)
#define SPRITE_CHAR_HEIGHT (380 / 2)
#define CHAR_WIDTH (300 / 2)
#define CHAR_HEIGHT (380 / 2)
#define CHARS_COUNT 8
#define TEXT_WIDTH (CHAR_WIDTH * CHARS_COUNT)
#define TEXT_HEIGHT (CHAR_HEIGHT)
#define WIGGLE_COUNT 3
#define WIGGLE_DURATION (0.40f / WIGGLE_COUNT)
#define COLON_INDEX 10
#define MAIN_COLOR_R 220
#define MAIN_COLOR_G 220
#define MAIN_COLOR_B 220
#define PAUSE_COLOR_R 220
#define PAUSE_COLOR_G 120
#define PAUSE_COLOR_B 120
#define BACKGROUND_COLOR_R 24
#define BACKGROUND_COLOR_G 24
#define BACKGROUND_COLOR_B 24
#define SCALE_FACTOR 0.15f
#define PENGER_SCALE 4
#define PENGER_STEPS_PER_SECOND 3

void secc(bool code)
{
    if (!code) {
        fprintf(stderr, "SDL pooped itself: %s\n", SDL_GetError());
        abort();
    }
}

void *secp(void *ptr)
{
    if (ptr == NULL) {
        fprintf(stderr, "SDL pooped itself: %s\n", SDL_GetError());
        abort();
    }

    return ptr;
}

SDL_Surface *load_png_file_as_surface(uint32_t *data, size_t width, size_t height)
{
    SDL_Surface* image_surface =
        secp(SDL_CreateSurfaceFrom(
                 (int) width,
                 (int) height,
                 SDL_PIXELFORMAT_ABGR8888,
                 data,
                 (int) width * 4));
    return image_surface;
}

SDL_Texture *load_digits_png_file_as_texture(SDL_Renderer *renderer)
{
    SDL_Surface *image_surface = load_png_file_as_surface(digits_data, digits_width, digits_height);
    return secp(SDL_CreateTextureFromSurface(renderer, image_surface));
}

#ifdef PENGER
SDL_Texture *load_penger_png_file_as_texture(SDL_Renderer *renderer)
{
    SDL_Surface *image_surface = load_png_file_as_surface(penger_data, penger_width, penger_height);
    return secp(SDL_CreateTextureFromSurface(renderer, image_surface));
}
#endif

void render_digit_at(SDL_Renderer *renderer, SDL_Texture *digits, size_t digit_index,
                     size_t wiggle_index, float *pen_x, float *pen_y, float user_scale, float fit_scale)
{
    const float effective_digit_width = (float) CHAR_WIDTH * user_scale * fit_scale;
    const float effective_digit_height = (float) CHAR_HEIGHT * user_scale * fit_scale;

    const SDL_FRect src_rect = {
        (float) digit_index * SPRITE_CHAR_WIDTH,
        (float) wiggle_index * SPRITE_CHAR_HEIGHT,
        SPRITE_CHAR_WIDTH,
        SPRITE_CHAR_HEIGHT
    };
    const SDL_FRect dst_rect = {
        *pen_x,
        *pen_y,
        effective_digit_width,
        effective_digit_height
    };
    SDL_RenderTexture(renderer, digits, &src_rect, &dst_rect);
    *pen_x += effective_digit_width;
}

#ifdef PENGER
void render_penger_at(SDL_Renderer *renderer, SDL_Texture *penger, float time, bool flipped, SDL_Window *window)
{
    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    int sps  = PENGER_STEPS_PER_SECOND;

    int step = (int)(time*sps)%(60*sps); //step index [0,60*sps-1]

    float progress  = step/(60.0f*sps); // [0,1]

    int frame_index = step%2;

    float penger_drawn_width = ((float)penger_width / 2) / PENGER_SCALE;

    float penger_walk_width = window_width + penger_drawn_width;

    const SDL_FRect src_rect = {
        (float)(penger_width / 2) * frame_index,
        0,
        (float)penger_width / 2,
        (float)penger_height
    };

    SDL_FRect dst_rect = {
        (float)penger_walk_width * progress - penger_drawn_width,
        (float)window_height - (penger_height / PENGER_SCALE),
        (float)(penger_width / 2) / PENGER_SCALE,
        (float)penger_height / PENGER_SCALE
    };

    SDL_RenderTextureRotated(renderer, penger, &src_rect, &dst_rect, 0, NULL, flipped);
}
#endif

void initial_pen(SDL_Window *window, float *pen_x, float *pen_y, float user_scale, float *fit_scale)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    float text_aspect_ratio = (float) TEXT_WIDTH / (float) TEXT_HEIGHT;
    float window_aspect_ratio = (float) w / (float) h;
    if(text_aspect_ratio > window_aspect_ratio) {
        *fit_scale = (float) w / (float) TEXT_WIDTH;
    } else {
        *fit_scale = (float) h / (float) TEXT_HEIGHT;
    }

    const float effective_digit_width = (float) CHAR_WIDTH * user_scale * *fit_scale;
    const float effective_digit_height = (float) CHAR_HEIGHT * user_scale * *fit_scale;
    *pen_x = w / 2 - effective_digit_width * CHARS_COUNT / 2;
    *pen_y = h / 2 - effective_digit_height / 2;
}

typedef enum {
    MODE_ASCENDING = 0,
    MODE_COUNTDOWN,
    MODE_CLOCK,
} Mode;

float parse_time(const char *time)
{
    float result = 0.0f;

    while (*time) {
        char *endptr = NULL;
        float x = strtof(time, &endptr);

        if (time == endptr) {
            fprintf(stderr, "`%s` is not a number\n", time);
            exit(1);
        }

        switch (*endptr) {
        case '\0':
        case 's': result += x;                 break;
        case 'm': result += x * 60.0f;         break;
        case 'h': result += x * 60.0f * 60.0f; break;
        default:
            fprintf(stderr, "`%c` is an unknown time unit\n", *endptr);
            exit(1);
        }

        time = endptr;
        if (*time) time += 1;
    }

    return result;
}

typedef struct {
    Uint32 frame_delay;
    float dt;
    Uint64 last_time;
} FpsDeltaTime;

FpsDeltaTime make_fpsdeltatime(const Uint32 fps_cap)
{
    return (FpsDeltaTime){
        .frame_delay=(1000 / fps_cap),
        .dt=0.0f,
        .last_time=SDL_GetPerformanceCounter(),
    };
}

void frame_start(FpsDeltaTime *fpsdt)
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 elapsed = now - fpsdt->last_time;
    fpsdt->dt = ((float)elapsed)  / ((float)SDL_GetPerformanceFrequency());
    // printf("FPS: %f | dt %f\n", 1.0 / fpsdt->dt, fpsdt->dt);
    fpsdt->last_time = now;
}

void frame_end(FpsDeltaTime *fpsdt)
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 elapsed = now - fpsdt->last_time;
    const Uint32 cap_frame_end = (Uint32) ((((float)elapsed) * 1000.0f) / ((float)SDL_GetPerformanceFrequency()));

    if (cap_frame_end < fpsdt->frame_delay) {
        SDL_Delay((fpsdt->frame_delay - cap_frame_end) );
    }
}

#define TITLE_CAP 256

typedef struct {
    int argc;
    char **argv;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *digits;
    #ifdef PENGER
    SDL_Texture *penger;
    #endif

    Mode mode;
    float displayed_time;
    bool paused;
    bool exit_after_countdown;

    size_t wiggle_index;
    float wiggle_cooldown;
    float user_scale;
    char prev_title[TITLE_CAP];
    FpsDeltaTime fps_dt;
} AppState;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    AppState *app = SDL_calloc(1, sizeof(AppState));
    *appstate = app;

    app->argc = argc;
    app->argv = argv;
    app->wiggle_cooldown = WIGGLE_DURATION;
    app->user_scale = 1.0f;
    app->fps_dt = make_fpsdeltatime(FPS);

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0) {
            app->paused = true;
        } else if (strcmp(argv[i], "-e") == 0) {
            app->exit_after_countdown = true;
        } else if (strcmp(argv[i], "clock") == 0) {
            app->mode = MODE_CLOCK;
        } else {
            app->mode = MODE_COUNTDOWN;
            app->displayed_time = parse_time(argv[i]);
        }
    }

    secc(SDL_Init(SDL_INIT_VIDEO));

    app->window =
        secp(SDL_CreateWindow(
                 "sowon",
                 TEXT_WIDTH, TEXT_HEIGHT*2,
                 SDL_WINDOW_RESIZABLE));

    app->renderer = secp(SDL_CreateRenderer(app->window, NULL));
    // Use VSync
    SDL_SetRenderVSync(app->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

    app->digits = load_digits_png_file_as_texture(app->renderer);

    #ifdef PENGER
    app->penger = load_penger_png_file_as_texture(app->renderer);
    #endif

    secc(SDL_SetTextureColorMod(app->digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));

    if (app->paused) {
        secc(SDL_SetTextureColorMod(app->digits, PAUSE_COLOR_R, PAUSE_COLOR_G, PAUSE_COLOR_B));
    } else {
        secc(SDL_SetTextureColorMod(app->digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));
    }

    return SDL_APP_CONTINUE;
}

// INPUT BEGIN //////////////////////////////
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState* app = (AppState*)appstate;

    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;

    case SDL_EVENT_KEY_DOWN: {
        switch (event->key.key) {
        case SDLK_ESCAPE:
            return true;

        case SDLK_SPACE: {
            app->paused = !app->paused;
            if (app->paused) {
                secc(SDL_SetTextureColorMod(app->digits, PAUSE_COLOR_R, PAUSE_COLOR_G, PAUSE_COLOR_B));
            } else {
                secc(SDL_SetTextureColorMod(app->digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));
            }
        } break;

        case SDLK_KP_PLUS:
        case SDLK_EQUALS: {
            app->user_scale += SCALE_FACTOR * app->user_scale;
        } break;

        case SDLK_KP_MINUS:
        case SDLK_MINUS: {
            app->user_scale -= SCALE_FACTOR * app->user_scale;
        } break;

        case SDLK_KP_0:
        case SDLK_0: {
            app->user_scale = 1.0f;
        } break;

        case SDLK_F5: {
            app->displayed_time = 0.0f;
            app->paused = false;
            for (int i = 1; i < app->argc; ++i) {
                if (strcmp(app->argv[i], "-p") == 0) {
                    app->paused = true;
                } else {
                    app->displayed_time = parse_time(app->argv[i]);
                }
            }
            if (app->paused) {
                secc(SDL_SetTextureColorMod(app->digits, PAUSE_COLOR_R, PAUSE_COLOR_G, PAUSE_COLOR_B));
            } else {
                secc(SDL_SetTextureColorMod(app->digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));
            }
        } break;

        case SDLK_F11: {
            SDL_WindowFlags window_flags;
            secc(window_flags = SDL_GetWindowFlags(app->window));
            if(window_flags & SDL_WINDOW_FULLSCREEN) {
                secc(SDL_SetWindowFullscreen(app->window, false));
            } else {
                secc(SDL_SetWindowFullscreen(app->window, true));
            }
        } break;
        }
    } break;

    case SDL_EVENT_MOUSE_WHEEL: {
        if (SDL_GetModState() & SDL_KMOD_CTRL) {
            if (event->wheel.y > 0) {
                app->user_scale += SCALE_FACTOR * app->user_scale;
            } else if (event->wheel.y < 0) {
                app->user_scale -= SCALE_FACTOR * app->user_scale;
            }
        }
    } break;

    default: {}
    }
    return SDL_APP_CONTINUE;
}
// INPUT END //////////////////////////////

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState* app = (AppState*)appstate;
    frame_start(&app->fps_dt);

    // RENDER BEGIN //////////////////////////////
    SDL_SetRenderDrawColor(app->renderer, BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, 255);
    SDL_RenderClear(app->renderer);
    {
        const size_t t = (size_t) floorf(fmaxf(app->displayed_time, 0.0f));

        // PENGER BEGIN //////////////////////////////

        #ifdef PENGER
        render_penger_at(app->renderer, app->penger, app->displayed_time, app->mode == MODE_COUNTDOWN, app->window);
        #endif

        // PENGER END //////////////////////////////

        // DIGITS BEGIN //////////////////////////////
        float pen_x, pen_y;
        float fit_scale = 1.0;
        initial_pen(app->window, &pen_x, &pen_y, app->user_scale, &fit_scale);


        // TODO: support amount of hours >99
        const size_t hours = t / 60 / 60;
        render_digit_at(app->renderer, app->digits, hours / 10,   app->wiggle_index      % WIGGLE_COUNT, &pen_x, &pen_y, app->user_scale, fit_scale);
        render_digit_at(app->renderer, app->digits, hours % 10,  (app->wiggle_index + 1) % WIGGLE_COUNT, &pen_x, &pen_y, app->user_scale, fit_scale);
        render_digit_at(app->renderer, app->digits, COLON_INDEX,  app->wiggle_index      % WIGGLE_COUNT, &pen_x, &pen_y, app->user_scale, fit_scale);

        const size_t minutes = t / 60 % 60;
        render_digit_at(app->renderer, app->digits, minutes / 10, (app->wiggle_index + 2) % WIGGLE_COUNT, &pen_x, &pen_y, app->user_scale, fit_scale);
        render_digit_at(app->renderer, app->digits, minutes % 10, (app->wiggle_index + 3) % WIGGLE_COUNT, &pen_x, &pen_y, app->user_scale, fit_scale);
        render_digit_at(app->renderer, app->digits, COLON_INDEX,  (app->wiggle_index + 1) % WIGGLE_COUNT, &pen_x, &pen_y, app->user_scale, fit_scale);

        const size_t seconds = t % 60;
        render_digit_at(app->renderer, app->digits, seconds / 10, (app->wiggle_index + 4) % WIGGLE_COUNT, &pen_x, &pen_y, app->user_scale, fit_scale);
        render_digit_at(app->renderer, app->digits, seconds % 10, (app->wiggle_index + 5) % WIGGLE_COUNT, &pen_x, &pen_y, app->user_scale, fit_scale);

        char title[TITLE_CAP];
        snprintf(title, sizeof(title), "%02zu:%02zu:%02zu - sowon", hours, minutes, seconds);
        if (strcmp(app->prev_title, title) != 0) {
            SDL_SetWindowTitle(app->window, title);
        }
        memcpy(title, app->prev_title, TITLE_CAP);
        // DIGITS END //////////////////////////////
    }
    SDL_RenderPresent(app->renderer);
    // RENDER END //////////////////////////////

    // UPDATE BEGIN //////////////////////////////
    if (app->wiggle_cooldown <= 0.0f) {
        app->wiggle_index++;
        app->wiggle_cooldown = WIGGLE_DURATION;
    }
    app->wiggle_cooldown -= app->fps_dt.dt;

    if (!app->paused) {
        switch (app->mode) {
        case MODE_ASCENDING: {
            app->displayed_time += app->fps_dt.dt;
        } break;
        case MODE_COUNTDOWN: {
            if (app->displayed_time > 1e-6) {
                app->displayed_time -= app->fps_dt.dt;
            } else {
                app->displayed_time = 0.0f;
                if (app->exit_after_countdown) {
                    return SDL_APP_SUCCESS;
                }
            }
        } break;
        case MODE_CLOCK: {
            float displayed_time_prev = app->displayed_time;
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            app->displayed_time = tm->tm_sec
                           + tm->tm_min  * 60.0f
                           + tm->tm_hour * 60.0f * 60.0f;
            if(app->displayed_time <= displayed_time_prev){
                //same second, keep previous count and add subsecond resolution for penger
                if(floorf(displayed_time_prev) == floorf(displayed_time_prev+app->fps_dt.dt)){ //check for no newsecond shenaningans from dt
                    app->displayed_time = displayed_time_prev + app->fps_dt.dt;
                }else{
                    app->displayed_time = displayed_time_prev;
                }
            }
        } break;
        }
    }
    // UPDATE END //////////////////////////////

    frame_end(&app->fps_dt);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;

    AppState* app = (AppState*)appstate;
    if (!app)
        return;

    SDL_DestroyTexture(app->digits);
    #ifdef PENGER
    SDL_DestroyTexture(app->penger);
    #endif
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_free(app);

    SDL_Quit();
}
