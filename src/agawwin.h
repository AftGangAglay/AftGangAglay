/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_W_WIN_H
#define AGA_W_WIN_H

/*
 * NOTE: Everything in here is from the future! Microsoft only started
 * 		 Supporting OpenGL in 1997-ish. Unfortunately shipping without Windows
 * 		 Support would be a bit of a death sentence so here we are.
 */

#include <windows.h>
#include <windowsx.h>

#define AGA_CLASS_NAME ("Aft Gang Aglay")

static const PIXELFORMATDESCRIPTOR pixel_format = {
	sizeof(PIXELFORMATDESCRIPTOR), 1,
	PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
	PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	32, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
};

static enum af_err aga_af_pathwinerr(
		const char* loc, const char* proc, const char* path) {

	if(loc) {
		DWORD err = GetLastError();
		LPSTR buf;
		DWORD written;
		if(!err) return AF_ERR_NONE;

		written = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, err, 0, (LPSTR) &buf, 0, 0);
		if(!written) {
			aga_log(__FILE__, "err: FormatMessageA failed");
			return AF_ERR_UNKNOWN;
		}

		if(path) aga_log(loc, "err: %s: %s `%s'", proc, buf, path);
		else aga_log(loc, "err: %s: %s", proc, buf);

		if(LocalFree(buf)) {
			aga_log(__FILE__, "err: LocalFree failed");
			return AF_ERR_UNKNOWN;
		}
	}

	return AF_ERR_UNKNOWN;
}

static enum af_err aga_af_winerr(const char* loc, const char* proc) {
	return aga_af_pathwinerr(loc, proc, 0);
}

#define AGA_AF_WINCHK(proc) \
	aga_af_chk(__FILE__, proc, aga_af_winerr(__FILE__, proc))

static LRESULT aga_winproc(
		HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) {

	struct aga_ctx* ctx;
	af_bool_t down = AF_TRUE;
	struct aga_keymap* keymap;

	if(msg == WM_NCCREATE) {
		CREATESTRUCTA* create = (void*) l_param;
		ctx = create->lpCreateParams;

		SetLastError(0);
		SetWindowLongPtrA(wnd, GWLP_USERDATA, (LONG_PTR) ctx);
		if(GetLastError()) AGA_AF_WINCHK("SetWindowLongPtrA");

		return TRUE;
	}

	SetLastError(0);
	if(!(ctx = (void*) GetWindowLongPtrA(wnd, GWLP_USERDATA))) {
		if(GetLastError()) AGA_AF_WINCHK("GetWindowLongPtrA");
		goto default_msg;
	}

	keymap = &ctx->keymap;

	switch(msg) {
		default: {
			default_msg:;
			return DefWindowProcA(wnd, msg, w_param, l_param);
		}

		case WM_KEYUP: down = AF_FALSE;
			AF_FALLTHROUGH;
			/* FALLTHRU */
		case WM_KEYDOWN: {
			ctx->keystates[w_param] = down;
			return 0;
		}

		case WM_MOUSEMOVE: {
			ctx->pointer_dx = GET_X_LPARAM(l_param) - keymap->keycode_len;
			ctx->pointer_dy = GET_Y_LPARAM(l_param) - keymap->keycode_min;
			keymap->keycode_len = GET_X_LPARAM(l_param);
			keymap->keycode_min = GET_Y_LPARAM(l_param);
			return 0;
		}

		case WM_NCDESTROY: {
			ctx->die = AF_TRUE;
			return 0;
		}
	}
}

/*
 * Many thanks to code yoinked from: https://github.com/quakeforge/quakeforge/
 * (see libs/video/targets/).
 */

enum af_err aga_mkctxdpy(struct aga_ctx* ctx, const char* display) {
	WNDCLASSA class;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(display);

	if(!(ctx->winenv.dpy = GetModuleHandleA(0))) {
		return aga_af_winerr(__FILE__, "GetModuleHandleA");
	}

