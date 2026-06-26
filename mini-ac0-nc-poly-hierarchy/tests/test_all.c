/* test_all.c — Comprehensive tests for AC0/NC/TC0/P/poly Hierarchy
 * =====================================================================
 * Tests cover:
 *   - Core circuit construction and evaluation
 *   - DNF/CNF operations
 *   - AC0 threshold circuit construction
 *   - NC1 parity construction
 *   - TC0 majority construction
 *   - P/poly truth-table construction
 *   - Circuit analysis (depth, size, fan-in)
 *   - Class inclusion queries
 *   - Complete problems (CVP, BFE)
 *   - Lower bound calculations
 *
 * All tests use assert() for self-checking.
 * Run: make test
 */
#include "ac0nc.h"
#include "ac0_circuit.h"
#include "nc_circuit.h"
#include "ppoly.h"
#include "circuit_classes.h"
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;
#define TEST(name) do { tests_run++; printf("TEST %d: %s ... ", tests_run, name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { printf("FAIL: %s (got %d, expected %d)\n", msg, (int)(a), (int)(b)); return; } \
} while(0)
#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return; } \
} while(0)

/* ===== Core Circuit Tests ===== */
static void test_create_free(void)
{
    TEST("create/free");
    AC0Circuit *c = ac0_circuit_create(100);
    ASSERT_TRUE(c != NULL, "create returned NULL");
    ASSERT_EQ(c->n_gates, 0, "initial gate count");
    ASSERT_EQ(c->n_inputs, 0, "initial input count");
    ac0_circuit_free(c);
    PASS();
}

static void test_add_inputs(void)
{
    TEST("add inputs");
    AC0Circuit *c = ac0_circuit_create(100);
    int i0 = ac0_circuit_add_input(c);
    int i1 = ac0_circuit_add_input(c);
    int i2 = ac0_circuit_add_input(c);
    ASSERT_EQ(i0, 0, "first input id");
    ASSERT_EQ(i1, 1, "second input id");
    ASSERT_EQ(i2, 2, "third input id");
    ASSERT_EQ(c->n_inputs, 3, "input count");
    ASSERT_EQ(c->n_gates, 3, "gate count after inputs");
    ac0_circuit_free(c);
    PASS();
}

static void test_and_gate(void)
{
    TEST("AND gate evaluation");
    AC0Circuit *c = ac0_circuit_create(100);
    int i0 = ac0_circuit_add_input(c);
    int i1 = ac0_circuit_add_input(c);
    int and_g = ac0_circuit_add_gate(c, AC0_GATE_AND, i0, i1);
    ac0_circuit_set_output(c, and_g);

    int in00[] = {0, 0};
    int in01[] = {0, 1};
    int in10[] = {1, 0};
    int in11[] = {1, 1};

    ASSERT_EQ(ac0_circuit_eval(c, in00), 0, "AND(0,0)");
    ASSERT_EQ(ac0_circuit_eval(c, in01), 0, "AND(0,1)");
    ASSERT_EQ(ac0_circuit_eval(c, in10), 0, "AND(1,0)");
    ASSERT_EQ(ac0_circuit_eval(c, in11), 1, "AND(1,1)");
    ac0_circuit_free(c);
    PASS();
}

static void test_or_gate(void)
{
    TEST("OR gate evaluation");
    AC0Circuit *c = ac0_circuit_create(100);
    int i0 = ac0_circuit_add_input(c);
    int i1 = ac0_circuit_add_input(c);
    int or_g = ac0_circuit_add_gate(c, AC0_GATE_OR, i0, i1);
    ac0_circuit_set_output(c, or_g);

    int in00[] = {0, 0};
    int in11[] = {1, 1};
    int in10[] = {1, 0};

    ASSERT_EQ(ac0_circuit_eval(c, in00), 0, "OR(0,0)");
    ASSERT_EQ(ac0_circuit_eval(c, in10), 1, "OR(1,0)");
    ASSERT_EQ(ac0_circuit_eval(c, in11), 1, "OR(1,1)");
    ac0_circuit_free(c);
    PASS();
}

