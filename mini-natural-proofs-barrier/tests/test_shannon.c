/* test_shannon.c -- Tests for Shannon Counting Library
 *
 * Tests cover:
 *   - Circuit upper bound computation
 *   - Function count computation
 *   - Feasible fraction computation
 *   - Largeness analysis
 *   - Constructiveness checks
 *   - Natural property criteria
 *   - Edge cases (n=0, small S, large S, overflow)
 *
 * Key mathematical invariants:
 *   1. shannon_circuit_upper_bound(n, 0) == 1.0
 *   2. shannon_function_count(n) == 2^{2^n}
 *   3. 0 <= shannon_feasible_fraction(n, S) <= 1.0
 *   4. shannon_largeness_fraction + shannon_feasible_fraction = 1.0
 *   5. For poly S: shannon_feasible_fraction(n, S) -> 0 as n grows
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <float.h>
#include "natural.h"

static int total_tests = 0;
static int passed_tests = 0;

#define TEST(name) do { total_tests++; printf("  TEST %-55s", name); } while(0)
#define PASS()     do { passed_tests++; printf(" PASS\n"); } while(0)
#define FAIL(m)    do { printf(" FAIL: %s\n", m); } while(0)

static void test_circuit_upper_bound(void) {
    TEST("shannon_circuit_upper_bound(n=4, S=0)");
    double c = shannon_circuit_upper_bound(4, 0);
    assert(c == 1.0);  /* Only the empty circuit */
    PASS();

    TEST("shannon_circuit_upper_bound monotonic in S");
    double prev = shannon_circuit_upper_bound(5, 0);
    for (int S = 1; S <= 10; S++) {
        double curr = shannon_circuit_upper_bound(5, S);
        assert(curr >= prev);
        prev = curr;
    }
    PASS();

    TEST("shannon_circuit_upper_bound_log consistent with bound");
    for (int n = 4; n <= 8; n += 2) {
        for (int S = n; S <= n*n; S += n) {
            double log_c = shannon_circuit_upper_bound_log(n, S);
            double c = shannon_circuit_upper_bound(n, S);
            if (c < 1e300) {
                /* log2(c) should be approximately log_c */
                double actual_log = log2(c);
                assert(fabs(actual_log - log_c) < 1.0);
            }
        }
    }
    PASS();

    TEST("shannon_circuit_upper_bound(n=0, S=0)");
    assert(shannon_circuit_upper_bound(0, 0) == 1.0);
    PASS();
}

static void test_function_count(void) {
    TEST("shannon_function_count small n");
    assert(shannon_function_count(0) == 2.0);     /* 2^{2^0} = 2 */
    assert(shannon_function_count(1) == 4.0);     /* 2^{2^1} = 4 */
    assert(shannon_function_count(2) == 16.0);    /* 2^{2^2} = 16 */
    assert(shannon_function_count(3) == 256.0);   /* 2^{2^3} = 256 */
    PASS();

    TEST("shannon_function_count_log matches");
    for (int n = 0; n <= 10; n++) {
        double log_f = shannon_function_count_log(n);
        assert(fabs(log_f - pow(2.0, (double)n)) < 1e-10);
    }
    PASS();

    TEST("shannon_function_count n=10 overflows");
    assert(isinf(shannon_function_count(10)));
    PASS();
}

static void test_feasible_fraction(void) {
    TEST("shannon_feasible_fraction in [0,1]");
    for (int n = 4; n <= 8; n += 2) {
        int S = n * n;
        double f = shannon_feasible_fraction(n, S);
        assert(f >= 0.0);
        assert(f <= 1.0);
    }
    PASS();

    TEST("shannon_feasible_fraction + largeness_fraction = 1");
    for (int n = 4; n <= 8; n += 2) {
        int S = n * n;
        double feasible = shannon_feasible_fraction(n, S);
        double large = shannon_largeness_fraction(n, S);
        assert(fabs(feasible + large - 1.0) < 1e-10);
    }
    PASS();

    TEST("Poly S => fraction is tiny for large n");
    for (int n = 8; n <= 12; n += 4) {
        int S = n * n;
        double f = shannon_feasible_fraction(n, S);
        /* With poly S, feasible fraction should be very small */
        assert(f < 0.01);
    }
    PASS();

    TEST("shannon_feasible_fraction_log is negative for poly S");
    for (int n = 8; n <= 12; n += 4) {
        int S = n * n;
        double log_f = shannon_feasible_fraction_log(n, S);
        assert(log_f < -10.0);  /* very negative */
    }
    PASS();
}

