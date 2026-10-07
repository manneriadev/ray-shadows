/*
ray.c - 2D ray casting with shadows (SDL2)
 
A light source (the small circle) emits rays in all directions.
Each ray stops at the first thing it hits: the obstacle circle or the
window border. Drag with the mouse to move the light source.
*/
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define WIDTH       1400
#define HEIGHT      900
#define RAY_COUNT   300
#define PI_D        3.14159265358979323846
#define COLOR_WHITE 0xffffffffu

typedef struct circle {
    double r;
    double pos_x;
    double pos_y;
} NODE_Circle;

typedef struct ray {
    double k;
    double pos_x1;
    double pos_y1;
    double pos_x2;
    double pos_y2;
} NODE_Ray;

static NODE_Circle main_circle = {80, 200, 200}; 
static NODE_Circle help_circle = {100, 500, 300}; 

static double angles_cos[RAY_COUNT];
static double angles_sin[RAY_COUNT];

static void require(void* ptr, const char* what);
static void make_circle(SDL_Surface* surface, double radius);
static void cast_rays(NODE_Circle light, NODE_Circle obstacle, NODE_Ray rays[RAY_COUNT]);
static void draw_rays(SDL_Renderer* render, const NODE_Ray rays[RAY_COUNT]);

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    NODE_Ray rays[RAY_COUNT];

    for(int i = 0; i < RAY_COUNT; ++i)
    {
        double angle = ((double)i / RAY_COUNT) * 2 * PI_D;
        angles_cos[i] = cos(angle);
        angles_sin[i] = sin(angle);
    }

    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Ray casting shadows", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    require(window, "window");

    // vsync keeps the CPU from spinning at 100%; fall back to software on machines without GPU 
    SDL_Renderer* render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!render) render = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    require(render, "renderer");

    SDL_Surface* circle1 = SDL_CreateRGBSurface(0, (int)(2 * main_circle.r), (int)(2 * main_circle.r), 32, 0, 0, 0, 0);
    SDL_Surface* circle2 = SDL_CreateRGBSurface(0, (int)(2 * help_circle.r), (int)(2 * help_circle.r), 32, 0, 0, 0, 0);
    require(circle1, "surface 1");
    require(circle2, "surface 2");

    make_circle(circle1, main_circle.r);
    make_circle(circle2, help_circle.r);

    SDL_Texture* c1 = SDL_CreateTextureFromSurface(render, circle1);
    SDL_Texture* c2 = SDL_CreateTextureFromSurface(render, circle2);
    require(c1, "texture 1");
    require(c2, "texture 2");

    SDL_Event event;
    int running = 1;
    int frames = 0;
    uint32_t last_tick = SDL_GetTicks();

    while(running)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT) running = 0;
            if(event.type == SDL_MOUSEMOTION && event.motion.state)
            {
                // the cursor holds the CENTER of the light source
                main_circle.pos_x = event.motion.x - main_circle.r;
                main_circle.pos_y = event.motion.y - main_circle.r;
            }
        }

        SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
        SDL_RenderClear(render);

        cast_rays(main_circle, help_circle, rays);
        draw_rays(render, rays);

        SDL_Rect new_rect = {(int)main_circle.pos_x, (int)main_circle.pos_y, (int)(2 * main_circle.r), (int)(2 * main_circle.r)};
        SDL_RenderCopy(render, c1, NULL, &new_rect);
        SDL_Rect new_rect_help = {(int)help_circle.pos_x, (int)help_circle.pos_y, (int)(2 * help_circle.r), (int)(2 * help_circle.r)};
        SDL_RenderCopy(render, c2, NULL, &new_rect_help);

        SDL_RenderPresent(render);

        // FPS counter in the window title
        ++frames;
        uint32_t now = SDL_GetTicks();
        if(now - last_tick >= 1000)
        {
            char title[64];
            snprintf(title, sizeof title, "Ray casting shadows - %d FPS", frames);
            SDL_SetWindowTitle(window, title);
            frames = 0;
            last_tick = now;
        }
    }

    SDL_DestroyTexture(c1);
    SDL_DestroyTexture(c2);
    SDL_FreeSurface(circle1);
    SDL_FreeSurface(circle2);
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

