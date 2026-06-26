/* lower_bounds_example.c -- Circuit Lower Bound Exploration
 *
 * Demonstrates fundamental circuit lower bound techniques:
 *   - Shannon's counting argument (1949)
 *   - Lupanov's upper bound (1958) - shows tightness
 *   - Hastad Switching Lemma for AC0 lower bounds
 *   - Gate elimination method
 *   - Neciporuk's bound for formula size
 *   - Williams' connection: SAT algorithms => lower bounds
 *
 * Key insight: almost all Boolean functions are HARD (require
 * exponential size circuits), yet we can only prove lower bounds
 * for very restricted circuit classes.
 */

#include "csat.h"

static void print_table_header(void)
{
    printf("%-4s %-18s %-18s %-16s
",
           "n", "Shannon LB", "Lupanov UB", "2^n/#functions");
    printf("---- ------------------ ------------------ ----------------
");
}

static void print_restriction_demo(void)
{
    printf("
--- Hastad Switching Lemma: PARITY not in AC0 ---
");
    printf("Under random restriction with prob p, a DNF/CNF of
");
    printf("bottom fan-in t collapses to a decision tree with
");
    printf("high probability. This yields exponential lower bounds
");
    printf("for constant-depth circuits computing PARITY.

");

    printf("Depth  MinCircuitSize
");
    printf("-----  ---------------
");
    for (int d = 2; d <= 6; d++) {
        double b = switching_lemma_bound(10, d, 4);
        printf("  %d    2^{%.1f} = %.0f
", d,
               pow(10.0, 1.0 / (double)(d - 1)), b);
    }
}

static void print_gate_elimination(void)
{
    printf("
--- Gate Elimination Method ---
");
    printf("For a formula of size m and depth d on n variables:
");
    printf("Removing one gate eliminates at most its fan-in variables.
");
    printf("This gives: m >= exp(Omega(n^{1/d})).

");

    printf("Example bounds for n=100:
");
    for (int d = 2; d <= 5; d++) {
        double b = gate_elimination_bound(100, 1000, d);
        printf("  Depth %d: min size >= %.0f
", d, b);
    }
}

static void print_williams_connection(void)
{
    printf("
--- Williams Algorithmic Method (2010-2014) ---
");
    printf("Breakthrough: connects ALGORITHMS to LOWER BOUNDS.
");
    printf("If Circuit-SAT for class C has non-trivial algorithm,
");
    printf("then NEXP is not contained in C.

");

    printf("Known results:
");
    printf("  AC0-SAT  in 2^{n-n^{1/O(d)}}  => NEXP not in AC0  (Williams 2010)
");
    printf("  ACC0-SAT in 2^{n-n^epsilon}   => NEXP not in ACC0 (Williams 2014)
");
    printf("  TC0-SAT  in 2^{n-n^delta}     => NEXP not in TC0  (known)
");
    printf("  C-SAT    in 2^n/n^k           => NEXP not in P/poly (OPEN!)

");

    int n = 20;
    printf("Speedup comparison for n=%d:
", n);
    printf("  Baseline 2^n = %.0f
", pow(2.0, (double)n));
    printf("  / n^2        = %.0f
", csat_williams_speedup(n, 2));
    printf("  / n^3        = %.0f
", csat_williams_speedup(n, 3));
    printf("  ACC0 bound   = %.0f
", williams_speedup_estimate(n, 0, 1));
}

int main(void)
{
    setbuf(stdout, NULL);

    printf("================================================================
");
    printf("  Circuit Lower Bounds: A Computational Exploration
");
    printf("================================================================

");

    printf("Boolean functions on n inputs: 2^{2^n}
");
    printf("Circuits of size s: ~2^{O(s log s)}
");
    printf("This counting gap implies most functions are HARD.

");

    print_table_header();
    for (int n = 3; n <= 12; n++) {
        double lb = shannon_bound(n);
        double ub = lupanov_bound(n);
        double total_funcs = pow(2.0, pow(2.0, (double)n));
        printf("%-4d %-18.1f %-18.1f 2^2^%d=%.0f
",
               n, lb, ub, n, log2(total_funcs));
    }

    print_restriction_demo();
    print_gate_elimination();
    print_williams_connection();

    printf("
================================================================
");
    printf("  Summary: Proving lower bounds is HARD.
");
    printf("  Natural Proofs Barrier (Razborov-Rudich 1997):
");
    printf("  Most "natural" proof techniques would break cryptography.
");
    printf("  Williams' method is one of the few that CIRCUMVENTS this!
");
    printf("================================================================
");
    return 0;
}
