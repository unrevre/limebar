#include "window.h"

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGError.h>
#include <CoreGraphics/CGGeometry.h>
#include <CoreGraphics/CGPath.h>
#include <CoreGraphics/CGWindowLevel.h>

#include <stdbool.h>
#include <stddef.h>

extern CGError CGSNewRegionWithRect(CGRect* rect, CFTypeRef* outRegion);
extern CGError SLSNewWindow(int cid, int type, float x, float y,
                            CFTypeRef region, uint32_t* wid);
extern CGError SLSSetWindowTags(int cid, uint32_t wid,
                                uint64_t* tags, int tag_size);
extern CGError SLSClearWindowTags(int cid, uint32_t wid,
                                  uint64_t* tags, int tag_size);
extern CGError SLSSetWindowResolution(int cid, uint32_t wid, double res);
extern CGError SLSSetWindowOpacity(int cid, uint32_t wid, bool isOpaque);
extern CGError SLSSetWindowLevel(int cid, uint32_t wid, int level);
extern CGContextRef SLWindowContextCreate(int cid, uint32_t wid,
                                          CFDictionaryRef options);

extern int g_connection;

static void set_window_path(struct CGPath* path, enum window_type type,
                            int width, int height, int xinset, int yinset,
                            int slant)
{
    CGPathMoveToPoint(path, NULL, xinset, yinset);

    int xse = width - xinset - ((type == 2) ? 0 : slant);
    int xnw = xinset + ((type == 0) ? 0 : slant);

    CGPathAddLineToPoint(path, NULL, xse, yinset);
    CGPathAddLineToPoint(path, NULL, width - xinset, height - yinset);
    CGPathAddLineToPoint(path, NULL, xnw, height - yinset);
    CGPathAddLineToPoint(path, NULL, xinset, yinset);
}

void window_init(struct window* window, enum window_type type, CGRect frame,
                 int xinset, int yinset, int slant)
{
    window->render_frame = (CGRect) {{0, 0}, frame.size};

    frame.origin.x -= xinset;
    frame.origin.y -= yinset;
    frame.size.width  += 2 * xinset;
    frame.size.height += 2 * yinset;

    CFTypeRef region;
    CGSNewRegionWithRect(&frame, &region);

    uint64_t set_tags = kCGSStickyTagBit
        | kCGSIgnoreForExposeTagBit
        | kCGSDisableShadowTagBit;
    uint64_t clear_tags = 0x200000000000;

    SLSNewWindow(g_connection, 2, 0, 0, region, &window->id);
    SLSSetWindowTags(g_connection, window->id, &set_tags, 64);
    SLSClearWindowTags(g_connection, window->id, &clear_tags, 64);
    SLSSetWindowResolution(g_connection, window->id, 2.0f);
    SLSSetWindowOpacity(g_connection, window->id, 0);
    SLSSetWindowLevel(g_connection, window->id, CGWindowLevelForKey(3));
    window->context = SLWindowContextCreate(g_connection, window->id, 0);

    frame.origin.x = 0;
    frame.origin.y = 0;

    window->frame = frame;

    window->border = CGPathCreateMutable();
    set_window_path(
        window->border,
        type,
        frame.size.width,
        frame.size.height,
        xinset,
        yinset,
        slant);

    window->background = CGPathCreateMutable();
    set_window_path(
        window->background,
        type,
        frame.size.width,
        frame.size.height,
        xinset * 2,
        yinset * 2,
        slant);
}
