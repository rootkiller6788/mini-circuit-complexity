/*============================================================================
 * razborov.c - Main Demo Entry Point (Razborov-Smolensky)
 *
 * Demonstrates the polynomial method for AC0[p] circuit lower bounds:
 *   1. GF(p) polynomial arithmetic
 *   2. Gate approximation by low-degree polynomials
 *   3. Circuit-to-polynomial translation
 *   4. MAJORITY degree lower bound
 *   5. Size lower bound computation
 *   6. Circuit class separations
 *
 * Nevanlinna Prize 1990 — A.A. Razborov
 *
 * References:
 *   Razborov, Mat. Zametki (1987)
 *   Smolensky, STOC (1987)
 *   Arora & Barak, "Computational Complexity", Ch.14.4
 *==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "razborov.h"

/*----------------------------------------------------------------------------
 * poly_arithmetic_demo — Exercise the GF(p) polynomial library
 *----------------------------------------------------------------------------*/
void poly_arithmetic_demo(void) {
    printf("\n================================================================\n");
    printf("  DEMO 1: GF(p) POLYNOMIAL ARITHMETIC\n");
    printf("================================================================\n\n");

    int p = 3;
    printf("Working over GF(%d)\n\n", p);

    /* Create polynomials */
    GFPoly* x0 = gf_poly_variable(p, 4, 0);
    GFPoly* x1 = gf_poly_variable(p, 4, 1);

    printf("--- Variable polynomials ---\n");
    printf("  x0 = ");
    gf_poly_print(x0);
    printf("\n  x1 = ");
    gf_poly_print(x1);
    printf("\n\n");

    /* Addition */
    GFPoly* s = gf_poly_add(x0, x1);
    printf("--- Addition: x0 + x1 = ");
    gf_poly_print(s);
    printf(" (degree=%d)\n\n", gf_poly_degree(s));

    /* Multiplication */
    GFPoly* m = gf_poly_mul(x0, x1);
    printf("--- Multiplication: x0 * x1 = ");
    gf_poly_print(m);
    printf(" (degree=%d)\n\n", gf_poly_degree(m));

    /* Evaluation */
    int test_inputs[4][4] = {
        {0, 0, 0, 0},
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {1, 1, 0, 0}
    };
    printf("--- Evaluation ---\n");
    for (int i = 0; i < 4; i++) {
        printf("  f(%d,%d,%d,%d) = %d\n",
               test_inputs[i][0], test_inputs[i][1],
               test_inputs[i][2], test_inputs[i][3],
               gf_poly_eval(s, test_inputs[i]));
    }
    printf("\n");

    /* Scalar multiplication */
    GFPoly* s2 = gf_poly_scalar_mul(x0, 2);
    printf("  2 * x0 = ");
    gf_poly_print(s2);
    printf(" (mod %d)\n\n", p);

    /* Power */
    GFPoly* x0p2 = gf_poly_pow(x0, 2);
    printf("  x0^2 = ");
    gf_poly_print(x0p2);
    printf(" (degree=%d, over GF(%d), x^2=x for Boolean domain)\n\n",
           gf_poly_degree(x0p2), p);

    /* Random polynomial */
    GFPoly* rnd = gf_poly_random(p, 5, 3, 5, 42);
    printf("--- Random polynomial (max_deg=3, max_terms=5) ---\n");
    printf("  ");
    gf_poly_print(rnd);
    printf("\n  degree=%d, terms=%d\n\n", gf_poly_degree(rnd), gf_poly_n_terms(rnd));

    /* Cleanup */
    gf_poly_free(x0);  gf_poly_free(x1);
    gf_poly_free(s);   gf_poly_free(m);
    gf_poly_free(s2);  gf_poly_free(x0p2);
    gf_poly_free(rnd);
}

/*----------------------------------------------------------------------------
 * gf_poly_sim_demo — Simulate circuit gates with polynomials
 *----------------------------------------------------------------------------*/
