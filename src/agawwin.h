/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_W_WIN_H
#define AGA_W_WIN_H

/*
 * NOTE: Everything in here is from the future! Microsoft only started
 * 		 Supporting OpenGL in 1997-ish. Unfortunately shipping without Windows
 * 		 Support would be a bit of a death sentence so here we are.
 */

#include <agaerr.h>

#define AGA_WANT_WINDOWS_H

#include <agaw32.h>

#define AGA_CLASS_NAME ("Aft Gang Aglay")

static const PIXELFORMATDESCRIPTOR pixel_format = {
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0,
		PFD_MAIN_PLANE, 0, 0, 0, 0 };

#define AGA_WINPROC_PACK_MAGIC (0x547123AA)

struct aga_winproc_pack {
	struct aga_keymap* keymap;
	struct aga_pointer* pointer;
	struct aga_buttons* buttons;
	aga_bool_t* die;
	aga_uint_t magic;
};

static void aga_setbuttondown(struct aga_buttons* b, enum aga_button t) {
	enum aga_button_state* v = &b->states[t];
	*v = ((*v == AGA_BUTTON_UP) ? AGA_BUTTON_CLICK : AGA_BUTTON_DOWN);
}

static LRESULT CALLBACK aga_winproc(
		HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) {

	struct aga_winproc_pack* pack;
	aga_bool_t down = AGA_TRUE;

	if(msg == WM_NCCREATE) return TRUE;

	SetLastError(0);
	if(!(pack = (void*) GetWindowLongPtrA(wnd, GWLP_USERDATA))) {
		/* TODO: We might be able to get out of this. */
		if(GetLastError()) {
			aga_win32_error(__FILE__, "GetWindowLongPtrA");
			aga_abort();
		}

		goto default_msg;
	}

	if(pack->magic != AGA_WINPROC_PACK_MAGIC) goto default_msg;

	switch(msg) {
		default: {
			default_msg:;
			return DefWindowProcA(wnd, msg, w_param, l_param);
		}

		case WM_KEYUP: down = AGA_FALSE;
			AGA_FALLTHROUGH;
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
				(void) aga_win32_error(__FILE__, "GetRawInputData");
				return 0;
			}

			if(!(data = malloc(input_size))) {
				(void) aga_errno(__FILE__, "malloc");
				return 0;
			}

			result = GetRawInputData(
					hnd, RID_INPUT, data, &input_size, header_size);
			if(result == err) {
				(void) aga_win32_error(__FILE__, "GetRawInputData");
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

		case WM_LBUTTONDOWN: {
			aga_setbuttondown(pack->buttons, AGA_BUTTON_LEFT);
			return 0;
		}
		case WM_LBUTTONUP: {
			pack->buttons->states[AGA_BUTTON_LEFT] = AGA_BUTTON_UP;
			return 0;
		}

		case WM_RBUTTONDOWN: {
			aga_setbuttondown(pack->buttons, AGA_BUTTON_RIGHT);
			return 0;
		}
		case WM_RBUTTONUP: {
			pack->buttons->states[AGA_BUTTON_RIGHT] = AGA_BUTTON_UP;
			return 0;
		}

		case WM_MBUTTONDOWN: {
			aga_setbuttondown(pack->buttons, AGA_BUTTON_MIDDLE);
			return 0;
		}
		case WM_MBUTTONUP: {
			pack->buttons->states[AGA_BUTTON_MIDDLE] = AGA_BUTTON_UP;
			return 0;
		}

		case WM_CLOSE: {
			if(!ReleaseDC(wnd, GetDC(wnd))) {
				(void) aga_win32_error(__FILE__, "ReleaseDC");
			}
			if(!DestroyWindow(wnd)) {
				(void) aga_win32_error(__FILE__, "DestroyWindow");
			}
			*pack->die = AGA_TRUE;
			return 0;
		}
	}
}

/*
 * Many thanks to code yoinked from: https://github.com/quakeforge/quakeforge/
 * (see libs/video/targets/).
 */

enum aga_result aga_mkwinenv(struct aga_winenv* env, const char* display) {
	WNDCLASSA class;

	if(!env) return AGA_RESULT_BAD_PARAM;

	(void) display;

	env->captured = AGA_FALSE;
	env->visible = AGA_TRUE;

	if(!(env->module = GetModuleHandleA(0))) {
		return aga_win32_error(__FILE__, "GetModuleHandleA");
	}

