/* test_all.c -- Comprehensive tests for NC Hierarchy module
 *
 * Tests cover:
 *   L1: Circuit construction and type definitions
 *   L2: Circuit evaluation, depth computation, classification
 *   L3: DAG property (no cycles), topological order
 *   L4: Hierarchy inclusions, Brent's Theorem
 *   L5: Parallel prefix, carry-lookahead, sorting, matrix ops
 *   L6: PARITY, MAJORITY, formula evaluation
 *
 * All tests use assert() for mathematical correctness verification.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "nc.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    printf("  TEST %-50s", name); \
} while(0)

#define PASS() do { \
    tests_passed++; \
    printf(" PASS\n"); \
} while(0)

#define FAIL(msg) do { \
    printf(" FAIL: %s\n", msg); \
} while(0)

/* ================================================================
 * L1: CIRCUIT CONSTRUCTION
 * ================================================================ */

static void test_circuit_create(void) {
    TEST("circuit_create/destroy");
    BoolCircuit *c = circuit_create(32, 2);
    assert(c != NULL);
    assert(c->num_gates == 0);
    assert(c->capacity >= 32);
    circuit_free(c);
    PASS();
}

static void test_circuit_add_gates(void) {
    TEST("circuit_add_input/gate/output");
    BoolCircuit *c = circuit_create(32, 2);
    int x0 = circuit_add_input(c, 0);
    int x1 = circuit_add_input(c, 1);
    int and_gate = circuit_add_gate(c, NC_AND, (int[]){x0, x1}, 2);
    circuit_add_output(c, and_gate);

    assert(c->num_gates == 3);
    assert(c->num_inputs == 2);
    assert(c->num_outputs == 1);
    assert(c->gates[x0].type == NC_INPUT);
    assert(c->gates[x0].input_bit == 0);
    assert(c->gates[and_gate].type == NC_AND);
    assert(c->gates[and_gate].num_inputs == 2);

    circuit_free(c);
    PASS();
}

static void test_circuit_constants(void) {
    TEST("circuit constants 0 and 1");
    BoolCircuit *c = circuit_create(16, 2);
    int c0 = circuit_add_constant(c, 0);
    int c1 = circuit_add_constant(c, 1);
    circuit_add_output(c, c0);
    circuit_add_output(c, c1);

    int *out = circuit_evaluate(c, NULL);
    assert(out[0] == 0);
    assert(out[1] == 1);
    free(out);
    circuit_free(c);
    PASS();
}

/* ================================================================
 * L2: CIRCUIT EVALUATION
 * ================================================================ */

static void test_circuit_eval_and(void) {
    TEST("circuit evaluation: AND");
    BoolCircuit *c = circuit_create(16, 2);
    int x0 = circuit_add_input(c, 0);
    int x1 = circuit_add_input(c, 1);
    int g = circuit_add_gate(c, NC_AND, (int[]){x0, x1}, 2);
    circuit_add_output(c, g);

    int in00[] = {0, 0}; int *out00 = circuit_evaluate(c, in00);
    assert(out00[0] == 0); free(out00);

    int in01[] = {0, 1}; int *out01 = circuit_evaluate(c, in01);
    assert(out01[0] == 0); free(out01);

    int in10[] = {1, 0}; int *out10 = circuit_evaluate(c, in10);
    assert(out10[0] == 0); free(out10);

    int in11[] = {1, 1}; int *out11 = circuit_evaluate(c, in11);
    assert(out11[0] == 1); free(out11);

    circuit_free(c);
    PASS();
}

static void test_circuit_eval_or(void) {
    TEST("circuit evaluation: OR");
    BoolCircuit *c = circuit_create(16, 2);
    int x0 = circuit_add_input(c, 0);
    int x1 = circuit_add_input(c, 1);
    int g = circuit_add_gate(c, NC_OR, (int[]){x0, x1}, 2);
    circuit_add_output(c, g);

    int in00[] = {0, 0}; assert(circuit_evaluate(c, in00)[0] == 0);
    int in01[] = {0, 1}; assert(circuit_evaluate(c, in01)[0] == 1);
    int in10[] = {1, 0}; assert(circuit_evaluate(c, in10)[0] == 1);
    int in11[] = {1, 1}; assert(circuit_evaluate(c, in11)[0] == 1);
    circuit_free(c);
    PASS();
}

