/*
 * Minimal stub of ngspice's sharedspice.h for KiCad WASM builds.
 *
 * Provides the type names referenced by kicad/eeschema/sim/ngspice.{h,cpp}
 * plus the declarations of the sharedspice CLIENT (sharedspice_client.cpp):
 * the real engine runs in the ngspice_service worker
 * (docs/features/ngspice-split/), and NGSPICE::init_dll()'s __EMSCRIPTEN__
 * branch binds its function pointers to the pcbjam_ngSpice_* forwarders
 * declared below instead of dlopen'ing libngspice.
 *
 * We intentionally do NOT define NGSPICE_PACKAGE_VERSION so that ngspice.h's
 * fallback `typedef bool NG_BOOL;` (line 46) provides the boolean type.
 */

#ifndef KICAD_WASM_NGSPICE_SHAREDSPICE_STUB_H
#define KICAD_WASM_NGSPICE_SHAREDSPICE_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ngcomplex {
    double cx_real;
    double cx_imag;
} ngcomplex_t;

struct vector_info {
    char*        v_name;
    int          v_type;
    short        v_flags;
    double*      v_realdata;
    ngcomplex_t* v_compdata;
    int          v_length;
};

typedef struct vector_info* pvector_info;

/* Opaque payload types for callbacks we never wire up (SendData/SendInitData). */
typedef struct vecvaluesall* pvecvaluesall;
typedef struct vecinfoall*   pvecinfoall;

/*
 * Function types (not pointers). ngspice.h references them as `SendChar*` etc.,
 * so the trailing star in the typedef site makes the pointer.
 */
typedef int (SendChar)(char*, int, void*);
typedef int (SendStat)(char*, int, void*);
typedef int (ControlledExit)(int, bool, bool, int, void*);
typedef int (SendData)(pvecvaluesall, int, int, void*);
typedef int (SendInitData)(pvecinfoall, int, void*);
typedef int (BGThreadRunning)(bool, int, void*);

/*
 * The sharedspice client (wasm/stubs/sharedspice_client.cpp): RPC forwarders
 * to the ngspice_service worker, signature-compatible with NGSPICE's private
 * function-pointer typedefs (the pcbjam_ prefix avoids shadowing by those
 * class-scope typedef names inside init_dll).
 */
void         pcbjam_ngSpice_Init(SendChar*, SendStat*, ControlledExit*, SendData*,
                                 SendInitData*, BGThreadRunning*, void*);
int          pcbjam_ngSpice_Circ(char** circarray);
int          pcbjam_ngSpice_Command(char* command);
pvector_info pcbjam_ngGet_Vec_Info(char* vecname);
char*        pcbjam_ngCM_Input_Path(const char* path);
char*        pcbjam_ngSpice_CurPlot(void);
char**       pcbjam_ngSpice_AllPlots(void);
char**       pcbjam_ngSpice_AllVecs(char* plotname);
bool         pcbjam_ngSpice_Running(void);

#ifdef __cplusplus
}
#endif

#endif /* KICAD_WASM_NGSPICE_SHAREDSPICE_STUB_H */
