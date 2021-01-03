#ifndef FONT_H
#define FONT_H

#include <CoreGraphics/CGGeometry.h>
#include <CoreText/CTFont.h>
#include <CoreText/CTLine.h>

struct component;

enum alignment_type
{
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT,
};

CTFontRef create_font(char* cstring);
CTLineRef create_line(CTFontRef font, char* cstring, CGRect* bounds,
                      float* ascent, float* descent);
void draw_text(struct component* component, char* text, float x, float y,
               enum alignment_type alignment_x);

#endif
