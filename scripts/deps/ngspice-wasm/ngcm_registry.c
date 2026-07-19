/* NGCM_REGISTRY_MARKER - appended to src/spicelib/devices/dev.c by
 * scripts/deps/build-ngspice.sh (idempotent: guarded by this marker).
 *
 * Registry of the statically-linked XSPICE code models. In the WASM build the
 * bundled .cm files are static archives (see ngcm_dlmain_static.c) and cannot
 * be dlopen'd; load_opus() consults this registry first (hook inserted by
 * build-ngspice.sh) and only falls back to dlopen - which then fails with
 * ngspice's normal error reporting - for paths it does not recognize, e.g.
 * user-compiled code models, which cannot exist as loadable binaries in wasm.
 *
 * Matching is by basename so the spinit "codemodel <path>/analog.cm" lines
 * keep working regardless of the install prefix baked into spinit.
 *
 * No coreitf wiring happens here: in a static link the code-model objects call
 * the MIF and cm_ core functions directly, so the dlmain.c indirection table
 * the dlopen path has to fill in does not exist.
 */
#if defined(NGCM_STATIC) && defined(XSPICE)

#include "ngspice/devdefs.h"
#include "ngspice/evtudn.h"
#include <string.h>

#define NGCM_DECL(cm) \
    extern SPICEdev *ngcm_##cm##_cmDEVices[]; \
    extern int ngcm_##cm##_cmDEVicesCNT; \
    extern Evt_Udn_Info_t *ngcm_##cm##_cmEVTudns[]; \
    extern int ngcm_##cm##_cmEVTudnCNT;

NGCM_DECL(analog)
NGCM_DECL(digital)
NGCM_DECL(spice2poly)
NGCM_DECL(table)
NGCM_DECL(tlines)
NGCM_DECL(xtradev)
NGCM_DECL(xtraevt)

struct ngcm_static_entry {
    const char *basename;
    SPICEdev **devs;
    int *devnum;
    Evt_Udn_Info_t **udns;
    int *udnnum;
};

#define NGCM_ENTRY(cm) \
    { #cm ".cm", ngcm_##cm##_cmDEVices, &ngcm_##cm##_cmDEVicesCNT, \
      ngcm_##cm##_cmEVTudns, &ngcm_##cm##_cmEVTudnCNT }

static const struct ngcm_static_entry ngcm_static_entries[] = {
    NGCM_ENTRY(analog),
    NGCM_ENTRY(digital),
    NGCM_ENTRY(spice2poly),
    NGCM_ENTRY(table),
    NGCM_ENTRY(tlines),
    NGCM_ENTRY(xtradev),
    NGCM_ENTRY(xtraevt),
};

/* Returns load_opus()-compatible status: 0 = registered, -1 = not a bundled
 * code model (caller falls through to the dlopen path). */
int ngcm_static_load(const char *path)
{
    const char *base = strrchr(path, '/');
    size_t i;

    base = base ? base + 1 : path;

    for (i = 0; i < sizeof(ngcm_static_entries) / sizeof(ngcm_static_entries[0]); i++) {
        const struct ngcm_static_entry *e = &ngcm_static_entries[i];

        if (strcmp(base, e->basename) == 0) {
            add_device(*e->devnum, e->devs, 1);
            add_udn(*e->udnnum, e->udns);
            return 0;
        }
    }

    return -1;
}

#endif /* NGCM_STATIC && XSPICE */
