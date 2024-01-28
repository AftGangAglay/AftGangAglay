/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_X_WIN_H
#define AGA_X_WIN_H

static const char* agax_chk_last = "xlib";

#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Wpedantic"
/* TODO: Check against return value for `0'. */
# define AGAX_CHK(proc, param) ({ agax_chk_last = #proc; proc param; })
#else
# define AGAX_CHK(call) call
#endif

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
	AGAX_CHK(XWarpPointer, (
		env->dpy, win->xwin, win->xwin, 0, 0, 0, 0, mid_x, mid_y));
}

static int aga_xerr_handler(Display* dpy, XErrorEvent* err) {
	aga_fixed_buf_t buf = { 0 };

	XGetErrorText(dpy, err->error_code, buf, sizeof(buf));
	aga_log(__FILE__, "err: %s: %s", agax_chk_last, buf);

	return 0;
}

enum af_err aga_mkwinenv(struct aga_winenv* env, const char* display) {
	GLXFBConfig* fb;
	int n_fb;
	XVisualInfo* vi;

	AF_PARAM_CHK(env);

	AGAX_CHK(XSetErrorHandler, (aga_xerr_handler));

	env->dpy = AGAX_CHK(XOpenDisplay, (display));
	AF_VERIFY(env->dpy, AF_ERR_UNKNOWN);

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
	env->wm_delete = AGAX_CHK(XInternAtom, (
		env->dpy, "WM_DELETE_WINDOW", True));
	AF_VERIFY(env->wm_delete != None, AF_ERR_BAD_PARAM);

	env->double_buffered = AF_TRUE;
	fb = glXChooseFBConfig(env->dpy, env->screen, double_buffer_fb, &n_fb);
	if(!fb) {
		env->double_buffered = AF_FALSE;
		fb = glXChooseFBConfig(
			env->dpy, env->screen, single_buffer_fb, &n_fb);
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

	AGAX_CHK(XCloseDisplay, (env->dpy));

	return AF_ERR_NONE;
}

enum af_err aga_mkkeymap(struct aga_keymap* keymap, struct aga_winenv* env) {
	int min, max;

	AF_PARAM_CHK(keymap);
	AF_PARAM_CHK(env);

	AGAX_CHK(XDisplayKeycodes, (env->dpy, &min, &max));

	keymap->keycode_len = max - min;
	keymap->keycode_min = min;

	keymap->keymap = AGAX_CHK(XGetKeyboardMapping, (
		env->dpy, min, keymap->keycode_len, &keymap->keysyms_per_keycode));
	AF_VERIFY(keymap->keymap, AF_ERR_UNKNOWN);

	keymap->keystates = calloc(
		keymap->keysyms_per_keycode * keymap->keycode_len, sizeof(af_bool_t));
	AF_VERIFY(keymap->keystates, AF_ERR_MEM);

	return AF_ERR_NONE;
}

enum af_err aga_killkeymap(struct aga_keymap* keymap) {
	AF_PARAM_CHK(keymap);

	XFree(keymap->keymap);
	free(keymap->keystates);

	return AF_ERR_NONE;
}

/*
 * NOTE: Creating a pixmap from a bitmap seems to die under WSLg X, so for now
 * we just use a sensible default cursor and hope the result isn't too
 * obtrusive.
 */
/*
Pixmap bitmap;
XColor black_col = { 0 };
char empty[8] = { 0 };
bitmap = XCreatePixmapFromBitmapData(
	env->dpy, win->xwin, empty, 1, 1, white, black, 1);
AF_VERIFY(bitmap != None, AF_ERR_MEM);
XFreePixmap(env->dpy, bitmap);
 */

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

	win->xwin = AGAX_CHK(XCreateSimpleWindow, (
		env->dpy, RootWindow(env->dpy, env->screen), 0, 0, width, height,
		8, white, black));
	AF_VERIFY(win->xwin != None, AF_ERR_UNKNOWN);

	AGAX_CHK(XSetStandardProperties, (
		env->dpy, win->xwin, "Aft Gang Aglay", "", None, argv, argc, 0));

	AGAX_CHK(XSelectInput, (env->dpy, win->xwin, mask));
	{
		Atom* prots;
		Atom* new_prots;
		int count;
		int res;

		res = AGAX_CHK(XGetWMProtocols, (
			env->dpy, win->xwin, &prots, &count));
		if(!res) {
			prots = 0;
			count = 0;
		}

		new_prots = malloc((count + 1) * sizeof(Atom));
		if(!new_prots) {
			if(prots) XFree(prots);
			return AF_ERR_MEM;
		}

		af_memcpy(new_prots, prots, count * sizeof(Atom));
		new_prots[count] = env->wm_delete;
		if(prots) XFree(prots);

		res = AGAX_CHK(XSetWMProtocols, (
			env->dpy, win->xwin, new_prots, count + 1));
		if(!res) {
			free(new_prots);
			return AF_ERR_UNKNOWN;
		}

		free(new_prots);
	}
	AGAX_CHK(XSetInputFocus, (
		env->dpy, win->xwin, RevertToNone, CurrentTime));

	AGAX_CHK(XMapRaised, (
		env->dpy, win->xwin));