static void test_circuit_eval_not(void) {
    TEST("circuit evaluation: NOT");
    BoolCircuit *c = circuit_create(16, 2);
    int x0 = circuit_add_input(c, 0);
    int g = circuit_add_gate(c, NC_NOT, (int[]){x0}, 1);
    circuit_add_output(c, g);

    int in0[] = {0}; assert(circuit_evaluate(c, in0)[0] == 1);
    int in1[] = {1}; assert(circuit_evaluate(c, in1)[0] == 0);
    circuit_free(c);
    PASS();
}

static void test_circuit_eval_xor(void) {
    TEST("circuit evaluation: XOR");
    BoolCircuit *c = circuit_create(16, 2);
    int x0 = circuit_add_input(c, 0);
    int x1 = circuit_add_input(c, 1);
    int g = circuit_add_gate(c, NC_XOR, (int[]){x0, x1}, 2);
    circuit_add_output(c, g);

    assert(circuit_evaluate(c, (int[]){0,0})[0] == 0);
    assert(circuit_evaluate(c, (int[]){0,1})[0] == 1);
    assert(circuit_evaluate(c, (int[]){1,0})[0] == 1);
    assert(circuit_evaluate(c, (int[]){1,1})[0] == 0);
    circuit_free(c);
    PASS();
}

static void test_circuit_eval_nand(void) {
    TEST("circuit evaluation: NAND (universal gate)");
    BoolCircuit *c = circuit_create(16, 2);
    int x0 = circuit_add_input(c, 0);
    int x1 = circuit_add_input(c, 1);
    int g = circuit_add_gate(c, NC_NAND, (int[]){x0, x1}, 2);
    circuit_add_output(c, g);

    assert(circuit_evaluate(c, (int[]){1,1})[0] == 0);
    assert(circuit_evaluate(c, (int[]){0,0})[0] == 1);
    assert(circuit_evaluate(c, (int[]){0,1})[0] == 1);
    circuit_free(c);
    PASS();
}

static void test_circuit_eval_majority(void) {
    TEST("circuit evaluation: MAJORITY");
    BoolCircuit *c = circuit_create(16, 16);
    int ins[5];
    for (int i = 0; i < 5; i++) ins[i] = circuit_add_input(c, i);
    int g = circuit_add_gate(c, NC_MAJORITY, ins, 5);
    circuit_add_output(c, g);

    /* 0 ones => 0 */
    assert(circuit_evaluate(c, (int[]){0,0,0,0,0})[0] == 0);
    /* 2 ones => 0 (need > 2 for majority of 5) */
    assert(circuit_evaluate(c, (int[]){1,1,0,0,0})[0] == 0);
    /* 3 ones => 1 */
    assert(circuit_evaluate(c, (int[]){1,1,1,0,0})[0] == 1);
    /* 5 ones => 1 */
    assert(circuit_evaluate(c, (int[]){1,1,1,1,1})[0] == 1);
    circuit_free(c);
    PASS();
}

/* ================================================================
 * L3: CIRCUIT DEPTH
 * ================================================================ */

static void test_circuit_depth(void) {
    TEST("circuit depth computation");
    BoolCircuit *c = circuit_create(16, 2);
    int x0 = circuit_add_input(c, 0);
    int x1 = circuit_add_input(c, 1);
    int g1 = circuit_add_gate(c, NC_AND, (int[]){x0, x1}, 2);
    int g2 = circuit_add_gate(c, NC_NOT, (int[]){g1}, 1);
    int g3 = circuit_add_gate(c, NC_OR, (int[]){g1, g2}, 2);
    circuit_add_output(c, g3);

    int d = circuit_depth(c);
    /* Path: x0/x1 (depth 0) -> g1 (depth 1) -> g2 (depth 2) -> g3 (depth 3) */
    assert(d == 3);
    circuit_free(c);
    PASS();
}

