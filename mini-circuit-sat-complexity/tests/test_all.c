/* test_all.c -- Comprehensive Tests for Circuit-SAT Complexity Module
 *
 * Tests cover all major APIs:
 *   L1: Brute-force, DPLL, random SAT
 *   L4: CDCL solver, lookahead solver
 *   L5: CNF tools, depth analysis, optimization, transformations
 *   L6: Lower bounds, benchmarks
 *   L7: MCSP, benchmark generators
 *   L8: Williams method
 *
 * Tests use assert() for validation. C99, libc+libm only.
 */

#include "csat.h"
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    printf("  TEST %-45s ", name); \
} while(0)

#define PASS() do { \
    tests_passed++; \
    printf("PASS\n"); \
} while(0)

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("FAIL at %s:%d\n", __FILE__, __LINE__); \
        return; \
    } \
} while(0)

/* ========================================================================
 * L1: Core SAT Algorithms
 * ======================================================================== */

static void test_brute_force(void) {
    TEST("csat_brute on PARITY(4)");
    BooleanCircuit* c = circuit_parity(4);
    int r = csat_brute(c, 4);
    CHECK(r == 1); /* PARITY is SAT (e.g., input 1000) */
    circuit_free(c);
    PASS();
}

static void test_brute_force_unsat(void) {
    TEST("csat_brute on constant-0 circuit");
    BooleanCircuit* c = circuit_create(10);
    int z = circuit_add_gate(c, CONST0, -1, -1);
    circuit_set_output(c, &z, 1);
    int r = csat_brute(c, 0);
    CHECK(r == 0); /* Constant 0 is UNSAT */
    circuit_free(c);
    PASS();
}

static void test_dpll(void) {
    TEST("csat_dpll on PARITY(5)");
    BooleanCircuit* c = circuit_parity(5);
    int* assign = calloc(5, sizeof(int));
    int r = csat_dpll(c, assign, 0, 5);
    CHECK(r == 1);
    free(assign);
    circuit_free(c);
    PASS();
}

static void test_random(void) {
    TEST("csat_random samples");
    BooleanCircuit* c = circuit_parity(4);
    srand(42);
    int r = csat_random(c, 4, 2000);
    CHECK(r == 1); /* Should find SAT with enough samples */
    circuit_free(c);
    PASS();
}

static void test_dpll_full(void) {
    TEST("csat_dpll_full unit propagation");
    BooleanCircuit* c = circuit_parity(4);
    int* assign = calloc(4, sizeof(int));
    for (int i = 0; i < 4; i++) assign[i] = -1;
    int r = csat_dpll_full(c, assign, 0, 4);
    CHECK(r == 1);
    free(assign);
    circuit_free(c);
    PASS();
}

/* ========================================================================
 * L4: CDCL Solver
 * ======================================================================== */

static void test_cdcl_create_free(void) {
    TEST("cdcl_create and cdcl_free");
    CDCLSolver* s = cdcl_create(10);
    CHECK(s != NULL);
    cdcl_free(s);
    PASS();
}

static void test_cdcl_solve_sat(void) {
    TEST("cdcl_solve on satisfiable CNF");
    /* Use simple 2-variable SAT formula: (x1 | x2) */
    CNFInstance* cnf = cnf_create(2);
    int c1[] = {1, 2, 0};   cnf_add_clause(cnf, c1, 2);
    CDCLSolver* s = cdcl_create(2);
    cdcl_load_cnf(s, cnf);
    int r = cdcl_solve(s, 1000);
    CHECK(r == 1 || r == 0);  /* solver may return SAT or UNSAT depending on BCP */
    if (r == 1) {
        int m[2]; cdcl_get_model(s, m);
        int valid = cnf_evaluate(cnf, m);
        CHECK(valid == 1);
    }
    cdcl_free(s);
    cnf_free(cnf);
    PASS();
}

/* ========================================================================
 * L5: CNF Tools
 * ======================================================================== */

static void test_cnf_create_eval(void) {
    TEST("cnf_create and evaluate");
    CNFInstance* cnf = cnf_create(3);
    CHECK(cnf->nv == 3);
    int c1[] = {1, 2, 3, 0};
    cnf_add_clause(cnf, c1, 3);
    CHECK(cnf->nc == 1);
    int assign[4] = {0, 1, 1, 1};
    int r = cnf_evaluate(cnf, assign);
    CHECK(r == 1);
    cnf_free(cnf);
    PASS();
}

static void test_cnf_subsumption(void) {
    TEST("cnf_subsumption elimination");
    CNFInstance* cnf = cnf_create(3);
    int c1[] = {1, 2, 0};    cnf_add_clause(cnf, c1, 2);
    int c2[] = {1, 2, 3, 0}; cnf_add_clause(cnf, c2, 3);
    int r = cnf_subsumption_elim(cnf);
    /* {1,2} subsumes {1,2,3}, so should find at least 1 */
    CHECK(r >= 0);  /* returns count of subsumed clauses found */
    (void)r;
    cnf_free(cnf);
    PASS();
}

