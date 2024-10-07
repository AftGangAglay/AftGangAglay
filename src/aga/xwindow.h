/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_X_WINDOW_H
#define AGA_X_WINDOW_H

#include <aga/gl.h>
#include <aga/log.h>
#include <aga/error.h>
#include <aga/utility.h>
#include <aga/draw.h>

/* TODO: Move shell utils elsewhere and remove this. */
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

static int aga_window_device_error_handler(
		Display* display, XErrorEvent* err) {

	aga_fixed_buf_t buf = { 0 };

	XGetErrorText(display, err->error_code, buf, sizeof(buf));
	aga_log(__FILE__, "err: %s: %s", agax_chk_last, buf);

	return 0;
}

enum aga_result aga_window_device_new(
		struct aga_window_device* env, const char* display) {

	if(!env) return AGA_RESULT_BAD_PARAM;

	AGAX_CHK(XSetErrorHandler, (aga_window_device_error_handler));

	env->capture = 0;
	env->display = AGAX_CHK(XOpenDisplay, (display));
	if(!env->display) return AGA_RESULT_ERROR;

	env->screen = DefaultScreen(env->display);

	env->wm_protocols = AGAX_CHK(XInternAtom,
							 	(env->display, "WM_PROTOCOLS", True));

	if(env->wm_protocols == None) return AGA_RESULT_BAD_PARAM;

	env->wm_delete = AGAX_CHK(XInternAtom,
							  (env->display, "WM_DELETE_WINDOW", True));

	if(env->wm_delete == None) return AGA_RESULT_BAD_PARAM;

	return AGA_RESULT_OK;
}

enum aga_result aga_window_device_delete(struct aga_window_device* env) {
	if(!env) return AGA_RESULT_BAD_PARAM;

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

static enum aga_result aga_window_set_glx(
		struct aga_window_device* env, struct aga_window* win) {

	static const char* const names[] = {
			"*bold*iso8859*", "*iso8859*", "*bold*", "*" };

	GLXFBConfig* fb;
	int n_fb;
	XVisualInfo* vi;

	Font font;
	int font_count = 0;
	XFontStruct* info;
	unsigned current = 0;

	int res;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	win->double_buffered = AGA_TRUE;
	fb = AGAX_CHK(glXChooseFBConfig,
				  (env->display, env->screen, double_buffer_fb, &n_fb));
	if(!fb) {
		win->double_buffered = AGA_FALSE;
		fb = AGAX_CHK(glXChooseFBConfig,
					  (env->display, env->screen, single_buffer_fb, &n_fb));
		if(!fb) return AGA_RESULT_ERROR;
	}

	/* TODO: `glXGetVisualFromFBConfig' is too new for us. */
	vi = AGAX_CHK(glXGetVisualFromFBConfig, (env->display, *fb));
	if(!vi) return AGA_RESULT_ERROR;

	win->glx = AGAX_CHK(glXCreateContext, (env->display, vi, 0, True));
	if(!win->glx) return AGA_RESULT_ERROR;

	XFree(vi);

	res = AGAX_CHK(glXMakeCurrent,(env->display, win->window, win->glx));
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
	 * NOTE: This function shouldn't produce GL errors, but it can leave behind
	 * 		 An error state sometimes in practice.
	 */
	AGAX_CHK(glXUseXFont, (font, 0, 256, AGA_FONT_LIST_BASE));
	(void) aga_error_gl(0, "glXUseXFont");

