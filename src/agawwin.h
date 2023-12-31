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
#include <hidusage.h>

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

		case WM_INPUT: {
			static const UINT err = (UINT) -1;
			static const UINT header_size = sizeof(RAWINPUTHEADER);

			UINT input_size;
			UINT result;
			HRAWINPUT hnd = (HRAWINPUT) l_param;
			RAWINPUT* data;

			/* We are in the bg and aren't consuming input. */
			if(GET_RAWINPUT_CODE_WPARAM(w_param) == RIM_INPUTSINK) return 1;

			result = GetRawInputData(
				hnd, RID_INPUT, 0, &input_size, header_size);
			if(result == err) {
				(void) aga_af_winerr(__FILE__, "GetRawInputData");
				return 0;
			}

			if(!(data = malloc(input_size))) {
				(void) aga_af_errno(__FILE__, "malloc");
				return 0;
			}

			result = GetRawInputData(
				hnd, RID_INPUT, data, &input_size, header_size);
			if(result == err) {
				(void) aga_af_winerr(__FILE__, "GetRawInputData");
				free(data);
				return 0;
			}

			if(data->header.dwType == RIM_TYPEMOUSE) {
				pack->pointer->dx = data->data.mouse.lLastX;
				pack->pointer->dy = data->data.mouse.lLastY;
			}

			free(data);
			return 0;
		}

		case WM_MOUSEMOVE: {
            pack->pointer->x = GET_X_LPARAM(l_param);
            pack->pointer->y = GET_Y_LPARAM(l_param);
			return 0;
		}

		case WM_CLOSE: {
			if(!ReleaseDC(wnd, GetDC(wnd))) {
				(void) aga_af_winerr(__FILE__, "ReleaseDC");
			}
            if(!DestroyWindow(wnd)) {
				(void) aga_af_winerr(__FILE__, "DestroyWindow");
			}
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
	HICON icon;

	AF_PARAM_CHK(env);

	(void) display;

	env->win32.captured = AF_FALSE;
	env->win32.visible = AF_TRUE;

	if(!(env->win32.module = GetModuleHandleA(0))) {
		return aga_af_winerr(__FILE__, "GetModuleHandleA");
	}

	if(!(icon = LoadIconA(env->win32.module, "AGAIcon"))) {
		(void) aga_af_winerr(__FILE__, "LoadIconA");
	}

	if(!(env->win32.cursor = LoadCursorA(0, IDC_ARROW))) {
		(void) aga_af_winerr(__FILE__, "LoadIconA");
	}

	class.style = CS_GLOBALCLASS;
	class.lpfnWndProc = (WNDPROC) aga_winproc;
	class.cbClsExtra = 0;
	class.cbWndExtra = 0;
	class.hInstance = env->win32.module;
	class.hIcon = icon;
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
	long mask = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	RAWINPUTDEVICE mouse = {
		HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, 0, 0 };

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

	mouse.hwndTarget = win->win.hwnd;
	if(RegisterRawInputDevices(&mouse, 1, sizeof(mouse))) {
		return aga_af_winerr(__FILE__, "RegisterRawInputDevices");
	}

	return AF_ERR_NONE;
}

enum af_err aga_killwin(union aga_winenv* env, struct aga_win* win) {
	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	return AF_ERR_NONE;
}

enum af_err aga_glctx(union aga_winenv* env, struct aga_win* win) {
	HGDIOBJ font;

	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	if(!(env->win32.wgl = wglCreateContext(win->dc))) {
		return aga_af_winerr(__FILE__, "wglCreateContext");
	}

	if(!wglMakeCurrent(win->dc, env->win32.wgl)) {
		return aga_af_winerr(__FILE__, "wglMakeCurrent");
	}

	if(!(font = GetStockObject(SYSTEM_FONT))) {
		return aga_af_winerr(__FILE__, "GetStockObject");
	}

	if(!SelectObject(win->dc, font)) {
		return aga_af_winerr(__FILE__, "SelectObject");
	}

	if(!wglUseFontBitmaps(win->dc, 0, 256, AGA_FONT_LIST_BASE)) {
		return aga_af_winerr(__FILE__, "wglUseFontBitmaps");
	}

	return AF_ERR_NONE;
}

static enum af_err aga_setclipcursor(struct aga_win* win, af_bool_t clip) {
	RECT rect;
	POINT begin;
	POINT end;

	AF_PARAM_CHK(win);

	if(clip) {
		if(!GetClientRect(win->win.hwnd, &rect)) {
			return aga_af_winerr(__FILE__, "GetClientRect");
		}

		begin.x = rect.left;
		begin.y = rect.top;
		end.x = rect.right;
		end.y = rect.bottom;

		if(!ClientToScreen(win->win.hwnd, &begin)) {
			return aga_af_winerr(__FILE__, "ClientToScreen");
		}
		if(!ClientToScreen(win->win.hwnd, &end)) {
			return aga_af_winerr(__FILE__, "ClientToScreen");
		}

		rect.left = begin.x;
		rect.top = begin.y;
		rect.right = end.x;
		rect.bottom = end.y;
	}

	ClipCursor(clip ? &rect : 0);

	return AF_ERR_NONE;
}

enum af_err aga_setcursor(
		union aga_winenv* env, struct aga_win* win, af_bool_t visible,
		af_bool_t captured) {

	AF_PARAM_CHK(env);
	AF_PARAM_CHK(win);

	SetCursor(visible ? env->win32.cursor : 0);
	env->win32.visible = visible;

	AF_CHK(aga_setclipcursor(win, captured));
	env->win32.captured = captured;

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

	if(env->win32.captured) {
		AF_CHK(aga_setclipcursor(win, GetActiveWindow() == win->win.hwnd));
	}

	SetCursor(env->win32.visible ? env->win32.cursor : 0);

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
		const char* message, const char* title, af_bool_t* response,
		af_bool_t is_error) {

	DWORD icon = is_error ? MB_ICONERROR : MB_ICONINFORMATION;
	DWORD flags = MB_YESNO | MB_TASKMODAL | icon;
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
