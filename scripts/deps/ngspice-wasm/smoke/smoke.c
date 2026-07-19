/*
 * Gate-1 smoke test for the wasm ngspice sharedspice static build.
 * Runs under node (see run-smoke.sh). Four scenarios, each printing
 * "SMOKE PASS <name>" on success; any failure prints "SMOKE FAIL <name>: why"
 * and exits non-zero at the end.
 *
 *   rc     - foreground .tran of an RC charge curve, numeric check
 *   xspice - gain a-device .op (proves the static code-model registry:
 *            the model only exists if spinit's codemodel lines resolved)
 *   cider  - numd (CIDER) silicon resistor DC sweep completes
 *   halt   - bg_run on a heavy deck, bg_halt mid-run, BGThreadRunning
 *            callback fires with finished=true (proves the real pthread path)
 *
 * Built with -sPROXY_TO_PTHREAD so main() may block (usleep) while ngspice's
 * own background thread simulates.
 */

#include <stdatomic.h>
#include <stdbool.h> /* sharedspice.h's NG_BOOL fallback typedef needs it */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ngspice/sharedspice.h"

static atomic_int g_bg_finished_events;
static atomic_int g_exit_called;
static atomic_int g_char_lines;
static int g_failures;

static int cb_send_char(char *what, int id, void *user)
{
    (void) id;
    (void) user;
    atomic_fetch_add(&g_char_lines, 1);
    if (getenv("SMOKE_VERBOSE"))
        fprintf(stderr, "[ngspice] %s\n", what);
    return 0;
}

static int cb_send_stat(char *what, int id, void *user)
{
    (void) what;
    (void) id;
    (void) user;
    return 0;
}

static int cb_controlled_exit(int status, NG_BOOL immediate, NG_BOOL quit,
                              int id, void *user)
{
    (void) immediate;
    (void) quit;
    (void) id;
    (void) user;
    fprintf(stderr, "[smoke] ControlledExit status=%d\n", status);
    atomic_store(&g_exit_called, 1);
    return 0;
}

static int cb_bg_running(NG_BOOL finished, int id, void *user)
{
    (void) id;
    (void) user;
    if (finished)
        atomic_fetch_add(&g_bg_finished_events, 1);
    return 0;
}

static void fail(const char *name, const char *why)
{
    printf("SMOKE FAIL %s: %s\n", name, why);
    g_failures++;
}

static void pass(const char *name)
{
    printf("SMOKE PASS %s\n", name);
}

/* Fetch a vector, returning its length and (optionally) the last real value. */
static int vec_last(const char *vec, int *len_out, double *last_out)
{
    pvector_info vi = ngGet_Vec_Info((char *) vec);

    if (!vi || vi->v_length <= 0)
        return -1;
    if (len_out)
        *len_out = vi->v_length;
    if (last_out) {
        if (vi->v_realdata)
            *last_out = vi->v_realdata[vi->v_length - 1];
        else if (vi->v_compdata)
            *last_out = vi->v_compdata[vi->v_length - 1].cx_real;
        else
            return -1;
    }
    return 0;
}

static int run_circ(const char *const *lines)
{
    /* ngSpice_Circ wants a NULL-terminated array of writable strings. */
    int n = 0;
    while (lines[n])
        n++;

    char **arr = malloc((size_t) (n + 1) * sizeof(char *));
    for (int i = 0; i < n; i++)
        arr[i] = strdup(lines[i]);
    arr[n] = NULL;

    int ret = ngSpice_Circ(arr);

    for (int i = 0; i < n; i++)
        free(arr[i]);
    free(arr);
    return ret;
}

static void test_rc(void)
{
    static const char *const deck[] = {
        "rc smoke",
        "V1 in 0 1",
        "R1 in out 1k",
        "C1 out 0 1u",
        ".tran 10u 5m",
        ".end",
        NULL,
    };

    if (run_circ(deck) != 0)
        return fail("rc", "ngSpice_Circ failed");
    if (ngSpice_Command("run") != 0)
        return fail("rc", "run command failed");

    int len = 0;
    double last = 0.0;
    if (vec_last("out", &len, &last) != 0)
        return fail("rc", "vector 'out' missing");

    /* 5 tau: v = 1 - exp(-5) = 0.99326 */
    if (last < 0.98 || last > 1.0) {
        char buf[128];
        snprintf(buf, sizeof buf, "v(out) final %.5f not in [0.98, 1.0] (len %d)",
                 last, len);
        return fail("rc", buf);
    }
    pass("rc");
}

static void test_xspice(void)
{
    static const char *const deck[] = {
        "xspice smoke",
        "V1 in 0 2",
        "A1 in aout gainblk",
        ".model gainblk gain(gain=3)",
        "R1 aout 0 1k",
        ".op",
        ".end",
        NULL,
    };

    if (run_circ(deck) != 0)
        return fail("xspice", "ngSpice_Circ failed (code models not registered?)");
    if (ngSpice_Command("run") != 0)
        return fail("xspice", "run command failed");

    double v = 0.0;
    if (vec_last("aout", NULL, &v) != 0)
        return fail("xspice", "vector 'aout' missing");
    if (v < 5.999 || v > 6.001) {
        char buf[96];
        snprintf(buf, sizeof buf, "v(aout) %.6f != 6.0", v);
        return fail("xspice", buf);
    }
    pass("xspice");
}

