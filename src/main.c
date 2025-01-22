#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct global {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct wl_shell *shell;
    struct wl_seat *seat;
    struct wl_pointer *pointer;
};

static void cursor_position(wl_fixed_t x, wl_fixed_t y) {
    printf("Cursor Position: (%d, %d)\n", wl_fixed_to_int(x), wl_fixed_to_int(y));
}

static void cursor_enter() {}

static void cursor_leave() {}

static void cursor_button() {}

static void cursor_axis() {}

static void cursor_frame() {}

static void cursor_axis_source() {}

static void cursor_axis_stop() {}

static void cursor_axis_discrete() {}

static void cursor_axis_value120() {}

static void cursor_axis_relative_direction() {}

static const struct wl_pointer_listener pointer_listener = {
    cursor_enter,
    cursor_leave,
    cursor_position,
    cursor_button,
    cursor_axis,
    cursor_frame,
    cursor_axis_source,
    cursor_axis_stop,
    cursor_axis_discrete,
    cursor_axis_value120,
    cursor_axis_relative_direction
};

static void registry_handle_global(void *data, struct wl_registry *registry,
                                   uint32_t name, const char *interface) {
    struct global *global = data;

    if (strcmp(interface, "wl_compositor") == 0) {
        global->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        global->shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
    } else if (strcmp(interface, "wl_seat") == 0) {
        global->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
    }
}

static void registry_handle_global_remove() {}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};

int main() {
    struct global global = {0};

    global.display = wl_display_connect(NULL);
    if (!global.display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return -1;
    }

    global.registry = wl_display_get_registry(global.display);
    wl_registry_add_listener(global.registry, &registry_listener, &global);
    wl_display_roundtrip(global.display); // Process events

    if (!global.compositor || !global.shell || !global.seat) {
        fprintf(stderr, "Failed to get compositor, shell, or seat\n");
        return -1;
    }


    global.surface = wl_compositor_create_surface(global.compositor);


    global.pointer = wl_seat_get_pointer(global.seat);
    wl_pointer_add_listener(global.pointer, &pointer_listener, &global);


    while (wl_display_dispatch(global.display) != -1) {
		// TODO: implement the rest of the solution
    }

    wl_display_disconnect(global.display);
    return 0;
}