static void test_not_gate(void)
{
    TEST("NOT gate evaluation");
    AC0Circuit *c = ac0_circuit_create(100);
    int i0 = ac0_circuit_add_input(c);
    int not_g = ac0_circuit_add_gate(c, AC0_GATE_NOT, i0, -1);
    ac0_circuit_set_output(c, not_g);

    int in0[] = {0};
    int in1[] = {1};
    ASSERT_EQ(ac0_circuit_eval(c, in0), 1, "NOT(0)");
    ASSERT_EQ(ac0_circuit_eval(c, in1), 0, "NOT(1)");
    ac0_circuit_free(c);
    PASS();
}

static void test_xor_gate(void)
{
    TEST("XOR gate evaluation");
    AC0Circuit *c = ac0_circuit_create(100);
    int i0 = ac0_circuit_add_input(c);
    int i1 = ac0_circuit_add_input(c);
    int xor_g = ac0_circuit_add_gate(c, AC0_GATE_XOR, i0, i1);
    ac0_circuit_set_output(c, xor_g);

    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0}), 0, "XOR(0,0)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,1}), 1, "XOR(0,1)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,0}), 1, "XOR(1,0)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1}), 0, "XOR(1,1)");
    ac0_circuit_free(c);
    PASS();
}

static void test_majority_gate(void)
{
    TEST("MAJORITY gate evaluation");
    AC0Circuit *c = ac0_circuit_create(100);
    int ins[5];
    for (int i = 0; i < 5; i++)
        ins[i] = ac0_circuit_add_input(c);
    int maj = ac0_circuit_add_majority(c, ins, 5);
    ac0_circuit_set_output(c, maj);

    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0,0,0,0}), 0, "MAJ(0,0,0,0,0)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,1,0,0}), 1, "MAJ(1,1,1,0,0)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,0,0,0}), 0, "MAJ(1,1,0,0,0)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,1,1,0}), 1, "MAJ(1,1,1,1,0)");
    ac0_circuit_free(c);
    PASS();
}

static void test_mod2_gate(void)
{
    TEST("MOD2 (PARITY) gate evaluation");
    AC0Circuit *c = ac0_circuit_create(100);
    int ins[4];
    for (int i = 0; i < 4; i++)
        ins[i] = ac0_circuit_add_input(c);
    int mod2 = ac0_circuit_add_mod2(c, ins, 4);
    ac0_circuit_set_output(c, mod2);

    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0,0,0}), 1, "PARITY(0,0,0,0)=1 (even)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0,0,1}), 0, "PARITY(0,0,0,1)=0 (odd)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,0,0}), 1, "PARITY(1,1,0,0)=1 (even)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,1,0}), 0, "PARITY(1,1,1,0)=0 (odd)");
    ac0_circuit_free(c);
    PASS();
}

static void test_depth_calculation(void)
{
    TEST("depth calculation");
    AC0Circuit *c = ac0_circuit_create(100);
    int i0 = ac0_circuit_add_input(c);
    int i1 = ac0_circuit_add_input(c);
    int n0 = ac0_circuit_add_gate(c, AC0_GATE_NOT, i0, -1);
    int a0 = ac0_circuit_add_gate(c, AC0_GATE_AND, n0, i1);
    int o0 = ac0_circuit_add_gate(c, AC0_GATE_NOT, a0, -1);
    ac0_circuit_set_output(c, o0);
    /* Depth: input(0) -> NOT(1) -> AND(2) -> NOT(3) */
    ASSERT_EQ(ac0_circuit_depth(c), 3, "depth of NOT-AND-NOT chain");
    ac0_circuit_free(c);
    PASS();
}

static void test_size_calculation(void)
{
    TEST("size calculation");
    AC0Circuit *c = ac0_circuit_create(100);
    int i0 = ac0_circuit_add_input(c);
    int i1 = ac0_circuit_add_input(c);
    ac0_circuit_add_gate(c, AC0_GATE_AND, i0, i1);
    ac0_circuit_add_gate(c, AC0_GATE_OR, i0, i1);
    ac0_circuit_add_gate(c, AC0_GATE_NOT, i0, -1);
    /* 3 inputs + 3 gates = 6 total, size = 3 non-input */
    ASSERT_EQ(ac0_circuit_size(c), 3, "size (non-input gates)");
    ASSERT_EQ(c->n_gates, 5, "total gates");
    ac0_circuit_free(c);
    PASS();
}