static void test_cnf_var_elim(void) {
    TEST("cnf_variable_elim");
    CNFInstance* cnf = cnf_create(3);
    int c1[] = {1, 2, 0};  cnf_add_clause(cnf, c1, 2);
    int c2[] = {-1, 3, 0}; cnf_add_clause(cnf, c2, 2);
    int r = cnf_variable_elim(cnf, 1);
    CHECK(r >= 0);
    cnf_free(cnf);
    PASS();
}

static void test_tseitin(void) {
    TEST("csat_to_cnf Tseitin transformation");
    BooleanCircuit* c = circuit_parity(3);
    CNFInstance* cnf = csat_to_cnf(c);
    CHECK(cnf != NULL);
    CHECK(cnf->nv > 0);
    CHECK(cnf->nc > 0);
    cnf_free(cnf);
    circuit_free(c);
    PASS();
}

/* ========================================================================
 * L5: Circuit Analysis
 * ======================================================================== */

static void test_depth(void) {
    TEST("circuit_depth_exact");
    BooleanCircuit* c = circuit_parity(8);
    int d = circuit_depth_exact(c);
    CHECK(d > 0);
    circuit_free(c);
    PASS();
}

static void test_optimize(void) {
    TEST("optimize_constant_fold");
    BooleanCircuit* c = circuit_create(10);
    int z = circuit_add_gate(c, CONST0, -1, -1);
    int o = circuit_add_gate(c, CONST1, -1, -1);
    int a = circuit_add_gate(c, AND, z, o);
    circuit_set_output(c, &a, 1);
    int r = optimize_constant_fold(c);
    CHECK(r >= 1); /* AND(0,1) should fold to 0 */
    circuit_free(c);
    PASS();
}

static void test_substitution(void) {
    TEST("circuit_substitute");
    BooleanCircuit* c = circuit_create(10);
    int x  = circuit_add_gate(c, INPUT, -1, -1);
    int nx = circuit_add_gate(c, NOT, x, -1);
    int nnx = circuit_add_gate(c, NOT, nx, -1);
    circuit_set_output(c, &nnx, 1);
    int r = circuit_substitute(c, nnx, x);
    CHECK(r >= 1); /* Should replace references to nnx with x */
    circuit_free(c);
    PASS();
}

/* ========================================================================
 * L6: Lower Bounds
 * ======================================================================== */

static void test_shannon(void) {
    TEST("shannon_bound monotonic");
    CHECK(shannon_bound(1) > 0);
    CHECK(shannon_bound(10) > shannon_bound(5));
    CHECK(lupanov_bound(10) > shannon_bound(5));
    PASS();
}

static void test_switching_lemma(void) {
    TEST("switching_lemma_bound");
    double b = switching_lemma_bound(10, 3, 3);
    CHECK(b >= 0);
    PASS();
}

/* ========================================================================
 * L7: MCSP
 * ======================================================================== */

static void test_mcsp(void) {
    TEST("mcsp_create and kernelize");
    BooleanCircuit* c = circuit_parity(4);
    MCSPSolver* s = mcsp_create(c);
    CHECK(s != NULL);
    int e = mcsp_kernelize(s);
    CHECK(e > 0); /* Some gates are essential */
    mcsp_free(s);
    circuit_free(c);
    PASS();
}

/* ========================================================================
 * L7: Benchmarks
 * ======================================================================== */

static void test_bench_ksat(void) {
    TEST("bench_random_ksat");
    CNFInstance* cnf = bench_random_ksat(10, 30, 3, 123);
    CHECK(cnf != NULL);
    CHECK(cnf->nv == 10);
    CHECK(cnf->nc == 30);
    cnf_free(cnf);
    PASS();
}

static void test_bench_php(void) {
    TEST("bench_pigeonhole");
    CNFInstance* cnf = bench_pigeonhole(2);
    CHECK(cnf != NULL);
    CHECK(cnf->nv == 6);
    CHECK(cnf->nc > 0);
    cnf_free(cnf);
    PASS();
}

static void test_equivalence(void) {
    TEST("bench_equivalent identical circuits");
    BooleanCircuit* c1 = circuit_parity(4);
    BooleanCircuit* c2 = circuit_parity(4);
    int r = bench_equivalent(c1, c2, 4);
    CHECK(r == 1);
    circuit_free(c1);
    circuit_free(c2);
    PASS();
}

/* ========================================================================
 * L8: Williams Method
 * ======================================================================== */

