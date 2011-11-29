#ifndef ERROR_H_
#define ERROR_H_

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

void
xexit (const char *what);

void
xfake_on_error_stack_trace(const char *a, ...);

#if defined X_ERROR_ASSERT_INSTEAD_OF_STACK_TRACE
#define g_xassert(exp) g_assert(exp)
#elif defined X_ERROR_STACK_TRACE_INSTEAD_OF_ASSERT
#define g_xassert(exp) ((exp) ? 1 : xfake_on_error_stack_trace("XFAKE_ON_ERROR_STACK_TRACE", (exp)))
#else
#error X_ERROR THING NOT DEFINED
#endif

#ifdef __cplusplus
}
#endif

#endif /* ERROR_H_ */
