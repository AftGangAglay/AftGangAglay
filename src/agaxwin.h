/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_X_WIN_H
#define AGA_X_WIN_H

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

static void aga_centreptr(struct aga_winenv* env, struct aga_win* win) {
	int mid_x = (int) win->width / 2;
	int mid_y = (int) win->height / 2;
	XWarpPointer(env->dpy, win->win.xwin, win->win.xwin, 0, 0, 0, 0, mid_x, mid_y);
}

static int aga_xerr_handler(Display* dpy, XErrorEvent* err) {
	aga_fixed_buf_t buf = { 0 };

	XGetErrorText(dpy, err->error_code, buf, sizeof(buf));

	aga_log(__FILE__, "err: xlib: %s", buf);

	return 0;
}

enum af_err aga_mkwinenv(struct aga_winenv* env, const char* display) {
	GLXFBConfig* fb;
	int n_fb;
	XVisualInfo* vi;

	AF_PARAM_CHK(env);

	XSetErrorHandler(aga_xerr_handler);

	AF_VERIFY(env->dpy = XOpenDisplay(display), AF_ERR_UNKNOWN);

	{
		int fl;

		env->dpy_fd = ConnectionNumber(env->dpy);
		if((fl = fcntl(env->dpy_fd, F_GETFL)) == -1) {
			return aga_af_errno(__FILE__, "fcntl");
		}
		fl |= O_NONBLOCK; /* TODO: We could use `O_ASYNC' for this. */
		if(fcntl(env->dpy_fd, F_SETFL, fl) == -1) {
			return aga_af_errno(__FILE__, "fcntl");
		}
	}

	env->screen = DefaultScreen(env->dpy);
	env->wm_delete = XInternAtom(env->dpy, "WM_DELETE_WINDOW", True);

	env->double_buffered = AF_TRUE;
	fb = glXChooseFBConfig(env->dpy, env->screen, double_buffer_fb, &n_fb);
	if(!fb) {
		env->double_buffered = AF_FALSE;
		fb = glXChooseFBConfig(env->dpy, env->screen, single_buffer_fb, &n_fb);
		AF_VERIFY(fb, AF_ERR_UNKNOWN);
	}

	/* TODO: `glXGetVisualFromFBConfig' is too new for us. */
	vi = glXGetVisualFromFBConfig(env->dpy, *fb);
	AF_VERIFY(vi, AF_ERR_UNKNOWN);

	env->glx = glXCreateContext(env->dpy, vi, 0, True);
	AF_VERIFY(env->glx, AF_ERR_UNKNOWN);

	XFree(vi);

	return AF_ERR_NONE;
}

enum af_err aga_killwinenv(struct aga_winenv* env) {
	AF_PARAM_CHK(env);

	glXDestroyContext(env->dpy, env->glx);

	XCloseDisplay(env->dpy);

	return AF_ERR_NONE;
}

enum af_err aga_mkkeymap(struct aga_keymap* keymap, struct aga_winenv* env) {
	int min, max;

	AF_PARAM_CHK(keymap);
	AF_PARAM_CHK(env);

	XDisplayKeycodes(env->dpy, &min, &max);

	keymap->keycode_len = max - min;
	keymap->keycode_min = min;

	keymap->keymap = XGetKeyboardMapping(
		env->dpy, min, keymap->keycode_len, &keymap->keysyms_per_keycode);

	keymap->keystates = calloc(
		keymap->keysyms_per_keycode * keymap->keycode_len,
		sizeof(af_bool_t));
	AF_VERIFY(keymap->keystates, AF_ERR_MEM);

	return AF_ERR_NONE;
}

enum af_err aga_killkeymap(struct aga_keymap* keymap) {
	AF_PARAM_CHK(keymap);

	XFree(keymap->keymap);
	free(keymap->keystates);

	return AF_ERR_NONE;
}

enum af_err aga_mkwin(
		af_size_t width, af_size_t height, struct aga_winenv* env,
		struct aga_win* win, int argc, char** argv) {

	af_ulong_t black, white;
	long mask = KeyPressMask | KeyReleaseMask | PointerMotionMask;

	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	black = BlackPixel(env->dpy, env->screen);
	white = WhitePixel(env->dpy, env->screen);

	win->width = width;
	win->height = height;

	win->win.xwin = XCreateSimpleWindow(
		env->dpy, RootWindow(env->dpy, env->screen), 0, 0, width, height, 8,
		white, black);

	XSetStandardProperties(
		env->dpy, win->win.xwin, "Aft Gang Aglay", "", None, argv, argc, 0);

	XSelectInput(
		env->dpy, win->win.xwin, mask);
	XSetWMProtocols(env->dpy, win->win.xwin, &env->wm_delete, 1);

	XMapRaised(env->dpy, win->win.xwin);

	return AF_ERR_NONE;
}

