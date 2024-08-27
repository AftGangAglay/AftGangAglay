/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_X_WIN_H
#define AGA_X_WIN_H

#include <agagl.h>
#include <agalog.h>
#include <agaerr.h>
#include <agautil.h>
#include <agadraw.h>

#define AGA_WANT_UNIX

#include <agastd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

/*
 * NOTE: `0xFF**' represents the highest class of "normal" keycodes which
 * 		 Are supported in `aganio' for X systems. We're sitting in a
 * 		 Transitional period before XKB when keyhandling was a bit all over the
 * 		 Place so this will have to do for now.
 */
#define AGAX_KEYMAX (0xFFFF)

static const char* agax_chk_last = "xlib";

/* TODO: Check against return value for `0'. */
#define AGAX_CHK(proc, param) (agax_chk_last = #proc, proc param)

static const int single_buffer_fb[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RENDER_TYPE, GLX_RGBA_BIT,

		GLX_RED_SIZE, GLX_DONT_CARE, GLX_GREEN_SIZE, GLX_DONT_CARE,
		GLX_BLUE_SIZE, GLX_DONT_CARE, GLX_DEPTH_SIZE, 1,

		None };

static const int double_buffer_fb[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DOUBLEBUFFER, True,

		GLX_RED_SIZE, GLX_DONT_CARE, GLX_GREEN_SIZE, GLX_DONT_CARE,
		GLX_BLUE_SIZE, GLX_DONT_CARE, GLX_DEPTH_SIZE, 1,

		None };

static void aga_centreptr(struct aga_winenv* env, struct aga_win* win) {
	int mid_x = (int) win->width / 2;
	int mid_y = (int) win->height / 2;
	AGAX_CHK(XWarpPointer,
			 (env->dpy, win->xwin, win->xwin, 0, 0, 0, 0, mid_x, mid_y));
}

static int aga_xerr_handler(Display* dpy, XErrorEvent* err) {
	aga_fixed_buf_t buf = { 0 };

	XGetErrorText(dpy, err->error_code, buf, sizeof(buf));
	aga_log(__FILE__, "err: %s: %s", agax_chk_last, buf);

	return 0;
}

enum aga_result aga_mkwinenv(struct aga_winenv* env, const char* display) {
	GLXFBConfig* fb;
	int n_fb;
	XVisualInfo* vi;

	if(!env) return AGA_RESULT_BAD_PARAM;

	AGAX_CHK(XSetErrorHandler, (aga_xerr_handler));

	env->captured = AGA_FALSE;
	env->dpy = AGAX_CHK(XOpenDisplay, (display));
	if(!env->dpy) return AGA_RESULT_ERROR;

	{
		int fl;

		env->dpy_fd = ConnectionNumber(env->dpy);
		if((fl = fcntl(env->dpy_fd, F_GETFL)) == -1) {
			return aga_errno(__FILE__, "fcntl");
		}

		fl |= O_NONBLOCK;
		if(fcntl(env->dpy_fd, F_SETFL, fl) == -1) {
			return aga_errno(__FILE__, "fcntl");
		}
	}

	env->screen = DefaultScreen(env->dpy);
	env->wm_delete = AGAX_CHK(XInternAtom,
							  (env->dpy, "WM_DELETE_WINDOW", True));
	if(env->wm_delete == None) return AGA_RESULT_BAD_PARAM;

	env->double_buffered = AGA_TRUE;
	fb = glXChooseFBConfig(env->dpy, env->screen, double_buffer_fb, &n_fb);
	if(!fb) {
		env->double_buffered = AGA_FALSE;
		fb = glXChooseFBConfig(
				env->dpy, env->screen, single_buffer_fb, &n_fb);
		if(!fb) return AGA_RESULT_ERROR;
	}

	/* TODO: `glXGetVisualFromFBConfig' is too new for us. */
	vi = glXGetVisualFromFBConfig(env->dpy, *fb);
	if(!vi) return AGA_RESULT_ERROR;

