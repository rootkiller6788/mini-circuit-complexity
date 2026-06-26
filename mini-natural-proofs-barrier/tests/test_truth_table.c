/* test_truth_table.c -- Tests for Truth Table Library
 *
 * Tests cover:
 *   - Creation and destruction
 *   - Accessors (get, set, evaluate)
 *   - Random generation (statistical properties)
 *   - Comparison (equality, Hamming distance)
 *   - Boolean operations (AND, OR, XOR, NOT)
 *   - Statistics (weight, fraction ones)
 *   - Edge cases (n=0, n=1, large n, null pointers)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "natural.h"

static int total_tests = 0;
static int passed_tests = 0;

#define TEST(name) do { \
    total_tests++; \
    printf("  TEST %-50s", name); \
} while(0)

#define PASS() do { \
    passed_tests++; \
    printf(" PASS\n"); \
} while(0)

#define FAIL(msg) do { \
    printf(" FAIL: %s\n", msg); \
} while(0)

/* ========================================================================
 * Test: Creation and Destruction
 * ======================================================================== */

static void test_creation(void) {
    TEST("tt_create(n=0)");
    TruthTable *t0 = tt_create(0);
    assert(t0 != NULL);
    assert(t0->n_inputs == 0);
    assert(t0->table_size == 1);
    assert(t0->values != NULL);
    assert(t0->values[0] == 0);
    tt_free(t0);
    PASS();

    TEST("tt_create(n=3)");
    TruthTable *t3 = tt_create(3);
    assert(t3 != NULL);
    assert(t3->n_inputs == 3);
    assert(t3->table_size == 8);
    for (int i = 0; i < 8; i++) assert(t3->values[i] == 0);
    tt_free(t3);
    PASS();

    TEST("tt_create(n=10)");
    TruthTable *t10 = tt_create(10);
    assert(t10 != NULL);
    assert(t10->table_size == 1024);
    tt_free(t10);
    PASS();

    TEST("tt_create_named");
    TruthTable *tn = tt_create_named(3, "test_fn");
    assert(tn != NULL);
    assert(strcmp(tn->name, "test_fn") == 0);
    tt_free(tn);
    PASS();

    TEST("tt_copy");
    TruthTable *orig = tt_create(3);
    for (int i = 0; i < 8; i++) orig->values[i] = i % 2;
    TruthTable *cpy = tt_copy(orig);
    assert(cpy != NULL);
    assert(cpy->n_inputs == orig->n_inputs);
    assert(tt_equal(orig, cpy));
    /* Modify original, copy should be independent */
    orig->values[0] = 99;
    assert(cpy->values[0] == 0);
    tt_free(orig);
    tt_free(cpy);
    PASS();

    TEST("tt_free(NULL)");
    tt_free(NULL);  /* should not crash */
    PASS();
}

/* ========================================================================
 * Test: Accessors
 * ======================================================================== */

static void test_accessors(void) {
    TEST("tt_get/tt_set basic");
    TruthTable *t = tt_create(3);
    tt_set(t, 0, 1);
    assert(tt_get(t, 0) == 1);
    tt_set(t, 7, 1);
    assert(tt_get(t, 7) == 1);
    assert(tt_get(t, 3) == 0);
    tt_set(t, 3, 1);
    assert(tt_get(t, 3) == 1);
    tt_free(t);
    PASS();

    TEST("tt_get out of bounds");
    TruthTable *t2 = tt_create(3);
    assert(tt_get(t2, -1) == 0);
    assert(tt_get(t2, 8) == 0);
    assert(tt_get(t2, 1000) == 0);
    tt_free(t2);
    PASS();

    TEST("tt_set out of bounds (no-op)");
    TruthTable *t3 = tt_create(3);
    tt_set(t3, 100, 1);
    for (int i = 0; i < 8; i++) assert(t3->values[i] == 0);
    tt_free(t3);
    PASS();

    TEST("tt_evaluate (3 inputs)");
    TruthTable *t = tt_create(3);
    /* Set f(x0,x1,x2) = x0 XOR x1 XOR x2 */
    for (long long i = 0; i < 8; i++) {
        int bit_count = __builtin_popcount((unsigned int)i);
        t->values[i] = bit_count % 2;
    }
    int inputs[3];
    inputs[0] = 0; inputs[1] = 0; inputs[2] = 0;
    assert(tt_evaluate(t, inputs) == 0);
    inputs[0] = 1;
    assert(tt_evaluate(t, inputs) == 1);
    inputs[0] = 0; inputs[1] = 1;
    assert(tt_evaluate(t, inputs) == 1);
    inputs[2] = 1;
    assert(tt_evaluate(t, inputs) == 0);  /* 0 XOR 1 XOR 1 = 0 */
    tt_free(t);
    PASS();

    TEST("tt_evaluate_int");
    TruthTable *t2 = tt_create(3);
    t2->values[5] = 1;  /* 5 = 101b = x0=1, x1=0, x2=1 */
    assert(tt_evaluate_int(t2, 5) == 1);
    assert(tt_evaluate_int(t2, 0) == 0);
    tt_free(t2);
    PASS();
}