void gf_poly_sim_demo(void) {
    printf("\n================================================================\n");
    printf("  DEMO 2: GATE POLYNOMIAL SIMULATION\n");
    printf("================================================================\n\n");

    int p = 3;
    printf("Working over GF(%d)\n\n", p);

    /* AND gate */
    int vars_and[] = {0, 1, 2};
    GFPoly* and_exact = poly_and_exact(p, vars_and, 3);
    printf("--- AND(x0, x1, x2) exact ---\n");
    printf("  polynomial: "); gf_poly_print(and_exact);
    printf("  degree=%d\n\n", gf_poly_degree(and_exact));

    int test[4][3] = {{0,0,0}, {1,0,0}, {1,1,0}, {1,1,1}};
    for (int t = 0; t < 4; t++) {
        int expected = test[t][0] && test[t][1] && test[t][2];
        int got = gf_poly_eval(and_exact, test[t]);
        printf("  AND(%d,%d,%d) = %d (expected %d) %s\n",
               test[t][0], test[t][1], test[t][2], got, expected,
               got == expected ? "OK" : "FAIL");
    }
    gf_poly_free(and_exact);

    /* OR gate */
    int vars_or[] = {0, 1};
    GFPoly* or_exact = poly_or_exact(p, vars_or, 2);
    printf("\n--- OR(x0, x1) exact ---\n");
    printf("  polynomial: "); gf_poly_print(or_exact);
    printf("  degree=%d, terms=%d\n\n", gf_poly_degree(or_exact),
           gf_poly_n_terms(or_exact));
    int test2[4][2] = {{0,0}, {0,1}, {1,0}, {1,1}};
    for (int t = 0; t < 4; t++) {
        int expected = test2[t][0] || test2[t][1];
        int got = gf_poly_eval(or_exact, test2[t]);
        printf("  OR(%d,%d) = %d (expected %d) %s\n",
               test2[t][0], test2[t][1], got, expected,
               got == expected ? "OK" : "FAIL");
    }
    gf_poly_free(or_exact);

    /* NOT gate */
    GFPoly* not_x0 = poly_not_exact(p, 0, 2);
    printf("\n--- NOT(x0) ---\n");
    printf("  polynomial: "); gf_poly_print(not_x0);
    printf("\n  NOT(0) = %d, NOT(1) = %d\n",
           gf_poly_eval(not_x0, (int[]){0,0}),
           gf_poly_eval(not_x0, (int[]){1,0}));
    gf_poly_free(not_x0);

    /* MOD_p gate */
    int vars_mod[] = {0, 1, 2, 3};
    GFPoly* mod3 = poly_modp_exact(p, vars_mod, 4);
    printf("\n--- MOD3(x0, x1, x2, x3) ---\n");
    printf("  degree=%d, terms=%d\n", gf_poly_degree(mod3),
           gf_poly_n_terms(mod3));
    for (int mask = 0; mask < 8; mask++) {
        int x[4] = { (mask>>0)&1, (mask>>1)&1, (mask>>2)&1, (mask>>3)&1 };
        int sum = x[0]+x[1]+x[2]+x[3];
        int expected = (sum % 3 == 0) ? 1 : 0;
        int got = gf_poly_eval(mod3, x);
        printf("  MOD3(%d,%d,%d,%d) = %d (sum=%d, expected %d) %s\n",
               x[0], x[1], x[2], x[3], got, sum, expected,
               got == expected ? "OK" : "FAIL");
    }
    gf_poly_free(mod3);

    /* Probabilistic AND */
    printf("\n--- Probabilistic AND(x0,x1,x2,x3) (seed=42) ---\n");
    GFPoly* prob_and = poly_and_probabilistic(p, vars_mod, 4, 42);
    printf("  degree=%d (constant for fixed p!)\n", gf_poly_degree(prob_and));
    for (int mask = 0; mask < 8; mask++) {
        int x[4] = { (mask>>0)&1, (mask>>1)&1, (mask>>2)&1, (mask>>3)&1 };
        int expected = x[0] && x[1] && x[2] && x[3];
        int got = gf_poly_eval(prob_and, x);
        printf("  ProbAND(%d,%d,%d,%d) = %d (expected %d) %s\n",
               x[0], x[1], x[2], x[3], got, expected,
               got == expected ? "OK" : "approx");
    }
    gf_poly_free(prob_and);
}