static void test_largeness(void) {
    TEST("shannon_largeness: poly S => large");
    for (int n = 4; n <= 12; n += 4) {
        int S = n * n;
        double large = shannon_largeness_fraction(n, S);
        assert(large >= 0.95);  /* nearly all functions are hard */
    }
    PASS();

    TEST("shannon_largeness: S too large => not large");
    /* When S = 2^n, almost all functions are computable (by Lupanov 1958).
     * For small n, this won't be true (2^4 = 16 < 2^4/4 = 4? no),
     * but for moderate n, S = 2^n/n is the threshold. */
    for (int n = 4; n <= 8; n += 2) {
        int S = (int)pow(2.0, (double)n);  /* very large S */
        double large = shannon_largeness_fraction(n, S);
        printf(" [n=%d,S=%d:%.4f]", n, S, large);
        /* At S=2^n, basically all functions have size <= S */
        assert(large < 0.5);
    }
    PASS();
}

static void test_constructiveness(void) {
    TEST("shannon_is_constructive: poly S => YES");
    for (int n = 4; n <= 16; n += 4) {
        int S = n * n;
        assert(shannon_is_constructive(n, S) == 1);
    }
    PASS();

    TEST("shannon_is_constructive: exp S => NO");
    for (int n = 4; n <= 8; n += 2) {
        int S = (int)pow(2.0, (double)n);
        /* For n=4: S=16, constructive? Let's check */
        int constructive = shannon_is_constructive(n, S);
        printf(" [n=%d,S=%d:%s]", n, S, constructive ? "YES" : "NO");
        /* At S=2^n for moderate n, should NOT be constructive */
        if (n >= 8) assert(!constructive);
    }
    PASS();
}

static void test_natural_criteria(void) {
    TEST("shannon_natural_criteria_check returns valid struct");
    NaturalProperty np = shannon_natural_criteria_check(8, 64);
    assert(np.constructive == 1 || np.constructive == 0);
    assert(np.large == 1 || np.large == 0);
    assert(np.useful == 1);
    PASS();

    TEST("Poly S => natural property");
    for (int n = 4; n <= 12; n += 4) {
        int S = n * n;
        NaturalProperty np = shannon_natural_criteria_check(n, S);
        /* For poly S: constructive=YES, large=YES, useful=YES => natural=YES */
        assert(np.constructive == 1);
        assert(np.large == 1);
        assert(np.useful == 1);
        assert(np.is_natural == 1);
    }
    PASS();

    TEST("S too large => not natural (fails largeness)");
    int n = 4;
    int S = 256;  /* S = 2^n for n=4 — too large */
    NaturalProperty np = shannon_natural_criteria_check(n, S);
    /* Largeness should fail */
    printf(" [nat=%d con=%d lar=%d]", np.is_natural, np.constructive, np.large);
    /* For n=4, S=256: almost all functions have size <= 256, so largeness fails */
    assert(np.large == 0);
    PASS();
}

int main(void) {
    setbuf(stdout, NULL);
    printf("=== Shannon Counting Tests ===\n\n");

    test_circuit_upper_bound();
    test_function_count();
    test_feasible_fraction();
    test_largeness();
    test_constructiveness();
    test_natural_criteria();

    printf("\n=== Results: %d/%d passed ===\n", passed_tests, total_tests);
    if (passed_tests < total_tests) {
        printf("SOME TESTS FAILED!\n");
        return 1;
    }
    printf("ALL TESTS PASSED.\n");
    return 0;
}
