/* example_hierarchy.c — Circuit Class Hierarchy Explorer
 * =====================================================================
 * Interactive (or batch) exploration of the circuit complexity
 * class hierarchy: AC0 < TC0 < NC1 < L < NL < NC < P < P/poly.
 *
 * Demonstrates:
 *   - Class inclusion relationships
 *   - Known separations
 *   - Open problems
 *   - Circuit construction at each level
 *
 * Build: gcc -I../include example_hierarchy.c ../src/*.c -lm -o example_hierarchy
 */
#include "ac0nc.h"
#include "ac0_circuit.h"
#include "nc_circuit.h"
#include "ppoly.h"
#include "circuit_classes.h"

static void print_separator(const char *title)
{
    printf("\n");
    for (int i = 0; i < 60; i++) printf("=");
    printf("\n  %s\n", title);
    for (int i = 0; i < 60; i++) printf("=");
    printf("\n\n");
}

int main(void)
{
    setbuf(stdout, NULL);

    print_separator("CIRCUIT COMPLEXITY HIERARCHY EXPLORER");

    /* CLASS LANDSCAPE */
    printf("THE LANDSCAPE:\n\n");
    printf("  AC0 < AC0[p] < TC0 <= NC1 <= L <= NL <= NC <= P <= P/poly\n");
    printf("                                      |\n");
    printf("                                 NL=coNL (IS 1988)\n\n");
    printf("  BPP <= P/poly (Adleman 1978)\n");
    printf("  BPP <= Sigma2 (Sipser-Gacs-Lautemann 1983)\n\n");
    printf("  Separations:\n");
    printf("    AC0 != NC1  (PARITY, Hastad 1986)\n");
    printf("    AC0[p] != NC1 [p!=2] (MAJORITY, Razborov-Smolensky 1987)\n");
    printf("    NEXP not in ACC0 (Williams 2014)\n\n");

    /* BUILD CIRCUITS AT EACH LEVEL */
    int n = 8;

    printf("CIRCUIT CONSTRUCTION AT EACH LEVEL (n=%d):\n\n", n);
    printf("%-12s %-10s %-10s %-10s\n", "Class", "Function", "Size", "Depth");
    printf("%-12s %-10s %-10s %-10s\n", "-----", "--------", "----", "-----");

    /* AC0: Threshold */
    AC0Circuit *ac0 = ac0_build_threshold(n, 3);
    if (ac0) {
        printf("%-12s %-10s %-10d %-10d\n", "AC0", "THR(3)",
               ac0_circuit_size(ac0), ac0_circuit_depth(ac0));
        ac0_circuit_free(ac0);
    }

    /* TC0: Majority */
    AC0Circuit *tc0 = tc0_build_majority(n);
    if (tc0) {
        printf("%-12s %-10s %-10d %-10d\n", "TC0", "MAJ",
               ac0_circuit_size(tc0), ac0_circuit_depth(tc0));
        ac0_circuit_free(tc0);
    }

    /* NC1: Parity */
    AC0Circuit *nc1 = nc1_build_parity(n);
    if (nc1) {
        printf("%-12s %-10s %-10d %-10d\n", "NC1", "PARITY",
               ac0_circuit_size(nc1), ac0_circuit_depth(nc1));
        ac0_circuit_free(nc1);
    }

    /* P/poly: Parity via truth table */
    AC0Circuit *pp = ppoly_build_parity(n > 4 ? 4 : n);
    if (pp) {
        printf("%-12s %-10s %-10d %-10d\n", "P/poly", "PARITY",
               ac0_circuit_size(pp), ac0_circuit_depth(pp));
        ac0_circuit_free(pp);
    }

    /* SCALING COMPARISON */
    printf("\nSCALING COMPARISON:\n\n");
    printf("%6s %14s %14s %14s %14s\n",
           "n", "AC0(THR)", "NC1(PAR)", "TC0(MAJ)", "P/poly(PAR)");
    printf("%6s %14s %14s %14s %14s\n",
           "-", "-------", "--------", "--------", "-----------");

    for (int nn = 4; nn <= 32; nn *= 2) {
        double ac0_sz = ac0_threshold_size_lower_bound(nn, nn/2);
        double nc1_sz = nc1_parity_size_upper_bound(nn);
        double tc0_sz = tc0_majority_size_upper_bound(nn);
        double pp_sz  = ppoly_min_size_upper_bound(nn);
        printf("%6d %14.2e %14.0f %14.0f %14.2e\n",
               nn, ac0_sz, nc1_sz, tc0_sz, pp_sz);
    }

    /* KNOWN INCLUSIONS */
    printf("\n");
    class_inclusion_demo();

    /* KNOWN SEPARATIONS */
    known_separations_print();

    /* OPEN PROBLEMS */
    open_problems_print();

    printf("\n====================================\n");
    printf("  Hierarchy exploration complete.\n");
    return 0;
}
