#ifndef WINDOW_H
#define WINDOW_H

#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGGeometry.h>
#include <CoreGraphics/CGPath.h>

#include <stdint.h>

#define kCGSDisableShadowTagBit         (1 <<  3)
#define kCGSIgnoreForExposeTagBit       (1 <<  7)
#define kCGSStickyTagBit                (1 << 11)

enum window_type
{
    WIN_BEG,
    WIN_MID,
    WIN_END,
};

struct window
{
    uint32_t id;
    enum window_type type;
    CGContextRef context;
    CGRect frame;
    CGRect render_frame;
    CGMutablePathRef border;
    CGMutablePathRef background;
};

void window_init(struct window* window, enum window_type type, CGRect frame,
                 int xinset, int yinset, int slant);

#endif
