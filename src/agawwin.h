/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_W_WIN_H
#define AGA_W_WIN_H

#include <windows.h>

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

		if(!LocalFree(buf)) {
			aga_log(__FILE__, "err: LocalFree failed");
			return AF_ERR_UNKNOWN;
		}
	}

	return AF_ERR_UNKNOWN;
}

static enum af_err aga_af_winerr(const char* loc, const char* proc) {
	return aga_af_pathwinerr(loc, proc, 0);
}

/*
 * Many thanks to code yoinked from: https://github.com/quakeforge/quakeforge/
 * (see libs/video/targets/).
 */

static LRESULT aga_winproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_KEYDOWN: {
			break;
		}
		case WM_KEYUP: {
			break;
		}

		case WM_LBUTTONDOWN: {
			break;
		}
		case WM_RBUTTONDOWN: {
			break;
		}

		case WM_MOUSEMOVE: {
			break;
		}

		case WM_NCDESTROY: {
			break;
		}

		default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 1;
}

enum af_err aga_mkctxdpy(struct aga_ctx* ctx, const char* display) {
	WNDCLASSA class;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(display);

	if(!(ctx->dpy = GetModuleHandleA(0))) {
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

	if(!(ctx->dpy_fd = RegisterClassA(&class))) {
		return aga_af_winerr(__FILE__, "RegisterClassA");
	}

	return AF_ERR_NONE;
}

enum af_err aga_killctxdpy(struct aga_ctx* ctx) {
	AF_PARAM_CHK(ctx);

	if(ctx->glx && !wglDeleteContext(ctx->glx)) {
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
		0, 0, ctx->dpy, 0);
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

	if(!ReleaseDC((void*) win->xwin, win->storage)) {
		return aga_af_winerr(__FILE__, "ReleaseDC");
	}

	if(!DestroyWindow((void*) win->xwin)) {
		return aga_af_winerr(__FILE__, "DestroyWindow");
	}

	return AF_ERR_NONE;
}

enum af_err aga_glctx(struct aga_ctx* ctx, struct aga_win* win) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(win);

	if(!(ctx->glx = wglCreateContext(win->storage))) {
		return aga_af_winerr(__FILE__, "wglCreateContext");
	}

	if(!wglMakeCurrent(win->storage, ctx->glx)) {
		return aga_af_winerr(__FILE__, "wglMakeCurrent");
	}

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
	(void) ctx;
	return AF_ERR_NONE;
}

#endif
