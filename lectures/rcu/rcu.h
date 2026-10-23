// From Linux

/*
 * __unqual_scalar_typeof(x) - Declare an unqualified scalar type, leaving
 *                             non-scalar types unchanged.
 */
/*
 * Prefer C11 _Generic for better compile-times and simpler code. Note: 'char'
 * is not type-compatible with 'signed char', and we define a separate case.
 */
#define __scalar_type_to_expr_cases(type)                               \
                unsigned type:  (unsigned type)0,                       \
                signed type:    (signed type)0

#define __unqual_scalar_typeof(x) typeof(                               \
                _Generic((x),                                           \
                         char:  (char)0,                                \
                         __scalar_type_to_expr_cases(char),             \
                         __scalar_type_to_expr_cases(short),            \
                         __scalar_type_to_expr_cases(int),              \
                         __scalar_type_to_expr_cases(long),             \
                         __scalar_type_to_expr_cases(long long),        \
                         default: (x)))

#define __READ_ONCE(x)  (*(const volatile __unqual_scalar_typeof(x) *)&(x))

#define compiletime_assert_rwonce_type(t)                                       \
/*
 * Use __READ_ONCE() instead of READ_ONCE() if you do not require any
 * atomicity. Note that this may result in tears!
 */
#ifndef __READ_ONCE
#define __READ_ONCE(x)  (*(const volatile __unqual_scalar_typeof(x) *)&(x))
#endif

#define READ_ONCE(x)                                                    \
({                                                                      \
        __READ_ONCE(x);                                                 \
})

#define rcu_dereference(p)      READ_ONCE(p)

void rcu_read_lock();
void rcu_read_unlock();
