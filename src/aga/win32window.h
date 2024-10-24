/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_WIN32_WINDOW_H
#define AGA_WIN32_WINDOW_H

/*
 * NOTE: Everything in here is from the future! Microsoft only started
 * 		 Supporting OpenGL in 1997-ish. Unfortunately shipping without Windows
 * 		 Support would be a bit of a death sentence so here we are. Apparently
 * 		 NT3.5 had GL support so we may need to look at that.
 */

#include <aga/error.h>
#include <aga/utility.h>

#define AGA_WANT_WINDOWS_H

#include <aga/win32.h>

#define AGA_CLASS_NAME ("AftGangAglay")

#define AGAW_KEYMAX (0xFF)

static const PIXELFORMATDESCRIPTOR pixel_format = {
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
/*
 * TODO: Broader palletization support -- user-specified palletes etc.
 * 		 See
 * 		 https://learn.microsoft.com/en-us/windows/win32/opengl/color-index-mode-and-windows-palette-management.
 */
#ifdef AGA_PALLETIZE
		PFD_TYPE_COLORINDEX,
#else
		PFD_TYPE_RGBA,
#endif
		24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0,
		PFD_MAIN_PLANE,
		0, 0, 0, 0
};

#define AGA_WINPROC_PACK_MAGIC (0x547123AA)

struct aga_winproc_pack {
	struct aga_window* win;
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

/*
 * TODO: Attach winpack to all windows so poll can be window-independent?
 * 		 Current setup does not handle multiwindow well -- especially teardown.
 */
static LRESULT CALLBACK aga_winproc(
		HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) {

	struct aga_winproc_pack* pack;
	aga_bool_t down = AGA_TRUE;

	if(msg == WM_NCCREATE) return TRUE;

	SetLastError(0);
	if(!(pack = (void*) GetWindowLongPtr(wnd, GWLP_USERDATA))) {
		/* TODO: We might be able to get out of this. */
		if(GetLastError()) {
			aga_win32_error(__FILE__, "GetWindowLongPtr");
			aga_error_abort();
		}

		goto default_msg;
	}

	if(pack->magic != AGA_WINPROC_PACK_MAGIC) goto default_msg;

	switch(msg) {
		default: {
			default_msg:;
			return DefWindowProc(wnd, msg, w_param, l_param);
		}

		case WM_KEYUP: {
			down = AGA_FALSE;
			AGA_FALLTHROUGH;
		}
		/* FALLTHROUGH */
		case WM_KEYDOWN: {
			pack->keymap->states[w_param] = down;
			return 0;
		}

		/* TODO: This isn't really era-appropriate. */
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

			if(!(data = aga_malloc(input_size))) {
				(void) aga_error_system(__FILE__, "aga_malloc");
				return 0;
			}

			result = GetRawInputData(
					hnd, RID_INPUT, data, &input_size, header_size);
			if(result == err) {
				(void) aga_win32_error(__FILE__, "GetRawInputData");
				aga_free(data);
				return 0;
			}

			if(data->header.dwType == RIM_TYPEMOUSE) {
				pack->pointer->dx = data->data.mouse.lLastX;
				pack->pointer->dy = data->data.mouse.lLastY;
			}

			aga_free(data);
			return 0;
		}

		case WM_MOUSEMOVE: {
			pack->pointer->x = GET_X_LPARAM(l_param) - pack->win->client_off_x;
			pack->pointer->y = GET_Y_LPARAM(l_param) - pack->win->client_off_y;
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
			*pack->die = AGA_TRUE;
			return 0;
		}
	}
}

/*
 * Many thanks to code taken from: https://github.com/quakeforge/quakeforge/
 * (see libs/video/targets/).
 */