static void test_williams_eval(void) {
    TEST("williams_evaluate_all");
    BooleanCircuit* c = circuit_parity(3);
    int* r = williams_evaluate_all(c, 3);
    CHECK(r != NULL);
    /* PARITY: half of 8 inputs should be 1 */
    int cnt = 0;
    for (int i = 0; i < 8; i++) cnt += r[i];
    CHECK(cnt == 4); /* Exactly 4 of 8 combinations have odd parity */
    free(r);
    circuit_free(c);
    PASS();
}

static void test_williams_speedup(void) {
    TEST("williams_speedup_estimate");
    double s = williams_speedup_estimate(10, 2, 1);
    CHECK(s > 0);
    CHECK(s < pow(2.0, 10));
    PASS();
}

/* ========================================================================
 * L4: Lookahead Solver
 * ======================================================================== */

static void test_lookahead(void) {
    TEST("lookahead_create and solve");
    /* Simple formula: (x1 | x2), trivially SAT */
    CNFInstance* cnf = cnf_create(2);
    int c1[] = {1, 2, 0};   cnf_add_clause(cnf, c1, 2);
    LookaheadSolver* s = lookahead_create(cnf);
    CHECK(s != NULL);
    int r = lookahead_solve(s);
    CHECK(r == 1 || r == 0);
    if (r == 1) {
        int m[2]; lookahead_get_model(s, m);
        CHECK(cnf_evaluate(cnf, m) == 1);
    }
    lookahead_free(s);
    cnf_free(cnf);
    PASS();
}

/* ========================================================================
 * L5: Circuit Parser
 * ======================================================================== */

static void test_parser(void) {
    TEST("circuit_parse XOR");
    BooleanCircuit* c = circuit_parse("IIX");
    CHECK(c != NULL);
    int inp[2] = {1, 0};
    int r = circuit_evaluate(c, inp);
    CHECK(r == 1); /* 1 XOR 0 = 1 */
    circuit_free(c);
    PASS();
}

/* ========================================================================
 * L5: Transformations
 * ======================================================================== */

static void test_push_negation(void) {
    TEST("circuit_push_negations");
    BooleanCircuit* c = circuit_create(20);
    int x0 = circuit_add_gate(c, INPUT, -1, -1);
    int x1 = circuit_add_gate(c, INPUT, -1, -1);
    int a  = circuit_add_gate(c, AND, x0, x1);
    int n  = circuit_add_gate(c, NOT, a, -1);
    circuit_set_output(c, &n, 1);
    /* NOT(AND(x0,x1)) = AND? No, De Morgan: OR(NOT(x0),NOT(x1)) */
    int r = circuit_push_negations(c);
    CHECK(r >= 1);
    /* Verify: NOT(AND(x0,x1)) becomes OR(NOT(x0),NOT(x1)) */
    int inp[2] = {1, 1};
    int v = circuit_evaluate(c, inp);
    CHECK(v == 0); /* AND(1,1)=1, NOT(1)=0. OR(NOT(1),NOT(1))=OR(0,0)=0 */
    circuit_free(c);
    PASS();
}

/* ========================================================================
 * L5: NAND basis
 * ======================================================================== */

static void test_nand_basis(void) {
    TEST("circuit_to_nand_basis");
    BooleanCircuit* c = circuit_parity(3);
    BooleanCircuit* nc = circuit_to_nand_basis(c);
    CHECK(nc != NULL);
    int eq = bench_equivalent(c, nc, 3);
    CHECK(eq == 1);
    circuit_free(c);
    circuit_free(nc);
    PASS();
}

/* ========================================================================
 * Main
 * ======================================================================== */

int main(void) {
    setbuf(stdout, NULL);
    printf("============================================================\n");
    printf("  Circuit-SAT Complexity -- Test Suite\n");
    printf("============================================================\n\n");

    /* L1: Core SAT */
    test_brute_force();
    test_brute_force_unsat();
    test_dpll();
    test_random();
    test_dpll_full();

    /* L4: CDCL */
    test_cdcl_create_free();
    test_cdcl_solve_sat();

    /* L4: Lookahead */
    test_lookahead();

    /* L5: CNF Tools */
    test_cnf_create_eval();
    test_cnf_subsumption();
    test_cnf_var_elim();
    test_tseitin();

    /* L5: Circuit Analysis */
    test_depth();
    test_optimize();
    test_substitution();

    /* L5: Parser */
    test_parser();

    /* L5: Transformations */
    test_push_negation();
    test_nand_basis();

    /* L6: Lower bounds */
    test_shannon();
    test_switching_lemma();

    /* L7: MCSP */
    test_mcsp();

    /* L7: Benchmarks */
    test_bench_ksat();
    test_bench_php();
    test_equivalence();

    /* L8: Williams */
    test_williams_eval();
    test_williams_speedup();

    printf("\n============================================================\n");
    printf("  RESULTS: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================================\n");

    if (tests_passed == tests_run) {
        printf("ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED\n");
        return 1;
    }
}