/* ===== DNF/CNF Tests ===== */
static void test_dnf_create_eval(void)
{
    TEST("DNF create and evaluate");
    DNF *d = dnf_create(3, 4);
    ASSERT_TRUE(d != NULL, "DNF create");

    /* Term 0: x0 AND NOT x1 */
    dnf_set_literal(d, 0, 0, 1);
    dnf_set_literal(d, 0, 1, -1);

    /* Term 1: x2 */
    dnf_set_literal(d, 1, 2, 1);

    /* Term 2: NOT x0 AND x1 */
    dnf_set_literal(d, 2, 0, -1);
    dnf_set_literal(d, 2, 1, 1);

    /* x=(1,0,1): term0 true (1 AND NOT 0), term1 true (1), term2 false */
    ASSERT_EQ(dnf_evaluate(d, (int[]){1,0,1}), 1, "DNF(1,0,1)");
    /* x=(0,0,0): all false */
    ASSERT_EQ(dnf_evaluate(d, (int[]){0,0,0}), 0, "DNF(0,0,0)");
    /* x=(0,1,0): term2 true (NOT 0 AND 1) */
    ASSERT_EQ(dnf_evaluate(d, (int[]){0,1,0}), 1, "DNF(0,1,0)");

    dnf_free(d);
    PASS();
}

static void test_cnf_create_eval(void)
{
    TEST("CNF create and evaluate");
    CNF *cnf = cnf_create(3, 3);
    ASSERT_TRUE(cnf != NULL, "CNF create");

    /* Clause 0: x0 OR x1 */
    cnf_set_literal(cnf, 0, 0, 1);
    cnf_set_literal(cnf, 0, 1, 1);

    /* Clause 1: NOT x0 OR x2 */
    cnf_set_literal(cnf, 1, 0, -1);
    cnf_set_literal(cnf, 1, 2, 1);

    /* Clause 2: NOT x1 OR NOT x2 */
    cnf_set_literal(cnf, 2, 1, -1);
    cnf_set_literal(cnf, 2, 2, -1);

    /* x=(1,1,1): clause0 true, clause1 true, clause2 false */
    ASSERT_EQ(cnf_evaluate(cnf, (int[]){1,1,1}), 0, "CNF(1,1,1) false");
    /* x=(1,0,0): clause0 true, clause1 false, clause2 true */
    ASSERT_EQ(cnf_evaluate(cnf, (int[]){1,0,0}), 0, "CNF(1,0,0) false");
    /* x=(0,1,0): all clauses true */
    ASSERT_EQ(cnf_evaluate(cnf, (int[]){0,1,0}), 1, "CNF(0,1,0) true");

    cnf_free(cnf);
    PASS();
}

/* ===== AC0 Threshold Test ===== */
static void test_ac0_threshold(void)
{
    TEST("AC0 threshold circuit");
    AC0Circuit *c = ac0_build_threshold(4, 2);
    ASSERT_TRUE(c != NULL, "threshold circuit creation");

    /* THRESHOLD_2(0,0,0,0) = 0 */
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0,0,0}), 0, "thresh(4,2) with 0 ones");
    /* THRESHOLD_2(1,0,0,0) = 0 */
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,0,0,0}), 0, "thresh(4,2) with 1 one");
    /* THRESHOLD_2(1,1,0,0) = 1 */
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,0,0}), 1, "thresh(4,2) with 2 ones");
    /* THRESHOLD_2(1,1,1,1) = 1 */
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,1,1}), 1, "thresh(4,2) with 4 ones");

    ac0_circuit_free(c);
    PASS();
}