	env->glx = glXCreateContext(env->dpy, vi, 0, True);
	if(!env->glx) return AGA_RESULT_ERROR;

	XFree(vi);

	return AGA_RESULT_OK;
}

enum aga_result aga_killwinenv(struct aga_winenv* env) {
	if(!env) return AGA_RESULT_BAD_PARAM;

	glXDestroyContext(env->dpy, env->glx);

	AGAX_CHK(XCloseDisplay, (env->dpy));

	return AGA_RESULT_OK;
}

enum aga_result aga_mkkeymap(
		struct aga_keymap* keymap, struct aga_winenv* env) {

	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!env) return AGA_RESULT_BAD_PARAM;

	keymap->keystates = aga_calloc(AGAX_KEYMAX, sizeof(aga_bool_t));
	if(!keymap->keystates) return AGA_RESULT_OOM;

	return AGA_RESULT_OK;
}

enum aga_result aga_killkeymap(struct aga_keymap* keymap) {
	if(!keymap) return AGA_RESULT_BAD_PARAM;

	aga_free(keymap->keystates);

	return AGA_RESULT_OK;
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
AGA_VERIFY(bitmap != None, AGA_RESULT_OOM);
XFreePixmap(env->dpy, bitmap);
 */

enum aga_result aga_mkwin(
		aga_size_t width, aga_size_t height, struct aga_winenv* env,
		struct aga_win* win, int argc, char** argv) {

	aga_ulong_t black, white;
	long mask = KeyPressMask | KeyReleaseMask |
					PointerMotionMask |
					ButtonPressMask | ButtonReleaseMask;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	black = BlackPixel(env->dpy, env->screen);
	white = WhitePixel(env->dpy, env->screen);

	win->width = width;
	win->height = height;

	win->xwin = AGAX_CHK(XCreateSimpleWindow, (env->dpy, RootWindow(
			env->dpy, env->screen), 0, 0, width, height, 8, white, black));
	if(win->xwin == None) return AGA_RESULT_ERROR;

	AGAX_CHK(XSetStandardProperties,
			 (env->dpy, win->xwin, "AftGangAglay", "", None, argv, argc, 0));

	AGAX_CHK(XSelectInput, (env->dpy, win->xwin, mask));
	{
		Atom* prots;
		Atom* new_prots;
		int count;
		int res;

		res = AGAX_CHK(XGetWMProtocols, (env->dpy, win->xwin, &prots, &count));
		if(!res) {
			prots = 0;
			count = 0;
		}

		new_prots = aga_malloc((count + 1) * sizeof(Atom));
		if(!new_prots) {
			if(prots) XFree(prots);
			return AGA_RESULT_OOM;
		}

		memcpy(new_prots, prots, count * sizeof(Atom));
		new_prots[count] = env->wm_delete;
		if(prots) XFree(prots);

		res = AGAX_CHK(XSetWMProtocols,
					   (env->dpy, win->xwin, new_prots, count + 1));
		if(!res) {
			aga_free(new_prots);
			return AGA_RESULT_ERROR;
		}

		aga_free(new_prots);
	}
	AGAX_CHK(XSetInputFocus, (env->dpy, win->xwin, RevertToNone, CurrentTime));

	AGAX_CHK(XMapRaised, (env->dpy, win->xwin));

	win->blank_cursor = AGAX_CHK(XCreateFontCursor, (env->dpy, XC_tcross));
	if(win->blank_cursor == None) return AGA_RESULT_ERROR;

	win->arrow_cursor = AGAX_CHK(XCreateFontCursor, (env->dpy, XC_arrow));
	if(win->arrow_cursor == None) return AGA_RESULT_ERROR;

	return AGA_RESULT_OK;
}

enum aga_result aga_killwin(struct aga_winenv* env, struct aga_win* win) {
	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	AGAX_CHK(XFreeCursor, (env->dpy, win->blank_cursor));
	AGAX_CHK(XFreeCursor, (env->dpy, win->arrow_cursor));
	AGAX_CHK(XDestroyWindow, (env->dpy, win->xwin));