/*----------------------------------------------------------------------------
 * mod_collision_demo — MOD gate collision analysis
 *----------------------------------------------------------------------------*/
void mod_collision_demo(void) {
    printf("\n================================================================\n");
    printf("  DEMO 3: MOD GATE COLLISION ANALYSIS\n");
    printf("================================================================\n\n");

    printf("Key observation of Razborov-Smolensky:\n\n");
    printf("  MOD_p over GF(p):  degree = p-1 = O(1)  [Fermat]\n");
    printf("  MOD_p over GF(q), q != p:  degree = Omega(n)  [HIGH]\n\n");

    printf("This asymmetry drives the separation:\n");
    printf("  AC0[p] can express MOD_p cheaply (degree p-1)\n");
    printf("  AC0[p] CANNOT express MOD_q cheaply (needs degree Omega(n))\n");
    printf("  => AC0[p] != AC0[q] for distinct primes p, q.\n\n");

    printf("--- Degree comparison table (n=64) ---\n");
    printf("  Function      over GF(2)  over GF(3)  over GF(5)\n");
    printf("  MAJORITY      deg=1       deg~=2.7     deg~=2.0\n");
    printf("  PARITY/MOD2   deg=1       deg~=16      deg~=16\n");
    printf("  MOD3          deg~=16     deg=2        deg~=16\n");
    printf("  MOD5          deg~=16     deg~=16      deg=4\n\n");

    printf("The p=2 exception:\n");
    printf("  Over GF(2), x^2 = x (Boolean identity).\n");
    printf("  PARITY = x1 + x2 + ... + xn (mod 2) has DEGREE 1!\n");
    printf("  MAJORITY over GF(2) also has low degree.\n");
    printf("  => AC0[2] CAN compute MAJORITY and PARITY.\n");
    printf("  => Razborov-Smolensky works only for primes p != 2.\n\n");

    printf("This is why ACC0 (which includes MOD_6 = MOD_2 AND MOD_3)\n");
    printf("is harder to separate from NC1 than AC0[p].\n");
}

/*----------------------------------------------------------------------------
 * rs_bench_demo — Lower bound benchmarks
 *----------------------------------------------------------------------------*/
void rs_bench_demo(void) {
    printf("\n================================================================\n");
    printf("  DEMO 4: LOWER BOUND COMPUTATION\n");
    printf("================================================================\n\n");

    int p = 3;
    printf("GF(%d) — MAJORITY lower bound analysis\n\n", p);

    printf("--- MAJORITY Degree Lower Bound ---\n");
    printf("  deg >= sqrt(n * ln(1/eps)) / 3\n\n");
    printf("  n       eps=0.1    eps=0.01   eps=0.001\n");
    for (int n = 16; n <= 256; n *= 4) {
        printf("  %-7d %-10.2f %-10.2f %-10.2f\n",
               n,
               rs_deg_lower_bound(n, 0.1),
               rs_deg_lower_bound(n, 0.01),
               rs_deg_lower_bound(n, 0.001));
    }

    printf("\n--- AC0[p] Degree Upper Bound ---\n");
    printf("  degree <= (c * log(S/eps))^d\n\n");
    printf("  depth  size=100  size=1K   size=10K  size=100K\n");
    for (int d = 1; d <= 4; d++) {
        printf("  %-6d %-9.1f %-9.1f %-9.1f %-9.1f\n",
               d,
               rs_circuit_degree_upper(d, 100, 0.1, p),
               rs_circuit_degree_upper(d, 1000, 0.1, p),
               rs_circuit_degree_upper(d, 10000, 0.1, p),
               rs_circuit_degree_upper(d, 100000, 0.1, p));
    }

    printf("\n--- Razborov Size Lower Bound ---\n");
    printf("  S >= exp(n^{1/(2d)} / 5)\n\n");
    printf("  n       d=2         d=3         d=4\n");
    for (int n = 16; n <= 256; n *= 2) {
        printf("  %-7d %-11.1e %-11.1e %-11.1e\n",
               n,
               rs_size_lower_bound(n, 2, p),
               rs_size_lower_bound(n, 3, p),
               rs_size_lower_bound(n, 4, p));
    }

    printf("\n--- Smolensky Size Lower Bound (p=3) ---\n");
    printf("  S >= 2^{c_p * n^{1/(2d)}}\n\n");
    printf("  n       d=2         d=3\n");
    for (int n = 16; n <= 256; n *= 4) {
        printf("  %-7d %-11.1e %-11.1e\n",
               n,
               rs_size_lower_bound_smolensky(n, 2, p),
               rs_size_lower_bound_smolensky(n, 3, p));
    }

    printf("\n--- Nontriviality Check ---\n");
    for (int n = 16; n <= 256; n *= 4) {
        for (int d = 2; d <= 4; d++) {
            int nt = rs_bound_is_nontrivial(n, d, p);
            printf("  n=%d d=%d: %s\n", n, d, nt ? "NONTRIVIAL" : "trivial");
        }
    }

    printf("\n--- Crossover Point ---\n");
    int xover = rs_find_crossover(4, 1000, 2, p, 3);
    if (xover > 0) {
        printf("  d=2: size_lb > n^3 at n >= %d\n", xover);
    }
}

