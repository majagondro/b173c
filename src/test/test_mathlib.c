
#include "test_base.h"

#include "mathlib.c" // include definitions
#include "mathlib.h"

int nth_arg(int n, int a, int b, int c)
{
    switch(n) {
    case 1:
        return a;
    case 2:
        return b;
    case 3:
        return c;
    default:
        return -35746983;
    }
}

TESTING_BEGIN()
    TEST(vec3, {
        vec3_t a, b;

        a = vec3(0, 0, 0);
        b = vec3_1(0);
        assert_vec3_equals(a, b);

        a = vec3(42, 100, 999);
        b = vec3(50, 70, 90);
        assert_vec3_not_equals(a, b);

        assert_float_equals(nth_arg(1, vec3_unpack(a)), 42);
        assert_float_equals(nth_arg(2, vec3_unpack(a)), 100);
        assert_float_equals(nth_arg(3, vec3_unpack(a)), 999);
        assert_float_equals(nth_arg(1, vec3_unpack(b)), 50);
        assert_float_equals(nth_arg(2, vec3_unpack(b)), 70);
        assert_float_equals(nth_arg(3, vec3_unpack(b)), 90);

        a = vec3(5, 7, -1);
        b = vec3(-8, 5, -3);
        assert_vec3_equals(vec3_add(a, b), vec3(-3, 12, -4));
        assert_vec3_equals(vec3_sub(a, b), vec3(13, 2, 2));
        assert_vec3_equals(vec3_mul(a, 2), vec3(10, 14, -2));
        assert_vec3_equals(vec3_mul(b, -0.5f), vec3(4.0f, -2.5f, 1.5f));
        assert_vec3_equals(vec3_div(a, 2), vec3(2.5f, 3.5f, -0.5f));
        assert_vec3_equals(vec3_invdiv(6, b), vec3(-0.75f, 1.2f, -2.0f));
        assert_vec3_equals(vec3_invert(a), vec3(-5, -7, 1));
        assert_vec3_equals(vec3_invert(b), vec3(8, -5, 3));

        assert_float_equals(vec3_len(vec3(1, 0, 0)), 1);
        assert_float_equals(vec3_len(vec3(0, 1, 0)), 1);
        assert_float_equals(vec3_len(vec3(0, 0, 1)), 1);
        assert_float_equals(vec3_len(vec3(1, 0, 1)), SQRT_2);
        assert_float_equals(vec3_len(vec3(1, 1, 1)), SQRT_3);
    })

    TEST(misc, {
        assert_float_equals(deg2rad(90.0f), PI * 0.5f);
        assert_float_equals(deg2rad(180.0f), PI);
        assert_float_equals(deg2rad(270.0f), PI * 1.5f);
        assert_float_equals(deg2rad(360.0f), PI * 2.0f);

        assert_float_equals(rad2deg(PI * 0.5f), 90.0f);
        assert_float_equals(rad2deg(PI), 180.0f);
        assert_float_equals(rad2deg(PI * 1.5f), 270.0f);
        assert_float_equals(rad2deg(PI * 2.0f), 360.0f);

        assert_float_equals(max(1.0f, 2.0f), 2.0f);
        assert_float_equals(max(SQRT_2, SQRT_3), SQRT_3);
        assert_float_equals(max(SQRT_3, PI), PI);
        assert_float_equals(max(PI, SQRT_3), PI);

        assert_float_equals(min(1.0f, 2.0f), 1.0f);
        assert_float_equals(min(SQRT_2, SQRT_3), SQRT_2);
        assert_float_equals(min(SQRT_3, PI), SQRT_3);
        assert_float_equals(min(PI, SQRT_3), SQRT_3);

        assert_float_equals(bound(-1.0f, 0.0f, 1.0f), 0.0f);
        assert_float_equals(bound(-1.0f, -100.0f, 1.0f), -1.0f);
        assert_float_equals(bound(-1.0f, 100.0f, 1.0f), 1.0f);
        assert_float_equals(bound(SQRT_2, 100.0f, SQRT_3), SQRT_3);
        assert_float_equals(bound(SQRT_2, 1.5f, SQRT_3), 1.5f);

        assert_float_equals(sign(-1), -1);
        assert_float_equals(sign(-100), -1);
        assert_float_equals(sign(-SQRT_2), -1);
        assert_float_equals(sign(-PI), -1);
        assert_float_equals(sign(1), 1);
        assert_float_equals(sign(0.0001f), 1);
        assert_float_equals(sign(0.5f), 1);
        assert_float_equals(sign(-0.5f), -1);
        assert_float_equals(sign(PI), 1);
        assert_float_equals(sign(PI * 42.0f * SQRT_2 * SQRT_3), 1);
    })

    TEST(bbox, {
        bbox_t a, b;

        a = (bbox_t){vec3(0, 0, 0), vec3(1, 1, 1)};
        a = bbox_offset(a, vec3(10.01f, -25.25f, 50.50f));
        assert_vec3_equals(a.mins, vec3(10.01f, -25.25f, 50.50f));
        assert_vec3_equals(a.maxs, vec3(11.01f, -24.25f, 51.50f));

        assert(bbox_null((bbox_t){vec3_1(-1), vec3_1(-1)}));
        assert(!bbox_null((bbox_t){vec3_1(1), vec3_1(-1)}));
        assert(!bbox_null((bbox_t){vec3_1(-1), vec3_1(1)}));
        assert(!bbox_null((bbox_t){vec3_1(-1), vec3_1(-0.9999f)}));

        a = (bbox_t){vec3_1(0.0f), vec3_1(1.0f)};
        b = bbox_offset(a, vec3_1(0.5f));

        assert(bbox_intersects(a, b));

        a.mins = vec3_invert(a.mins);
        a.maxs = vec3_invert(a.maxs);

        assert(!bbox_intersects(a, b));

        assert(bbox_intersects_line(
            (bbox_t){vec3_1(0), vec3_1(1)},
            vec3_1(-1), vec3_1(2),
            NULL
        ));

        assert(!bbox_intersects_line(
            (bbox_t){vec3_1(0), vec3_1(1)},
            vec3_1(-1), vec3_1(-2),
            NULL
        ));

        assert(!bbox_intersects_line(
            (bbox_t){vec3_1(0), vec3_1(1)},
            vec3(0, 0, -0.1f), vec3(1, 0, -0.1f),
            NULL
        ));

        assert(!bbox_intersects_line(
            (bbox_t){vec3_1(0), vec3_1(1)},
            vec3(0, 0, -1.0f), vec3(1, 0, -0.01f),
            NULL
        ));
    })

    TEST(mat4, {
        mat4_t a, b;

        mat4_identity(a);
        assert_float_equals(a[0][0], 1);
        assert_float_equals(a[1][1], 1);
        assert_float_equals(a[2][2], 1);
        assert_float_equals(a[3][3], 1);
    })
TESTING_END()