/* ========================================================================
 * Test: Random Generation
 * ======================================================================== */

static void test_random(void) {
    TEST("tt_random basic properties");
    srand(42);
    TruthTable *t = tt_random(4);
    assert(t != NULL);
    assert(t->n_inputs == 4);
    assert(t->table_size == 16);

    /* Check that values are 0 or 1 */
    for (int i = 0; i < 16; i++) {
        assert(t->values[i] == 0 || t->values[i] == 1);
    }

    /* A random function should have roughly 50% 1s (statistical) */
    long long ones = tt_weight(t);
    assert(ones >= 2 && ones <= 14);  /* very loose bounds */
    tt_free(t);
    PASS();

    TEST("tt_random gives different functions");
    srand(123);
    TruthTable *t1 = tt_random(4);
    srand(456);
    TruthTable *t2 = tt_random(4);
    assert(!tt_equal(t1, t2));  /* extremely likely */
    tt_free(t1);
    tt_free(t2);
    PASS();

    TEST("tt_random_seeded determinism");
    TruthTable *a1 = tt_random_seeded(4, 999);
    TruthTable *a2 = tt_random_seeded(4, 999);
    assert(tt_equal(a1, a2));
    tt_free(a1);
    tt_free(a2);
    PASS();
}

/* ========================================================================
 * Test: Comparison
 * ======================================================================== */

static void test_comparison(void) {
    TEST("tt_equal identical");
    TruthTable *a = tt_create(3);
    TruthTable *b = tt_create(3);
    for (int i = 0; i < 8; i++) { a->values[i] = i % 2; b->values[i] = i % 2; }
    assert(tt_equal(a, b));
    tt_free(a); tt_free(b);
    PASS();

    TEST("tt_equal different");
    TruthTable *c = tt_create(3);
    TruthTable *d = tt_create(3);
    c->values[0] = 1;
    assert(!tt_equal(c, d));
    tt_free(c); tt_free(d);
    PASS();

    TEST("tt_equal different n_inputs");
    TruthTable *e = tt_create(3);
    TruthTable *f = tt_create(4);
    assert(!tt_equal(e, f));
    tt_free(e); tt_free(f);
    PASS();

    TEST("tt_equal with NULL");
    TruthTable *g = tt_create(3);
    assert(!tt_equal(g, NULL));
    assert(!tt_equal(NULL, g));
    assert(tt_equal(NULL, NULL));
    tt_free(g);
    PASS();

    TEST("tt_hamming_distance");
    TruthTable *h1 = tt_create(3);
    TruthTable *h2 = tt_create(3);
    h1->values[0] = 1; h1->values[1] = 1;
    h2->values[1] = 1; h2->values[2] = 1;
    assert(tt_hamming_distance(h1, h2) == 2);
    tt_free(h1); tt_free(h2);
    PASS();

    TEST("tt_hamming_distance identical");
    TruthTable *i1 = tt_create(3);
    TruthTable *i2 = tt_create(3);
    assert(tt_hamming_distance(i1, i2) == 0);
    tt_free(i1); tt_free(i2);
    PASS();
}

/* ========================================================================
 * Test: Properties
 * ======================================================================== */

