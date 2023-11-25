/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_X_WIN_H
#define AGA_X_WIN_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

static const int single_buffer_fb[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,

		GLX_RED_SIZE, GLX_DONT_CARE,
		GLX_GREEN_SIZE, GLX_DONT_CARE,
		GLX_BLUE_SIZE, GLX_DONT_CARE,
		GLX_DEPTH_SIZE, 1,

		None
};

static const int double_buffer_fb[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DOUBLEBUFFER, True,

		GLX_RED_SIZE, GLX_DONT_CARE,
		GLX_GREEN_SIZE, GLX_DONT_CARE,
		GLX_BLUE_SIZE, GLX_DONT_CARE,
		GLX_DEPTH_SIZE, 1,

		None
};

static void aga_centreptr(struct aga_ctx* ctx) {
	int mid_x = (int) ctx->settings.width / 2;
	int mid_y = (int) ctx->settings.height / 2;
	XWarpPointer(
		ctx->winenv.dpy, ctx->win.xwin, ctx->win.xwin,
		0, 0, 0, 0, mid_x, mid_y);
}

static int aga_xerr_handler(Display* dpy, XErrorEvent* err) {
	aga_fixed_buf_t buf = { 0 };

	XGetErrorText(dpy, err->error_code, buf, sizeof(buf));

	aga_log(__FILE__, "err: xlib: %s", buf);

	return 0;
}

enum af_err aga_mkctxdpy(struct aga_ctx* ctx, const char* display) {
	GLXFBConfig* fb;
	int n_fb;
	XVisualInfo* vi;

	AF_PARAM_CHK(ctx);

	XSetErrorHandler(aga_xerr_handler);

	AF_VERIFY(ctx->winenv.dpy = XOpenDisplay(display), AF_ERR_UNKNOWN);

	{
		int fl;

		ctx->winenv.dpy_fd = ConnectionNumber(ctx->winenv.dpy);
		if((fl = fcntl(ctx->winenv.dpy_fd, F_GETFL)) == -1) {
			return aga_af_errno(__FILE__, "fcntl");
		}
		fl |= O_NONBLOCK; /* TODO: We could use `O_ASYNC' for this. */
		if(fcntl(ctx->winenv.dpy_fd, F_SETFL, fl) == -1) {
			return aga_af_errno(__FILE__, "fcntl");
		}
	}

	ctx->winenv.screen = DefaultScreen(ctx->winenv.dpy);

	ctx->winenv.wm_delete = XInternAtom(ctx->winenv.dpy, "WM_DELETE_WINDOW", True);

	{
		int min, max;
		struct aga_keymap* keymap = &ctx->keymap;

		XDisplayKeycodes(ctx->winenv.dpy, &min, &max);

		keymap->keycode_len = max - min;
		keymap->keycode_min = min;

		ctx->keymap.keymap = XGetKeyboardMapping(
			ctx->winenv.dpy, min, keymap->keycode_len,
			&keymap->keysyms_per_keycode);

		ctx->keystates = calloc(
			keymap->keysyms_per_keycode * keymap->keycode_len,
			sizeof(af_bool_t));
		AF_VERIFY(ctx->keystates, AF_ERR_MEM);
	}

	ctx->winenv.double_buffered = AF_TRUE;
	fb = glXChooseFBConfig(ctx->winenv.dpy, ctx->winenv.screen,
						   double_buffer_fb, &n_fb);
	if(!fb) {
		ctx->winenv.double_buffered = AF_FALSE;
		fb = glXChooseFBConfig(ctx->winenv.dpy, ctx->winenv.screen,
							   single_buffer_fb, &n_fb);
		AF_VERIFY(fb, AF_ERR_UNKNOWN);
	}

	/* TODO: `glXGetVisualFromFBConfig' is too new for us. */
	vi = glXGetVisualFromFBConfig(ctx->winenv.dpy, *fb);
	AF_VERIFY(vi, AF_ERR_UNKNOWN);

	ctx->winenv.glx = glXCreateContext(ctx->winenv.dpy, vi, 0, True);
	AF_VERIFY(ctx->winenv.glx, AF_ERR_UNKNOWN);

	XFree(vi);

	return AF_ERR_NONE;
}

enum af_err aga_killctxdpy(struct aga_ctx* ctx) {
	AF_PARAM_CHK(ctx);

	glXDestroyContext(ctx->winenv.dpy, ctx->winenv.glx);

	XFree(ctx->keymap.keymap);
	free(ctx->keystates);

	XCloseDisplay(ctx->winenv.dpy);

	return AF_ERR_NONE;
}

