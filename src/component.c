#include "component.h"

#include "font.h"
#include "util.h"

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFString.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGDirectDisplay.h>
#include <CoreGraphics/CGError.h>
#include <CoreGraphics/CGGeometry.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define set_component_color(f, component, color)    \
    f(component->window.context,                    \
      component->color.r,                           \
      component->color.g,                           \
      component->color.b,                           \
      component->color.a);

extern CGError SLSDisableUpdate(int cid);
extern CGError SLSReenableUpdate(int cid);
extern CGError SLSOrderWindow(int cid, uint32_t wid, int mode,
                              uint32_t relativeToWID);

extern int g_connection;
extern int g_counter;

static struct color color_from_hex(uint32_t color)
{
    struct color result;
    result.p = color;
    result.r = ((color >> 0x10) & 0xff) / 255.0f;
    result.g = ((color >> 0x08) & 0xff) / 255.0f;
    result.b = ((color >> 0x00) & 0xff) / 255.0f;
    result.a = ((color >> 0x18) & 0xff) / 255.0f;
    return result;
}

static CGRect calculate_bounds(uint32_t id, float x, float y, float w, float h)
{
    CGRect bounds = CGDisplayBounds(id);
    int cx = (int)(bounds.origin.x + (x * bounds.size.width));
    int cy = (int)(bounds.origin.y + (y * bounds.size.height));
    int cw = (int)(w * bounds.size.width);
    int ch = (int)(h * bounds.size.height);
    return (CGRect) {{cx, cy}, {cw, ch}};
}

static void component_init_none(struct component* component)
{
    (void)component;
}

static void component_init_shell(struct component* component)
{
    struct component_shell* data = (struct component_shell*)component;

    (void)data;
}

static void component_init_power(struct component* component)
{
    struct component_power* data = (struct component_power*)component;

    (void)data;
}

static void component_init_time(struct component* component)
{
    struct component_time* data = (struct component_time*)component;

    snprintf(data->icon, sizeof(data->icon), "♪ ");
}

static void (*component_init_map[])(struct component* component) =
{
    [CPT_NULL]  = component_init_none,
    [CPT_SHELL] = component_init_shell,
    [CPT_POWER] = component_init_power,
    [CPT_TIME]  = component_init_time,
};

static void component_init(struct component* component,
                           enum component_type ctype, enum window_type wtype,
                           float x, float y, float w, float h,
                           uint32_t fg, uint32_t bg, uint32_t bd)
{
    uint32_t id = CGMainDisplayID();
    CGRect frame = calculate_bounds(id, x, y, w, h);

    component->type = ctype;
    component->fg_color = color_from_hex(fg);
    component->bg_color = color_from_hex(bg);
    component->bd_color = color_from_hex(bd);
    component->font = create_font("Monaco:Regular:10.0");
    component->refresh_rate = 1;
    component->update = 1;
    component->cache = 1;

    component_init_map[component->type](component);

    window_init(&component->window, wtype, frame, 6, 1, 9);
}

void* component_create_null(float x, float y, float w, float h,
                            uint32_t fg, uint32_t bg, uint32_t bd,
                            enum window_type type)
{
    struct component_null* data = malloc(sizeof(struct component_null));
    memset(data, 0, sizeof(struct component_null));
    component_init(&data->component, CPT_NULL, type, x, y, w, h, fg, bg, bd);
    return &data->component;
}

void* component_create_shell(float x, float y, float w, float h,
                             uint32_t fg, uint32_t bg, uint32_t bd,
                             char* command, enum window_type type)
{
    struct component_shell* data = malloc(sizeof(struct component_shell));
    memset(data, 0, sizeof(struct component_shell));
    component_init(&data->component, CPT_SHELL, type, x, y, w, h, fg, bg, bd);
    data->command = command;
    return &data->component;
}

void* component_create_power(float x, float y, float w, float h,
                             uint32_t fg, uint32_t bg, uint32_t bd,
                             enum window_type type)
{
    struct component_power* data = malloc(sizeof(struct component_power));
    memset(data, 0, sizeof(struct component_power));
    component_init(&data->component, CPT_POWER, type, x, y, w, h, fg, bg, bd);
    return &data->component;
}

void* component_create_time(float x, float y, float w, float h,
                            uint32_t fg, uint32_t bg, uint32_t bd,
                            enum window_type type)
{
    struct component_time* data = malloc(sizeof(struct component_time));
    memset(data, 0, sizeof(struct component_time));
    component_init(&data->component, CPT_TIME, type, x, y, w, h, fg, bg, bd);
    return &data->component;
}

static void component_update_null(struct component* component)
{
    (void)component;
}

static void component_update_shell(struct component* component)
{
    struct component_shell* data = (struct component_shell*)component;

    int length = 0;
    int nbytes = 0;
    char* result = NULL;
    char buffer[BUFSIZ];
    int cache = -1;

    FILE* handle = popen(data->command, "r");
    if (!handle) return;

    while ((nbytes = read(fileno(handle), buffer, sizeof(buffer) - 1)) > 0) {
        result = realloc(result, length + nbytes + 1);
        if (!result) break;

        memcpy(result + length, buffer, nbytes);
        length += nbytes;
    }

    if (result && nbytes != -1) {
        result[length] = '\0';
        data->output = result;

        cache = hash(result);
    } else {
        data->output = "";
    }

    if (cache != component->cache) {
        component->cache = cache;
        component->update = 1;
    }

    pclose(handle);
}

