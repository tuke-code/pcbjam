/*
 * Static-build replacement for ngspice's src/xspice/icm/dlmain.c.
 *
 * In the WASM build the XSPICE code models (.cm) cannot be dlopen'd, so each
 * code-model directory is compiled into a static archive instead of a shared
 * object (see build-ngspice.sh, NGCM_STATIC). This TU provides only the two
 * model tables, renamed per code model (ngcm_<cm>_cmDEVices etc.) so all seven
 * archives can coexist in one image. Everything else dlmain.c contains - the
 * CMdevs()/CMudns() accessor exports and the coreitf-forwarding wrappers for
 * the MIF core functions - is deliberately omitted: the registry appended to
 * dev.c (ngcm_registry.c) reads the tables directly, and in a static link the
 * code-model objects bind straight to the real core functions, which the
 * wrappers would otherwise collide with.
 *
 * Compiled once per code model with -DNGCM_NAME=<cm> and -I<cm-build-dir> so
 * the cmpp-generated cmextrn.h/cminfo.h/udnextrn.h/udninfo.h of that model
 * are picked up.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ngspice/config.h"
#include "ngspice/cpextern.h"
#include "ngspice/devdefs.h"
#include "ngspice/dstring.h"
#include "ngspice/dllitf.h"
#include "ngspice/evtudn.h"
#include "ngspice/inpdefs.h"
#include "ngspice/inertial.h"
#include "cmextrn.h"
#include "udnextrn.h"

#ifndef NGCM_NAME
#error "ngcm_dlmain_static.c must be compiled with -DNGCM_NAME=<code model name>"
#endif

#define NGCM_PASTE2(a, b) a##b
#define NGCM_PASTE(a, b) NGCM_PASTE2(a, b)
#define NGCM_SYM(s) NGCM_PASTE(NGCM_PASTE(ngcm_, NGCM_NAME), NGCM_PASTE2(_, s))

SPICEdev *NGCM_SYM(cmDEVices)[] = {
#include "cminfo.h"
    NULL
};

int NGCM_SYM(cmDEVicesCNT) =
    sizeof(NGCM_SYM(cmDEVices)) / sizeof(SPICEdev *) - 1;

Evt_Udn_Info_t *NGCM_SYM(cmEVTudns)[] = {
#include "udninfo.h"
    NULL
};

int NGCM_SYM(cmEVTudnCNT) =
    sizeof(NGCM_SYM(cmEVTudns)) / sizeof(Evt_Udn_Info_t *) - 1;
