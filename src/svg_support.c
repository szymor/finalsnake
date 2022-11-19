#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
#include <stdio.h>
#include <math.h>
/* See if an image is contained in a data source */

int IMG_isSVG(SDL_RWops* src)
{
    Sint64 start;
    int is_SVG = 0;
    char magic[4096];
    size_t magic_len;
    if(!src)
        return 0;
    start = SDL_RWtell(src);
    magic_len = SDL_RWread(src, magic, 1, sizeof(magic)-1);
    magic[magic_len] = '\0';
    if(SDL_strstr(magic, "<svg"))
    {
        is_SVG = 1;
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    return is_SVG;
}

/* Load a SVG type image from an SDL datasource */
SDL_Surface* SVG_LoadSizedSVG_RW(const char* src, int width, int height,
	int cr, int cg, int cb, int ca, float angle)
{
    struct NSVGimage* image;
    struct NSVGrasterizer* rasterizer;
    SDL_Surface* surface = NULL;
    float scale = 1.0f;

    /* For now just just use default units of pixels at 96 DPI */
    image = nsvgParseFromFile(src, "px", 96, angle);
    if(!image){
        return NULL;
    }
    if(!image || image->width <= 0.0f || image->height <= 0.0f)
    {
        IMG_SetError("Couldn't parse SVG image");
        return NULL;
    }
    rasterizer = nsvgCreateRasterizer();
    if(!rasterizer)
    {
        IMG_SetError("Couldn't create SVG rasterizer");
        nsvgDelete(image);
        return NULL;
    }

    if(width > 0 && height > 0)
    {
        float scale_x = (float)width / image->width;
        float scale_y = (float)height / image->height;

        scale = SDL_min(scale_x, scale_y);
    }
    else if(width > 0)
    {
        scale = (float)width / image->width;
    }
    else if(height > 0)
    {
        scale = (float)height / image->height;
    }
    else
    {
        scale = 1.0f;
    }

    surface = SDL_CreateRGBSurface(0,
            (int)ceilf(image->width * scale),
            (int)ceilf(image->height * scale),
            32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if(!surface)
    {
        nsvgDeleteRasterizer(rasterizer);
        nsvgDelete(image);
        return NULL;
    }

    nsvgRasterize(rasterizer, image, 0.0f, 0.0f, scale,
            (unsigned char*)surface->pixels,
            surface->w, surface->h, surface->pitch,
            cr, cg, cb, ca);
    nsvgDeleteRasterizer(rasterizer);
    nsvgDelete(image);
    return surface;
}