	/* TODO: Re-enable icon. */
	/*
	if(!(icon = LoadIconA(env->module, MAKEINTRESOURCEA(AGA_ICON_RESOURCE)))) {
		(void) aga_win32_error(__FILE__, "LoadIconA");
	}
	 */

	if(!(env->cursor = LoadCursorA(0, IDC_ARROW))) {
		(void) aga_win32_error(__FILE__, "LoadIconA");
	}

	class.style = CS_GLOBALCLASS;
	class.lpfnWndProc = aga_winproc;
	class.cbClsExtra = 0;
	class.cbWndExtra = 0;
	class.hInstance = env->module;
	class.hIcon = 0;
	class.hCursor = 0;
	class.hbrBackground = 0;
	class.lpszMenuName = 0;
	class.lpszClassName = AGA_CLASS_NAME;

	if(!(env->class = RegisterClassA(&class))) {
		return aga_win32_error(__FILE__, "RegisterClassA");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_killwinenv(struct aga_winenv* env) {
	if(!env) return AGA_RESULT_BAD_PARAM;

	if(env->wgl && !wglDeleteContext(env->wgl)) {
		return aga_win32_error(__FILE__, "wglDeleteContext");
	}

	if(!UnregisterClassA(AGA_CLASS_NAME, 0)) {
		return aga_win32_error(__FILE__, "UnregisterClassA");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_mkkeymap(
		struct aga_keymap* keymap, struct aga_winenv* env) {

	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!env) return AGA_RESULT_BAD_PARAM;

	/* VK_OEM_CLEAR + 1 */
	keymap->keystates = calloc(0xFF, sizeof(aga_bool_t));
	if(!keymap->keystates) return AGA_RESULT_OOM;

	return AGA_RESULT_OK;
}

enum aga_result aga_killkeymap(struct aga_keymap* keymap) {
	if(!keymap) return AGA_RESULT_BAD_PARAM;

	free(keymap->keystates);

	return AGA_RESULT_OK;
}

enum aga_result aga_mkwin(
		aga_size_t width, aga_size_t height, struct aga_winenv* env,
		struct aga_win* win, int argc, char** argv) {

	int ind;
	long mask = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	RAWINPUTDEVICE mouse = {
			HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, 0, 0 };

	(void) argc;
	(void) argv;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	win->width = width;
	win->height = height;

	win->hwnd = CreateWindowA(AGA_CLASS_NAME, "Aft Gang Aglay", mask,
							  CW_USEDEFAULT, CW_USEDEFAULT, (int) width,
							  (int) height, 0, 0, env->module, 0);
	if(!win->hwnd) return aga_win32_error(__FILE__, "CreateWindowA");
	if(!ShowWindow((void*) win->hwnd, SW_SHOWNORMAL)) {
		return aga_win32_error(__FILE__, "ShowWindow");
	}

	if(!(win->dc = GetDC((void*) win->hwnd))) {
		return aga_win32_error(__FILE__, "GetDC");
	}

	if(!(ind = ChoosePixelFormat(win->dc, &pixel_format))) {
		return aga_win32_error(__FILE__, "ChoosePixelFormat");
	}
	if(!SetPixelFormat(win->dc, ind, &pixel_format)) {
		return aga_win32_error(__FILE__, "SetPixelFormat");
	}

	mouse.hwndTarget = win->hwnd;
	if(RegisterRawInputDevices(&mouse, 1, sizeof(mouse))) {
		return aga_win32_error(__FILE__, "RegisterRawInputDevices");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_killwin(struct aga_winenv* env, struct aga_win* win) {
	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	return AGA_RESULT_OK;
}

enum aga_result aga_keylook(
		struct aga_keymap* keymap, aga_uint8_t sym, aga_bool_t* state) {

	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!state) return AGA_RESULT_BAD_PARAM;

	if(!keymap->keystates) return AGA_RESULT_ERROR;
	*state = keymap->keystates[sym];

	return AGA_RESULT_OK;
}

enum aga_result aga_glctx(struct aga_winenv* env, struct aga_win* win) {
	HGDIOBJ font;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	if(!(env->wgl = wglCreateContext(win->dc))) {
		return aga_win32_error(__FILE__, "wglCreateContext");
	}

	if(!wglMakeCurrent(win->dc, env->wgl)) {
		return aga_win32_error(__FILE__, "wglMakeCurrent");
	}

	if(!(font = GetStockObject(SYSTEM_FONT))) {
		return aga_win32_error(__FILE__, "GetStockObject");
	}

	if(!SelectObject(win->dc, font)) {
		return aga_win32_error(__FILE__, "SelectObject");
	}

	if(!wglUseFontBitmaps(win->dc, 0, 256, AGA_FONT_LIST_BASE)) {
		return aga_win32_error(__FILE__, "wglUseFontBitmaps");
	}

	return AGA_RESULT_OK;
}

static enum aga_result aga_setclipcursor(struct aga_win* win, aga_bool_t clip) {
	RECT rect;
	POINT begin;
	POINT end;

	if(!win) return AGA_RESULT_BAD_PARAM;

	if(clip) {
		if(!GetClientRect(win->hwnd, &rect)) {
			return aga_win32_error(__FILE__, "GetClientRect");
		}

		begin.x = rect.left;
		begin.y = rect.top;
		end.x = rect.right;
		end.y = rect.bottom;

		if(!ClientToScreen(win->hwnd, &begin)) {
			return aga_win32_error(__FILE__, "ClientToScreen");
		}
		if(!ClientToScreen(win->hwnd, &end)) {
			return aga_win32_error(__FILE__, "ClientToScreen");
		}

		/*
		 * NOTE: There seems to be a bit of a wonky activation area for the
		 * Resize cursor, so we have to do a bit of fanagling here. Better to
		 * Have a too-small clip area than have the mouse "leaking" out of the
		 * Side of the window.
		 */
		rect.left = begin.x;
		rect.top = begin.y + 1;
		rect.right = end.x - 2;
		rect.bottom = end.y - 1;
	}

	ClipCursor(clip ? &rect : 0);

	return AGA_RESULT_OK;
}

enum aga_result aga_setcursor(
		struct aga_winenv* env, struct aga_win* win, aga_bool_t visible,
		aga_bool_t captured) {

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	SetCursor(visible ? env->cursor : 0);
	env->visible = visible;

	env->captured = captured;

	return aga_setclipcursor(win, captured);
}

enum aga_result aga_swapbuf(struct aga_winenv* env, struct aga_win* win) {
	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	if(!SwapBuffers(win->dc)) {
		return aga_win32_error(__FILE__, "SwapBuffers");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_poll(
		struct aga_winenv* env, struct aga_keymap* keymap, struct aga_win* win,
		struct aga_pointer* pointer, aga_bool_t* die,
		struct aga_buttons* buttons) {

	enum aga_result result;
	MSG msg;
	struct aga_winproc_pack pack;
	aga_size_t i;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;
	if(!pointer) return AGA_RESULT_BAD_PARAM;
	if(!die) return AGA_RESULT_BAD_PARAM;
	if(!buttons) return AGA_RESULT_BAD_PARAM;

	pack.die = die;
	pack.keymap = keymap;
	pack.pointer = pointer;
	pack.buttons = buttons;
	pack.magic = AGA_WINPROC_PACK_MAGIC;

	for(i = 0; i < AGA_LEN(buttons->states); ++i) {
		if(buttons->states[i] == AGA_BUTTON_CLICK) {
			buttons->states[i] = AGA_BUTTON_DOWN;
		}
	}

	if(env->captured) {
		result = aga_setclipcursor(win, GetActiveWindow() == win->hwnd);
		if(result) return result;
	}

	SetCursor(env->visible ? env->cursor : 0);

	SetLastError(0);
	SetWindowLongPtrA(win->hwnd, GWLP_USERDATA, (LONG_PTR) &pack);
	if(GetLastError()) return aga_win32_error(__FILE__, "SetWindowLongPtrA");

	while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_diag(
		const char* message, const char* title, aga_bool_t* response,
		aga_bool_t is_error) {

	DWORD icon = is_error ? MB_ICONERROR : MB_ICONINFORMATION;
	DWORD flags = MB_YESNO | MB_TASKMODAL | icon;
	int res;

	if(!message) return AGA_RESULT_BAD_PARAM;
	if(!title) return AGA_RESULT_BAD_PARAM;
	if(!response) return AGA_RESULT_BAD_PARAM;

	if(!(res = MessageBoxA(0, message, title, flags))) {
		return aga_win32_error(__FILE__, "MessageBoxA");
	}

	*response = (res == IDYES);

	return AGA_RESULT_OK;
}

enum aga_result aga_shellopen(const char* uri) {
	int flags = SW_SHOWNORMAL;

	if(!uri) return AGA_RESULT_BAD_PARAM;

	if((INT_PTR) ShellExecuteA(0, 0, uri, 0, 0, flags) > 32) {
		return AGA_RESULT_OK;
	}

	return aga_win32_error_path(__FILE__, "ShellExecuteA", uri);
}

#endif