	win->blank_cursor = AGAX_CHK(XCreateFontCursor, (
		env->dpy, XC_tcross));
	AF_VERIFY(win->blank_cursor != None, AF_ERR_UNKNOWN);
	win->arrow_cursor = AGAX_CHK(XCreateFontCursor, (
		env->dpy, XC_arrow));
	AF_VERIFY(win->arrow_cursor != None, AF_ERR_UNKNOWN);

	return AF_ERR_NONE;
}

enum af_err aga_killwin(struct aga_winenv* env, struct aga_win* win) {
	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	AGAX_CHK(XFreeCursor, (env->dpy, win->blank_cursor));
	AGAX_CHK(XFreeCursor, (env->dpy, win->arrow_cursor));
	AGAX_CHK(XDestroyWindow, (env->dpy, win->xwin));

	return AF_ERR_NONE;
}

enum af_err aga_glctx(struct aga_winenv* env, struct aga_win* win) {
	static const char* const names[] = {
		"*bold*iso8859*",
		"*iso8859*",
		"*bold*",
		"*"
	};

	Font font;
	int nfonts = 0;
	XFontStruct* info;
	unsigned current = 0;

	int res;

	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	res = glXMakeContextCurrent(
		env->dpy, win->xwin, win->xwin, env->glx);
	AF_VERIFY(res, AF_ERR_UNKNOWN);

	while(AF_TRUE) {
		char** fontname;
		if(current >= AF_ARRLEN(names)) {
			aga_log(__FILE__, "err: no fonts available");
			return AF_ERR_BAD_OP;
		}

		aga_log(__FILE__, "Trying font pattern `%s'", names[current]);
		fontname = AGAX_CHK(XListFonts, (
			env->dpy, names[current], 1, &nfonts));

		if(nfonts) {
			AF_VERIFY(fontname, AF_ERR_UNKNOWN);
			aga_log(__FILE__, "Using x11 font `%s'", *fontname);
			AGAX_CHK(XFreeFontNames, (fontname));
			break;
		}

		AGAX_CHK(XFreeFontNames, (fontname));
		current++;
	}

	info = AGAX_CHK(XLoadQueryFont, (env->dpy, names[current]));
	AF_VERIFY(info, AF_ERR_UNKNOWN);
	font = info->fid;

	glXUseXFont(font, 0, 256, AGA_FONT_LIST_BASE);

	AGAX_CHK(XUnloadFont, (env->dpy, font));

	return AF_ERR_NONE;
}

enum af_err aga_setcursor(
		struct aga_winenv* env, struct aga_win* win, af_bool_t visible,
		af_bool_t captured) {

	af_ulong_t cur;

	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	cur = visible ? win->arrow_cursor : win->blank_cursor;
	AGAX_CHK(XDefineCursor, (env->dpy, win->xwin, cur));

	if(captured) {
		aga_centreptr(env, win);
	}

	return AF_ERR_NONE;
}

enum af_err aga_swapbuf(struct aga_winenv* env, struct aga_win* win) {
	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	if(env->double_buffered) glXSwapBuffers(env->dpy, win->xwin);

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

	/*
	 * TODO: This appears to fail under WSLg unless a timeout of 1ms is set
	 * 		 Which feels like we're doing something deeply importable here.
	 * 		 Let's try to figure this out!
	 */
	if((rdy = poll(&pollfd, 1, 0)) == -1) {
		return aga_af_errno(__FILE__, "poll");
	}

	if(rdy && (pollfd.revents & POLLIN)) {
		while(AGAX_CHK(XPending, (env->dpy)) > 0) {
			af_bool_t press = AF_FALSE;

			AGAX_CHK(XNextEvent, (env->dpy, &event));

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

					if(event.xmotion.window != win->xwin) break;

					if(event.xmotion.x != mid_x || event.xmotion.y != mid_y) {
						aga_centreptr(env, win);
					}

					pointer->dx = event.xmotion.x - mid_x;
					pointer->dy = event.xmotion.y - mid_y;

					break;
				}

				case ClientMessage: {
					if(event.xclient.window == win->xwin) {
						*die = AF_TRUE;
					}
					break;
				}
			}
		}
	}

	return AF_ERR_NONE;
}

/* TODO: Use `xdg' for diag/shellopen when available. */

enum af_err aga_diag(
		const char* message, const char* title, af_bool_t* response,
		af_bool_t is_error) {

	AF_PARAM_CHK(message);
	AF_PARAM_CHK(title);
	AF_PARAM_CHK(response);

	if(isatty(STDIN_FILENO)) {
		int c = 'N';
		const char* err = is_error ? "err: " : "";

		aga_log(__FILE__, "%s%s: %s (Y/N)", err, title, message);
		if((c = getchar()) == EOF) {
			if(ferror(stdin)) return aga_af_errno(__FILE__, "getchar");
		}
		*response = (toupper(c) == 'Y');
	}
	else *response = AF_FALSE;

	return AF_ERR_NONE;
}

enum af_err aga_shellopen(const char* uri) {
	AF_PARAM_CHK(uri);

	aga_log(__FILE__, "%s", uri);

	return AF_ERR_NONE;
}

#endif
