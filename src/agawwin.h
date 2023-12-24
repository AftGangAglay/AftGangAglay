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

#include <agaw32.h>

#include <windows.h>
#include <windowsx.h>

#define AGA_CLASS_NAME ("Aft Gang Aglay")

static const PIXELFORMATDESCRIPTOR pixel_format = {
	sizeof(PIXELFORMATDESCRIPTOR), 1,
	PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
	PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	32, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
};

#define AGA_WINPROC_PACK_MAGIC (0x547123AA)

struct aga_winproc_pack {
	struct aga_keymap* keymap;
	struct aga_pointer* pointer;
	af_bool_t* die;
	af_uint_t magic;
};

static LRESULT aga_winproc(
		HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) {

    struct aga_winproc_pack* pack;
	af_bool_t down = AF_TRUE;

	if(msg == WM_NCCREATE) return TRUE;

	SetLastError(0);
	if(!(pack = (void*) GetWindowLongPtrA(wnd, GWLP_USERDATA))) {
		if(GetLastError()) AGA_AF_WINCHK("GetWindowLongPtrA");
		goto default_msg;
	}

	if(pack->magic != AGA_WINPROC_PACK_MAGIC) goto default_msg;

	switch(msg) {
		default: {
			default_msg:;
			return DefWindowProcA(wnd, msg, w_param, l_param);
		}

		case WM_KEYUP: down = AF_FALSE;
			AF_FALLTHROUGH;
			/* FALLTHRU */
		case WM_KEYDOWN: {
			pack->keymap->keystates[w_param] = down;
			return 0;
		}

		case WM_MOUSEMOVE: {
			pack->pointer->dx = GET_X_LPARAM(l_param) - pack->pointer->x;
			pack->pointer->dy = GET_Y_LPARAM(l_param) - pack->pointer->y;
            pack->pointer->x = GET_X_LPARAM(l_param);
            pack->pointer->y = GET_Y_LPARAM(l_param);
			return 0;
		}

		case WM_CLOSE: {
            if(!DestroyWindow(wnd)) (void) aga_af_winerr(__FILE__, "DestroyWindow");
			*pack->die = AF_TRUE;
			return 0;
		}
	}
}

/*
 * Many thanks to code yoinked from: https://github.com/quakeforge/quakeforge/
 * (see libs/video/targets/).
 */

enum af_err aga_mkwinenv(union aga_winenv* env, const char* display) {
	WNDCLASSA class;

	AF_PARAM_CHK(env);

	(void) display;

	if(!(env->win32.module = GetModuleHandleA(0))) {
		return aga_af_winerr(__FILE__, "GetModuleHandleA");
	}

	class.style = CS_GLOBALCLASS;
	class.lpfnWndProc = (WNDPROC) aga_winproc;
	class.cbClsExtra = 0;
	class.cbWndExtra = 0;
	class.hInstance = env->win32.module;
	class.hIcon = 0;
	class.hCursor = 0;
	class.hbrBackground = 0;
	class.lpszMenuName = 0;
	class.lpszClassName = AGA_CLASS_NAME;

	if(!(env->win32.class = RegisterClassA(&class))) {
		return aga_af_winerr(__FILE__, "RegisterClassA");
	}

	return AF_ERR_NONE;
}

enum af_err aga_killwinenv(union aga_winenv* env) {
	AF_PARAM_CHK(env);

	if(env->win32.wgl && !wglDeleteContext(env->win32.wgl)) {
		return aga_af_winerr(__FILE__, "wglDeleteContext");
	}

	if(!UnregisterClassA(AGA_CLASS_NAME, 0)) {
		return aga_af_winerr(__FILE__, "UnregisterClassA");
	}

	return AF_ERR_NONE;
}

enum af_err aga_mkkeymap(struct aga_keymap* keymap, union aga_winenv* env) {
	AF_PARAM_CHK(keymap);
	AF_PARAM_CHK(env);

	keymap->keysyms_per_keycode = 1;
	keymap->keycode_len = 0xFF;

	keymap->keystates = calloc(0xFF, sizeof(af_bool_t)); /* VK_OEM_CLEAR + 1 */
	AF_VERIFY(keymap->keystates, AF_ERR_MEM);

	return AF_ERR_NONE;
}

enum af_err aga_killkeymap(struct aga_keymap* keymap) {
	AF_PARAM_CHK(keymap);

	return AF_ERR_NONE;
}

