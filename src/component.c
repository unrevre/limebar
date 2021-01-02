#include "component.h"

#include "font.h"
#include "window.h"

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

static void component_init(struct component* component,
                           enum component_type type,
                           float x, float y, float w, float h,
                           uint32_t fg, uint32_t bg, uint32_t bd)
{
    uint32_t id = CGMainDisplayID();
    CGRect frame = calculate_bounds(id, x, y, w, h);

    component->type = type;
    component->bd_width = 2;
    component->bd_color = color_from_hex(bd);
    component->fg_color = color_from_hex(fg);
    component->bg_color = color_from_hex(bg);
    component->font = create_font("Monaco:Regular:10.0");

    window_init(&component->window, frame, component->bd_width);
    CGContextSetLineWidth(component->window.context, component->bd_width);
    set_component_color(CGContextSetRGBStrokeColor, component, bd_color);
    set_component_color(CGContextSetRGBFillColor, component, bg_color);
}

void* component_create_shell(float x, float y, float w, float h,
                             uint32_t fg, uint32_t bg, uint32_t bd,
                             char* command)
{
    struct component_shell* shell = malloc(sizeof(struct component_shell));
    memset(shell, 0, sizeof(struct component_shell));
    component_init(&shell->component, CPT_SHELL, x, y, w, h, fg, bg, bd);
    shell->command = command;
    return &shell->component;
}

void* component_create_power(float x, float y, float w, float h,
                             uint32_t fg, uint32_t bg, uint32_t bd)
{
    struct component_power* power = malloc(sizeof(struct component_power));
    memset(power, 0, sizeof(struct component_power));
    component_init(&power->component, CPT_POWER, x, y, w, h, fg, bg, bd);
    return &power->component;
}

void* component_create_time(float x, float y, float w, float h,
                            uint32_t fg, uint32_t bg, uint32_t bd)
{
    struct component_time* time = malloc(sizeof(struct component_time));
    memset(time, 0, sizeof(struct component_time));
    component_init(&time->component, CPT_TIME, x, y, w, h, fg, bg, bd);
    return &time->component;
}

static void component_update_shell(struct component* component)
{
    struct component_shell* data = (struct component_shell*)component;

    int length = 0;
    int nbytes = 0;
    char* result = NULL;
    char buffer[BUFSIZ];

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
    } else {
        data->output = "";
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

    if (has_battery) {
        snprintf(data->output, sizeof(data->output), "%' ' 3d%%", percent);
        snprintf(data->icon, sizeof(data->icon), (is_charging) ? "+" : "-");
    } else {
        snprintf(data->output, sizeof(data->output), "A/C");
        snprintf(data->icon, sizeof(data->icon), "=");
    }
}

static void component_update_time(struct component* component)
{
    struct component_time* data = (struct component_time*)component;

    time_t rawtime;
    time(&rawtime);

    struct tm* timeinfo = localtime(&rawtime);
    if (timeinfo) {
        snprintf(data->output, sizeof(data->output), "%02d:%02d",
            timeinfo->tm_hour, timeinfo->tm_min);
    }

    snprintf(data->icon, sizeof(data->icon), "♪ ");
}

static void (*component_update_map[])(struct component* component) =
{
    [CPT_SHELL] = component_update_shell,
    [CPT_POWER] = component_update_power,
    [CPT_TIME]  = component_update_time,
};

void component_update(struct component* component)
{
    component_update_map[component->type](component);
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
    [CPT_SHELL] = component_render_shell,
    [CPT_POWER] = component_render_power,
    [CPT_TIME]  = component_render_time,
};

void component_render(struct component* component)
{
    SLSDisableUpdate(g_connection);
    SLSOrderWindow(g_connection, component->window.id, 0, 0);
    CGContextClearRect(component->window.context, component->window.frame);

    set_component_color(CGContextSetRGBFillColor, component, bg_color);
    CGContextAddPath(component->window.context, component->window.background);
    CGContextFillPath(component->window.context);

    set_component_color(CGContextSetRGBFillColor, component, fg_color);
    component_render_map[component->type](component);

    set_component_color(CGContextSetRGBStrokeColor, component, bd_color);
    CGContextSetLineWidth(component->window.context, component->bd_width);
    CGContextAddPath(component->window.context, component->window.border);
    CGContextStrokePath(component->window.context);

    CGContextFlush(component->window.context);
    SLSOrderWindow(g_connection, component->window.id, 1, 0);
    SLSReenableUpdate(g_connection);
}
