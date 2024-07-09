#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "vroot.h"

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

void point_polar(XPoint* p, float cx, float cy, float r, float a){
	p->x = rintf(cx + r * cos(a));
	p->y = rintf(cy - r * sin(a));
}

void point_rotate(XPoint* p, float cx, float cy, float x, float y, float a){
	float dx = x - cx;
	float dy = -(y - cy);
	float a0 = atan2(dy, dx);
	float r = sqrt(dx * dx + dy * dy);
	p->x = rintf(cx + r * cos(a0 + a));
	p->y = rintf(cy - r * sin(a0 + a));
}

void point_interpolate(XPoint* p, XPoint* p1, XPoint* p2, float t){
	p->x = rintf(p1->x * (1 - t) + p2->x * t);
	p->y = rintf(p1->y * (1 - t) + p2->y * t);
}

int main() {
	Display* dpy = XOpenDisplay(getenv("DISPLAY"));
	Window root = DefaultRootWindow(dpy);
	XWindowAttributes wa;
	XGetWindowAttributes(dpy, root, &wa);
	Pixmap double_buffer = XCreatePixmap(dpy, root, wa.width, wa.height, wa.depth);

	GC gc = XCreateGC(dpy, root, 0, NULL);
	
	XSetBackground(dpy, gc, BlackPixelOfScreen(DefaultScreenOfDisplay(dpy)));
	XFillRectangle(dpy, double_buffer, gc, 0, 0, wa.width, wa.height);
	XCopyArea(dpy, double_buffer, root, gc, 0, 0, wa.width, wa.height, 0, 0);

	float cx = (float)wa.width / 2;
	float cy = (float)wa.height / 2;
	float r = (float)wa.height / 16;
	
	float t = 0.;
	float t_effective = 0.;
	float angle = 0.;

	XRectangle clip_rect[1] = {{ .x = cx - r - 1, .y = cy - r - 1, .width = 2 * r + 2, .height = 2 * r + 2 }};
	int n_sub = 10;
	int n_points = 3 * (n_sub + 1);
	XPoint T_initial[n_points];
	point_polar(&T_initial[0], cx, cy, r, 0);
	point_polar(&T_initial[n_sub + 1], cx, cy, r, 2 * M_PI / 3);
	point_polar(&T_initial[2 * n_sub + 2], cx, cy, r, 4 * M_PI / 3);
	for (int i = 1; i < n_sub + 1; i++) {
		float t = (float)i / (n_sub + 1);
		point_interpolate(&T_initial[i], &T_initial[0], &T_initial[n_sub + 1], t);
		point_interpolate(&T_initial[n_sub + 1 + i], &T_initial[n_sub + 1], &T_initial[2 * n_sub + 2], t);
		point_interpolate(&T_initial[2 * n_sub + 2 + i], &T_initial[2 * n_sub + 2], &T_initial[0], t);
	}
	float radii[n_points];
	for (int i = 0; i < n_points; i++) {
		radii[i] = sqrt((T_initial[i].x - cx) * (T_initial[i].x - cx) + (T_initial[i].y - cy) * (T_initial[i].y - cy)) / r;
		radii[i] = powf(radii[i], 0.6);
	}
	XPoint T[n_points];

	struct timespec delay = (struct timespec){ .tv_sec = 0, .tv_nsec = 1000000 };

	while (1) {
		XSetForeground(dpy, gc, BlackPixelOfScreen(DefaultScreenOfDisplay(dpy)));
		XFillRectangle(dpy, double_buffer, gc, cx - r - 1, cy - r - 1, 2 * r + 2, 2 * r + 2);

		for (int i = 0; i < n_points; i++) {
			t_effective = floor(t * 6) + powf(fmod(t * 6, 1.), radii[i]);
			angle = M_PI / 2 + M_PI / 3 * (floor(t_effective) + sin(fmod(t_effective, 1.) * M_PI / 2));
			point_rotate(&T[i], cx, cy, T_initial[i].x, T_initial[i].y, angle);
		}
		XSetForeground(dpy, gc, WhitePixelOfScreen(DefaultScreenOfDisplay(dpy)));
		XFillPolygon(dpy, double_buffer, gc, T, n_points, Nonconvex, CoordModeOrigin);

		XSetClipRectangles(dpy, gc, 0, 0, clip_rect, 1, Unsorted);
		XCopyArea(dpy, double_buffer, root, gc, 0, 0, wa.width, wa.height, 0, 0);
		XFlush(dpy);

		t = fmod(t + 0.000251, 1.);

		nanosleep(&delay, NULL);
	}
	XCloseDisplay(dpy);
}
