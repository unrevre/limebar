#include "component.h"

#include <unistd.h>

int g_connection;

extern int SLSMainConnectionID(void);

int main(int argc, char* argv[])
{
    g_connection = SLSMainConnectionID();

    struct component* front = component_create_null(
        0.0075f, 0.0075f, 0.1035f, 0.0225f,
        0xff393939, 0xfff7abac, 0xffa09f93,
        WIN_BEG);
    struct component* end = component_create_null(
        0.9750f, 0.0075f, 0.0180f, 0.0225f,
        0xff393939, 0xffc2c2e0, 0xffa09f93,
        WIN_END);

    component_render(front);
    component_render(end);

    struct component* title = component_create_shell(
        0.1105f, 0.0075f, 0.7560f, 0.0225f,
        0xff393939, 0xffffe0a3, 0xffa09f93,
        "yabai -m query --windows --window | jq -r '.app + \" | \" + .title'",
        WIN_MID);
    struct component* power = component_create_power(
        0.8660f, 0.0075f, 0.0550f, 0.0225f,
        0xff393939, 0xffc1e0c1, 0xffa09f93,
        WIN_MID);
    struct component* time = component_create_time(
        0.9205f, 0.0075f, 0.0550f, 0.0225f,
        0xff393939, 0xffa3c1e0, 0xffa09f93,
        WIN_MID);

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