	return AGA_RESULT_OK;
}

enum aga_result aga_keylook(
		struct aga_keymap* keymap, unsigned sym, aga_bool_t* state) {

	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!state) return AGA_RESULT_BAD_PARAM;

	if(!keymap->keystates) return AGA_RESULT_ERROR;

	if(sym > AGAX_KEYMAX) return AGA_RESULT_BAD_OP;

	*state = keymap->keystates[sym];

	return AGA_RESULT_OK;
}

enum aga_result aga_glctx(struct aga_winenv* env, struct aga_win* win) {
	static const char* const names[] = {
			"*bold*iso8859*", "*iso8859*", "*bold*", "*" };

	Font font;
	int nfonts = 0;
	XFontStruct* info;
	unsigned current = 0;

	int res;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	res = glXMakeContextCurrent(
			env->dpy, win->xwin, win->xwin, env->glx);
	if(!res) return AGA_RESULT_ERROR;

	while(AGA_TRUE) {
		char** fontname;
		if(current >= AGA_LEN(names)) {
			aga_log(__FILE__, "err: no fonts available");
			return AGA_RESULT_BAD_OP;
		}

		aga_log(__FILE__, "Trying font pattern `%s'...", names[current]);
		fontname = AGAX_CHK(XListFonts, (env->dpy, names[current], 1, &nfonts));

		if(nfonts) {
			if(!fontname) return AGA_RESULT_ERROR;
			aga_log(__FILE__, "Using x11 font `%s'", *fontname);
			AGAX_CHK(XFreeFontNames, (fontname));
			break;
		}

		AGAX_CHK(XFreeFontNames, (fontname));
		current++;
	}

	info = AGAX_CHK(XLoadQueryFont, (env->dpy, names[current]));
	if(!info) return AGA_RESULT_ERROR;

	font = info->fid;

	/*
	 * NOTE: Technically this function shouldn't produce GL errors, but we can
	 * 		 Leave behind an error state sometimes in practice.
	 */
	glXUseXFont(font, 0, 256, AGA_FONT_LIST_BASE);
	(void) aga_gl_error(0, "glXUseXFont");

	AGAX_CHK(XUnloadFont, (env->dpy, font));

	return AGA_RESULT_OK;
}