enum af_err aga_mkwin(
		af_size_t width, af_size_t height, union aga_winenv* env,
		struct aga_win* win, int argc, char** argv) {

	int ind;
	long mask = WS_CAPTION | WS_VISIBLE | WS_OVERLAPPEDWINDOW;

	(void) argc;
	(void) argv;

	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	win->width = width;
	win->height = height;

	win->win.hwnd = CreateWindowA(
		AGA_CLASS_NAME, "Aft Gang Aglay", mask, CW_USEDEFAULT, CW_USEDEFAULT,
		width, height, 0, 0, env->win32.module, 0);
	if(!win->win.hwnd) return aga_af_winerr(__FILE__, "CreateWindowA");
	if(!ShowWindow((void*) win->win.hwnd, SW_SHOWNORMAL)) {
		return aga_af_winerr(__FILE__, "ShowWindow");
	}

	if(!(win->dc = GetDC((void*) win->win.hwnd)))  {
		return aga_af_winerr(__FILE__, "GetDC");
	}

	if(!(ind = ChoosePixelFormat(win->dc, &pixel_format))) {
		return aga_af_winerr(__FILE__, "ChoosePixelFormat");
	}
	if(!SetPixelFormat(win->dc, ind, &pixel_format)) {
		return aga_af_winerr(__FILE__, "SetPixelFormat");
	}

	return AF_ERR_NONE;
}

enum af_err aga_killwin(union aga_winenv* env, struct aga_win* win) {
	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	/*
	if(!ReleaseDC((void*) win->xwin, win->dc)) {
		return aga_af_winerr(__FILE__, "ReleaseDC");
	}

	if(!DestroyWindow((void*) win->xwin)) {
		return aga_af_winerr(__FILE__, "DestroyWindow");
	}
	 */

	return AF_ERR_NONE;
}

enum af_err aga_glctx(union aga_winenv* env, struct aga_win* win) {
	RECT r;

	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	if(!(env->win32.wgl = wglCreateContext(win->dc))) {
		return aga_af_winerr(__FILE__, "wglCreateContext");
	}

	if(!wglMakeCurrent(win->dc, env->win32.wgl)) {
		return aga_af_winerr(__FILE__, "wglMakeCurrent");
	}

	if(!SelectObject(win->dc, GetStockObject(SYSTEM_FONT))) {
		return aga_af_winerr(__FILE__, "SelectObject");
	}

	if(!wglUseFontBitmaps(win->dc, 0, 255, AGA_FONT_LIST_BASE)) {
		return aga_af_winerr(__FILE__, "wglUseFontBitmaps");
	}

	SetCapture((void*) win->win.hwnd);
	if(!GetWindowRect((void*) win->win.hwnd, &r)) {
		return aga_af_winerr(__FILE__, "GetWindowRect");
	}
	if(!ClipCursor(&r)) return aga_af_winerr(__FILE__, "ClipCursor");

	return AF_ERR_NONE;
}

enum af_err aga_swapbuf(union aga_winenv* env, struct aga_win* win) {
	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	if(!SwapBuffers(win->dc)) {
		return aga_af_winerr(__FILE__, "SwapBuffers");
	}

	return AF_ERR_NONE;
}

enum af_err aga_poll(
		union aga_winenv* env, struct aga_keymap* keymap, struct aga_win* win,
		struct aga_pointer* pointer, af_bool_t* die) {

	MSG msg;
    struct aga_winproc_pack pack;

	AF_PARAM_CHK(env);
	AF_PARAM_CHK(keymap);
	AF_PARAM_CHK(win);
	AF_PARAM_CHK(pointer);
	AF_PARAM_CHK(die);

    pack.die = die;
    pack.keymap = keymap;
    pack.pointer = pointer;
	pack.magic = AGA_WINPROC_PACK_MAGIC;

    SetLastError(0);
    SetWindowLongPtrA(win->win.hwnd, GWLP_USERDATA, (LONG_PTR) &pack);
    if(GetLastError()) AGA_AF_WINCHK("SetWindowLongPtrA");

	while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	return AF_ERR_NONE;
}

enum af_err aga_diag(
		const char* message, const char* title, af_bool_t* response) {

	DWORD flags = MB_YESNO | MB_ICONINFORMATION | MB_TASKMODAL;
	int res;

	AF_PARAM_CHK(message);
	AF_PARAM_CHK(title);
	AF_PARAM_CHK(response);

	if(!(res = MessageBoxA(0, message, title, flags))) {
		return aga_af_winerr(__FILE__, "MessageBoxA");
	}

	*response = (res == IDYES);

	return AF_ERR_NONE;
}

enum af_err aga_shellopen(const char* uri) {
	int flags = SW_SHOWNORMAL;

	AF_PARAM_CHK(uri);

	if((INT_PTR) ShellExecuteA(0, 0, uri, 0, 0, flags) > 32) {
		return AF_ERR_NONE;
	}

	return aga_af_pathwinerr(__FILE__, "ShellExecuteA", uri);
}

#endif
