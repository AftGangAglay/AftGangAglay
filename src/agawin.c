/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <poll.h>
#include <fcntl.h>
#include <errno.h>

static const int single_buffer_fb[] = {
	GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	GLX_RENDER_TYPE, GLX_RGBA_BIT,

	GLX_RED_SIZE, GLX_DONT_CARE,
	GLX_GREEN_SIZE, GLX_DONT_CARE,
	GLX_BLUE_SIZE, GLX_DONT_CARE,
	GLX_DEPTH_SIZE, GLX_DONT_CARE,

	None
};

static const int double_buffer_fb[] = {
	GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	GLX_RENDER_TYPE, GLX_RGBA_BIT,
	GLX_DOUBLEBUFFER, True,

	GLX_RED_SIZE, GLX_DONT_CARE,
	GLX_GREEN_SIZE, GLX_DONT_CARE,
	GLX_BLUE_SIZE, GLX_DONT_CARE,
	GLX_DEPTH_SIZE, GLX_DONT_CARE,

	None
};

static int aga_xerr_handler(Display* dpy, XErrorEvent* err) {
	char buff[2048 + 1] = { 0 };

	XGetErrorText(dpy, err->error_code, buff, sizeof(buff));
	aga_fatal("xlib: %s", buff);
}

enum af_err aga_mkctxdpy(struct aga_ctx* ctx) {
	GLXFBConfig* fb;
	int n_fb;
	XVisualInfo* vi;

	AF_PARAM_CHK(ctx);

	AF_VERIFY(ctx->dpy = XOpenDisplay(0), AF_ERR_UNKNOWN);

	{
		int fl;

		ctx->dpy_fd = ConnectionNumber(ctx->dpy);
		if((fl = fcntl(ctx->dpy_fd, F_GETFL)) == -1) aga_errno_chk("fcntl");
		fl |= O_NONBLOCK; /* TODO: We could use `O_ASYNC' for this. */
		if(fcntl(ctx->dpy_fd, F_SETFL, fl) == -1) aga_errno_chk("fcntl");
	}

	ctx->screen = DefaultScreen(ctx->dpy);

	ctx->wm_delete = XInternAtom(ctx->dpy, "WM_DELETE_WINDOW", True);

	ctx->double_buffered = AF_TRUE;
	fb = glXChooseFBConfig(ctx->dpy, ctx->screen, double_buffer_fb, &n_fb);
	if(!fb) {
		ctx->double_buffered = AF_FALSE;
		fb = glXChooseFBConfig(ctx->dpy, ctx->screen, single_buffer_fb, &n_fb);
		AF_VERIFY(fb, AF_ERR_UNKNOWN);
	}

	/* TODO: `glXGetVisualFromFBConfig' is too new for us. */
	vi = glXGetVisualFromFBConfig(ctx->dpy, *fb);
	AF_VERIFY(vi, AF_ERR_UNKNOWN);

	ctx->glx = glXCreateContext(ctx->dpy, vi, 0, True);
	AF_VERIFY(ctx->glx, AF_ERR_UNKNOWN);

	XFree(vi);

	XSetErrorHandler(aga_xerr_handler);

	return AF_ERR_NONE;
}

enum af_err aga_killctxdpy(struct aga_ctx* ctx) {
	AF_PARAM_CHK(ctx);

	glXDestroyContext(ctx->dpy, ctx->glx);

	XCloseDisplay(ctx->dpy);

	return AF_ERR_NONE;
}

enum af_err aga_mkwin(struct aga_ctx* ctx, struct aga_win* win) {
	af_ulong_t black, white;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	black = BlackPixel(ctx->dpy, ctx->screen);
	white = WhitePixel(ctx->dpy, ctx->screen);

	win->xwin = XCreateSimpleWindow(
		ctx->dpy, RootWindow(ctx->dpy, ctx->screen),
		0, 0, ctx->settings.width, ctx->settings.height,
		8, white, black);

	XSetStandardProperties(
		ctx->dpy, win->xwin, "Aft Gang Aglay", "", None,
		ctx->argv, ctx->argc, 0);

	XSelectInput(ctx->dpy, win->xwin, NoEventMask);
	XSetWMProtocols(ctx->dpy, win->xwin, &ctx->wm_delete, 1);

	XMapRaised(ctx->dpy, win->xwin);

	return AF_ERR_NONE;
}

enum af_err aga_killwin(struct aga_ctx* ctx, struct aga_win* win) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	XDestroyWindow(ctx->dpy, win->xwin);

	return AF_ERR_NONE;
}

enum af_err aga_glctx(struct aga_ctx* ctx, struct aga_win* win) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	AF_VERIFY(
		glXMakeContextCurrent(ctx->dpy, win->xwin, win->xwin, ctx->glx),
		AF_ERR_UNKNOWN);

	return AF_ERR_NONE;
}

enum af_err aga_swapbuf(struct aga_ctx* ctx, struct aga_win* win) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	if(ctx->double_buffered) glXSwapBuffers(ctx->dpy, win->xwin);

	return AF_ERR_NONE;
}

enum af_err aga_poll(struct aga_ctx* ctx) {
	XEvent event;
	struct pollfd pollfd;
	int rdy;

	AF_PARAM_CHK(ctx);

	pollfd.fd = ctx->dpy_fd;
	pollfd.events = POLLIN;

	if((rdy = poll(&pollfd, 1, 0)) == -1) aga_errno_chk("poll");

	if(rdy && (pollfd.revents & POLLIN)) {
		while(XPending(ctx->dpy) > 0) {
			XNextEvent(ctx->dpy, &event);

			switch(event.type) {
				default: break;

				case ClientMessage: {
					if(event.xclient.window == ctx->win.xwin) {
						ctx->die = AF_TRUE;
					}
					break;
				}
			}
		}
	}

	return AF_ERR_NONE;
}
