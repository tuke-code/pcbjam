/*
 * WASM implementation of libcontext using Emscripten Asyncify
 *
 * Emscripten's Asyncify allows us to implement fiber/coroutine semantics
 * by saving and restoring the WebAssembly call stack.
 *
 * Implementation strategy:
 * - Each fiber context stores an Asyncify data buffer
 * - jump_fcontext suspends current execution and resumes target
 * - make_fcontext creates a new context with a function entry point
 *
 * Note: This requires the WASM module to be compiled with:
 *   -sASYNCIFY=1
 *   -sASYNCIFY_STACK_SIZE=65536  (or larger if needed)
 *
 * For more information on Asyncify:
 * https://emscripten.org/docs/porting/asyncify.html
 */

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fiber.h>
#endif

// Match the API from libcontext.h
#define LIBCONTEXT_CALL_CONVENTION

#ifdef __cplusplus
extern "C" {
#endif

namespace libcontext
{

// Context structure that stores fiber state
struct fiber_context {
    emscripten_fiber_t fiber;
    void (*entry_func)(intptr_t);
    intptr_t entry_arg;
    bool initialized;
    bool running;
    // Stack for asyncify data
    char asyncify_stack[65536];
    // C stack
    char* c_stack;
    size_t c_stack_size;
};

// Current running context
static fiber_context* g_current_context = nullptr;
static fiber_context g_main_context;
static bool g_main_initialized = false;

// Fiber entry wrapper
static void fiber_entry_wrapper(void* arg)
{
    fiber_context* ctx = (fiber_context*)arg;
    if (ctx && ctx->entry_func) {
        ctx->entry_func(ctx->entry_arg);
    }
    // If entry function returns, we need to handle it
    // In original libcontext, this would call _exit
    // For WASM, we'll just return to main context
}

typedef void* fcontext_t;

void LIBCONTEXT_CALL_CONVENTION release_fcontext( fcontext_t ctx )
{
#ifdef __EMSCRIPTEN__
    if (ctx) {
        fiber_context* fctx = (fiber_context*)ctx;
        if (fctx->c_stack) {
            free(fctx->c_stack);
        }
        free(fctx);
    }
#endif
}

intptr_t LIBCONTEXT_CALL_CONVENTION jump_fcontext( fcontext_t* ofc, fcontext_t nfc,
        intptr_t vp, bool preserve_fpu )
{
#ifdef __EMSCRIPTEN__
    // Initialize main context if needed
    if (!g_main_initialized) {
        memset(&g_main_context, 0, sizeof(g_main_context));
        emscripten_fiber_init_from_current_context(
            &g_main_context.fiber,
            g_main_context.asyncify_stack,
            sizeof(g_main_context.asyncify_stack)
        );
        g_main_context.initialized = true;
        g_main_context.running = true;
        g_current_context = &g_main_context;
        g_main_initialized = true;
    }

    fiber_context* old_ctx = g_current_context;
    fiber_context* new_ctx = (fiber_context*)nfc;

    if (!new_ctx || !new_ctx->initialized) {
        fprintf(stderr, "jump_fcontext: invalid target context\n");
        return 0;
    }

    // Store the argument in the new context
    new_ctx->entry_arg = vp;

    // Save the old context pointer
    if (ofc) {
        *ofc = (fcontext_t)old_ctx;
    }

    // Switch contexts
    g_current_context = new_ctx;
    old_ctx->running = false;
    new_ctx->running = true;

    // Perform the fiber switch
    emscripten_fiber_swap(&old_ctx->fiber, &new_ctx->fiber);

    // When we return here, we've been switched back to
    // Return the value passed to us
    return g_current_context->entry_arg;
#else
    return 0;
#endif
}

fcontext_t LIBCONTEXT_CALL_CONVENTION make_fcontext( void* sp, size_t size,
        void (* fn)( intptr_t ) )
{
#ifdef __EMSCRIPTEN__
    // Allocate context structure
    fiber_context* ctx = (fiber_context*)malloc(sizeof(fiber_context));
    if (!ctx) {
        return nullptr;
    }
    memset(ctx, 0, sizeof(fiber_context));

    ctx->entry_func = fn;
    ctx->entry_arg = 0;
    ctx->c_stack = (char*)sp - size;  // sp points to top of stack
    ctx->c_stack_size = size;

    // Initialize the fiber
    // Note: sp is the TOP of the stack (highest address)
    // The stack grows downward, so we need to pass the bottom
    void* stack_bottom = (char*)sp - size;

    emscripten_fiber_init(
        &ctx->fiber,
        fiber_entry_wrapper,
        ctx,                              // User data for entry function
        stack_bottom,                     // C stack (bottom)
        size,                             // C stack size
        ctx->asyncify_stack,              // Asyncify stack
        sizeof(ctx->asyncify_stack)       // Asyncify stack size
    );

    ctx->initialized = true;
    ctx->running = false;

    return (fcontext_t)ctx;
#else
    return nullptr;
#endif
}

}; // namespace libcontext

#ifdef __cplusplus
};
#endif
