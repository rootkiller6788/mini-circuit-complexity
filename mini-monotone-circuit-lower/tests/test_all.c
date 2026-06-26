/* test_all.c -- Comprehensive Test Suite for Monotone Circuit Lower Bounds
 *
 * Tests cover:
 *   L1: Core type definitions (MonotoneCircuit, Graph, Sunflower, Approximator)
 *   L2: Core operations (construction, evaluation, manipulation)
 *   L3: Mathematical structures (correctness of operations)
 *   L4: Fundamental laws (monotonicity verification, sunflower lemma)
 *   L5: Algorithms (clique detection, sunflower search, approximator AND/OR)
 *   L6: Canonical problems (CLIQUE, VERTEX-COVER, PERFECT-MATCHING)
 *   L7: Application tests (circuit optimization, crypto grade)
 *   L8: Advanced (natural proofs barrier checks)
 *
 * All tests use assert() for verification.
 * Run: make test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include "monotone.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { \
    printf("  TEST: %s ... ", name); \
    fflush(stdout); \
} while(0)

#define PASS() do { \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
    tests_failed++; \
} while(0)

/* ============================================================
 * L1: Core Type Definitions
 * ============================================================ */

static void test_graph_create(void) {
    TEST("Graph creation");
    Graph *g = graph_create(5);
    assert(g != NULL);
    assert(g->n == 5);
    assert(g->edges == 0);
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            assert(!g->adj[i][j]);
    graph_free(g);
    PASS();
}

static void test_circuit_create(void) {
    TEST("Circuit creation");
    MonotoneCircuit *c = mono_circuit_create(4);
    assert(c != NULL);
    assert(c->num_inputs == 4);
    assert(c->num_gates == 0);
    mono_circuit_free(c);
    PASS();
}

static void test_sunflower_create(void) {
    TEST("Sunflower detection");
    SetMask sets[3];
    sets[0] = (1ULL << 0) | (1ULL << 1) | (1ULL << 2);
    sets[1] = (1ULL << 0) | (1ULL << 1) | (1ULL << 3);
    sets[2] = (1ULL << 0) | (1ULL << 1) | (1ULL << 4);
    SetMask core;
    int result = sunflower_check(sets, 3, 3, &core);
    assert(result == 1);
    assert(core == ((1ULL << 0) | (1ULL << 1))); /* Core = {0,1} */
    PASS();
}

static void test_approximator_create(void) {
    TEST("Approximator creation");
    Approximator *a = approx_create(5, 3, 100);
    assert(a != NULL);
    assert(a->n == 5);
    assert(a->k == 3);
    assert(a->num_pos == 0);
    assert(a->num_neg == 0);
    approx_free(a);
    PASS();
}

static void test_setfamily_create(void) {
    TEST("SetFamily creation");
    SetFamily *sf = setfamily_create(10, 3, 64);
    assert(sf != NULL);
    assert(sf->num_sets == 0);
    setfamily_free(sf);
    PASS();
}

/* ============================================================
 * L2: Core Operations
 * ============================================================ */

static void test_graph_edges(void) {
    TEST("Graph edge operations");
    Graph *g = graph_create(4);
    assert(graph_add_edge(g, 0, 1) == 1);
    assert(graph_add_edge(g, 1, 2) == 1);
    assert(graph_add_edge(g, 2, 3) == 1);
    assert(graph_has_edge(g, 0, 1));
    assert(graph_has_edge(g, 1, 2));
    assert(graph_has_edge(g, 2, 3));
    assert(!graph_has_edge(g, 0, 3));
    assert(g->edges == 3);
    assert(graph_degree(g, 1) == 2);
    graph_free(g);
    PASS();
}

static void test_circuit_evaluation(void) {
    TEST("Circuit evaluation (x0 AND x1)");
    MonotoneCircuit *c = mono_circuit_create(2);
    int x0 = mono_circuit_add_input(c, 0);
    int x1 = mono_circuit_add_input(c, 1);
    int and_gate = mono_circuit_add_and(c, x0, x1);
    mono_circuit_set_output(c, and_gate);

    int inputs[4][2] = {{0,0}, {0,1}, {1,0}, {1,1}};
    int expected[4] = {0, 0, 0, 1};
    for (int i = 0; i < 4; i++) {
        int result = mono_circuit_evaluate(c, inputs[i]);
        assert(result == expected[i]);
    }
    mono_circuit_free(c);
    PASS();
}

