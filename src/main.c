#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xfixes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAGNIFIER_SIZE 100
#define ZOOM_FACTOR 2
#define DOT_SIZE 4
#define DOT_OPACITY 0.5 // 0 -> fully transparent, 1 -> fully visible

unsigned long blend_colors(unsigned long color1, unsigned long color2, double opacity)
{
	int r1 = (color1 >> 16) & 0xFF;
	int g1 = (color1 >> 8) & 0xFF;
	int b1 = color1 & 0xFF;

	int r2 = (color2 >> 16) & 0xFF;
	int g2 = (color2 >> 8) & 0xFF;
	int b2 = color2 & 0xFF;

	int r = (int)(r1 * (1 - opacity) + r2 * opacity);
	int g = (int)(g1 * (1 - opacity) + g2 * opacity);
	int b = (int)(b1 * (1 - opacity) + b2 * opacity);

	return (r << 16) | (g << 8) | b;
}

void scale_image(XImage *src, XImage *dest, int zoom_factor)
{
	for (int y = 0; y < dest->height; y++) {
		for (int x = 0; x < dest->width; x++) {
			int src_x = x / zoom_factor;
			int src_y = y / zoom_factor;
			XPutPixel(dest, x, y, XGetPixel(src, src_x, src_y));
		}
	}
}

int main()
{
	Display *display;
	Window root, magnifier;
	XGCValues gc_values;
	GC gc;
	XEvent event;
	XFixesCursorImage *cursor_image;
	int screen;
	unsigned long black, red;
	int x, y;
	XImage *src_image, *dest_image;

	display = XOpenDisplay(NULL);
	if (display == NULL)
	{
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}

	screen = DefaultScreen(display);
	root = RootWindow(display, screen);
	black = BlackPixel(display, screen);

	XColor color;
	if (!XParseColor(display, DefaultColormap(display, screen), "red", &color))
	{
		fprintf(stderr, "Failed to parse color 'red'\n");
		exit(1);
	}
	red = color.pixel;

	XSetWindowAttributes attrs;
	attrs.override_redirect = True; // enable borderless
	attrs.background_pixel = black;

	magnifier = XCreateWindow(display, root, 0, 0, MAGNIFIER_SIZE, MAGNIFIER_SIZE, 0,
							  CopyFromParent, InputOutput, CopyFromParent,
							  CWOverrideRedirect | CWBackPixel, &attrs);

	XSelectInput(display, magnifier, ExposureMask);
	XMapWindow(display, magnifier);

	gc = XCreateGC(display, magnifier, 0, &gc_values);

	int event_base, error_base;
	if (!XFixesQueryExtension(display, &event_base, &error_base))
	{
		fprintf(stderr, "XFixes extension not available\n");
		exit(1);
	}

	dest_image = XCreateImage(display, DefaultVisual(display, screen), DefaultDepth(display, screen),
							  ZPixmap, 0, NULL, MAGNIFIER_SIZE, MAGNIFIER_SIZE, 32, 0);
	dest_image->data = malloc(dest_image->bytes_per_line * dest_image->height);

	int screen_width = DisplayWidth(display, screen);
	int screen_height = DisplayHeight(display, screen);

	if (XGrabPointer(display, root, False, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime) != GrabSuccess)
	{
		fprintf(stderr, "Failed to grab the pointer.\n");
		exit(1);
	}

	while (1)
	{
		XFixesSelectCursorInput(display, root, XFixesDisplayCursorNotifyMask);
		cursor_image = XFixesGetCursorImage(display);
		x = cursor_image->x;
		y = cursor_image->y;
		
		// make captured aria within screen bounds
		int capture_x = x - MAGNIFIER_SIZE / (2 * ZOOM_FACTOR);
		int capture_y = y - MAGNIFIER_SIZE / (2 * ZOOM_FACTOR);
		int capture_width = MAGNIFIER_SIZE / ZOOM_FACTOR;
		int capture_height = MAGNIFIER_SIZE / ZOOM_FACTOR;

		if (capture_x < 0) capture_x = 0;
		if (capture_y < 0) capture_y = 0;
		if (capture_x + capture_width > screen_width) capture_x = screen_width - capture_width;
		if (capture_y + capture_height > screen_height) capture_y = screen_height - capture_height;

		src_image = XGetImage(display, root, capture_x, capture_y, capture_width, capture_height, AllPlanes, ZPixmap);

		if (src_image && dest_image)
		{
			scale_image(src_image, dest_image, ZOOM_FACTOR);

			// draws the dot in the center
			int center_x = MAGNIFIER_SIZE / 2;
			int center_y = MAGNIFIER_SIZE / 2;
			for (int dy = -DOT_SIZE / 2; dy <= DOT_SIZE / 2; dy++)
			{
				for (int dx = -DOT_SIZE / 2; dx <= DOT_SIZE / 2; dx++)
				{
					int px = center_x + dx;
					int py = center_y + dy;
					if (px >= 0 && px < MAGNIFIER_SIZE && py >= 0 && py < MAGNIFIER_SIZE)
					{
						unsigned long pixel = XGetPixel(dest_image, px, py);
						unsigned long blended_pixel = blend_colors(pixel, red, DOT_OPACITY);
						XPutPixel(dest_image, px, py, blended_pixel);
					}
				}
			}

			XPutImage(display, magnifier, gc, dest_image, 0, 0, 0, 0, MAGNIFIER_SIZE, MAGNIFIER_SIZE);
			XDestroyImage(src_image);
		}

		XMoveWindow(display, magnifier, x + 20, y + 20);

		while (XPending(display))
		{
			XNextEvent(display, &event);

			if (event.type == Expose)
			{
				if (src_image && dest_image)
				{
					XPutImage(display, magnifier, gc, dest_image, 0, 0, 0, 0, MAGNIFIER_SIZE, MAGNIFIER_SIZE);
				}
			}
			else if (event.type == ButtonPress)
			{
				if (event.xbutton.button == Button1)
				{
					int center_x = x;
					int center_y = y;

					if (center_x >= 0 && center_x < screen_width && center_y >= 0 && center_y < screen_height)
					{
						XImage *pixel_image = XGetImage(display, root, center_x, center_y, 1, 1, AllPlanes, ZPixmap);
						if (pixel_image)
						{
							unsigned long pixel = XGetPixel(pixel_image, 0, 0);
							int r = (pixel >> 16) & 0xFF;
							int g = (pixel >> 8) & 0xFF;
							int b = pixel & 0xFF;
							printf("#%02X%02X%02X\n", r, g, b);
							XDestroyImage(pixel_image);
							
							XUngrabPointer(display, CurrentTime); // just to be safe

							XDestroyImage(dest_image);
							XFreeGC(display, gc);
							XDestroyWindow(display, magnifier);
							XCloseDisplay(display);
							return EXIT_SUCCESS;
						}
					}
					else
					{
						fprintf(stderr, "Cursor is outside screen bounds\n");
					}
				}
			}
		}

		XFree(cursor_image);

		usleep(10000);
	}


	return EXIT_SUCCESS;
}