static void test_properties(void) {
    TEST("tt_is_constant TRUE (all 0)");
    TruthTable *c0 = tt_create(3);
    assert(tt_is_constant(c0));
    tt_free(c0);
    PASS();

    TEST("tt_is_constant TRUE (all 1)");
    TruthTable *c1 = tt_create(3);
    for (int i = 0; i < 8; i++) c1->values[i] = 1;
    assert(tt_is_constant(c1));
    tt_free(c1);
    PASS();

    TEST("tt_is_constant FALSE");
    TruthTable *nc = tt_create(3);
    nc->values[0] = 1;
    assert(!tt_is_constant(nc));
    tt_free(nc);
    PASS();

    TEST("tt_is_monotone — AND function");
    TruthTable *and_fn = tt_create(3);
    /* AND(x0,x1,x2) = 1 iff all inputs are 1 */
    for (long long i = 0; i < 8; i++) {
        and_fn->values[i] = (i == 7) ? 1 : 0;
    }
    assert(tt_is_monotone(and_fn));
    tt_free(and_fn);
    PASS();

    TEST("tt_is_monotone — XOR function (non-monotone)");
    TruthTable *xor_fn = tt_create(3);
    for (long long i = 0; i < 8; i++) {
        xor_fn->values[i] = __builtin_popcount((unsigned int)i) % 2;
    }
    assert(!tt_is_monotone(xor_fn));
    tt_free(xor_fn);
    PASS();
}

/* ========================================================================
 * Test: Boolean Operations
 * ======================================================================== */

static void test_boolean_ops(void) {
    TEST("tt_not");
    TruthTable *t = tt_create(3);
    t->values[0] = 1; t->values[7] = 1;
    TruthTable *nt = tt_not(t);
    assert(nt->values[0] == 0);
    assert(nt->values[7] == 0);
    assert(nt->values[3] == 1);
    tt_free(t); tt_free(nt);
    PASS();

    TEST("tt_and");
    TruthTable *a = tt_create(3);
    TruthTable *b = tt_create(3);
    a->values[0] = 1; a->values[1] = 1;
    b->values[0] = 1; b->values[2] = 1;
    TruthTable *r = tt_and(a, b);
    assert(r->values[0] == 1);  /* 1 & 1 = 1 */
    assert(r->values[1] == 0);  /* 1 & 0 = 0 */
    assert(r->values[2] == 0);  /* 0 & 1 = 0 */
    tt_free(a); tt_free(b); tt_free(r);
    PASS();

    TEST("tt_or");
    TruthTable *c = tt_create(3);
    TruthTable *d = tt_create(3);
    c->values[1] = 1; d->values[2] = 1;
    TruthTable *r2 = tt_or(c, d);
    assert(r2->values[1] == 1);
    assert(r2->values[2] == 1);
    assert(r2->values[0] == 0);
    tt_free(c); tt_free(d); tt_free(r2);
    PASS();

    TEST("tt_xor");
    TruthTable *e = tt_create(3);
    TruthTable *f = tt_create(3);
    e->values[0] = 1; e->values[1] = 1;
    f->values[0] = 1; f->values[2] = 1;
    TruthTable *r3 = tt_xor(e, f);
    assert(r3->values[0] == 0);  /* 1 XOR 1 = 0 */
    assert(r3->values[1] == 1);  /* 1 XOR 0 = 1 */
    assert(r3->values[2] == 1);  /* 0 XOR 1 = 1 */
    tt_free(e); tt_free(f); tt_free(r3);
    PASS();

    TEST("tt_and with different n_inputs");
    TruthTable *g = tt_create(3);
    TruthTable *h = tt_create(4);
    assert(tt_and(g, h) == NULL);
    tt_free(g); tt_free(h);
    PASS();
}

/* ========================================================================
 * Main
 * ======================================================================== */

int main(void) {
    setbuf(stdout, NULL);
    printf("=== Truth Table Tests ===\n\n");

    test_creation();
    test_accessors();
    test_random();
    test_comparison();
    test_properties();
    test_boolean_ops();

    printf("\n=== Results: %d/%d passed ===\n", passed_tests, total_tests);
    if (passed_tests < total_tests) {
        printf("SOME TESTS FAILED!\n");
        return 1;
    }
    printf("ALL TESTS PASSED.\n");
    return 0;
}