/*----------------------------------------------------------------------------
 * rs_history_demo — Historical context and theorem overview
 *----------------------------------------------------------------------------*/
void rs_history_demo(void) {
    printf("\n================================================================\n");
    printf("  DEMO 5: HISTORICAL CONTEXT\n");
    printf("================================================================\n\n");

    printf("Timeline of AC0 circuit lower bounds:\n\n");
    printf("  1981: Furst, Saxe, Sipser — AC0 cannot compute PARITY\n");
    printf("        (random restrictions method)\n\n");
    printf("  1985: Razborov — super-polynomial lower bounds for\n");
    printf("        MONOTONE circuits (Nevanlinna Prize 1990)\n\n");
    printf("  1987: Hastad — Switching Lemma: depth-d AC0 requires\n");
    printf("        size exp(Omega(n^{1/(d-1)})) for PARITY\n\n");
    printf("  1987: Razborov — AC0[p] lower bounds via polynomial method\n");
    printf("        MAJORITY not in AC0[p] for p != 2\n\n");
    printf("  1987: Smolensky — extended to MOD_p gates, proved\n");
    printf("        AC0[p] != AC0[q] for distinct primes p, q\n\n");
    printf("  1990: Razborov awarded Nevanlinna Prize\n\n");
    printf("  1997: Razborov, Rudich — Natural Proofs barrier\n\n");
    printf("  2014: Williams — NEXP not in ACC0 (different method)\n\n");

    printf("The polynomial method remains one of the few successful\n");
    printf("techniques for proving circuit lower bounds.\n\n");
    printf("Key equation (MOD_p via Fermat):\n");
    printf("  MOD_p(x) = 1 - (x_1 + ... + x_n)^{p-1}  (mod p)\n");
    printf("  Degree = p-1 = O(1) for fixed prime p.\n");
}

/*----------------------------------------------------------------------------
 * razborov_smolensky_demo — Master demo
 *----------------------------------------------------------------------------*/
void razborov_smolensky_demo(void) {
    printf("\n================================================================\n");
    printf("  RAZBOROV-SMOLENSKY POLYNOMIAL METHOD\n");
    printf("  Nevanlinna Prize 1990\n");
    printf("================================================================\n");
    printf("\nCentral result: MAJORITY is NOT in AC0[p] for any prime p != 2.\n");
    printf("Method: replace circuit gates by low-degree GF(p) polynomials.\n\n");

    poly_arithmetic_demo();
    gf_poly_sim_demo();
    mod_collision_demo();
    rs_bench_demo();
    rs_print_separation_table();
    rs_history_demo();
    rs_print_acc0_frontier();
}
