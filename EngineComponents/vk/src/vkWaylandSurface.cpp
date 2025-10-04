#include "vkRenderSystem.hpp"

#include <RenderSystemInterface.hpp>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-server.h>
#include <fmt/printf.h>
#include <xdg-shell-client-protocol.h>
#include <sys/mman.h>
#include <fcntl.h>


extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface xdg_popup_interface;
extern const struct wl_interface xdg_positioner_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;

inline const struct wl_interface *xdg_shell_types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	&xdg_positioner_interface,
	&xdg_surface_interface,
	&wl_surface_interface,
	&xdg_toplevel_interface,
	&xdg_popup_interface,
	&xdg_surface_interface,
	&xdg_positioner_interface,
	&xdg_toplevel_interface,
	&wl_seat_interface,
	NULL,
	NULL,
	NULL,
	&wl_seat_interface,
	NULL,
	&wl_seat_interface,
	NULL,
	NULL,
	&wl_output_interface,
	&wl_seat_interface,
	NULL,
	&xdg_positioner_interface,
	NULL,
};

inline const struct wl_message xdg_wm_base_requests[] = {
	{ "destroy", "", xdg_shell_types + 0 },
	{ "create_positioner", "n", xdg_shell_types + 4 },
	{ "get_xdg_surface", "no", xdg_shell_types + 5 },
	{ "pong", "u", xdg_shell_types + 0 },
};

inline const struct wl_message xdg_wm_base_events[] = {
	{ "ping", "u", xdg_shell_types + 0 },
};

inline const struct wl_interface xdg_wm_base_interface = {
	"xdg_wm_base", 6,
	4, xdg_wm_base_requests,
	1, xdg_wm_base_events,
};

inline const struct wl_message xdg_positioner_requests[] = {
	{ "destroy", "", xdg_shell_types + 0 },
	{ "set_size", "ii", xdg_shell_types + 0 },
	{ "set_anchor_rect", "iiii", xdg_shell_types + 0 },
	{ "set_anchor", "u", xdg_shell_types + 0 },
	{ "set_gravity", "u", xdg_shell_types + 0 },
	{ "set_constraint_adjustment", "u", xdg_shell_types + 0 },
	{ "set_offset", "ii", xdg_shell_types + 0 },
	{ "set_reactive", "3", xdg_shell_types + 0 },
	{ "set_parent_size", "3ii", xdg_shell_types + 0 },
	{ "set_parent_configure", "3u", xdg_shell_types + 0 },
};

inline const struct wl_interface xdg_positioner_interface = {
	"xdg_positioner", 6,
	10, xdg_positioner_requests,
	0, NULL,
};

inline const wl_message xdg_surface_requests[] = {
	{ "destroy", "", xdg_shell_types + 0 },
	{ "get_toplevel", "n", xdg_shell_types + 7 },
	{ "get_popup", "n?oo", xdg_shell_types + 8 },
	{ "set_window_geometry", "iiii", xdg_shell_types + 0 },
	{ "ack_configure", "u", xdg_shell_types + 0 },
};

inline const struct wl_message xdg_surface_events[] = {
	{ "configure", "u", xdg_shell_types + 0 },
};

inline const struct wl_interface xdg_surface_interface = {
	"xdg_surface", 6,
	5, xdg_surface_requests,
	1, xdg_surface_events,
};

static const struct wl_message xdg_toplevel_requests[] = {
	{ "destroy", "", xdg_shell_types + 0 },
	{ "set_parent", "?o", xdg_shell_types + 11 },
	{ "set_title", "s", xdg_shell_types + 0 },
	{ "set_app_id", "s", xdg_shell_types + 0 },
	{ "show_window_menu", "ouii", xdg_shell_types + 12 },
	{ "move", "ou", xdg_shell_types + 16 },
	{ "resize", "ouu", xdg_shell_types + 18 },
	{ "set_max_size", "ii", xdg_shell_types + 0 },
	{ "set_min_size", "ii", xdg_shell_types + 0 },
	{ "set_maximized", "", xdg_shell_types + 0 },
	{ "unset_maximized", "", xdg_shell_types + 0 },
	{ "set_fullscreen", "?o", xdg_shell_types + 21 },
	{ "unset_fullscreen", "", xdg_shell_types + 0 },
	{ "set_minimized", "", xdg_shell_types + 0 },
};