enum aga_result aga_window_device_new(
		struct aga_window_device* env, const char* display) {

	WNDCLASSA class;

	if(!env) return AGA_RESULT_BAD_PARAM;

	(void) display;

	env->captured = AGA_FALSE;
	env->visible = AGA_TRUE;

	if(!(env->module = GetModuleHandle(0))) {
		return aga_win32_error(__FILE__, "GetModuleHandle");
	}

	/* TODO: Re-enable icon. */
	/*
	if(!(icon = LoadIcon(env->module, MAKEINTRESOURCE(AGA_ICON_RESOURCE)))) {
		(void) aga_win32_error(__FILE__, "LoadIcon");
	}
	 */

	if(!(env->cursor = LoadCursor(0, IDC_ARROW))) {
		(void) aga_win32_error(__FILE__, "LoadIcon");
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

	if(!(env->class = RegisterClass(&class))) {
		return aga_win32_error(__FILE__, "RegisterClass");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_window_device_delete(struct aga_window_device* env) {
	if(!env) return AGA_RESULT_BAD_PARAM;

	if(!UnregisterClass(AGA_CLASS_NAME, 0)) {
		return aga_win32_error(__FILE__, "UnregisterClass");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_keymap_new(
		struct aga_keymap* keymap, struct aga_window_device* env) {

	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!env) return AGA_RESULT_BAD_PARAM;

	/* VK_OEM_CLEAR + 1 */
	keymap->states = aga_calloc(AGAW_KEYMAX, sizeof(aga_bool_t));
	if(!keymap->states) return aga_error_system(__FILE__, "aga_calloc");

	return AGA_RESULT_OK;
}

enum aga_result aga_keymap_delete(struct aga_keymap* keymap) {
	if(!keymap) return AGA_RESULT_BAD_PARAM;

	aga_free(keymap->states);

	return AGA_RESULT_OK;
}

static enum aga_result aga_window_set_wgl(
		struct aga_window_device* env, struct aga_window* win, void* dc) {

	HGDIOBJ font;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	if(!(win->wgl = wglCreateContext(dc))) {
		return aga_win32_error(__FILE__, "wglCreateContext");
	}

	if(!wglMakeCurrent(dc, win->wgl)) {
		return aga_win32_error(__FILE__, "wglMakeCurrent");
	}

	/*
	 * TODO: This can be shared via. `wglShareLists' between all window wgl
	 * contexts.
	 */
	{
		if(!(font = GetStockObject(SYSTEM_FONT))) {
			return aga_win32_error(__FILE__, "GetStockObject");
		}

		if(!SelectObject(dc, font)) {
			return aga_win32_error(__FILE__, "SelectObject");
		}

		if(!wglUseFontBitmaps(dc, 0, 256, AGA_FONT_LIST_BASE)) {
			return aga_win32_error(__FILE__, "wglUseFontBitmaps");
		}
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_window_new(
		aga_size_t width, aga_size_t height, const char* title,
		struct aga_window_device* env, struct aga_window* win,
		aga_bool_t do_wgl, int argc, char** argv) {

	static const long mask = WS_VISIBLE | WS_OVERLAPPEDWINDOW;

	enum aga_result result;

	RAWINPUTDEVICE mouse = {
			HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, 0, 0 };

	int ind;
    void* dc;

	(void) argc;
	(void) argv;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	/* TODO: Leaky error states. */
	win->hwnd = CreateWindow(
            AGA_CLASS_NAME, title, mask,
            CW_USEDEFAULT, CW_USEDEFAULT, (int) width, (int) height,
            0, 0, env->module, 0);
    if(!win->hwnd) return aga_win32_error(__FILE__, "CreateWindow");

    if(!ShowWindow((void*) win->hwnd, SW_SHOWNORMAL)) {
		return aga_win32_error(__FILE__, "ShowWindow");
	}

	if(!(dc = GetDC((void*) win->hwnd))) {
		return aga_win32_error(__FILE__, "GetDC");
	}

	if(!(ind = ChoosePixelFormat(dc, &pixel_format))) {
		return aga_win32_error(__FILE__, "ChoosePixelFormat");
	}
	if(!SetPixelFormat(dc, ind, &pixel_format)) {
		return aga_win32_error(__FILE__, "SetPixelFormat");
	}

	if(do_wgl) {
		result = aga_window_set_wgl(env, win, dc);
		if(result) return result;
	}
	else win->wgl = 0;

	if(!ReleaseDC(win->hwnd, dc)) {
		return aga_win32_error(__FILE__, "ReleaseDC");
	}

	/*
	 * TODO: This isn't really era appropriate -- probably need to replicate
	 * 		 The X API.
	 */
	mouse.hwndTarget = win->hwnd;
	if(!RegisterRawInputDevices(&mouse, 1, sizeof(mouse))) {
		return aga_win32_error(__FILE__, "RegisterRawInputDevices");
	}

	{
		RECT r;
		if(!GetClientRect(win->hwnd, &r)) {
			return aga_win32_error(__FILE__, "GetClientRect");
		}

		win->width = r.right - r.left;
		win->height = r.bottom - r.top;

		win->client_off_x = r.left;
		win->client_off_y = r.top;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_window_delete(
		struct aga_window_device* env, struct aga_window* win) {

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

    if(win->wgl && !wglDeleteContext(win->wgl)) {
        return aga_win32_error(__FILE__, "wglDeleteContext");
    }

	if(DestroyWindow(win->hwnd)) {
		return aga_win32_error(__FILE__, "DestroyWindow");
	}

    return AGA_RESULT_OK;
}

enum aga_result aga_window_select(
		struct aga_window_device* env, struct aga_window* win) {

	void* dc;

	(void) env;

	if(!win) return AGA_RESULT_BAD_PARAM;

	if(!(dc = GetDC((void*) win->hwnd))) {
		return aga_win32_error(__FILE__, "GetDC");
	}

	if(!wglMakeCurrent(dc, win->wgl)) {
		return aga_win32_error(__FILE__, "wglMakeCurrent");
	}

	if(!ReleaseDC(win->hwnd, dc)) {
		return aga_win32_error(__FILE__, "ReleaseDC");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_keymap_lookup(
		struct aga_keymap* keymap, unsigned sym, aga_bool_t* state) {

	if(!keymap) return AGA_RESULT_BAD_PARAM;
	if(!state) return AGA_RESULT_BAD_PARAM;

	if(!keymap->states) return AGA_RESULT_ERROR;

	if(sym > AGAW_KEYMAX) return AGA_RESULT_BAD_OP;

	*state = keymap->states[sym];

	return AGA_RESULT_OK;
}

static enum aga_result aga_setclipcursor(
		struct aga_window* win, aga_bool_t clip) {

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

enum aga_result aga_window_set_cursor(
		struct aga_window_device* env, struct aga_window* win,
		aga_bool_t visible, aga_bool_t captured) {

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	SetCursor(visible ? env->cursor : 0);
	env->visible = visible;

	env->captured = captured;

	return aga_setclipcursor(win, captured);
}

enum aga_result aga_window_swap(
		struct aga_window_device* env, struct aga_window* win) {

	void* dc;

	if(!env) return AGA_RESULT_BAD_PARAM;
	if(!win) return AGA_RESULT_BAD_PARAM;

	if(!(dc = GetDC((void*) win->hwnd))) {
		return aga_win32_error(__FILE__, "GetDC");
	}

	if(!SwapBuffers(dc)) {
		return aga_win32_error(__FILE__, "SwapBuffers");
	}

	if(!ReleaseDC(win->hwnd, dc)) {
		return aga_win32_error(__FILE__, "ReleaseDC");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_window_device_poll(
		struct aga_window_device* env, struct aga_keymap* keymap,
		struct aga_window* win, struct aga_pointer* pointer, aga_bool_t* die,
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
	pack.win = win;

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
	SetWindowLongPtr(win->hwnd, GWLP_USERDATA, (LONG_PTR) &pack);
	if(GetLastError()) return aga_win32_error(__FILE__, "SetWindowLongPtr");

	while(PeekMessage(&msg, win->hwnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_dialog(
		const char* message, const char* title, aga_bool_t* response,
		aga_bool_t is_error) {

	DWORD icon = is_error ? MB_ICONERROR : MB_ICONINFORMATION;
	DWORD flags = MB_YESNO | MB_TASKMODAL | icon;
	int res;

	if(!message) return AGA_RESULT_BAD_PARAM;
	if(!title) return AGA_RESULT_BAD_PARAM;
	if(!response) return AGA_RESULT_BAD_PARAM;

	if(!(res = MessageBox(0, message, title, flags))) {
		return aga_win32_error(__FILE__, "MessageBox");
	}

	*response = (res == IDYES);

	return AGA_RESULT_OK;
}

static enum aga_result aga_windiagerr(DWORD err) {
	switch(err) {
		default: return AGA_RESULT_ERROR;
		case CDERR_DIALOGFAILURE: return AGA_RESULT_ERROR;
		case CDERR_FINDRESFAILURE: return AGA_RESULT_ERROR;
		case CDERR_INITIALIZATION: return AGA_RESULT_OOM;
		case CDERR_LOADRESFAILURE: return AGA_RESULT_ERROR;
		case CDERR_LOADSTRFAILURE: return AGA_RESULT_ERROR;
		case CDERR_LOCKRESFAILURE: return AGA_RESULT_BAD_OP;
		case CDERR_MEMALLOCFAILURE: return AGA_RESULT_OOM;
		case CDERR_MEMLOCKFAILURE: return AGA_RESULT_BAD_OP;
		case CDERR_NOHINSTANCE: return AGA_RESULT_ERROR;
		case CDERR_NOHOOK: return AGA_RESULT_ERROR;
		case CDERR_NOTEMPLATE: return AGA_RESULT_ERROR;
		case CDERR_REGISTERMSGFAIL: return AGA_RESULT_ERROR;
		case CDERR_STRUCTSIZE: return AGA_RESULT_BAD_PARAM;
		case FNERR_BUFFERTOOSMALL: return AGA_RESULT_ERROR;
		case FNERR_INVALIDFILENAME: return AGA_RESULT_BAD_PARAM;
		case FNERR_SUBCLASSFAILURE: return AGA_RESULT_OOM;
	}
}

enum aga_result aga_dialog_file(char** result) {
	OPENFILENAMEA openfile = { 0 };

	if(!result) return AGA_RESULT_BAD_PARAM;

	/* Not ideal but seems to be correct for this particular invoc. pattern. */
	if(!(*result = aga_calloc(MAX_PATH, sizeof(char)))) return AGA_RESULT_OOM;

	openfile.lStructSize = sizeof(openfile);
	openfile.lpstrFilter = "All Files\0*.*\0\0";
	openfile.lpstrFile = *result;
	openfile.nMaxFile = MAX_PATH;
	openfile.lpstrInitialDir = ".";
	openfile.Flags = OFN_FILEMUSTEXIST;

	if(!GetOpenFileName(&openfile)) {
		aga_free(*result);
		return aga_windiagerr(CommDlgExtendedError());
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_shell_open(const char* uri) {
	int flags = SW_SHOWNORMAL;

	if(!uri) return AGA_RESULT_BAD_PARAM;

	if((INT_PTR) ShellExecute(0, 0, uri, 0, 0, flags) > 32) {
		return AGA_RESULT_OK;
	}

	return aga_win32_error_path(__FILE__, "ShellExecute", uri);
}

#endif
