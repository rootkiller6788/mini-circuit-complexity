/* circuit_classes.c -- Circuit Complexity Classes
 *
 * AC0, ACC0, TC0, NC1, P/poly hierarchy and membership.
 *
 * Hierarchy: AC0 < AC0[p] < ACC0 <= TC0 <= NC1 <= P/poly
 *
 * Key results:
 *   PARITY not in AC0 (Furst-Saxe-Sipser/Ajtai 1981/1983)
 *   MAJORITY not in AC0[p] for prime p (Razborov-Smolensky 1987)
 *   NEXP not in ACC0 (Williams 2014)
 *
 * L1: Class definitions
 * L2: Hierarchy relationships
 * L4: Known separations and containments
 * L6: Shannon counting, FSS, Razborov-Smolensky
 */

#include "csat.h"

static const char* class_name(CircuitClass cls) {
    switch (cls) {
        case CLASS_AC0:   return "AC0";
        case CLASS_AC1:   return "AC1";
        case CLASS_NC1:   return "NC1";
        case CLASS_TC0:   return "TC0";
        case CLASS_P_POLY: return "P/poly";
        case CLASS_NC:    return "NC";
        case CLASS_AC:    return "AC";
        default:          return "UNKNOWN";
    }
}

void class_print_desc(ClassDesc* d)
{
    if (!d) return;
    printf("Class: %s\n", class_name(d->cls));
    if (d->depth >= 0) printf("  Depth: %d\n", d->depth);
    if (d->fanin >= 0) printf("  Fan-in: %d\n", d->fanin);
    printf("  Alternations: %d\n", d->alternations);
}

int class_membership_check(const BooleanCircuit* c, const ClassDesc* d) {
    (void)c; (void)d;
    return 0;
}

static void demo_shannon_counting(void) {
    printf("\n--- Shannon Counting (1949) ---\n");
    printf("Almost all Boolean functions need Omega(2^n/n) gates.\n\n");
    printf("Proof: 2^{2^n} functions, < 2^{O(s log s)} circuits size s.\n");
    printf("=> s < 2^n/n => too few circuits for most functions.\n");
    printf("Lupanov (1958): 2^n/n*(1+o(1)) is tight upper bound.\n\n");
    printf("%4s %16s %16s\n", "n", "Shannon LB", "Lupanov UB");
    for (int n = 3; n <= 10; n++) {
        printf("%4d %15.1f %15.1f\n", n,
               shannon_bound(n), lupanov_bound(n));
    }
}

static void demo_fss_ajtai(void) {
    printf("\n--- Furst-Saxe-Sipser/Ajtai (1981/1983) ---\n");
    printf("PARITY not in AC0. Requires EXPONENTIAL size.\n\n");
    printf("Proof: Hastad Switching Lemma + random restrictions.\n");
    printf("  Step 1: Apply random restriction (p ~ 1/log n).\n");
    printf("  Step 2: Each AND/OR gate becomes small decision tree.\n");
    printf("  Step 3: By switching lemma, circuit collapses to DT.\n");
    printf("  Step 4: PARITY cannot be approximated by small DT.\n\n");
    printf("First non-trivial circuit lower bound!\n");
    printf("Launched modern circuit complexity.\n");
}

static void demo_razborov_smolensky(void) {
    printf("\n--- Razborov-Smolensky (1987) ---\n");
    printf("MAJORITY not in AC0[p] for any prime p.\n");
    printf("Method: polynomial approximation over GF(p).\n");
    printf("  AC0[p]: AND, OR, NOT, MODp gates.\n");
    printf("  Every AC0[p] circuit approx by low-degree polynomial.\n");
    printf("  MAJ requires degree Omega(n). Contradiction!\n\n");
    printf("Known: AC0 < AC0[p] for prime p.\n");
    printf("Known: NEXP not in ACC0 (Williams 2014).\n");
    printf("Open:  ACC0 vs NC1, ACC0 vs NEXP.\n");
}

static void demo_williams_breakthrough(void) {
    printf("\n--- Williams Algorithmic Method (2010-2014) ---\n");
    printf("NEXP not in ACC0 (Williams 2014, JACM).\n\n");
    printf("Key: SAT algorithms => circuit lower bounds.\n");
    printf("  ACC0-SAT in O(2^n / n^{omega(1)}) time\n");
    printf("  => NEXP not in ACC0.\n\n");
    printf("Williams proved ACC0-SAT in 2^{n - n^delta} time\n");
    printf("using rectangular matrix multiplication.\n\n");
    printf("ONLY known non-trivial lower bound against\n");
    printf("unrestricted circuit class for NEXP!\n");
}

void circuit_classes_demo(void) {
    printf("\n============================================================\n");
    printf("  Circuit Complexity Classes\n");
    printf("============================================================\n\n");

    printf("Hierarchy: AC0 < AC0[p] < ACC0 <= TC0 <= NC1 <= P/poly\n\n");
    printf("  AC0:   constant depth, unbounded fan-in AND/OR/NOT\n");
    printf("  ACC0:  AC0 + MOD gates\n");
    printf("  TC0:   constant depth, MAJ/THR gates\n");
    printf("  NC1:   O(log n) depth, bounded fan-in\n");
    printf("  P/poly: polynomial-size circuit families\n\n");

    /* Build sample circuits */
    int n = 8;
    BooleanCircuit* par = circuit_parity(n);
    printf("PARITY(%d): size=%d, depth=%d\n", n,
           circuit_size(par), circuit_depth(par));
    printf("  PARITY requires EXPONENTIAL size for AC0.\n");
    circuit_free(par);

    BooleanCircuit* maj = circuit_majority(n);
    printf("MAJORITY(%d): size=%d, depth=%d\n", n,
           circuit_size(maj), circuit_depth(maj));
    printf("  MAJORITY requires EXPONENTIAL size for AC0[p].\n");
    circuit_free(maj);

    demo_shannon_counting();
    demo_fss_ajtai();
    demo_razborov_smolensky();
    demo_williams_breakthrough();
}