	class.style = CS_GLOBALCLASS;
	class.lpfnWndProc = (WNDPROC) aga_winproc;
	class.cbClsExtra = 0;
	class.cbWndExtra = 0;
	class.hInstance = ctx->dpy;
	class.hIcon = 0;
	class.hCursor = 0;
	class.hbrBackground = 0;
	class.lpszMenuName = 0;
	class.lpszClassName = AGA_CLASS_NAME;

	if(!(ctx->winenv.dpy_fd = RegisterClassA(&class))) {
		return aga_af_winerr(__FILE__, "RegisterClassA");
	}

	ctx->keystates = calloc(0xFF, sizeof(af_bool_t)); /* VK_OEM_CLEAR + 1 */
	AF_VERIFY(ctx->keystates, AF_ERR_MEM);

	return AF_ERR_NONE;
}

enum af_err aga_killctxdpy(struct aga_ctx* ctx) {
	AF_PARAM_CHK(ctx);

	if(ctx->winenv.glx && !wglDeleteContext(ctx->winenv.glx)) {
		return aga_af_winerr(__FILE__, "wglDeleteContext");
	}

	if(!UnregisterClassA(AGA_CLASS_NAME, 0)) {
		return aga_af_winerr(__FILE__, "UnregisterClassA");
	}

	return AF_ERR_NONE;
}

enum af_err aga_mkwin(struct aga_ctx* ctx, struct aga_win* win) {
	int ind;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	win->xwin = (af_ulong_t) CreateWindowA(
		AGA_CLASS_NAME, "Aft Gang Aglay",
		WS_CAPTION | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		ctx->settings.width, ctx->settings.height,
		0, 0, ctx->dpy, ctx);
	if(!win->xwin) return aga_af_winerr(__FILE__, "CreateWindowA");
	if(!ShowWindow((void*) win->xwin, SW_SHOWNORMAL)) {
		return aga_af_winerr(__FILE__, "ShowWindow");
	}

	if(!(win->storage = GetDC((void*) win->xwin)))  {
		return aga_af_winerr(__FILE__, "GetDC");
	}

	if(!(ind = ChoosePixelFormat(win->storage, &pixel_format))) {
		return aga_af_winerr(__FILE__, "ChoosePixelFormat");
	}
	if(!SetPixelFormat(win->storage, ind, &pixel_format)) {
		return aga_af_winerr(__FILE__, "SetPixelFormat");
	}

	return AF_ERR_NONE;
}

enum af_err aga_killwin(struct aga_ctx* ctx, struct aga_win* win) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	/*
	if(!ReleaseDC((void*) win->xwin, win->storage)) {
		return aga_af_winerr(__FILE__, "ReleaseDC");
	}

	if(!DestroyWindow((void*) win->xwin)) {
		return aga_af_winerr(__FILE__, "DestroyWindow");
	}
	 */

	return AF_ERR_NONE;
}

enum af_err aga_glctx(struct aga_ctx* ctx, struct aga_win* win) {
	RECT r;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	if(!(ctx->winenv.glx = wglCreateContext(win->storage))) {
		return aga_af_winerr(__FILE__, "wglCreateContext");
	}

	if(!wglMakeCurrent(win->storage, ctx->winenv.glx)) {
		return aga_af_winerr(__FILE__, "wglMakeCurrent");
	}

	SetCapture((void*) win->xwin);
	if(!GetWindowRect((void*) win->xwin, &r)) {
		return aga_af_winerr(__FILE__, "GetWindowRect");
	}
	if(!ClipCursor(&r)) return aga_af_winerr(__FILE__, "ClipCursor");

	return AF_ERR_NONE;
}

enum af_err aga_swapbuf(struct aga_ctx* ctx, struct aga_win* win) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	if(!SwapBuffers(win->storage)) {
		return aga_af_winerr(__FILE__, "SwapBuffers");
	}

	return AF_ERR_NONE;
}

enum af_err aga_poll(struct aga_ctx* ctx) {
	MSG msg;

	AF_PARAM_CHK(ctx);

	while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	return AF_ERR_NONE;
}

#endif