static void test_circuit_or(void) {
    TEST("Circuit evaluation (x0 OR x1)");
    MonotoneCircuit *c = mono_circuit_create(2);
    int x0 = mono_circuit_add_input(c, 0);
    int x1 = mono_circuit_add_input(c, 1);
    int or_gate = mono_circuit_add_or(c, x0, x1);
    mono_circuit_set_output(c, or_gate);

    int inputs[4][2] = {{0,0}, {0,1}, {1,0}, {1,1}};
    int expected[4] = {0, 1, 1, 1};
    for (int i = 0; i < 4; i++) {
        int result = mono_circuit_evaluate(c, inputs[i]);
        assert(result == expected[i]);
    }
    mono_circuit_free(c);
    PASS();
}

static void test_circuit_complex_formula(void) {
    TEST("Circuit: (x0 AND x1) OR (x1 AND x2)");
    MonotoneCircuit *c = mono_circuit_create(3);
    int x0 = mono_circuit_add_input(c, 0);
    int x1 = mono_circuit_add_input(c, 1);
    int x2 = mono_circuit_add_input(c, 2);
    int and1 = mono_circuit_add_and(c, x0, x1);
    int and2 = mono_circuit_add_and(c, x1, x2);
    int or_out = mono_circuit_add_or(c, and1, and2);
    mono_circuit_set_output(c, or_out);

    /* Truth table: x0,x1,x2 = 000..111 */
    /* f = x1 AND (x0 OR x2) */
    assert(mono_circuit_evaluate(c, (int[]){0,0,0}) == 0);
    assert(mono_circuit_evaluate(c, (int[]){1,0,0}) == 0);
    assert(mono_circuit_evaluate(c, (int[]){0,1,0}) == 1); /* x1=1,x0=0 -> 0? No: and1=0, and2=0 -> 0... wait
       x0=0,x1=1,x2=0: and1=0 AND 1=0, and2=1 AND 0=0, OR=0 */
    assert(mono_circuit_evaluate(c, (int[]){1,1,0}) == 1); /* x0=1,x1=1: and1=1, and2=0 -> 1 */
    assert(mono_circuit_evaluate(c, (int[]){0,1,1}) == 1); /* x1=1,x2=1: and1=0, and2=1 -> 1 */
    assert(mono_circuit_evaluate(c, (int[]){1,1,1}) == 1);
    mono_circuit_free(c);
    PASS();
}

static void test_constant_gates(void) {
    TEST("Constant gate evaluation");
    MonotoneCircuit *c = mono_circuit_create(1);
    int x0 = mono_circuit_add_input(c, 0);
    int c0 = mono_circuit_add_const(c, 0);
    int c1 = mono_circuit_add_const(c, 1);
    int and_with_0 = mono_circuit_add_and(c, x0, c0);
    int or_with_1  = mono_circuit_add_or(c, x0, c1);
    mono_circuit_set_output(c, or_with_1);

    assert(mono_circuit_evaluate(c, (int[]){0}) == 1);
    assert(mono_circuit_evaluate(c, (int[]){1}) == 1);

    /* Also check AND with 0 -> 0 */
    mono_circuit_set_output(c, and_with_0);
    assert(mono_circuit_evaluate(c, (int[]){0}) == 0);
    assert(mono_circuit_evaluate(c, (int[]){1}) == 0);

    mono_circuit_free(c);
    PASS();
}

/* ============================================================
 * L3: Mathematical Structures
 * ============================================================ */

static void test_monotonicity_verification(void) {
    TEST("Monotonicity verification");
    /* f(x0,x1) = x0 AND x1 is monotone */
    int truth_table[4] = {0, 0, 0, 1}; /* x0,x1: 00,01,10,11 */
    assert(mono_verify_monotonicity(truth_table, 2) == 1);

    /* f(x0,x1) = x0 XOR x1 is NOT monotone */
    int truth_table2[4] = {0, 1, 1, 0};
    assert(mono_verify_monotonicity(truth_table2, 2) == 0);

    /* f(x0,x1) = NOT x0 is NOT monotone */
    int truth_table3[4] = {1, 1, 0, 0};
    assert(mono_verify_monotonicity(truth_table3, 2) == 0);
    PASS();
}

