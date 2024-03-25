#ifndef B173C_TEST_BASE_H
#define B173C_TEST_BASE_H

static int _test_success = 0;
static int _test_total = 0;
static int _test_reterrcode = 0;
static const char *_test_name;

#include <stdio.h>
#include "mathlib.h"

#define TEST(name, ...) impl_test_begin(#name); __VA_ARGS__; impl_test_end();
#define TESTING_END() return _test_reterrcode; }
#define TESTING_BEGIN() int main(void) {

static inline void impl_test_begin(const char *funcname)
{
    _test_total = 0;
    _test_success = 0;
    _test_name = funcname;

    printf("\n------ %s ------\n", funcname);
}

static inline void impl_test_end(void)
{
    printf("%d/%d succeeded: ", _test_success, _test_total);
    if(_test_success != _test_total) {
        _test_reterrcode = 1;
        printf("TESTS FAILED!\n");
    } else {
        printf("ALL OK\n");
    }
}

#define assert_impl_params const char *file, int line

#define assert(a) impl_assert(a, __FILE__, __LINE__, #a)
static inline int impl_assert(int a, assert_impl_params, const char *condstr)
{
    if(a) {
        _test_success++;
    } else {
        printf("\n%s: %s:%d: `%s` FAILED\n", _test_name, file, line, condstr);
    }
    _test_total++;

    return a;
}

#define assert_vec3_equals(a, b) impl_assert_vec3_equals(a, b, __FILE__, __LINE__, #a, #b, "assert_vec3_equals("#a", "#b")")
static inline void impl_assert_vec3_equals(vec3_t a, vec3_t b, assert_impl_params, const char *v1, const char *v2, const char *condstr)
{
    if(!impl_assert(vec3_equ(a, b), file, line, condstr)) {
        printf("WITH VALUES:\n");
        printf("  %s = [%.16f, %.16f, %.16f]\n", v1, vec3_unpack(a));
        printf("  %s = [%.16f, %.16f, %.16f]\n\n", v2, vec3_unpack(b));
    }
}

#define assert_float_equals(a, b) impl_assert_float_equals(a, b, __FILE__, __LINE__, #a, #b, "assert_float_equals("#a", "#b")")
static inline void impl_assert_float_equals(float a, float b, assert_impl_params, const char *v1, const char *v2, const char *condstr)
{
    if(!impl_assert(fequ(a, b), file, line, condstr)) {
        printf("WITH VALUES:\n");
        printf("  %s = %.16f\n", v1, a);
        printf("  %s = %.16f\n\n", v2, b);
    }
}
#define assert_vec3_not_equals(a, b) impl_assert_vec3_not_equals(a, b, __FILE__, __LINE__, #a, #b, "assert_vec3_equals("#a", "#b")")
static inline void impl_assert_vec3_not_equals(vec3_t a, vec3_t b, assert_impl_params, const char *v1, const char *v2, const char *condstr)
{
    if(!impl_assert(!vec3_equ(a, b), file, line, condstr)) {
        printf("WITH VALUES:\n");
        printf("  %s = [%.16f, %.16f, %.16f]\n", v1, vec3_unpack(a));
        printf("  %s = [%.16f, %.16f, %.16f]\n\n", v2, vec3_unpack(b));
    }
}

#endif
