/* demo.c -- NC Hierarchy End-to-End Demonstration
 *
 * Demonstrates all key concepts of the NC hierarchy:
 *   L1: Circuit construction
 *   L2: Circuit evaluation and classification
 *   L4: Hierarchy theorems
 *   L5: Parallel prefix, addition, sorting, matrix ops
 *   L6: PARITY, MAJORITY, Formula Evaluation
 *   L7: VLSI bounds
 *
 * Reference: Pippenger (1979), Arora & Barak (2009)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nc.h"

int main(void) {
    setbuf(stdout, NULL);

    printf("================================================================
");
    printf("  NC HIERARCHY — NICK''S CLASS
");
    printf("  Comprehensive End-to-End Demonstration
");
    printf("================================================================
");

    /* Run all demos */
    nc_hierarchy_demo();
    nc_adder_demo();
    parallel_prefix_demo();
    formula_eval_demo();
    bitonic_sort_demo();
    matrix_mult_demo();
    graph_reachability_demo();
    nc_bench_demo();

    printf("
================================================================
");
    printf("  DEMO COMPLETE
");
    printf("================================================================
");
    return 0;
}