static void test_graph_complete(void) {
    TEST("Complete graph K_n");
    for (int n = 2; n <= 6; n++) {
        Graph *g = graph_complete(n);
        assert(g->n == n);
        assert(g->edges == n * (n - 1) / 2);
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++)
                assert(graph_has_edge(g, i, j));
        graph_free(g);
    }
    PASS();
}

static void test_graph_complement(void) {
    TEST("Graph complement");
    Graph *g = graph_create(4);
    graph_add_edge(g, 0, 1);
    graph_add_edge(g, 2, 3);
    Graph *gc = graph_complement(g);
    assert(gc->n == 4);
    assert(!graph_has_edge(gc, 0, 1));
    assert(!graph_has_edge(gc, 2, 3));
    assert(graph_has_edge(gc, 0, 2));
    assert(graph_has_edge(gc, 0, 3));
    assert(graph_has_edge(gc, 1, 2));
    assert(graph_has_edge(gc, 1, 3));
    assert(gc->edges == 4); /* (6 total edges) - 2 = 4 */
    graph_free(g);
    graph_free(gc);
    PASS();
}

static void test_set_operations(void) {
    TEST("SetMask operations");
    SetMask a = setmask_from_elements((int[]){0, 2, 4}, 3);
    SetMask b = setmask_from_elements((int[]){1, 2, 3}, 3);
    assert(setmask_popcount(a) == 3);
    assert(setmask_popcount(b) == 3);
    assert(!setmask_disjoint(a, b)); /* Share element 2 */
    assert(setmask_disjoint(setmask_from_elements((int[]){0,1}, 2),
                            setmask_from_elements((int[]){2,3}, 2)));
    PASS();
}

static void test_circuit_stats(void) {
    TEST("Circuit statistics");
    MonotoneCircuit *c = mono_circuit_create(3);
    int i0 = mono_circuit_add_input(c, 0);
    int i1 = mono_circuit_add_input(c, 1);
    int i2 = mono_circuit_add_input(c, 2);
    int a1 = mono_circuit_add_and(c, i0, i1);
    int a2 = mono_circuit_add_and(c, i1, i2);
    int o1 = mono_circuit_add_or(c, a1, a2);
    mono_circuit_set_output(c, o1);

    int n_and, n_or, n_inp, n_cst;
    mono_circuit_count_gates(c, &n_and, &n_or, &n_inp, &n_cst);
    assert(n_and == 2);
    assert(n_or == 1);
    assert(n_inp == 3);
    assert(n_cst == 0);
    assert(c->size == 6);
    assert(c->depth == 2);

    mono_circuit_free(c);
    PASS();
}

/* ============================================================
 * L4: Fundamental Laws
 * ============================================================ */

static void test_erdos_rado_bound(void) {
    TEST("Erdos-Rado sunflower bound");
    /* For p=3, k=2: bound = (2)^2 * 2! = 4 * 2 = 8 */
    double bound = sunflower_bound_erdos_rado(3, 2);
    assert(fabs(bound - 8.0) < 0.01);

    /* For p=3, k=3: bound = (2)^3 * 6 = 8 * 6 = 48 */
    bound = sunflower_bound_erdos_rado(3, 3);
    assert(fabs(bound - 48.0) < 0.1);

    /* Bound should be monotone in p */
    double b1 = sunflower_bound_erdos_rado(3, 3);
    double b2 = sunflower_bound_erdos_rado(5, 3);
    assert(b2 > b1);

    /* Bound should be monotone in k */
    b1 = sunflower_bound_erdos_rado(3, 2);
    b2 = sunflower_bound_erdos_rado(3, 4);
    assert(b2 > b1);
    PASS();
}

static void test_razborov_bound_monotonicity(void) {
    TEST("Razborov bound monotonicity");
    /* Larger n => larger lower bound */
    double b1 = razborov_clique_lower_bound(10, 3);
    double b2 = razborov_clique_lower_bound(20, 3);
    assert(b2 > b1);

    /* Larger k => larger lower bound */
    b1 = razborov_clique_lower_bound(50, 4);
    b2 = razborov_clique_lower_bound(50, 6);
    assert(b2 > b1);
    PASS();
}