static void test_circuit_no_cycle(void) {
    TEST("circuit cycle detection: valid DAG");
    BoolCircuit *c = circuit_create(16, 2);
    int x0 = circuit_add_input(c, 0);
    int g1 = circuit_add_gate(c, NC_NOT, (int[]){x0}, 1);
    circuit_add_output(c, g1);

    assert(circuit_has_cycle(c) == 0);
    circuit_free(c);
    PASS();
}

static void test_circuit_is_tree(void) {
    TEST("circuit tree detection");
    BoolCircuit *c = circuit_create(16, 2);
    int x0 = circuit_add_input(c, 0);
    int x1 = circuit_add_input(c, 1);
    int g = circuit_add_gate(c, NC_AND, (int[]){x0, x1}, 2);
    circuit_add_output(c, g);

    assert(circuit_is_tree(c) == 1);  /* fan-out ≤ 1 for all */
    circuit_free(c);
    PASS();
}

static void test_circuit_is_monotone(void) {
    TEST("circuit monotonicity detection");
    BoolCircuit *c = circuit_create(16, 2);
    int x0 = circuit_add_input(c, 0);
    int x1 = circuit_add_input(c, 1);
    int g = circuit_add_gate(c, NC_AND, (int[]){x0, x1}, 2);
    circuit_add_output(c, g);

    assert(circuit_is_monotone(c) == 1);

    /* Add NOT gate */
    int g2 = circuit_add_gate(c, NC_NOT, (int[]){x0}, 1);
    assert(circuit_is_monotone(c) == 0);
    circuit_free(c);
    PASS();
}

/* ================================================================
 * L4: HIERARCHY THEOREMS
 * ================================================================ */

static void test_hierarchy_inclusions(void) {
    TEST("NC hierarchy inclusion theorems (L4)");
    /* Verify known inclusion relationships return true */
    assert(hierarchy_nc0_in_ac0() == 1);      /* NC0 in AC0 */
    assert(hierarchy_ac0_in_tc0() == 1);      /* AC0 in TC0 */
    assert(hierarchy_ac0_neq_tc0() == 1);     /* AC0 != TC0 */
    assert(hierarchy_nc1_in_l() == 1);        /* NC1 in L */
    assert(hierarchy_l_in_nl() == 1);          /* L in NL */
    assert(hierarchy_nl_equals_conl() == 1);   /* NL = coNL */
    assert(hierarchy_nl_in_nc2() == 1);        /* NL in NC2 */
    assert(hierarchy_nc_in_p() == 1);          /* NC in P */
    assert(hierarchy_p_in_pspace() == 1);      /* P in PSPACE */
    PASS();
}

static void test_brent_theorem(void) {
    TEST("Brent's Theorem (L5)");
    double Ts = 1000.0;
    double Ti = 20.0;
    int p = brent_optimal_processors(Ts, Ti);
    assert(p > 0);

    double Tp = brent_parallel_time(Ts, p, Ti);
    assert(Tp <= Ts);  /* parallel time ≤ sequential time */

    double sp = brent_speedup(Ts, Tp);
    assert(sp >= 1.0);  /* speedup ≥ 1 */

    double ef = brent_efficiency(Ts, Tp, p);
    assert(ef >= 0.0 && ef <= 1.0);
    PASS();
}

/* ================================================================
 * L5: PARALLEL PREFIX
 * ================================================================ */