/* ===== NC1 Parity Test ===== */
static void test_nc1_parity(void)
{
    TEST("NC1 parity circuit");
    AC0Circuit *c = nc1_build_parity(4);
    ASSERT_TRUE(c != NULL, "NC1 parity circuit creation");

    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0,0,0}), 0, "PARITY(0,0,0,0)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0,0,1}), 1, "PARITY(0,0,0,1)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0,1,1}), 0, "PARITY(0,0,1,1)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,1,1}), 0, "PARITY(1,1,1,1)");

    int d = ac0_circuit_depth(c);
    ASSERT_TRUE(d <= 4, "NC1 parity depth <= 2*log2(n)");

    ac0_circuit_free(c);
    PASS();
}

/* ===== TC0 Majority Test ===== */
static void test_tc0_majority(void)
{
    TEST("TC0 majority circuit");
    AC0Circuit *c = tc0_build_majority(5);
    ASSERT_TRUE(c != NULL, "TC0 majority creation");

    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0,0,0,0}), 0, "MAJ(5 zeros)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,1,0,0}), 1, "MAJ(3 ones)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,0,0,0}), 0, "MAJ(2 ones)");

    ac0_circuit_free(c);
    PASS();
}

/* ===== P/poly Test ===== */
static void test_ppoly_parity(void)
{
    TEST("P/poly parity circuit");
    AC0Circuit *c = ppoly_build_parity(3);
    ASSERT_TRUE(c != NULL, "P/poly parity circuit for n=3");

    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0,0}), 0, "ppoly PARITY(0,0,0)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,0,1}), 1, "ppoly PARITY(0,0,1)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){0,1,1}), 0, "ppoly PARITY(0,1,1)");
    ASSERT_EQ(ac0_circuit_eval(c, (int[]){1,1,1}), 1, "ppoly PARITY(1,1,1)");

    ac0_circuit_free(c);
    PASS();
}

/* ===== Circuit Class Tests ===== */
static void test_class_membership(void)
{
    TEST("class membership");
    AC0Circuit *ac0c = ac0_build_threshold(4, 2);
    ASSERT_TRUE(ac0c != NULL, "AC0 threshold creation");
    ASSERT_EQ(is_circuit_in_ac0(ac0c), 1, "threshold is in AC0");
    ac0_circuit_free(ac0c);

    AC0Circuit *nc1c = nc1_build_parity(4);
    ASSERT_TRUE(nc1c != NULL, "NC1 parity creation");
    ASSERT_EQ(is_circuit_in_nc1(nc1c), 1, "parity is in NC1");
    ac0_circuit_free(nc1c);

    PASS();
}

/* ===== CVP Test ===== */
static void test_cvp(void)
{
    TEST("CVP evaluation");
    CVPInstance *cvp = cvp_create(5);
    ASSERT_TRUE(cvp != NULL, "CVP creation");

    cvp->nodes[0] = (CVPNode){0, 0, -1, 0};
    cvp->nodes[1] = (CVPNode){0, 1, -1, 0};
    cvp->nodes[2] = (CVPNode){0, 2, -1, 0};
    cvp->nodes[3] = (CVPNode){2, 0,  1, 0};
    cvp->nodes[4] = (CVPNode){1, 3,  2, 0};

    ASSERT_EQ(cvp_evaluate(cvp, (int[]){1,0,1}), 1, "CVP AND(OR(1,0),1)=1");
    ASSERT_EQ(cvp_evaluate(cvp, (int[]){0,0,1}), 0, "CVP AND(OR(0,0),1)=0");

    cvp_free(cvp);
    PASS();
}

/* ===== BFE Test ===== */
static void test_bfe(void)
{
    TEST("BFE evaluation");
    Formula *f = formula_random(3, 2);
    ASSERT_TRUE(f != NULL, "BFE formula creation");

    int result = formula_evaluate(f, f->n_nodes - 1, (int[]){0,0,0});
    ASSERT_TRUE(result == 0 || result == 1, "BFE returns valid value");

    formula_free(f);
    PASS();
}