// Filled circle: (x - r)^2 + (y - r)^2 <= r^2, surface size is 2r x 2r
static void make_circle(SDL_Surface* surface, double radius)
{
    Uint32 key = SDL_MapRGBA(surface->format, 0, 0, 0, 0);
    SDL_FillRect(surface, NULL, key);
    SDL_SetColorKey(surface, SDL_TRUE, key);

    int size = (int)(2 * radius);
    for(int y = 0; y < size; ++y)
    {
        for(int x = 0; x < size; ++x)
        {
            double dx = x - radius;
            double dy = y - radius;
            if(dx * dx + dy * dy <= radius * radius)
            {
                SDL_Rect pixel = {x, y, 1, 1};
                SDL_FillRect(surface, &pixel, COLOR_WHITE);
            }
        }
    }
}

/*
Ray i:  P(t) = (x0, y0) + t * (cos(theta_i), sin(theta_i)),  t > 0
Nearest hit among: obstacle circle (quadratic equation) and the 4 window borders.
*/
static void cast_rays(NODE_Circle light, NODE_Circle obstacle, NODE_Ray rays[RAY_COUNT])
{
    // everything that does not depend on the ray is computed once
    const double r_squared = obstacle.r * obstacle.r;
    const double x0 = light.pos_x + light.r;
    const double y0 = light.pos_y + light.r;
    const double a = obstacle.pos_x + obstacle.r;
    const double b = obstacle.pos_y + obstacle.r;
    const double dx = x0 - a;
    const double dy = y0 - b;
    const double C_I = dx * dx + dy * dy - r_squared;

    for(int i = 0; i < RAY_COUNT; ++i)
    {
        const double cos_i = angles_cos[i];
        const double sin_i = angles_sin[i];

        double t_min = 1e9;
        int is_found = 0;

        const double B_I = 2 * (cos_i * dx + sin_i * dy);
        const double D = B_I * B_I - 4 * C_I;
        if(D > 0)
        {
            const double root = sqrt(D);
            const double t1 = (-B_I + root) / 2;
            if(t1 > 0 && t1 < t_min) { t_min = t1; is_found = 1; }
            const double t2 = (-B_I - root) / 2;
            if (t2 > 0 && t2 < t_min) { t_min = t2; is_found = 1; }
        }
        else if(D == 0)
        {
            const double t = -B_I / 2;
            if (t > 0 && t < t_min) { t_min = t; is_found = 1; }
        }

        if(cos_i != 0)
        {
            const double t_left = (0 - x0) / cos_i;
            if(t_left > 0)
            {
                const double y = y0 + t_left * sin_i;
                if(y >= 0 && y <= HEIGHT && t_left < t_min) { t_min = t_left; is_found = 1; }
            }
            const double t_right = (WIDTH - x0) / cos_i;
            if(t_right > 0)
            {
                const double y = y0 + t_right * sin_i;
                if(y >= 0 && y <= HEIGHT && t_right < t_min) { t_min = t_right; is_found = 1; }
            }
        }

        if(sin_i != 0)
        {
            const double t_top = (0 - y0) / sin_i;
            if(t_top > 0)
            {
                const double x = x0 + t_top * cos_i;
                if(x >= 0 && x <= WIDTH && t_top < t_min) { t_min = t_top; is_found = 1; }
            }
            const double t_bottom = (HEIGHT - y0) / sin_i;
            if(t_bottom > 0)
            {
                const double x = x0 + t_bottom * cos_i;
                if(x >= 0 && x <= WIDTH && t_bottom < t_min) { t_min = t_bottom; is_found = 1; }
            }
        }

        rays[i].k = ((double)i / RAY_COUNT) * 2 * PI_D;
        rays[i].pos_x1 = x0;
        rays[i].pos_y1 = y0;
        rays[i].pos_x2 = is_found ? x0 + t_min * cos_i : x0;
        rays[i].pos_y2 = is_found ? y0 + t_min * sin_i : y0;
    }
}

static void draw_rays(SDL_Renderer* render, const NODE_Ray rays[RAY_COUNT])
{
    SDL_SetRenderDrawColor(render, 128, 128, 128, 255);
    for(int i = 0; i < RAY_COUNT; ++i)
    {
        if(rays[i].pos_x1 == rays[i].pos_x2 && rays[i].pos_y1 == rays[i].pos_y2) continue;
        SDL_RenderDrawLine(render, (int)rays[i].pos_x1, (int)rays[i].pos_y1, (int)rays[i].pos_x2, (int)rays[i].pos_y2);
    }
}

static void require(void* ptr, const char* what)
{
    if(!ptr)
    {
        fprintf(stderr, "failed to create %s: %s\n", what, SDL_GetError());
        exit(EXIT_FAILURE);
    }
}