static const wl_message xdg_toplevel_events[] = {
	{ "configure", "iia", xdg_shell_types + 0 },
	{ "close", "", xdg_shell_types + 0 },
	{ "configure_bounds", "4ii", xdg_shell_types + 0 },
	{ "wm_capabilities", "5a", xdg_shell_types + 0 },
};

inline const wl_interface xdg_toplevel_interface = {
	"xdg_toplevel", 6,
	14, xdg_toplevel_requests,
	4, xdg_toplevel_events,
};

inline const wl_message xdg_popup_requests[] = {
	{ "destroy", "", xdg_shell_types + 0 },
	{ "grab", "ou", xdg_shell_types + 22 },
	{ "reposition", "3ou", xdg_shell_types + 24 },
};

inline const wl_message xdg_popup_events[] = {
	{ "configure", "iiii", xdg_shell_types + 0 },
	{ "popup_done", "", xdg_shell_types + 0 },
	{ "repositioned", "3u", xdg_shell_types + 0 },
};

inline const wl_interface xdg_popup_interface = {
	"xdg_popup", 6,
	3, xdg_popup_requests,
	3, xdg_popup_events,
};

namespace FlexKit
{
    struct waylandWindow {
        wl_display*		display			= nullptr;
        wl_registry*	registry		= nullptr;
        wl_compositor*	compositor		= nullptr;
        wl_surface*		surface			= nullptr;
    	wl_shm*			wlShm			= nullptr;
        xdg_wm_base*	wm_base			= nullptr;
		xdg_surface*	xdgSurface		= nullptr;
    	xdg_toplevel*	xdgToplevel		= nullptr;
    };

    static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
    {
        xdg_wm_base_pong(xdg_wm_base, serial);
    }

    static const xdg_wm_base_listener xdg_wm_base_listener = {
        .ping = xdg_wm_base_ping,
    };