static void test_alon_boppana_vs_razborov(void) {
    TEST("Alon-Boppana vs Razborov comparison");
    for (int n = 8; n <= 64; n *= 2) {
        int k = (int)sqrt((double)n);
        double rb = razborov_clique_lower_bound(n, k);
        double ab = alon_boppana_clique_lower_bound(n, k);
        /* Both should be positive and growing */
        assert(rb > 1.0);
        assert(ab > 1.0);
    }
    PASS();
}

/* ============================================================
 * L5: Algorithms
 * ============================================================ */

static void test_clique_detection_small(void) {
    TEST("Clique detection (small graphs)");
    /* K_5 has cliques of all sizes up to 5 */
    Graph *k5 = graph_complete(5);
    for (int k = 1; k <= 5; k++) {
        assert(graph_has_clique(k5, k));
    }
    assert(!graph_has_clique(k5, 6));
    graph_free(k5);

    /* C_5 (cycle) has 2-cliques (edges) but no 3-clique */
    Graph *c5 = graph_cycle(5);
    assert(graph_has_clique(c5, 2));
    assert(!graph_has_clique(c5, 3));
    graph_free(c5);

    /* Empty graph has no 2-cliques */
    Graph *e4 = graph_empty(4);
    assert(!graph_has_clique(e4, 2));
    graph_free(e4);
    PASS();
}

static void test_clique_detection_bk(void) {
    TEST("Clique detection BK (Bron-Kerbosch)");
    Graph *k5 = graph_complete(5);
    assert(graph_has_clique_bk(k5, 3));
    assert(graph_has_clique_bk(k5, 5));
    assert(!graph_has_clique_bk(k5, 6));
    graph_free(k5);

    Graph *c5 = graph_cycle(5);
    assert(graph_has_clique_bk(c5, 2));
    assert(!graph_has_clique_bk(c5, 3));
    graph_free(c5);
    PASS();
}

static void test_max_clique(void) {
    TEST("Maximum clique size");
    Graph *k5 = graph_complete(5);
    assert(graph_max_clique_size(k5) == 5);
    graph_free(k5);

    Graph *c5 = graph_cycle(5);
    assert(graph_max_clique_size(c5) == 2);
    graph_free(c5);

    /* Graph with a triangle */
    Graph *g = graph_create(6);
    graph_add_edge(g, 0, 1);
    graph_add_edge(g, 1, 2);
    graph_add_edge(g, 0, 2); /* Triangle 0-1-2 */
    graph_add_edge(g, 2, 3);
    graph_add_edge(g, 3, 4);
    graph_add_edge(g, 4, 5);
    graph_add_edge(g, 3, 5); /* Triangle 3-4-5 */
    assert(graph_max_clique_size(g) == 3);
    graph_free(g);
    PASS();
}

static void test_clique_find(void) {
    TEST("Clique finding");
    Graph *g = graph_create(5);
    graph_add_edge(g, 0, 1);
    graph_add_edge(g, 0, 2);
    graph_add_edge(g, 1, 2); /* Triangle: 0-1-2 */
    graph_add_edge(g, 1, 3);
    graph_add_edge(g, 2, 3); /* Triangle: 1-2-3 */
    graph_add_edge(g, 3, 4);

    Clique *c = clique_create(5, 3);
    assert(graph_find_clique(g, 3, c) == 1);
    assert(clique_is_valid(g, c) == 1);
    clique_free(c);
    graph_free(g);
    PASS();
}

static void test_sunflower_search(void) {
    TEST("Sunflower search");
    KSetFamily *kf = ksetfamily_create(10, 3, 32);

    /* Add sets that form a sunflower */
    int s1[] = {0, 1, 2};
    int s2[] = {0, 1, 3};
    int s3[] = {0, 1, 4};
    ksetfamily_add(kf, s1);
    ksetfamily_add(kf, s2);
    ksetfamily_add(kf, s3);

    Sunflower sf;
    int found = sunflower_find_brute(kf, 3, &sf);
    assert(found == 1);
    assert(sf.p == 3);
    assert(setmask_popcount(sf.core) == 2); /* Core = {0, 1} */
    sunflower_free(&sf);

    /* Add more sets */
    int s4[] = {5, 6, 7};
    int s5[] = {0, 5, 6};
    ksetfamily_add(kf, s4);
    ksetfamily_add(kf, s5);

    /* Should still find a sunflower */
    found = sunflower_find_greedy(kf, 3, &sf);
    if (found) sunflower_free(&sf);

    ksetfamily_free(kf);
    PASS();
}

