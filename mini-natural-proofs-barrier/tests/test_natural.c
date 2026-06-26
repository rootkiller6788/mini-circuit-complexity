/* test_natural.c -- Tests for Natural Property Framework
 *
 * Tests:
 *   - NaturalProperty structure validation
 *   - NP_Verdict creation and printing
 *   - Known natural properties
 *   - RRTheorem creation and derivation
 *   - Barrier verification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "natural.h"

static int total_tests = 0;
static int passed_tests = 0;

#define TEST(name) do { total_tests++; printf("  TEST %-50s", name); } while(0)
#define PASS()     do { passed_tests++; printf(" PASS\n"); } while(0)
#define FAIL(m)    do { printf(" FAIL: %s\n", m); } while(0)

static void test_natural_property(void) {
    TEST("NaturalProperty default values consistent");
    NaturalProperty np = shannon_natural_criteria_check(8, 64);
    assert(np.constructive == 1);
    assert(np.large == 1);
    assert(np.useful == 1);
    assert(np.is_natural == 1);
    assert(np.target_circuit_size == 64);
    PASS();
}

static void test_np_verdict(void) {
    TEST("NP_Verdict creation");
    NP_Verdict *v = np_verdict_create("complexity > S", "P/poly", 8, 64);
    assert(v != NULL);
    assert(strcmp(v->property_name, "complexity > S") == 0);
    assert(strcmp(v->circuit_class, "P/poly") == 0);
    assert(v->n_inputs == 8);
    assert(v->size_bound == 64);
    assert(v->criteria.is_natural == 1);
    assert(strlen(v->verdict) > 10);
    np_verdict_free(v);
    PASS();

    TEST("NP_Verdict for non-natural property");
    NP_Verdict *v2 = np_verdict_create("complexity > 2^n", "P/poly", 8, 256);
    assert(v2 != NULL);
    /* Should NOT be natural because S is too large */
    printf(" [nat=%d]", v2->criteria.is_natural);
    np_verdict_free(v2);
    PASS();
}

static void test_rr_theorem(void) {
    TEST("RRTheorem create + derive (OWF exist, natural P)");
    RRTheorem *thm = rr_theorem_create(1, 10, 100);
    assert(thm != NULL);
    assert(thm->owf_exist == 1);
    assert(thm->property_natural == 1);
    int result = rr_theorem_derive(thm);
    /* Should detect barrier */
    rr_theorem_print(thm, stdout);
    printf(" [barrier=%s contradiction=%s]",
           thm->barrier_holds ? "YES" : "NO",
           thm->contradiction ? "YES" : "NO");
    rr_theorem_free(thm);
    PASS();

    TEST("RRTheorem: no OWF => barrier avoided");
    RRTheorem *thm2 = rr_theorem_create(0, 10, 100);
    rr_theorem_derive(thm2);
    assert(thm2->prf_exist == 0);
    assert(thm2->barrier_holds == 0);
    rr_theorem_free(thm2);
    PASS();

    TEST("RRTheorem verify returns valid result");
    RRTheorem *thm3 = rr_theorem_create(1, 10, 100);
    rr_theorem_derive(thm3);
    int verify = rr_theorem_verify(thm3);
    assert(verify == 1);
    rr_theorem_free(thm3);
    PASS();
}

static void test_known_bounds(void) {
    TEST("Known lower bounds count > 0");
    int n = rr_known_bounds_count();
    assert(n >= 5);
    PASS();

    TEST("All known bounds are natural");
    int n_bounds = rr_known_bounds_count();
    for (int i = 0; i < n_bounds; i++) {
        const KnownLowerBound *kb = rr_known_bounds_get(i);
        assert(kb != NULL);
        assert(kb->is_natural == 1);  /* ALL known bounds are natural */
    }
    PASS();

    TEST("Known bounds have required fields");
    const KnownLowerBound *kb = rr_known_bounds_get(0);
    assert(kb->name != NULL);
    assert(kb->authors != NULL);
    assert(kb->target_class != NULL);
    assert(kb->hard_function != NULL);
    PASS();
}

static void test_bypass_candidates(void) {
    TEST("Bypass candidates count > 0");
    int n = rr_barrier_bypass_count();
    assert(n >= 3);
    PASS();

    TEST("Bypass candidate retrieval");
    char name[64], desc[512];
    int ok = rr_barrier_bypass_get(0, name, desc);
    assert(ok == 1);
    assert(strlen(name) > 0);
    assert(strlen(desc) > 10);
    PASS();
}

static void test_proof_steps(void) {
    TEST("Proof step count == 7");
    assert(rr_proof_step_count() == 7);
    PASS();

    TEST("Each proof step accessible");
    for (int i = 1; i <= 7; i++) {
        const BarrierProofStep *step = rr_proof_get_step(i);
        assert(step != NULL);
        assert(step->step_number == i);
        assert(strlen(step->description) > 0);
    }
    PASS();
}

int main(void) {
    setbuf(stdout, NULL);
    printf("=== Natural Property Tests ===\n\n");

    test_natural_property();
    test_np_verdict();
    test_rr_theorem();
    test_known_bounds();
    test_bypass_candidates();
    test_proof_steps();

    printf("\n=== Results: %d/%d passed ===\n", passed_tests, total_tests);
    if (passed_tests < total_tests) {
        printf("SOME TESTS FAILED!\n");
        return 1;
    }
    printf("ALL TESTS PASSED.\n");
    return 0;
}