enum af_err aga_mkwin(
		struct aga_ctx* ctx, struct aga_win* win, int argc, char** argv) {

	af_ulong_t black, white;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	black = BlackPixel(ctx->winenv.dpy, ctx->winenv.screen);
	white = WhitePixel(ctx->winenv.dpy, ctx->winenv.screen);

	win->xwin = XCreateSimpleWindow(
		ctx->winenv.dpy, RootWindow(ctx->winenv.dpy, ctx->winenv.screen),
		0, 0, ctx->settings.width, ctx->settings.height,
		8, white, black);

	XSetStandardProperties(
		ctx->winenv.dpy, win->xwin, "Aft Gang Aglay", "", None, argv, argc, 0);

	XSelectInput(
		ctx->winenv.dpy, win->xwin,
		KeyPressMask | KeyReleaseMask | PointerMotionMask);
	XSetWMProtocols(ctx->winenv.dpy, win->xwin, &ctx->winenv.wm_delete, 1);

	XMapRaised(ctx->winenv.dpy, win->xwin);

	return AF_ERR_NONE;
}

enum af_err aga_killwin(struct aga_ctx* ctx, struct aga_win* win) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	XDestroyWindow(ctx->winenv.dpy, win->xwin);

	return AF_ERR_NONE;
}

enum af_err aga_glctx(struct aga_ctx* ctx, struct aga_win* win) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	AF_VERIFY(
		glXMakeContextCurrent(
			ctx->winenv.dpy, win->xwin, win->xwin, ctx->winenv.glx),
		AF_ERR_UNKNOWN);
	XSetInputFocus(ctx->winenv.dpy, win->xwin, RevertToNone, CurrentTime);

	aga_centreptr(ctx);

	{
		Cursor hidden;
		Pixmap bitmap;
		XColor black = { 0 };
		char empty[8] = { 0 };

		bitmap = XCreateBitmapFromData(
			ctx->winenv.dpy, win->xwin, empty, 1, 1);
		AF_VERIFY(bitmap, AF_ERR_UNKNOWN);

		hidden = XCreatePixmapCursor(
			ctx->winenv.dpy, bitmap, bitmap, &black, &black, 0, 0);
		XDefineCursor(ctx->winenv.dpy, win->xwin, hidden);
		XFreePixmap(ctx->winenv.dpy, bitmap);
		XFreeCursor(ctx->winenv.dpy, hidden);
	}

	{
		Font font;
		int nfonts = 0;
		XFontStruct* info;
		static const char* const fnames[] = {
			"*bold*iso8859*",
			"*iso8859*",
			"*bold*",
			"*"
		};
		unsigned currentf = 0;

		while(AF_TRUE) {
			char** fontname;
			if(currentf >= AF_ARRLEN(fnames)) {
				aga_log(__FILE__, "err: no fonts available");
				return AF_ERR_BAD_OP;
			}
			aga_log(__FILE__, "Trying font pattern `%s'", fnames[currentf]);
			fontname = XListFonts(ctx->winenv.dpy, fnames[currentf], 1, &nfonts);
			if(nfonts) {
				aga_log(__FILE__, "Using x11 font `%s'", *fontname);
				XFreeFontNames(fontname);
				break;
			}
			XFreeFontNames(fontname);
			currentf++;
		}

		info = XLoadQueryFont(ctx->winenv.dpy, fnames[currentf]);

		AF_VERIFY(info, AF_ERR_UNKNOWN);
		font = info->fid;

		glXUseXFont(font, 0, 256, AGA_FONT_LIST_BASE);

		XUnloadFont(ctx->winenv.dpy, font);
	}

	return AF_ERR_NONE;
}

enum af_err aga_swapbuf(struct aga_ctx* ctx, struct aga_win* win) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	if(ctx->winenv.double_buffered) glXSwapBuffers(ctx->winenv.dpy, win->xwin);

	return AF_ERR_NONE;
}

enum af_err aga_poll(struct aga_ctx* ctx) {
	XEvent event;
	struct pollfd pollfd;
	int rdy;

	AF_PARAM_CHK(ctx);

	pollfd.fd = ctx->winenv.dpy_fd;
	pollfd.events = POLLIN;

	if((rdy = poll(&pollfd, 1, 0)) == -1) {
		return aga_af_errno(__FILE__, "poll");
	}

	if(rdy && (pollfd.revents & POLLIN)) {
		while(XPending(ctx->winenv.dpy) > 0) {
			af_bool_t press = AF_FALSE;

			XNextEvent(ctx->winenv.dpy, &event);

			switch(event.type) {
				default: break;

				case KeyPress: press = AF_TRUE;
					AF_FALLTHROUGH;
					/* FALLTHRU */
				case KeyRelease: {
					unsigned keycode = event.xkey.keycode;
					af_size_t keysym_idx =
						(keycode - ctx->keymap.keycode_min) *
						ctx->keymap.keysyms_per_keycode;
					af_ulong_t keysym = ctx->keymap.keymap[keysym_idx];

					ctx->keystates[keysym] = press;

					break;
				}

				case MotionNotify: {
					int mid_x = (int) ctx->settings.width / 2;
					int mid_y = (int) ctx->settings.height / 2;

					if(event.xmotion.x != mid_x || event.xmotion.y != mid_y) {
						aga_centreptr(ctx);
					}

					ctx->pointer_dx = event.xmotion.x - mid_x;
					ctx->pointer_dy = event.xmotion.y - mid_y;

					break;
				}

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

#endif
