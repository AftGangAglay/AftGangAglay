/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_X_WIN_H
#define AGA_X_WIN_H

#include <aga/gl.h>
#include <aga/log.h>
#include <aga/error.h>
#include <aga/utility.h>
#include <aga/draw.h>

#define AGA_WANT_UNIX
#include <aga/std.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

/*
 * NOTE: `0xFF**' represents the highest class of "normal" keycodes which
 * 		 Are supported in `aganio' for X systems. We're sitting in a
 * 		 Transitional period before XKB when keyhandling was a bit all over the
 * 		 Place so this will have to do for now.
 */
#define AGAX_KEY_MAX (0xFFFF)

static const char* agax_chk_last = "xlib";

/* TODO: Check against return value for `0'. */
#define AGAX_CHK(proc, param) (agax_chk_last = #proc, proc param)

static const int single_buffer_fb[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RENDER_TYPE, GLX_RGBA_BIT,

		GLX_RED_SIZE, GLX_DONT_CARE, GLX_GREEN_SIZE, GLX_DONT_CARE,
		GLX_BLUE_SIZE, GLX_DONT_CARE, GLX_DEPTH_SIZE, 1,

		None
};

static const int double_buffer_fb[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DOUBLEBUFFER, True,

		GLX_RED_SIZE, GLX_DONT_CARE, GLX_GREEN_SIZE, GLX_DONT_CARE,
		GLX_BLUE_SIZE, GLX_DONT_CARE, GLX_DEPTH_SIZE, 1,

		None
};

static void aga_window_center_pointer(
		struct aga_window_device* env, struct aga_window* win) {

	int x = (int) win->width / 2;
	int y = (int) win->height / 2;

	AGAX_CHK(XWarpPointer,
			 (env->display, win->window, win->window, 0, 0, 0, 0, x, y));
}

static int aga_window_device_error_handler(
		Display* display, XErrorEvent* err) {

	aga_fixed_buf_t buf = { 0 };

	XGetErrorText(display, err->error_code, buf, sizeof(buf));
	aga_log(__FILE__, "err: %s: %s", agax_chk_last, buf);

	return 0;
}

enum aga_result aga_window_device_new(
		struct aga_window_device* env, const char* display) {

	GLXFBConfig* fb;
	int n_fb;
	XVisualInfo* vi;

	if(!env) return AGA_RESULT_BAD_PARAM;

	AGAX_CHK(XSetErrorHandler, (aga_window_device_error_handler));

	env->captured = AGA_FALSE;
	env->display = AGAX_CHK(XOpenDisplay, (display));
	if(!env->display) return AGA_RESULT_ERROR;

	{
		int fl;

		env->display_fd = ConnectionNumber(env->display);
		if((fl = fcntl(env->display_fd, F_GETFL)) == -1) {
			return aga_error_system(__FILE__, "fcntl");
		}

		fl |= O_NONBLOCK;
		if(fcntl(env->display_fd, F_SETFL, fl) == -1) {
			return aga_error_system(__FILE__, "fcntl");
		}
	}

	env->screen = DefaultScreen(env->display);
	env->wm_delete = AGAX_CHK(XInternAtom,
							  (env->display, "WM_DELETE_WINDOW", True));
	if(env->wm_delete == None) return AGA_RESULT_BAD_PARAM;

	env->double_buffered = AGA_TRUE;
	fb = glXChooseFBConfig(env->display, env->screen, double_buffer_fb, &n_fb);
	if(!fb) {
		env->double_buffered = AGA_FALSE;
		fb = glXChooseFBConfig(
				env->display, env->screen, single_buffer_fb, &n_fb);
		if(!fb) return AGA_RESULT_ERROR;
	}

	/* TODO: `glXGetVisualFromFBConfig' is too new for us. */
	vi = glXGetVisualFromFBConfig(env->display, *fb);
	if(!vi) return AGA_RESULT_ERROR;

	env->glx = glXCreateContext(env->display, vi, 0, True);
	if(!env->glx) return AGA_RESULT_ERROR;

	XFree(vi);

	return AGA_RESULT_OK;
}

enum aga_result aga_window_device_delete(struct aga_window_device* env) {
	if(!env) return AGA_RESULT_BAD_PARAM;

	glXDestroyContext(env->display, env->glx);

	AGAX_CHK(XCloseDisplay, (env->display));

	return AGA_RESULT_OK;
}