	static void randname(char *buf)
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		long r = ts.tv_nsec;
		for (int i = 0; i < 6; ++i) {
			buf[i] = 'A'+(r&15)+(r&16)*2;
			r >>= 5;
		}
	}

	static int create_shm_file(void)
	{
		int retries = 100;
		do {
			char name[] = "/wl_shm-XXXXXX";
			randname(name + sizeof(name) - 7);
			--retries;
			int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
			if (fd >= 0) {
				shm_unlink(name);
				return fd;
			}
		} while (retries > 0 && errno == EEXIST);
		return -1;
	}

	int allocate_shm_file(size_t size)
	{
		int fd = create_shm_file();
		if (fd < 0)
			return -1;
		int ret;
		do {
			ret = ftruncate(fd, size);
		} while (ret < 0 && errno == EINTR);
		if (ret < 0) {
			close(fd);
			return -1;
		}
		return fd;
	}

	static void wl_buffer_release(void *data, wl_buffer *wl_buffer)
	{
		/* Sent by the compositor when it's no longer using this buffer */
		wl_buffer_destroy(wl_buffer);
	}

	static const wl_buffer_listener wl_buffer_listener = {
		.release = wl_buffer_release,
	};

	static wl_buffer* draw_frame(waylandWindow* state)
	{
		const int width = 640, height = 480;
		int stride = width * 4;
		int size = stride * height;

		int fd = allocate_shm_file(size);
		if (fd == -1) {
			return NULL;
		}

		uint32_t* data = (uint32_t*)mmap(NULL, size,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (data == MAP_FAILED) {
			close(fd);
			return NULL;
		}

		struct wl_shm_pool *pool = wl_shm_create_pool(state->wlShm, fd, size);
		struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0,
				width, height, stride, WL_SHM_FORMAT_XRGB8888);
		wl_shm_pool_destroy(pool);
		close(fd);

		/* Draw checkerboxed background */
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				if ((x + y / 8 * 8) % 16 < 8)
					data[y * width + x] = 0xFF666666;
				else
					data[y * width + x] = 0xFFEEEEEE;
			}
		}

		munmap(data, size);
		wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
		return buffer;
	}

	static void xdg_surface_configure(void* data, xdg_surface* xdg_surface, uint32_t serial)
	{
		fmt::print("xdg_surface_configure\n");
		waylandWindow* state = (waylandWindow*)data;
		xdg_surface_ack_configure(xdg_surface, serial);

		auto* buffer = draw_frame(state);
		wl_surface_attach(state->surface, buffer, 0, 0);
		wl_surface_commit(state->surface);
	}

	static const xdg_surface_listener xdg_surface_listener = {
		.configure = xdg_surface_configure,
	};

    void global_registry_handler(void* wlWindow, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
    {
        auto* window = (waylandWindow*)wlWindow;

        if (strcmp(interface, "wl_compositor") == 0) {
            window->compositor = (wl_compositor*)wl_registry_bind(registry,
                          id,
                          &wl_compositor_interface,
                          1);
        } else if (strcmp(interface, "xdg_wm_base") == 0) {
            window->wm_base = (xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
        	xdg_wm_base_add_listener(window->wm_base, &xdg_wm_base_listener, window);
        } else if (strcmp(interface, wl_shm_interface.name) == 0) {
	        window->wlShm = (wl_shm*)wl_registry_bind(registry, id, &wl_shm_interface, 1);
        }
    }

    void global_registry_remover(void* wlWindow, struct wl_registry *registry, uint32_t id)
    {
        auto* window = (waylandWindow*)wlWindow;
    }

    static const wl_registry_listener registry_listener = {
        global_registry_handler,
        global_registry_remover
    };

	struct WayLandSurface : IRenderWindow {
		ResourceHandle GetBackBuffer() const override{
			return InvalidHandle;
		}

		uint2 GetWH() const override{
			return {};
		}

		bool Present(const uint32_t syncInternal = 0, const uint32_t flags = 0) override{
			return true;
		}

		void Resize(const uint2 WH) override{

		}

		void Release() override {

		}

        waylandWindow wlWindow;
	};

	void ProcessEvents(const waylandWindow& window) {
		while (wl_display_dispatch(window.display) != -1) {
			/* This space deliberately left blank */
		}
	}

	void ProcessEvents(IRenderWindow& window)
	{
		auto& impl = static_cast<WayLandSurface&>(window);
		ProcessEvents(impl.wlWindow);
	}

    IRenderWindow* CreateWaylandSurface(IRenderSystem& renderSystem, uint2 WH, DeviceFormat)
    {
		auto& vkRS		= static_cast<VK_internal::vkRenderSystem&>(renderSystem);
    	auto& surface	= vkRS.allocator->allocate<WayLandSurface>();
		auto& wlWindow	= surface.wlWindow;

        wlWindow.display = wl_display_connect(nullptr);
        wlWindow.registry = wl_display_get_registry(wlWindow.display);

        wl_registry_add_listener(wlWindow.registry, &registry_listener, &wlWindow);
        wl_display_dispatch(wlWindow.display);
        wl_display_roundtrip(wlWindow.display);

        wlWindow.surface	= wl_compositor_create_surface(wlWindow.compositor);
    	wlWindow.xdgSurface	= xdg_wm_base_get_xdg_surface(wlWindow.wm_base, wlWindow.surface);

    	xdg_surface_add_listener(wlWindow.xdgSurface, &xdg_surface_listener, &wlWindow);
    	wlWindow.xdgToplevel = xdg_surface_get_toplevel(wlWindow.xdgSurface);

    	xdg_toplevel_set_title(wlWindow.xdgToplevel, "Hello World!");
    	wl_surface_commit(wlWindow.surface);

        return &surface;
    }
}