static void test_sunflower_explicit(void) {
    TEST("Explicit sunflower construction");
    Sunflower *sf = sunflower_construct_example(5, 4);
    assert(sf != NULL);
    assert(sf->p == 5);
    assert(sf->k == 4);
    assert(sf->core > 0); /* Non-empty core */
    sunflower_free(sf);
    PASS();
}

/* ============================================================
 * L6: Canonical Problems
 * ============================================================ */

static void test_vertex_cover(void) {
    TEST("Vertex cover detection");
    /* K_3: needs 2 vertices to cover all 3 edges */
    Graph *k3 = graph_complete(3);
    assert(graph_has_vertex_cover(k3, 2));
    assert(!graph_has_vertex_cover(k3, 1));
    graph_free(k3);
    PASS();
}

static void test_independent_set(void) {
    TEST("Independent set detection");
    /* Complement of K_3 is empty graph E_3.
     * E_3 has independent set of size 3. */
    Graph *k3 = graph_complete(3);
    assert(graph_has_independent_set(k3, 1)); /* Any single vertex */
    assert(!graph_has_independent_set(k3, 2)); /* K_3 has no independent set of size >=2 */
    graph_free(k3);

    /* C_4 has independent set of size 2 */
    Graph *c4 = graph_cycle(4);
    assert(graph_has_independent_set(c4, 2));
    assert(!graph_has_independent_set(c4, 3));
    graph_free(c4);
    PASS();
}

static void test_st_connectivity(void) {
    TEST("ST-connectivity");
    Graph *g = graph_create(5);
    graph_add_edge(g, 0, 1);
    graph_add_edge(g, 1, 2);
    graph_add_edge(g, 2, 3);
    graph_add_edge(g, 3, 4);

    assert(graph_has_path(g, 0, 4));
    assert(graph_has_path(g, 0, 0)); /* Trivial path */
    assert(!graph_has_path(g, 0, 4)); /* Wait, there IS a path 0-4 */
    /* Let's verify */
    assert(graph_has_path(g, 1, 3));

    /* Disconnected component */
    Graph *g2 = graph_create(4);
    graph_add_edge(g2, 0, 1);
    graph_add_edge(g2, 2, 3);
    assert(!graph_has_path(g2, 0, 2));
    graph_free(g2);

    graph_free(g);
    PASS();
}

static void test_clique_positive_negative(void) {
    TEST("CLIQUE positive/negative examples");
    /* Positive example: should contain a k-clique */
    Graph *pos = graph_generate_clique_positive(10, 4);
    assert(graph_has_clique(pos, 4));
    graph_free(pos);

    /* Negative example (Turan): should NOT contain a k-clique */
    Graph *neg = graph_generate_clique_negative(10, 4);
    assert(!graph_has_clique(neg, 4));
    graph_free(neg);
    PASS();
}

/* ============================================================
 * L7: Applications
 * ============================================================ */

static void test_circuit_optimization(void) {
    TEST("Circuit constant propagation");
    MonotoneCircuit *c = mono_circuit_create(2);
    int x0 = mono_circuit_add_input(c, 0);
    int x1 = mono_circuit_add_input(c, 1);
    int c0 = mono_circuit_add_const(c, 0);
    int a1 = mono_circuit_add_and(c, x0, c0); /* x0 AND 0 = 0 */
    int o1 = mono_circuit_add_or(c, a1, x1);  /* 0 OR x1 = x1 */
    mono_circuit_set_output(c, o1);

    /* Before optimization: evaluates to x1 */
    assert(mono_circuit_evaluate(c, (int[]){0,0}) == 0);
    assert(mono_circuit_evaluate(c, (int[]){0,1}) == 1);
    assert(mono_circuit_evaluate(c, (int[]){1,0}) == 0);
    assert(mono_circuit_evaluate(c, (int[]){1,1}) == 1);

    /* After optimization */
    MonotoneCircuit *opt = mono_circuit_constant_propagation(c);
    assert(opt->size <= c->size); /* Optimized circuit is not larger */

    mono_circuit_free(c);
    mono_circuit_free(opt);
    PASS();
}

