#include <Carbon/Carbon.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int g_connection;

#include "component.c"
#include "font.c"
#include "window.c"

extern int SLSMainConnectionID(void);

int main(int argc, char* argv[])
{
    g_connection = SLSMainConnectionID();

    struct component* title = component_create_shell(
        0.1875f, 0.0075f, 0.6250f, 0.0225f,
        0xff2d2d2d, 0xffe8e6df, 0xffffcc66,
        "yabai -m query --windows --window | grep title | cut -d'\"' -f4");
    struct component* power = component_create_power(
        0.8750f, 0.0075f, 0.0500f, 0.0225f,
        0xff2d2d2d, 0xffe8e6df, 0xff99cc99);
    struct component* time = component_create_time(
        0.9375f, 0.0075f, 0.0500f, 0.0225f,
        0xff2d2d2d, 0xffe8e6df, 0xff6699cc);

    for (;;) {
        component_update(title);
        component_render(title);

        component_update(power);
        component_render(power);

        component_update(time);
        component_render(time);

        sleep(1);
    }

    return 0;
}
