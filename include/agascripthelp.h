/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPTHELP_H
#define AGA_SCRIPTHELP_H

/*
 * NOTE: I'm okay with allowing a bit more macro magic in here to reduce the
 * 		 Overall verbosity of the Python glue code.
 */

#define AGA_SCRIPTPROC(name) \
	static aga_pyobject_t agan_##name(AGA_UNUSED aga_pyobject_t self, \
			AGA_UNUSED aga_pyobject_t arg)

#define AGA_NONERET \
	do { \
		INCREF(None); \
		return None; \
	} while(0)

#define AGA_ARGLIST(type) (arg && is_##type##object(arg))
#define AGA_ARG(var, n, type) \
	(((var) = gettupleitem(arg, (n))) && is_##type##object((var)))

#define AGA_ARGERR(func, types) \
	do { \
		err_setstr(TypeError, func "() arguments must be " types); \
		return 0; \
	} while(0)

#define AGA_SCRIPTVAL(var, obj, type) \
	do { \
		(var) = get##type##value((obj)); \
		if(err_occurred()) return 0; \
	} while(0)

#define AGA_SCRIPTBOOL(var, obj) \
	do { \
		(var) = !!getintvalue((obj)); \
		if(err_occurred()) return 0; \
	} while(0)

#define AGA_NEWOBJ(var, type, args) \
	do { \
		(var) = new##type##object args; \
		if(!(var)) return 0; \
	} while(0)

#define AGA_SETLISTITEM(list, n, value) \
	if(setlistitem(list, n, value) == -1) return 0
#define AGA_GETLISTITEM(list, n, var) \
	if(!((var) = getlistitem(list, n))) return 0

#define AGA_GETTUPLEITEM(tuple, n, var) \
	if(!((var) = gettupleitem(tuple, n))) return 0

#define AGA_GETATTR(class, name, var) \
	if(!((var) = getattr(class, (char*) (name)))) return 0

#endif
