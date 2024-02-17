/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agagl.h>
#include <agaresult.h>
#include <agalog.h>
#include <agaerr.h>
#include <agascript.h>
#include <agapack.h>
#include <agapyinc.h>
#include <agautil.h>
#include <agascripthelp.h>

#include <aganobj.h>

/*
 * TODO: Failure states here are super leaky - we can probably compartmentalise
 * 		 This function a lot more to help remedy this.
 */
AGA_SCRIPTPROC(mkobj) {
        struct aga_nativeptr* nativeptr;
        struct agan_object* obj;
        aga_pyobject_t retval;

        struct aga_respack* pack;

        if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return 0;

        if(!AGA_ARGLIST(string)) AGA_ARGERR("mkobj", "string");

        AGA_NEWOBJ(retval, nativeptr, ());
        nativeptr = (struct aga_nativeptr*) retval;

        if(!(nativeptr->ptr = calloc(1, sizeof(struct agan_object))))
            return err_nomem();

        obj = nativeptr->ptr;
        if(!(obj->transform = newdictobject())) return 0;

        return (aga_pyobject_t) retval;
}

AGA_SCRIPTPROC(killobj) {
        struct aga_nativeptr* nativeptr;
        struct agan_object* obj;

        if(!AGA_ARGLIST(nativeptr)) AGA_ARGERR("killobj", "nativeptr");

        nativeptr = (struct aga_nativeptr*) arg;
        obj = nativeptr->ptr;

        glDeleteLists(obj->drawlist, 1);
        if(aga_script_glerr("glDeleteLists")) return 0;

        DECREF(obj->transform);

        AGA_NONERET;
}

AGA_SCRIPTPROC(inobj) {
        aga_pyobject_t retval = False;
        aga_pyobject_t o, j, dbg, point, flobj, scale, pos;
        float pt[3];
        float mins[3];
        float maxs[3];
        float f;
        aga_bool_t p, d;
        aga_size_t i;
        struct agan_object* obj;

        if(!AGA_ARGLIST(tuple) || !AGA_ARG(o, 0, nativeptr) ||
            !AGA_ARG(point, 1, list) || !AGA_ARG(j, 2, int) ||
            !AGA_ARG(dbg, 3, int)) {

            AGA_ARGERR("inobj", "nativeptr, list, int and int");
        }

        AGA_SCRIPTBOOL(p, j);
        AGA_SCRIPTBOOL(d, dbg);

        obj = ((struct aga_nativeptr*) o)->ptr;
        memcpy(mins, obj->min_extent, sizeof(mins));
        memcpy(maxs, obj->max_extent, sizeof(maxs));

        if(!(scale = dictlookup(obj->transform, "scale"))) return 0;
        if(!(pos = dictlookup(obj->transform, "pos"))) return 0;

        for(i = 0; i < 3; ++i) {
            AGA_GETLISTITEM(point, i, flobj);
            AGA_SCRIPTVAL(pt[i], flobj, float);

            AGA_GETLISTITEM(scale, i, flobj);
            AGA_SCRIPTVAL(f, flobj, float);

            mins[i] *= f;
            maxs[i] *= f;
        }

        for(i = 0; i < 3; ++i) {
            AGA_GETLISTITEM(pos, i, flobj);
            AGA_SCRIPTVAL(f, flobj, float);

            mins[i] += f;
            maxs[i] += f;
        }

        /* TODO: Once cells are implemented check against current cell rad. */

        if(pt[0] > mins[0] && (p || pt[1] > mins[1]) && pt[2] > mins[2]) {
            if(pt[0] < maxs[0] && (p || pt[1] < maxs[1]) && pt[2] < maxs[2]) {
                retval = True;
            }
        }

        if(d) {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glDisable(GL_DEPTH_TEST);
            glBegin(GL_LINE_STRIP);
            glColor3f(0.0f, 1.0f, 0.0f);
                glVertex3f(mins[0], maxs[1], maxs[2]);
                glVertex3f(maxs[0], maxs[1], maxs[2]);
                glVertex3f(mins[0], mins[1], maxs[2]);
                glVertex3f(maxs[0], mins[1], maxs[2]);
                glVertex3f(maxs[0], mins[1], mins[2]);
                glVertex3f(maxs[0], maxs[1], maxs[2]);
                glVertex3f(maxs[0], maxs[1], mins[2]);
                glVertex3f(mins[0], maxs[1], maxs[2]);
                glVertex3f(mins[0], maxs[1], mins[2]);
                glVertex3f(mins[0], mins[1], maxs[2]);
                glVertex3f(mins[0], mins[1], mins[2]);
                glVertex3f(maxs[0], mins[1], mins[2]);
                glVertex3f(mins[0], maxs[1], mins[2]);
                glVertex3f(maxs[0], maxs[1], mins[2]);
            glEnd();
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            glEnable(GL_DEPTH_TEST);
        }

        INCREF(retval);
        return retval;
}

/* TODO: Avoid reloading conf for every call. */
AGA_SCRIPTPROC(objconf) {
        enum aga_result result;

        void* fp;

        aga_pyobject_t o, l, retval;

        struct aga_conf_node conf;
        struct agan_object* obj;

        if(!AGA_ARGLIST(tuple) || !AGA_ARG(o, 0, nativeptr) ||
            !AGA_ARG(l, 1, list)) {

            AGA_ARGERR("objconf", "nativeptr and list");
        }

        obj = ((struct aga_nativeptr*) o)->ptr;

        result = aga_resseek(obj->res, &fp);
        if(aga_script_err("aga_resseek", result)) return 0;

        result = aga_mkconf(fp, obj->res->size, &conf);
        if(aga_script_err("aga_mkconf", result)) return 0;

        retval = agan_scriptconf(&conf, AF_TRUE, l);
        if(!retval) return 0;

        result = aga_killconf(&conf);
        if(aga_script_err("aga_killconf", result)) return 0;

        return retval;
}

AGA_SCRIPTPROC(putobj) {
        struct aga_nativeptr* nativeptr;
        struct agan_object* obj;

        if(!AGA_ARGLIST(nativeptr)) AGA_ARGERR("putobj", "nativeptr");

        nativeptr = (struct aga_nativeptr*) arg;
        obj = nativeptr->ptr;

        glCallList(obj->drawlist);
        if(aga_script_glerr("glCallList")) return 0;

        AGA_NONERET;
}

AGA_SCRIPTPROC(objtrans) {
        struct agan_object* obj;

        if(!AGA_ARGLIST(nativeptr)) AGA_ARGERR("objtrans", "nativeptr");

        obj = ((struct aga_nativeptr*) arg)->ptr;

        return obj->transform;
}
