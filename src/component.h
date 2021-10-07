#ifndef COMPONENT_H
#define COMPONENT_H

#include "window.h"

#include <CoreText/CTFont.h>

#include <stdint.h>

struct color
{
    int p;
    float r;
    float g;
    float b;
    float a;
};

enum component_type
{
    CPT_SHELL,
    CPT_POWER,
    CPT_TIME,
};

struct component
{
    enum component_type type;
    struct window window;
    struct color bd_color;
    struct color fg_color;
    struct color bg_color;
    CTFontRef font;
};

struct component_shell
{
    struct component component;
    char* command;
    char* output;
};

struct component_time
{
    struct component component;
    char icon[4];
    char output[16];
};

struct component_power
{
    struct component component;
    char icon[4];
    char output[16];
};

void* component_create_shell(float x, float y, float w, float h,
                             uint32_t fg, uint32_t bg, uint32_t bd,
                             char* command);
void* component_create_power(float x, float y, float w, float h,
                             uint32_t fg,  uint32_t bg, uint32_t bd);
void* component_create_time(float x, float y, float w, float h,
                            uint32_t fg, uint32_t bg, uint32_t bd);

void component_update(struct component* component);
void component_render(struct component* component);

#endif
