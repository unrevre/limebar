#ifndef WINDOW_H
#define WINDOW_H

#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGGeometry.h>
#include <CoreGraphics/CGPath.h>

#include <stdint.h>

#define kCGSDisableShadowTagBit         (1 <<  3)
#define kCGSIgnoreForExposeTagBit       (1 <<  7)
#define kCGSStickyTagBit                (1 << 11)

struct window
{
    uint32_t id;
    CGContextRef context;
    CGRect frame;
    CGRect render_frame;
    CGMutablePathRef border;
    CGMutablePathRef background;
};

void window_init(struct window* window, CGRect frame, int border_width);

#endif