enum aga_result aga_setcursor(
		struct aga_winenv* env, struct aga_win* win, aga_bool_t visible,
		aga_bool_t captured) {

	aga_ulong_t cur;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	env->captured = captured;

	cur = visible ? win->arrow_cursor : win->blank_cursor;
	AGAX_CHK(XDefineCursor, (env->dpy, win->xwin, cur));

	if(captured) {
		aga_centreptr(env, win);
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_swapbuf(struct aga_winenv* env, struct aga_win* win) {
	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	if(env->double_buffered) glXSwapBuffers(env->dpy, win->xwin);

	return AGA_RESULT_OK;
}

enum aga_result aga_poll(
		struct aga_winenv* env, struct aga_keymap* keymap, struct aga_win* win,
		struct aga_pointer* pointer, aga_bool_t* die,
		struct aga_buttons* buttons) {

	XEvent event;
	struct pollfd pollfd;
	int rdy;
	unsigned i;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;
	if(!pointer) return AGA_RESULT_BAD_PARAM;
	if(!die) return AGA_RESULT_BAD_PARAM;
	if(!buttons) return AGA_RESULT_BAD_PARAM;

	for(i = 0; i < AGA_LEN(buttons->states); ++i) {
		if(buttons->states[i] == AGA_BUTTON_CLICK) {
			buttons->states[i] = AGA_BUTTON_DOWN;
		}
	}

	pollfd.fd = env->dpy_fd;
	pollfd.events = POLLIN;

	/*
	 * TODO: This appears to fail under WSLg unless a timeout of 1ms is set
	 * 		 Which feels like we're doing something deeply importable here.
	 * 		 Let's try to figure this out!
	 */
	if((rdy = poll(&pollfd, 1, 1)) == -1) {
		return aga_errno(__FILE__, "poll");
	}

	if(rdy && (pollfd.revents & POLLIN)) {
		while(AGAX_CHK(XPending, (env->dpy)) > 0) {
			aga_bool_t press = AGA_FALSE;

			AGAX_CHK(XNextEvent, (env->dpy, &event));

			switch(event.type) {
				default: break;

				/*
				 * NOTE: This assumes `Button1 -> LMB', `Button2 -> RMB' and
				 * 		 `Button3 -> MMB' which (apparently) isn't always true.
				 */
				case ButtonPress: {
					press = AGA_TRUE;
					AGA_FALLTHROUGH;
				}
				/* FALLTHROUGH */
				case ButtonRelease: {
					enum aga_button button = event.xbutton.button - 1;
					enum aga_button_state state = press ? AGA_BUTTON_CLICK :
														  AGA_BUTTON_UP;
					buttons->states[button] = state;
					break;
				}

#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
				case KeyPress: {
					press = AGA_TRUE;
					AGA_FALLTHROUGH;
				}
				/* FALLTHROUGH */
				case KeyRelease: {
					unsigned keycode = event.xkey.keycode;
					KeySym keysym = XKeycodeToKeysym(env->dpy, keycode, 0);

					if(keysym > AGAX_KEYMAX) break; /* Key out of range. */

					keymap->keystates[keysym] = press;
					break;
				}
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

				case MotionNotify: {
					int mid_x = (int) win->width / 2;
					int mid_y = (int) win->height / 2;

					if(event.xmotion.window != win->xwin) break;

					if(event.xmotion.x != mid_x || event.xmotion.y != mid_y) {
						if(env->captured) aga_centreptr(env, win);
					}

					pointer->dx = event.xmotion.x - pointer->x;
					pointer->dy = event.xmotion.y - pointer->y;

					pointer->x = event.xmotion.x;
					pointer->y = event.xmotion.y;

					break;
				}

				case ClientMessage: {
					if(event.xclient.window == win->xwin) {
						*die = AGA_TRUE;
					}
					break;
				}
			}
		}
	}

	return AGA_RESULT_OK;
}

/* TODO: Use `xdg' for diag/shellopen when available. */

enum aga_result aga_diag(
		const char* message, const char* title, aga_bool_t* response,
		aga_bool_t is_error) {

	if(!message) return AGA_RESULT_BAD_PARAM;
	if(!title) return AGA_RESULT_BAD_PARAM;
	if(!response) return AGA_RESULT_BAD_PARAM;

	if(isatty(STDIN_FILENO)) {
		int c = 'N';
		const char* err = is_error ? "err: " : "";

		aga_log(__FILE__, "%s%s: %s (Y/N)", err, title, message);
		if((c = getchar()) == EOF) {
			if(ferror(stdin)) return aga_errno(__FILE__, "getchar");
		}
		*response = (toupper(c) == 'Y');
	}
	else { *response = AGA_FALSE; }

	return AGA_RESULT_OK;
}

enum aga_result aga_filediag(char** result) {
	/*
	 * TODO: `_POSIX_C_SOURCE=2` doesn't seem to define `PATH_MAX` even though
	 * 		 `_POSIX_C_SOURCE=1` does.
	 */
	static const aga_size_t path_max = 4096;

	if(!result) return AGA_RESULT_BAD_PARAM;

	if(!(*result = aga_calloc(path_max, sizeof(char)))) return AGA_RESULT_OOM;

	if(!fgets(*result, (int) path_max, stdin)) {
		if(feof(stdin)) {
			*result[0] = 0;
			return AGA_RESULT_OK;
		}

		return aga_errno(__FILE__, "fgets");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_shellopen(const char* uri) {
	if(!uri) return AGA_RESULT_BAD_PARAM;

	aga_log(__FILE__, "%s", uri);

	return AGA_RESULT_OK;
}

#endif