static void component_update_power(struct component* component)
{
    struct component_power* data = (struct component_power*)component;

    CFTypeRef ps_info = IOPSCopyPowerSourcesInfo();
    CFTypeRef ps_list = IOPSCopyPowerSourcesList(ps_info);

    bool has_battery = false;
    bool is_charging = false;
    int cur_cap = 0;
    int max_cap = 0;
    int percent = 0;
    int cache;

    int ps_count = ps_list ? CFArrayGetCount(ps_list) : 0;
    for (int i = 0; i < ps_count; ++i) {
        CFDictionaryRef ps = IOPSGetPowerSourceDescription(
            ps_info, CFArrayGetValueAtIndex(ps_list, i));
        if (!ps) continue;

        CFTypeRef ps_type = CFDictionaryGetValue(ps, CFSTR(kIOPSTypeKey));
        if (!ps_type || !CFEqual(ps_type, CFSTR(kIOPSInternalBatteryType)))
            continue;

        CFTypeRef ps_cur = CFDictionaryGetValue(
            ps, CFSTR(kIOPSCurrentCapacityKey));
        if (!ps_cur) continue;

        CFTypeRef ps_max = CFDictionaryGetValue(
            ps, CFSTR(kIOPSMaxCapacityKey));
        if (!ps_max) continue;

        CFTypeRef ps_charge = CFDictionaryGetValue(
            ps, CFSTR(kIOPSPowerSourceStateKey));
        if (!ps_charge) continue;

        has_battery = true;
        is_charging = !CFEqual(ps_charge, CFSTR(kIOPSBatteryPowerValue));

        CFNumberGetValue((CFNumberRef)ps_cur, kCFNumberSInt32Type, &cur_cap);
        CFNumberGetValue((CFNumberRef)ps_max, kCFNumberSInt32Type, &max_cap);

        percent = (int)((double)cur_cap / (double)max_cap * 100);

        break;
    }

    if (ps_list) CFRelease(ps_list);
    if (ps_info) CFRelease(ps_info);

    cache = (has_battery) ? ((is_charging) ? percent : -percent) : -999;

    if (cache != component->cache) {
        component->cache = cache;

        if (has_battery) {
            snprintf(data->output, sizeof(data->output), "%' ' 3d%%", percent);
            snprintf(data->icon, sizeof(data->icon), (is_charging) ? "+" : "-");
        } else {
            snprintf(data->output, sizeof(data->output), "A/C");
            snprintf(data->icon, sizeof(data->icon), "=");
        }

        component->update = 1;
    }
}

static void component_update_time(struct component* component)
{
    struct component_time* data = (struct component_time*)component;

    time_t rawtime;
    time(&rawtime);

    int cache;

    struct tm* timeinfo = localtime(&rawtime);
    if (timeinfo) {
        cache = (timeinfo->tm_hour << 8) + timeinfo->tm_min;

        if (cache != component->cache) {
            component->cache = cache;

            snprintf(data->output, sizeof(data->output), "%02d:%02d",
                timeinfo->tm_hour, timeinfo->tm_min);

            component->update = 1;
        }
    }
}

static void (*component_update_map[])(struct component* component) =
{
    [CPT_NULL]  = component_update_null,
    [CPT_SHELL] = component_update_shell,
    [CPT_POWER] = component_update_power,
    [CPT_TIME]  = component_update_time,
};

void component_update(struct component* component)
{
    if (g_counter % component->refresh_rate != 0)
        return;

    component_update_map[component->type](component);
}

static void component_render_null(struct component* component)
{
    struct component_null* data = (struct component_null*)component;
    (void)data;
}

static void component_render_shell(struct component* component)
{
    struct component_shell* data = (struct component_shell*)component;
    draw_text(component, data->output, 0.5, 0.5, ALIGN_CENTER);
}

static void component_render_power(struct component* component)
{
    struct component_power* data = (struct component_power*)component;
    draw_text(component, data->icon, 0.3250, 0.5, ALIGN_CENTER);
    draw_text(component, data->output, 0.50, 0.5, ALIGN_CENTER);
}

static void component_render_time(struct component* component)
{
    struct component_time* data = (struct component_time*)component;
    draw_text(component, data->icon, 0.3000, 0.5, ALIGN_CENTER);
    draw_text(component, data->output, 0.56, 0.5, ALIGN_CENTER);
}

static void (*component_render_map[])(struct component* component) =
{
    [CPT_NULL]  = component_render_null,
    [CPT_SHELL] = component_render_shell,
    [CPT_POWER] = component_render_power,
    [CPT_TIME]  = component_render_time,
};

void component_render(struct component* component)
{
    if (!component->update)
        return;

    SLSDisableUpdate(g_connection);
    SLSOrderWindow(g_connection, component->window.id, 0, 0);
    CGContextClearRect(component->window.context, component->window.frame);

    set_component_color(CGContextSetRGBFillColor, component, bd_color);
    CGContextAddPath(component->window.context, component->window.border);
    CGContextFillPath(component->window.context);

    set_component_color(CGContextSetRGBFillColor, component, bg_color);
    CGContextAddPath(component->window.context, component->window.background);
    CGContextFillPath(component->window.context);

    set_component_color(CGContextSetRGBFillColor, component, fg_color);
    component_render_map[component->type](component);

    CGContextFlush(component->window.context);
    SLSOrderWindow(g_connection, component->window.id, 1, 0);
    SLSReenableUpdate(g_connection);

    component->update = 0;
}