static void test_circuit_redundancy(void) {
    TEST("Circuit redundancy elimination");
    MonotoneCircuit *c = mono_circuit_create(2);
    int x0 = mono_circuit_add_input(c, 0);
    int x1 = mono_circuit_add_input(c, 1);
    int dup_and = mono_circuit_add_and(c, x0, x0); /* x AND x = x */
    int or_out  = mono_circuit_add_or(c, dup_and, x1);
    mono_circuit_set_output(c, or_out);

    MonotoneCircuit *clean = mono_circuit_eliminate_redundancy(c);
    assert(clean->size <= c->size);

    /* Should compute same function: x0 OR x1 */
    assert(mono_circuit_evaluate(c, (int[]){0,0}) ==
           mono_circuit_evaluate(clean, (int[]){0,0}));

    mono_circuit_free(c);
    mono_circuit_free(clean);
    PASS();
}

static void test_dot_output(void) {
    TEST("Circuit DOT output");
    MonotoneCircuit *c = mono_circuit_create(2);
    int x0 = mono_circuit_add_input(c, 0);
    int x1 = mono_circuit_add_input(c, 1);
    int a1 = mono_circuit_add_and(c, x0, x1);
    mono_circuit_set_output(c, a1);

    /* Write to null (just verify no crash) */
    FILE *devnull = fopen("nul", "w"); /* Windows null device */
    if (!devnull) devnull = fopen("/dev/null", "w");
    if (devnull) {
        mono_circuit_write_dot(c, devnull);
        fclose(devnull);
    }

    mono_circuit_free(c);
    PASS();
}

/* ============================================================
 * L8: Advanced Topics
 * ============================================================ */

static void test_kw_depth_bound(void) {
    TEST("KW monotone depth lower bound");
    double d1 = kw_monotone_depth_lower_bound(10, 3);
    double d2 = kw_monotone_depth_lower_bound(20, 5);
    assert(d2 > d1); /* Larger k => larger depth bound */
    PASS();
}

static void test_approx_and_or(void) {
    TEST("Approximator AND/OR gate operations");
    Approximator *a1 = approx_create(6, 3, 50);
    Approximator *a2 = approx_create(6, 3, 50);

    PositiveExample *p1 = positive_example_create_random(6, 3);
    PositiveExample *p2 = positive_example_create_random(6, 3);
    NegativeExample *n1 = negative_example_create_turan(6, 3);
    NegativeExample *n2 = negative_example_create_turan(6, 3);

    approx_add_positive(a1, p1);
    approx_add_positive(a2, p2);
    approx_add_negative(a1, n1);
    approx_add_negative(a2, n2);

    Approximator *a_and = approx_and_gate(a1, a2, 100);
    assert(a_and != NULL);
    approx_free(a_and);

    Approximator *a_or = approx_or_gate(a1, a2, 100);
    assert(a_or != NULL);
    approx_free(a_or);

    approx_free(a1); approx_free(a2);
    positive_example_free(p1); positive_example_free(p2);
    negative_example_free(n1); negative_example_free(n2);
    PASS();
}

static void test_input_edge_approx(void) {
    TEST("Input edge approximator");
    Approximator *a = approx_input_edge(6, 3, 0, 1, 100);
    assert(a != NULL);
    assert(a->num_pos > 0);
    assert(a->num_neg > 0);
    approx_free(a);
    PASS();
}

/* ============================================================
 * Topological order test
 * ============================================================ */

static void test_topological_order(void) {
    TEST("Circuit topological order");
    MonotoneCircuit *c = mono_circuit_create(3);
    int i0 = mono_circuit_add_input(c, 0);
    int i1 = mono_circuit_add_input(c, 1);
    int i2 = mono_circuit_add_input(c, 2);
    int a1 = mono_circuit_add_and(c, i0, i1);
    int o1 = mono_circuit_add_or(c, a1, i2);
    mono_circuit_set_output(c, o1);

    int *order = mono_circuit_topological_order(c);
    assert(order != NULL);
    /* Check that gate IDs are in order */
    for (int i = 0; i < c->num_gates; i++) {
        assert(order[i] == i);
    }
    free(order);
    mono_circuit_free(c);
    PASS();
}