	AGAX_CHK(XUnloadFont, (env->display, font));

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

static const long aga_global_window_mask = KeyPressMask | KeyReleaseMask |
					 PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

enum aga_result aga_window_new(
		aga_size_t width, aga_size_t height, const char* title,
		struct aga_window_device* env, struct aga_window* win,
		aga_bool_t do_glx, int argc, char** argv) {

	enum aga_result result;

	aga_ulong_t black, white;
	aga_ulong_t root;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	black = BlackPixel(env->display, env->screen);
	white = WhitePixel(env->display, env->screen);
	root = RootWindow(env->display, env->screen);

	win->width = width;
	win->height = height;

	win->window = AGAX_CHK(XCreateSimpleWindow,
						   (env->display, root, 0, 0, width, height, 8,
						   white, black));
	if(win->window == None) return AGA_RESULT_ERROR;

	AGAX_CHK(XSetStandardProperties,
			 (env->display, win->window, title, "", None, argv, argc, 0));

	AGAX_CHK(XSelectInput,
			 (env->display, win->window, aga_global_window_mask));

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

		new_protocols = aga_malloc((count + 1) * sizeof(Atom));
		if(!new_protocols) {
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

	if(do_glx) {
		result = aga_window_set_glx(env, win);
		if(result) return result;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_window_delete(
		struct aga_window_device* env, struct aga_window* win) {

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	if(win->glx) AGAX_CHK(glXDestroyContext, (env->display, win->glx));

	AGAX_CHK(XFreeCursor, (env->display, win->blank_cursor));
	AGAX_CHK(XFreeCursor, (env->display, win->arrow_cursor));
	AGAX_CHK(XDestroyWindow, (env->display, win->window));

	return AGA_RESULT_OK;
}

enum aga_result aga_window_select(
		struct aga_window_device* env, struct aga_window* win) {

	int res;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	res = AGAX_CHK(glXMakeCurrent,(env->display, win->window, win->glx));
	if(!res) return AGA_RESULT_ERROR;

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

enum aga_result aga_window_set_cursor(
		struct aga_window_device* env, struct aga_window* win,
		aga_bool_t visible, aga_bool_t captured) {

	aga_ulong_t cur;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	env->capture = captured ? win : 0;

	cur = visible ? win->arrow_cursor : win->blank_cursor;
	AGAX_CHK(XDefineCursor, (env->display, win->window, cur));

	return AGA_RESULT_OK;
}

enum aga_result aga_window_swap(
		struct aga_window_device* env, struct aga_window* win) {

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	if(win->double_buffered) {
		AGAX_CHK(glXSwapBuffers, (env->display, win->window));
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_window_device_poll(
		struct aga_window_device* env, struct aga_keymap* keymap,
		struct aga_window* window, struct aga_pointer* pointer,
		aga_bool_t* die, struct aga_buttons* buttons) {

	XEvent event;
	unsigned i;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!window) return AGA_RESULT_BAD_PARAM;
	if(!pointer) return AGA_RESULT_BAD_PARAM;
	if(!die) return AGA_RESULT_BAD_PARAM;
	if(!buttons) return AGA_RESULT_BAD_PARAM;

	for(i = 0; i < AGA_LEN(buttons->states); ++i) {
		if(buttons->states[i] == AGA_BUTTON_CLICK) {
			buttons->states[i] = AGA_BUTTON_DOWN;
		}
	}

	/* Thanks to https://stackoverflow.com/a/78649018/13771204. */

	/*
	 * TODO: With these `XCheck*WindowEvent' functions we now have an event
	 * 		 Queue split by window -- which means `poll' invocations can be
	 * 		 Restructured elsewhere.
	 */
	while(AGA_TRUE) {
		int ret = XCheckTypedWindowEvent(
				env->display, window->window, ClientMessage, &event);

		if(!ret) break;

		if(event.xclient.message_type == env->wm_protocols) {
			aga_ulong_t atom = event.xclient.data.l[0];
			if(atom == env->wm_delete) {
				*die = AGA_TRUE;
			}
		}
	}

	while(AGA_TRUE) {
		aga_bool_t press = AGA_FALSE;

		int ret = XCheckWindowEvent(
				env->display, window->window, aga_global_window_mask, &event);

		if(!ret) break;

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

				if(event.xbutton.window != window->window) continue;

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

				if(event.xkey.window != window->window) continue;

				if(keysym > AGAX_KEY_MAX) break; /* Key out of range. */

				keymap->states[keysym] = press;
				break;
			}
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

			case MotionNotify: {
				struct aga_window* capture = env->capture;

				if(!capture) {
					update_pointer: {
						pointer->dx = event.xmotion.x - pointer->x;
						pointer->dy = event.xmotion.y - pointer->y;

						pointer->x = event.xmotion.x;
						pointer->y = event.xmotion.y;

						break;
					}
				}

				/* Handle captured pointer. */
				{
					aga_ulong_t win;
					aga_bool_t centred;
					int x = (int) capture->width / 2;
					int y = (int) capture->height / 2;

					win = capture->window;
					centred = (event.xmotion.x == x && event.xmotion.y == y);

					if(capture->window == event.xmotion.window && !centred) {
						AGAX_CHK(XWarpPointer,
								 (env->display, win, win, 0, 0, 0, 0, x, y));
					}

					if(!centred) goto update_pointer;

					break;
				}
			}

			case ClientMessage: {
				if(event.xclient.message_type == env->wm_protocols) {
					aga_ulong_t atom = event.xclient.data.l[0];
					if(atom == env->wm_delete) {
						*die = AGA_TRUE;
					}
				}

				break;
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