static void test_prefix_correctness(void) {
    TEST("parallel prefix correctness");
    int n = 8;
    int x[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int p_seq[8], p_tree[8], p_ks[8], p_bk[8];

    prefix_sequential(x, n, p_seq, (BinOp)(int(*)(int,int))0);
    /* Use a known operation */
    int op_add_impl(int a, int b) { return a + b; }
    prefix_sequential(x, n, p_seq, op_add_impl);

    /* Check sequential prefix */
    assert(p_seq[0] == 1);
    assert(p_seq[1] == 3);
    assert(p_seq[2] == 6);
    assert(p_seq[3] == 10);

    /* Verify all methods produce same results */
    prefix_parallel_tree(x, n, p_tree, op_add_impl);
    prefix_kogge_stone(x, n, p_ks, op_add_impl);
    prefix_brent_kung(x, n, p_bk, op_add_impl);

    assert(verify_prefix(x, n, p_tree, op_add_impl) == 1);
    assert(verify_prefix(x, n, p_ks, op_add_impl) == 1);
    assert(verify_prefix(x, n, p_bk, op_add_impl) == 1);

    for (int i = 0; i < n; i++) {
        assert(p_seq[i] == p_tree[i]);
        assert(p_seq[i] == p_ks[i]);
        assert(p_seq[i] == p_bk[i]);
    }
    PASS();
}

/* ================================================================
 * L5: ARITHMETIC
 * ================================================================ */

static void test_carry_lookahead(void) {
    TEST("carry-lookahead addition");
    int a[4] = {1,0,1,0};  /* 0101 = 5 (little-endian) */
    int b[4] = {1,1,0,0};  /* 0011 = 3 */
    int sum_rip[4], sum_cla[4], sum_cs[4];
    int carry;

    ripple_carry_add_bits(a, b, 4, sum_rip, &carry);
    carry_lookahead_add_bits(a, b, 4, sum_cla);
    carry_select_add_bits(a, b, 4, sum_cs);

    /* 5 + 3 = 8 = 1000 binary = {0,0,0,1} little-endian */
    assert(sum_rip[0] == 0 && sum_rip[1] == 0 && sum_rip[2] == 0 && sum_rip[3] == 1);
    for (int i = 0; i < 4; i++) {
        assert(sum_rip[i] == sum_cla[i]);
        assert(sum_rip[i] == sum_cs[i]);
    }
    assert(carry == 0); /* 4 bits, no overflow beyond 4 */
    PASS();
}

static void test_carry_save_adder(void) {
    TEST("carry-save adder 3:2");
    int a[4] = {1,0,0,0};  /* 1 */
    int b[4] = {1,0,0,0};  /* 1 */
    int c[4] = {1,0,0,0};  /* 1 */
    int sum[5], carry[5];

    carry_save_adder_3to2(a, b, c, 4, sum, carry);

    /* 1+1+1=3=011 binary: sum=1, carry=1 (shifted) */
    assert(sum[0] == 1);   /* 1+1+1=3 → sum bit = 1 */
    assert(carry[0] == 1); /* carry bit = 1 → shifted: 2^1 */
    PASS();
}

/* ================================================================
 * L6: PARITY AND MAJORITY
 * ================================================================ */

static void test_parity_function(void) {
    TEST("PARITY function correctness");
    assert(parity_function((int[]){0,0,0}, 3) == 0);
    assert(parity_function((int[]){1,0,0}, 3) == 1);
    assert(parity_function((int[]){1,1,0}, 3) == 0);
    assert(parity_function((int[]){1,1,1}, 3) == 1);

    /* Test all inputs for n=4 */
    for (int s = 0; s < 16; s++) {
        int x[4] = {(s>>0)&1, (s>>1)&1, (s>>2)&1, (s>>3)&1};
        BoolCircuit *pc = build_parity_circuit(4);
        int *out = circuit_evaluate(pc, x);
        int expected = parity_function(x, 4);
        assert(out[0] == expected);
        free(out);
        circuit_free(pc);
    }
    PASS();
}

static void test_majority_function(void) {
    TEST("MAJORITY function correctness");
    assert(majority_function((int[]){0,0,0}, 3) == 0);
    assert(majority_function((int[]){1,0,0}, 3) == 0);
    assert(majority_function((int[]){1,1,0}, 3) == 1);
    assert(majority_function((int[]){1,1,1}, 3) == 1);

    /* Test all inputs for n=5 */
    for (int s = 0; s < 32; s++) {
        int x[5] = {(s>>0)&1,(s>>1)&1,(s>>2)&1,(s>>3)&1,(s>>4)&1};
        BoolCircuit *mc = build_majority_circuit(5);
        int *out = circuit_evaluate(mc, x);
        int expected = majority_function(x, 5);
        assert(out[0] == expected);
        free(out);
        circuit_free(mc);
    }
    PASS();
}

/* ================================================================
 * L6: FORMULA EVALUATION
 * ================================================================ */

static void test_formula_evaluation(void) {
    TEST("formula evaluation (NC1-complete)");
    /* AND(OR(x0,x1), NOT(x2)) */
    FormulaNode *f = formula_and(
        formula_or(formula_leaf(0), formula_leaf(1)),
        formula_not(formula_leaf(2)));

    assert(formula_eval_recursive(f, (int[]){1,0,0}) == 1);
    assert(formula_eval_recursive(f, (int[]){0,0,0}) == 0);
    assert(formula_eval_recursive(f, (int[]){0,1,1}) == 0);

    assert(formula_size(f) == 5);
    assert(formula_depth(f) == 3);
    formula_free(f);
    PASS();
}

static void test_formula_operators(void) {
    TEST("formula logical operators (XOR, IMPLIES, EQUIV)");
    /* x0 XOR x1 */
    FormulaNode *f_xor = formula_xor(formula_leaf(0), formula_leaf(1));
    assert(formula_eval_recursive(f_xor, (int[]){0,0}) == 0);
    assert(formula_eval_recursive(f_xor, (int[]){0,1}) == 1);
    assert(formula_eval_recursive(f_xor, (int[]){1,0}) == 1);
    assert(formula_eval_recursive(f_xor, (int[]){1,1}) == 0);
    formula_free(f_xor);

    /* MAJ3 */
    FormulaNode *f_maj = formula_majority3(
        formula_leaf(0), formula_leaf(1), formula_leaf(2));
    assert(formula_eval_recursive(f_maj, (int[]){1,1,1}) == 1);
    assert(formula_eval_recursive(f_maj, (int[]){1,1,0}) == 1);
    assert(formula_eval_recursive(f_maj, (int[]){1,0,0}) == 0);
    formula_free(f_maj);
    PASS();
}

/* ================================================================
 * L5: SORTING
 * ================================================================ */

static int sorted(const int *a, int n) {
    for (int i = 1; i < n; i++)
        if (a[i-1] > a[i]) return 0;
    return 1;
}

static void test_bitonic_sort(void) {
    TEST("bitonic sort correctness");
    for (int n = 4; n <= 16; n *= 2) {
        int *a = (int *)malloc((size_t)n * sizeof(int));
        for (int i = 0; i < n; i++)
            a[i] = (n - i) * 7 % 100;
        bitonic_sort(a, n);
        assert(sorted(a, n));
        free(a);
    }
    PASS();
}

static void test_odd_even_sort(void) {
    TEST("odd-even merge sort correctness");
    for (int n = 4; n <= 16; n *= 2) {
        int *a = (int *)malloc((size_t)n * sizeof(int));
        for (int i = 0; i < n; i++)
            a[i] = (i * 31 + 17) % 50;
        odd_even_merge_sort(a, n);
        assert(sorted(a, n));
        free(a);
    }
    PASS();
}

/* ================================================================
 * L5: MATRIX OPERATIONS
 * ================================================================ */

static void test_matrix_multiply(void) {
    TEST("matrix multiplication (sequential vs NC)");
    int n = 4;
    double A[16] = {1,2,0,0, 0,3,4,0, 0,0,5,6, 0,0,0,7};
    double B[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    double C_seq[16], C_nc[16];

    matrix_multiply_sequential(A, B, C_seq, n);
    matrix_multiply_nc(A, B, C_nc, n);

    for (int i = 0; i < n*n; i++)
        assert(fabs(C_seq[i] - C_nc[i]) < 1e-6);
    PASS();
}

/* ================================================================
 * L5: GRAPH OPERATIONS
 * ================================================================ */

static void test_transitive_closure(void) {
    TEST("transitive closure (NL in NC^2)");
    /* Path graph: 0->1->2->3 */
    int adj[16] = {0,1,0,0, 0,0,1,0, 0,0,0,1, 0,0,0,0};
    int closure[16];
    transitive_closure_nc(adj, 4, closure);

    /* Check path existence */
    assert(closure[0*4 + 0] == 1); /* reflexive */
    assert(closure[0*4 + 1] == 1);
    assert(closure[0*4 + 2] == 1);
    assert(closure[0*4 + 3] == 1);
    assert(closure[1*4 + 3] == 1);
    assert(closure[3*4 + 0] == 0); /* no path back */
    PASS();
}

/* ================================================================
 * L2: NCi CLASSIFICATION
 * ================================================================ */

static void test_nc_classification(void) {
    TEST("NC class classification");
    /* Build a parity circuit of size n=8 → should classify as NC1 */
    BoolCircuit *pc = build_parity_circuit(8);
    ClassBounds b = circuit_classify(pc, 8);
    assert(b.depth_class <= 1); /* NC0 or NC1 */
    circuit_free(pc);

    /* Build a majority circuit → should classify as TC0 */
    BoolCircuit *mc = build_majority_circuit(8);
    b = circuit_classify(mc, 8);
    assert(b.class_id == CLASS_TC0);
    circuit_free(mc);
    PASS();
}

/* ================================================================
 * L7: VLSI BOUNDS
 * ================================================================ */

static void test_vlsi_bounds(void) {
    TEST("VLSI area-time bounds (L7)");
    VLSIBounds vb = vlsi_area_time_bound(16, 4.0);
    assert(vb.at2_bound > 0);
    assert(vb.area > 0);
    assert(vb.time_per_op > 0);
    PASS();
}

/* ================================================================
 * MAIN
 * ================================================================ */

int main(void) {
    setbuf(stdout, NULL);

    printf("================================================================\n");
    printf("  NC HIERARCHY — COMPREHENSIVE TEST SUITE\n");
    printf("================================================================\n\n");

    /* L1: Definitions */
    printf("--- L1: Definitions ---\n");
    test_circuit_create();
    test_circuit_add_gates();
    test_circuit_constants();

    /* L2: Core Concepts */
    printf("\n--- L2: Core Concepts ---\n");
    test_circuit_eval_and();
    test_circuit_eval_or();
    test_circuit_eval_not();
    test_circuit_eval_xor();
    test_circuit_eval_nand();
    test_circuit_eval_majority();
    test_nc_classification();

    /* L3: Mathematical Structures */
    printf("\n--- L3: Mathematical Structures ---\n");
    test_circuit_depth();
    test_circuit_no_cycle();
    test_circuit_is_tree();
    test_circuit_is_monotone();

    /* L4: Fundamental Laws */
    printf("\n--- L4: Fundamental Laws ---\n");
    test_hierarchy_inclusions();
    test_brent_theorem();

    /* L5: Algorithms */
    printf("\n--- L5: Algorithms ---\n");
    test_prefix_correctness();
    test_carry_lookahead();
    test_carry_save_adder();
    test_bitonic_sort();
    test_odd_even_sort();
    test_matrix_multiply();
    test_transitive_closure();

    /* L6: Canonical Problems */
    printf("\n--- L6: Canonical Problems ---\n");
    test_parity_function();
    test_majority_function();
    test_formula_evaluation();
    test_formula_operators();

    /* L7: Applications */
    printf("\n--- L7: Applications ---\n");
    test_vlsi_bounds();

    printf("\n================================================================\n");
    printf("  RESULTS: %d/%d tests passed\n", tests_passed, tests_run);
    printf("================================================================\n");

    if (tests_passed == tests_run) {
        printf("\n  ALL TESTS PASSED\n\n");
        return 0;
    } else {
        printf("\n  %d TESTS FAILED\n\n", tests_run - tests_passed);
        return 1;
    }
}