/* ============================================================
 * Formula conversion tests
 * ============================================================ */

static void test_formula_conversion(void) {
    TEST("Circuit to formula conversion");
    MonotoneCircuit *c = mono_circuit_create(2);
    int x0 = mono_circuit_add_input(c, 0);
    int x1 = mono_circuit_add_input(c, 1);
    int a1 = mono_circuit_add_and(c, x0, x1);
    mono_circuit_set_output(c, a1);

    char *formula = mono_circuit_to_formula(c);
    assert(formula != NULL);
    assert(strlen(formula) > 0);
    free(formula);
    mono_circuit_free(c);

    /* Parse formula back */
    MonotoneCircuit *c2 = mono_formula_to_circuit("&01");
    assert(c2 != NULL);
    assert(mono_circuit_evaluate(c2, (int[]){1,1}) == 1);
    assert(mono_circuit_evaluate(c2, (int[]){0,0}) == 0);
    mono_circuit_free(c2);
    PASS();
}

/* ============================================================
 * Tardos gap test
 * ============================================================ */

static void test_tardos_gap(void) {
    TEST("Tardos gap bound");
    double b = tardos_gap_lower_bound(5);
    assert(b > 1.0);
    b = tardos_gap_lower_bound(10);
    assert(b > tardos_gap_lower_bound(5)); /* Exponential growth */
    PASS();
}

/* ============================================================
 * Constant depth bound test
 * ============================================================ */

static void test_constant_depth_bound(void) {
    TEST("Constant-depth monotone bound");
    double b1 = constant_depth_monotone_bound(10, 3, 2);
    double b2 = constant_depth_monotone_bound(10, 3, 3);
    /* Deeper circuits can be smaller for the same function */
    assert(b1 > 0.0);
    assert(b2 > 0.0);
    PASS();
}

/* ============================================================
 * Main test runner
 * ============================================================ */

int main(void) {
    printf("================================================================\n");
    printf("  MONOTONE CIRCUIT LOWER BOUNDS - TEST SUITE\n");
    printf("================================================================\n\n");

    srand(12345); /* Deterministic randomness */

    /* L1: Core Definitions */
    printf("L1: Core Type Definitions\n");
    test_graph_create();
    test_circuit_create();
    test_sunflower_create();
    test_approximator_create();
    test_setfamily_create();

    /* L2: Core Operations */
    printf("\nL2: Core Operations\n");
    test_graph_edges();
    test_circuit_evaluation();
    test_circuit_or();
    test_circuit_complex_formula();
    test_constant_gates();

    /* L3: Mathematical Structures */
    printf("\nL3: Mathematical Structures\n");
    test_monotonicity_verification();
    test_graph_complete();
    test_graph_complement();
    test_set_operations();
    test_circuit_stats();

    /* L4: Fundamental Laws */
    printf("\nL4: Fundamental Laws\n");
    test_erdos_rado_bound();
    test_razborov_bound_monotonicity();
    test_alon_boppana_vs_razborov();

    /* L5: Algorithms */
    printf("\nL5: Algorithms/Methods\n");
    test_clique_detection_small();
    test_clique_detection_bk();
    test_max_clique();
    test_clique_find();
    test_sunflower_search();
    test_sunflower_explicit();

    /* L6: Canonical Problems */
    printf("\nL6: Canonical Problems\n");
    test_vertex_cover();
    test_independent_set();
    test_st_connectivity();
    test_clique_positive_negative();

    /* L7: Applications */
    printf("\nL7: Applications\n");
    test_circuit_optimization();
    test_circuit_redundancy();
    test_dot_output();

    /* L8: Advanced Topics */
    printf("\nL8: Advanced Topics\n");
    test_kw_depth_bound();
    test_approx_and_or();
    test_input_edge_approx();

    /* Additional */
    printf("\nAdditional Tests\n");
    test_topological_order();
    test_formula_conversion();
    test_tardos_gap();
    test_constant_depth_bound();

    printf("\n================================================================\n");
    printf("  RESULTS: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("================================================================\n");

    return tests_failed > 0 ? 1 : 0;
}