enum af_err aga_killwin(struct aga_winenv* env, struct aga_win* win) {
	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	XDestroyWindow(env->dpy, win->win.xwin);

	return AF_ERR_NONE;
}

enum af_err aga_glctx(struct aga_winenv* env, struct aga_win* win) {
	static const char* const names[] = {
		"*bold*iso8859*",
		"*iso8859*",
		"*bold*",
		"*"
	};

	Cursor cur;
	Pixmap bitmap;
	XColor black = { 0 };
	char empty[8] = { 0 };

	Font font;
	int nfonts = 0;
	XFontStruct* info;
	unsigned current = 0;

	int res;

	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	res = glXMakeContextCurrent(
		env->dpy, win->win.xwin, win->win.xwin, env->glx);
	AF_VERIFY(res, AF_ERR_UNKNOWN);

	XSetInputFocus(env->dpy, win->win.xwin, RevertToNone, CurrentTime);

	aga_centreptr(env, win);

	bitmap = XCreateBitmapFromData(env->dpy, win->win.xwin, empty, 1, 1);
	AF_VERIFY(bitmap, AF_ERR_UNKNOWN);

	cur = XCreatePixmapCursor(env->dpy, bitmap, bitmap, &black, &black, 0, 0);
	XDefineCursor(env->dpy, win->win.xwin, cur);
	XFreePixmap(env->dpy, cur);
	XFreeCursor(env->dpy, cur);

	while(AF_TRUE) {
		char** fontname;
		if(current >= AF_ARRLEN(names)) {
			aga_log(__FILE__, "err: no fonts available");
			return AF_ERR_BAD_OP;
		}

		aga_log(__FILE__, "Trying font pattern `%s'", names[current]);
		fontname = XListFonts(env->dpy, names[current], 1, &nfonts);
		if(nfonts) {
			aga_log(__FILE__, "Using x11 font `%s'", *fontname);
			XFreeFontNames(fontname);
			break;
		}

		XFreeFontNames(fontname);
		current++;
	}

	info = XLoadQueryFont(env->dpy, names[current]);

	AF_VERIFY(info, AF_ERR_UNKNOWN);
	font = info->fid;

	glXUseXFont(font, 0, 256, AGA_FONT_LIST_BASE);

	XUnloadFont(env->dpy, font);

	return AF_ERR_NONE;
}

enum af_err aga_swapbuf(struct aga_winenv* env, struct aga_win* win) {
	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	if(env->double_buffered) glXSwapBuffers(env->dpy, win->win.xwin);

	return AF_ERR_NONE;
}

enum af_err aga_poll(
		struct aga_winenv* env, struct aga_keymap* keymap, struct aga_win* win,
		struct aga_pointer* pointer, af_bool_t* die) {

	XEvent event;
	struct pollfd pollfd;
	int rdy;

	AF_PARAM_CHK(env);
	AF_PARAM_CHK(keymap);
	AF_PARAM_CHK(pointer);

	pollfd.fd = env->dpy_fd;
	pollfd.events = POLLIN;

	if((rdy = poll(&pollfd, 1, 0)) == -1) {
		return aga_af_errno(__FILE__, "poll");
	}

	if(rdy && (pollfd.revents & POLLIN)) {
		while(XPending(env->dpy) > 0) {
			af_bool_t press = AF_FALSE;

			XNextEvent(env->dpy, &event);

			switch(event.type) {
				default: break;

				case KeyPress: press = AF_TRUE;
					AF_FALLTHROUGH;
					/* FALLTHRU */
				case KeyRelease: {
					unsigned keycode = event.xkey.keycode;
					af_size_t keysym_idx =
						(keycode - keymap->keycode_min) *
						keymap->keysyms_per_keycode;
					af_ulong_t keysym = keymap->keymap[keysym_idx];
					af_size_t bound = keymap->keysyms_per_keycode *
						keymap->keycode_len;

					if(keysym < bound) keymap->keystates[keysym] = press;

					break;
				}

				case MotionNotify: {
					int mid_x = (int) win->width / 2;
					int mid_y = (int) win->height / 2;

					if(event.xmotion.window != win->win.xwin) break;

					if(event.xmotion.x != mid_x || event.xmotion.y != mid_y) {
						aga_centreptr(env, win);
					}

					pointer->dx = event.xmotion.x - mid_x;
					pointer->dy = event.xmotion.y - mid_y;

					break;
				}

				case ClientMessage: {
					if(event.xclient.window == win->win.xwin) {
						*die = AF_TRUE;
					}
					break;
				}
			}
		}
	}

	return AF_ERR_NONE;
}

#endif