static void test_cider(void)
{
    /* Reduced examples/cider/resistor/sires.cir: numd level=1 needs the whole
     * CIDER machinery (mesh, doping, mobility models) to produce a current. */
    static const char *const deck[] = {
        "cider smoke - silicon resistor",
        "VPP 1 0 2v",
        "VNN 2 0 0.0v",
        "D1 1 2 M_RES AREA=1",
        ".MODEL M_RES numd level=1",
        "+ options resistor defa=1p",
        "+ x.mesh loc=0.0 num=1",
        "+ x.mesh loc=1.0 num=21",
        "+ domain   num=1 material=1",
        "+ material num=1 silicon",
        "+ doping unif n.type conc=2.5e16",
        "+ models bgn srh conctau auger concmob fieldmob",
        ".DC VPP 0.0v 2.01v 0.5v",
        ".END",
        NULL,
    };

    if (run_circ(deck) != 0)
        return fail("cider", "ngSpice_Circ failed (CIDER not compiled in?)");
    if (ngSpice_Command("run") != 0)
        return fail("cider", "run command failed");

    int len = 0;
    double i_last = 0.0;
    if (vec_last("vpp#branch", &len, &i_last) != 0)
        return fail("cider", "vector 'vpp#branch' missing");
    if (len < 4)
        return fail("cider", "DC sweep produced too few points");
    if (!(i_last < 0.0) || i_last < -1.0)
        return fail("cider", "resistor current magnitude implausible");
    pass("cider");
}

static void test_halt(void)
{
    /* Heavy enough that bg_halt lands mid-run: a long transient of a 100-stage
     * nonlinear RC/diode ladder, storage bounded via .save. */
    const int stages = 100;
    const char **deck = malloc((size_t) (stages * 3 + 8) * sizeof(char *));
    char **owned = malloc((size_t) (stages * 3 + 8) * sizeof(char *));
    int n = 0;

    owned[n] = strdup("halt smoke - rc/diode ladder");
    deck[n] = owned[n];
    n++;
    owned[n] = strdup("V1 n0 0 SIN(0 5 10k)");
    deck[n] = owned[n];
    n++;
    for (int i = 0; i < stages; i++) {
        char line[96];
        snprintf(line, sizeof line, "R%d n%d n%d 100", i + 1, i, i + 1);
        owned[n] = strdup(line);
        deck[n] = owned[n];
        n++;
        snprintf(line, sizeof line, "C%d n%d 0 10n", i + 1, i + 1);
        owned[n] = strdup(line);
        deck[n] = owned[n];
        n++;
        snprintf(line, sizeof line, "D%d n%d 0 dmod", i + 1, i + 1);
        owned[n] = strdup(line);
        deck[n] = owned[n];
        n++;
    }
    owned[n] = strdup(".model dmod d(is=1e-14)");
    deck[n] = owned[n];
    n++;
    owned[n] = strdup(".save v(n100)");
    deck[n] = owned[n];
    n++;
    owned[n] = strdup(".tran 100n 10");
    deck[n] = owned[n];
    n++;
    owned[n] = strdup(".end");
    deck[n] = owned[n];
    n++;
    deck[n] = NULL;
    owned[n] = NULL;

    int circ_ret = ngSpice_Circ((char **) deck);
    for (int i = 0; i < n; i++)
        free(owned[i]);
    free(owned);
    free(deck);

    if (circ_ret != 0)
        return fail("halt", "ngSpice_Circ failed");

    int before = atomic_load(&g_bg_finished_events);

    if (ngSpice_Command("bg_run") != 0)
        return fail("halt", "bg_run command failed");

    /* Give the background thread time to actually start and chew. */
    usleep(400 * 1000);

    if (!ngSpice_running())
        return fail("halt", "ngSpice_running false 400ms into a 10s transient "
                            "(bg thread never started or deck too light)");

    if (ngSpice_Command("bg_halt") != 0)
        return fail("halt", "bg_halt command failed");

    /* bg_halt joins the bg thread; give the finished callback a moment. */
    for (int i = 0; i < 50 && atomic_load(&g_bg_finished_events) == before; i++)
        usleep(100 * 1000);

    if (atomic_load(&g_bg_finished_events) == before)
        return fail("halt", "BGThreadRunning(finished) never fired after bg_halt");
    if (ngSpice_running())
        return fail("halt", "still running after bg_halt");

    pass("halt");
}

int main(int argc, char **argv)
{
    /* Optional argv[1]: run only the named scenario (rc|xspice|cider|halt). */
    const char *only = argc > 1 ? argv[1] : NULL;

    int ret = ngSpice_Init(cb_send_char, cb_send_stat, cb_controlled_exit,
                           NULL, NULL, cb_bg_running, NULL);
    if (ret != 0) {
        printf("SMOKE FAIL init: ngSpice_Init returned %d\n", ret);
        return 1;
    }
    printf("SMOKE PASS init\n");

    if (!only || !strcmp(only, "rc"))
        test_rc();
    if (!only || !strcmp(only, "xspice"))
        test_xspice();
    if (!only || !strcmp(only, "cider"))
        test_cider();
    if (!only || !strcmp(only, "halt"))
        test_halt();

    if (atomic_load(&g_exit_called))
        fail("exit", "ControlledExit fired during the smoke run");

    printf(g_failures ? "SMOKE RESULT: %d failure(s)\n" : "SMOKE RESULT: all passed\n",
           g_failures);
    return g_failures ? 1 : 0;
}