enum aga_result aga_keymap_new(
		struct aga_keymap* keymap, struct aga_window_device* env) {

	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!env) return AGA_RESULT_BAD_PARAM;

	keymap->states = aga_calloc(AGAX_KEY_MAX, sizeof(aga_bool_t));
	if(!keymap->states) return AGA_RESULT_OOM;

	return AGA_RESULT_OK;
}

enum aga_result aga_keymap_delete(struct aga_keymap* keymap) {
	if(!keymap) return AGA_RESULT_BAD_PARAM;

	aga_free(keymap->states);

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
	env->display, win->window, empty, 1, 1, white, black, 1);
AGA_VERIFY(bitmap != None, AGA_RESULT_OOM);
XFreePixmap(env->display, bitmap);
 */

enum aga_result aga_window_new(
		aga_size_t width, aga_size_t height, struct aga_window_device* env,
		struct aga_window* win, int argc, char** argv) {

	static const char name[] = "AftGangAglay";

	aga_ulong_t black, white;
	long mask = KeyPressMask | KeyReleaseMask |
					PointerMotionMask |
					ButtonPressMask | ButtonReleaseMask;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	black = BlackPixel(env->display, env->screen);
	white = WhitePixel(env->display, env->screen);

	win->width = width;
	win->height = height;

	win->window = AGAX_CHK(XCreateSimpleWindow, (env->display, RootWindow(
			env->display, env->screen), 0, 0, width, height, 8, white, black));
	if(win->window == None) return AGA_RESULT_ERROR;

	AGAX_CHK(XSetStandardProperties,
			 (env->display, win->window, name, "", None, argv, argc, 0));

	AGAX_CHK(XSelectInput, (env->display, win->window, mask));
	{
		Atom* protocols;
		Atom* new_protocols;
		int count;
		int res;

		res = AGAX_CHK(XGetWMProtocols,
					   (env->display, win->window, &protocols, &count));
		if(!res) {
			protocols = 0;
			count = 0;
		}

		if(!(new_protocols = aga_malloc((count + 1) * sizeof(Atom)))) {
			if(protocols) XFree(protocols);
			return AGA_RESULT_OOM;
		}

		aga_memcpy(new_protocols, protocols, count * sizeof(Atom));
		new_protocols[count] = env->wm_delete;
		if(protocols) XFree(protocols);

		res = AGAX_CHK(XSetWMProtocols,
					   (env->display, win->window, new_protocols, count + 1));
		if(!res) {
			aga_free(new_protocols);
			return AGA_RESULT_ERROR;
		}

		aga_free(new_protocols);
	}
	AGAX_CHK(XSetInputFocus,
			 (env->display, win->window, RevertToNone, CurrentTime));

	AGAX_CHK(XMapRaised, (env->display, win->window));

	win->blank_cursor = AGAX_CHK(XCreateFontCursor, (env->display, XC_tcross));
	if(win->blank_cursor == None) return AGA_RESULT_ERROR;

	win->arrow_cursor = AGAX_CHK(XCreateFontCursor, (env->display, XC_arrow));
	if(win->arrow_cursor == None) return AGA_RESULT_ERROR;

	return AGA_RESULT_OK;
}

enum aga_result aga_window_delete(
		struct aga_window_device* env, struct aga_window* win) {

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	AGAX_CHK(XFreeCursor, (env->display, win->blank_cursor));
	AGAX_CHK(XFreeCursor, (env->display, win->arrow_cursor));
	AGAX_CHK(XDestroyWindow, (env->display, win->window));

	return AGA_RESULT_OK;
}

enum aga_result aga_keymap_lookup(
		struct aga_keymap* keymap, unsigned sym, aga_bool_t* state) {

	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!state) return AGA_RESULT_BAD_PARAM;

	if(!keymap->states) return AGA_RESULT_ERROR;

	if(sym > AGAX_KEY_MAX) return AGA_RESULT_BAD_OP;

	*state = keymap->states[sym];

	return AGA_RESULT_OK;
}

