#pragma once

#ifdef __linux__
#include <ucontext.h>
#include <sys/mman.h>
#endif

#include <assert.h>
#include <cstdlib>

namespace edu {
    namespace pfm {

//------------------------------------------------------------
//--- Platform-independent definitions
//------------------------------------------------------------

        enum mem_access_t {
            MemAccess_None,
            MemAccess_ReadWrite
        };

//------------------------------------------------------------
//--- LINUX
//------------------------------------------------------------
#ifdef __linux__

//------------------------------------------------------------
//--- LINUX Atomics
//------------------------------------------------------------

#define atomicAdd(ptr, val) __sync_fetch_and_add(ptr, val)

//------------------------------------------------------------
//--- LINUX Fibers
//------------------------------------------------------------

        // We pass these signatures as a sanity check.
        const size_t __FIBER_SIGNATURE_PREFIX = 0xdeadbeef;
        const size_t __FIBER_SIGNATURE_SUFFIX = 0xabcd1234;

        typedef ucontext_t fiber_context_t;
        typedef void (*fiber_context_entry_func_t)(void *user_data);

        // Entry-point for fiber. Calls user's fiber entry point.
        void __fiber_entry(size_t signature_prefix,
                           fiber_context_entry_func_t entry_func,
                           void *user_data,
                           size_t signature_suffix) {
            assert(signature_prefix == __FIBER_SIGNATURE_PREFIX);
            assert(signature_suffix == __FIBER_SIGNATURE_SUFFIX);

            entry_func(user_data);
        }

        // Initialize a fiber context.
        bool init_fiber_context(fiber_context_t *ctx,
                                fiber_context_t *ctx_exit,
                                void *stack,
                                size_t stacklen,
                                fiber_context_entry_func_t entry_func,
                                void *user_data) {
            if(getcontext(ctx) == -1)
                return false;
            ctx->uc_stack.ss_sp = stack;
            ctx->uc_stack.ss_size = stacklen;
            ctx->uc_link = ctx_exit;
            makecontext(ctx, (void (*)())__fiber_entry,
                        4, __FIBER_SIGNATURE_PREFIX,
                        entry_func,
                        user_data,
                        __FIBER_SIGNATURE_SUFFIX);
            return true;
        }

        // Switch into the fiber context specified in ctx_enter
        // and save the current state in ctx_save.
        bool switch_fiber_context(fiber_context_t *ctx_save,
                                  fiber_context_t *ctx_enter) {
            return swapcontext(ctx_save, ctx_enter) != -1;
        }

        // Dispose any resources for a context, if needed.
        void dispose_fiber_context(fiber_context_t &ctx) {
            //no-op
        }

//------------------------------------------------------------
//--- LINUX Memory Management
//------------------------------------------------------------
        void *alloc(unsigned len) {
            return mmap(nullptr,
                        len,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1, // some systems require fd = -1 for anonymous
                        0);
        }

        // true = succeed
        bool dealloc(void *addr, unsigned len) {
            return 0 == munmap(addr, len);
        }

        // true = succeed
        bool set_mem_access(void *addr, unsigned len, mem_access_t access) {
            int prot;
            switch(access) {
            case MemAccess_None:
                prot = PROT_NONE;
                break;
            case MemAccess_ReadWrite:
                prot = PROT_READ | PROT_WRITE;
                break;
            default:
                abort();
            }

            return 0 == mprotect(addr, len, prot);
        }

#endif

    }
}