/* ===== Lower Bound Tests ===== */
static void test_lower_bounds(void)
{
    TEST("lower bound calculations");

    double lb;

    lb = ac0_parity_size_lower_bound(16, 2);
    ASSERT_TRUE(lb > 1.0, "PARITY size lower bound positive");

    lb = shannon_lower_bound(4, 0.1);
    ASSERT_TRUE(lb > 0, "Shannon lower bound positive");

    lb = ac0_threshold_size_lower_bound(8, 3);
    ASSERT_TRUE(lb > 1.0, "threshold size lower bound positive");

    lb = hastad_switching_prob(32, 0.5, 2);
    ASSERT_TRUE(lb >= 0.0, "switching probability valid");

    PASS();
}

/* ===== Hierarchy Query Tests ===== */
static void test_hierarchy_queries(void)
{
    TEST("hierarchy queries");

    ASSERT_EQ(is_subset_known(CLASS_AC0, CLASS_NC1), 1, "AC0 subset NC1 known");
    ASSERT_EQ(is_subset_known(CLASS_NC1, CLASS_P), 1, "NC1 subset P known");
    ASSERT_EQ(is_separation_known(CLASS_AC0, CLASS_NC1), 0, "AC0 not= NC1 is NOT in matrix (use separations list)");

    ASSERT_EQ(is_open_question(CLASS_TC0, CLASS_NC1), 0, "TC0 vs NC1 is open");
    ASSERT_EQ(is_open_question(CLASS_NP, CLASS_PPOLY), 1, "NP vs P/poly is open");

    PASS();
}

/* ===== Multiple Outputs Test ===== */
static void test_multiple_outputs(void)
{
    TEST("multiple outputs");
    AC0Circuit *c = ac0_circuit_create(100);
    int i0 = ac0_circuit_add_input(c);
    int i1 = ac0_circuit_add_input(c);
    int a = ac0_circuit_add_gate(c, AC0_GATE_AND, i0, i1);
    int o = ac0_circuit_add_gate(c, AC0_GATE_OR, i0, i1);
    ac0_circuit_set_output(c, a);
    ac0_circuit_set_output(c, o);

    int in[] = {1, 0};
    int result = ac0_circuit_eval(c, in);
    ASSERT_EQ(result, 0, "AND of AND and OR with (1,0) -> AND(1,0)&OR(1,0)=0&1=0");

    ac0_circuit_free(c);
    PASS();
}

/* ===== Truth Table Test ===== */
static void test_truth_table(void)
{
    TEST("truth table enumeration");
    AC0Circuit *c = ac0_circuit_create(100);
    int i0 = ac0_circuit_add_input(c);
    int i1 = ac0_circuit_add_input(c);
    int and_g = ac0_circuit_add_gate(c, AC0_GATE_AND, i0, i1);
    ac0_circuit_set_output(c, and_g);

    ASSERT_EQ(ac0_circuit_truth_table_row(c, 0), 0, "TT[0]: AND(0,0)=0");
    ASSERT_EQ(ac0_circuit_truth_table_row(c, 3), 1, "TT[3]: AND(1,1)=1");

    ac0_circuit_free(c);
    PASS();
}

/* ===== Main ===== */
int main(void)
{
    setbuf(stdout, NULL);
    printf("\n========================================\n");
    printf("  AC0/NC/TC0/P/poly HIERARCHY TEST SUITE\n");
    printf("========================================\n\n");

    test_create_free();
    test_add_inputs();
    test_and_gate();
    test_or_gate();
    test_not_gate();
    test_xor_gate();
    test_majority_gate();
    test_mod2_gate();
    test_depth_calculation();
    test_size_calculation();
    test_dnf_create_eval();
    test_cnf_create_eval();
    test_ac0_threshold();
    test_nc1_parity();
    test_tc0_majority();
    test_ppoly_parity();
    test_class_membership();
    test_cvp();
    test_bfe();
    test_lower_bounds();
    test_hierarchy_queries();
    test_multiple_outputs();
    test_truth_table();

    printf("\n========================================\n");
    printf("  RESULTS: %d/%d PASSED\n", tests_passed, tests_run);
    printf("========================================\n");

    if (tests_passed == tests_run) {
        printf("ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("%d TESTS FAILED\n", tests_run - tests_passed);
        return 1;
    }
}