enum aga_result aga_window_device_glx_new(
		struct aga_window_device* env, struct aga_window* win) {

	static const char* const names[] = {
			"*bold*iso8859*", "*iso8859*", "*bold*", "*" };

	Font font;
	int font_count = 0;
	XFontStruct* info;
	unsigned current = 0;

	int res;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	res = glXMakeContextCurrent(
			env->display, win->window, win->window, env->glx);
	if(!res) return AGA_RESULT_ERROR;

	while(AGA_TRUE) {
		char** fontname;
		if(current >= AGA_LEN(names)) {
			aga_log(__FILE__, "err: no fonts available");
			return AGA_RESULT_BAD_OP;
		}

		aga_log(__FILE__, "Trying font pattern `%s'...", names[current]);
		fontname = AGAX_CHK(XListFonts,
							(env->display, names[current], 1, &font_count));

		if(font_count) {
			if(!fontname) return AGA_RESULT_ERROR;

			aga_log(__FILE__, "Using x11 font `%s'", *fontname);

			AGAX_CHK(XFreeFontNames, (fontname));

			break;
		}

		AGAX_CHK(XFreeFontNames, (fontname));
		current++;
	}

	info = AGAX_CHK(XLoadQueryFont, (env->display, names[current]));
	if(!info) return AGA_RESULT_ERROR;

	font = info->fid;

	/*
	 * NOTE: Technically this function shouldn't produce GL errors, but we can
	 * 		 Leave behind an error state sometimes in practice.
	 */
	glXUseXFont(font, 0, 256, AGA_FONT_LIST_BASE);
	(void) aga_error_gl(0, "glXUseXFont");

	AGAX_CHK(XUnloadFont, (env->display, font));

	return AGA_RESULT_OK;
}

enum aga_result aga_window_set_cursor(
		struct aga_window_device* env, struct aga_window* win, aga_bool_t visible,
		aga_bool_t captured) {

	aga_ulong_t cur;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	env->captured = captured;

	cur = visible ? win->arrow_cursor : win->blank_cursor;
	AGAX_CHK(XDefineCursor, (env->display, win->window, cur));

	if(captured) aga_window_center_pointer(env, win);

	return AGA_RESULT_OK;
}

enum aga_result aga_window_swap(
		struct aga_window_device* env, struct aga_window* win) {

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	if(env->double_buffered) glXSwapBuffers(env->display, win->window);

	return AGA_RESULT_OK;
}

enum aga_result aga_window_device_poll(
		struct aga_window_device* env, struct aga_keymap* keymap,
		struct aga_window* win, struct aga_pointer* pointer, aga_bool_t* die,
		struct aga_buttons* buttons) {

	XEvent event;
	struct pollfd fd;
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

	fd.fd = env->display_fd;
	fd.events = POLLIN;

	/*
	 * TODO: This appears to fail under WSLg unless a timeout of 1ms is set
	 * 		 Which feels like we're doing something deeply importable here.
	 * 		 Let's try to figure this out!
	 */
	if((rdy = poll(&fd, 1, 1)) == -1) {
		return aga_error_system(__FILE__, "poll");
	}

	if(rdy && (fd.revents & POLLIN)) {
		while(AGAX_CHK(XPending, (env->display)) > 0) {
			aga_bool_t press = AGA_FALSE;

			AGAX_CHK(XNextEvent, (env->display, &event));

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
					enum aga_button_state state;
					enum aga_button button = event.xbutton.button - 1;

					if(press) state = AGA_BUTTON_CLICK;
					else state = AGA_BUTTON_UP;

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
					KeySym keysym = XKeycodeToKeysym(env->display, keycode, 0);

					if(keysym > AGAX_KEY_MAX) break; /* Key out of range. */

					keymap->states[keysym] = press;
					break;
				}
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

				case MotionNotify: {
					int mid_x = (int) win->width / 2;
					int mid_y = (int) win->height / 2;

					if(event.xmotion.window != win->window) break;

					if(event.xmotion.x != mid_x || event.xmotion.y != mid_y) {
						if(env->captured) aga_window_center_pointer(env, win);
					}

					pointer->dx = event.xmotion.x - pointer->x;
					pointer->dy = event.xmotion.y - pointer->y;

					pointer->x = event.xmotion.x;
					pointer->y = event.xmotion.y;

					break;
				}

				case ClientMessage: {
					if(event.xclient.window == win->window) {
						*die = AGA_TRUE;
					}
					break;
				}
			}
		}
	}

	return AGA_RESULT_OK;
}

/* TODO: Use `xdg' for dialog/shell_open when available. */

enum aga_result aga_dialog(
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
			if(ferror(stdin)) return aga_error_system(__FILE__, "getchar");
		}
		*response = (toupper(c) == 'Y');
	}
	else { *response = AGA_FALSE; }

	return AGA_RESULT_OK;
}

enum aga_result aga_dialog_file(char** result) {
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

		return aga_error_system(__FILE__, "fgets");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_shell_open(const char* uri) {
	if(!uri) return AGA_RESULT_BAD_PARAM;

	aga_log(__FILE__, "%s", uri);

	return AGA_RESULT_OK;
}

#endif
