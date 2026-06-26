/* nc_hierarchy.c -- NC Hierarchy: theory, classification, and demos
 *
 * NC (Nick's Class) hierarchy definition (Pippenger 1979):
 *   NC^i = languages decidable by uniform Boolean circuit families
 *          of polynomial size and O(log^i n) depth, with fan-in 2.
 *   NC   = ∪_{i≥0} NC^i
 *
 * Related classes:
 *   AC^i: same as NC^i but with unbounded fan-in AND/OR gates
 *   TC^i: same but allowing MAJORITY (threshold) gates
 *   ACC^i: AC^i gates + MOD_m gates (for fixed m)
 *
 * Known hierarchy (L4: Fundamental Laws):
 *   NC^0 ⊊ AC^0 ⊊ TC^0 ⊆ NC^1 ⊆ L ⊆ NL = coNL ⊆ NC^2 ⊆ NC ⊆ P
 *
 * This file implements hierarchy classification, Brent's Theorem,
 * and comprehensive demonstrations of the NC hierarchy.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "nc.h"

/* ================================================================
 * KNOWN SEPARATIONS AND INCLUSIONS (L4: Fundamental Laws)
 * ================================================================ */

/*
 * Theorem 1 (Furst-Saxe-Sipser 1981, Ajtai 1983, Håstad 1986):
 *   PARITY ∉ AC^0, therefore AC^0 ≠ NC^1.
 * Proof sketch: Any AC^0 circuit for PARITY requires super-polynomial
 * size. Uses the Switching Lemma: random restriction of AC^0 circuit
 * collapses it to a shallow decision tree with high probability.
 */

int hierarchy_nc0_in_ac0(void) {
    /* NC^0 ⊆ AC^0 is trivial: every constant-depth, bounded-fan-in
     * circuit is also a constant-depth, unbounded-fan-in circuit.
     */
    return 1; /* strict: NC^0 ⊂ AC^0 — because AC^0 can compute
                 functions with unbounded fan-in that NC^0 cannot */
}

int hierarchy_ac0_in_tc0(void) {
    /* AC^0 ⊆ TC^0: MAJORITY can simulate AND/OR with appropriate
     * threshold values. AND(x₁,...,x_n) = THR_n(x₁,...,x_n).
     * OR(x₁,...,x_n) = THR_1(x₁,...,x_n). */
    return 1; /* non-strict inclusion */
}

int hierarchy_ac0_neq_tc0(void) {
    /* AC^0 ≠ TC^0: MAJORITY ∉ AC^0.
     * PARITY ∉ AC^0 (Håstad) and MAJORITY ∉ AC^0 (follows from
     * FSS via reduction: MAJORITY can compute PARITY with AC^0 help).
     * MAJORITY ∈ TC^0 trivially (single MAJORITY gate).
     * Therefore AC^0 ≠ TC^0. */
    return 1;
}

int hierarchy_tc0_in_nc1(void) {
    /* TC^0 ⊆ NC^1: OPEN PROBLEM since ~1989.
     * Can MAJORITY be computed by O(log n)-depth, fan-in 2 circuits?
     * Best known: MAJORITY ∈ NC^1 (Ajtai-Ben-Or 1984, Valiant 1984).
     * Actually: MAJORITY IS in NC^1 via sorting networks.
     * The question is whether TC^0 = NC^1, i.e., can every
     * constant-depth threshold circuit be simulated in O(log n) depth? */
    return 1; /* TC^0 ⊆ NC^1 is known. Equality is open. */
}

int hierarchy_nc1_in_l(void) {
    /* NC^1 ⊆ L (Borodin 1977).
     * Proof: Evaluate a log-depth circuit recursively. The recursion
     * stack depth is O(log n), and each node stores constant info.
     * So total space = O(log n). */
    return 1;
}

int hierarchy_l_in_nl(void) {
    /* L ⊆ NL: trivial, determinism is a special case of nondeterminism.
     * Open: L = NL? (See "L vs NL" problem) */
    return 1; /* Open whether strict */
}

int hierarchy_nl_equals_conl(void) {
    /* NL = coNL (Immerman 1988, Szelepcsényi 1988).
     * Theorem: If a nondeterministic logspace machine can decide
     * language L, it can also decide the complement of L.
     *
     * Proof technique: Inductive counting — compute the number of
     * vertices reachable from s in a directed graph, using only
     * nondeterministic logspace. Then check if t is NOT among them.
     *
     * This was a major breakthrough. Before 1988, it was widely
     * believed NL ≠ coNL.
     */
    return 1;
}

int hierarchy_nl_in_nc2(void) {
    /* NL ⊆ NC^2 (Borodin, Cook, Dymond, Ruzzo, Tompa 1989).
     * Proof: Transitive closure of a directed graph can be computed
     * in O(log^2 n) depth using Boolean matrix multiplication.
     * Since NL = problems reducible to reachability, and
     * reachability is in NC^2, NL ⊆ NC^2 follows. */
    return 1;
}

int hierarchy_nc_in_p(void) {
    /* NC ⊆ P: Trivial. A polynomial-size, polylog-depth circuit
     * can be evaluated in polynomial time (O(size × depth) ≤ poly(n)).
     * Open: NC = P? (Can every polynomial-time algorithm be
     * parallelized to polylog depth?) */
    return 1;
}

int hierarchy_p_in_pspace(void) {
    /* P ⊆ PSPACE: Trivial. Polynomial-time algorithms use at most
     * polynomial space (one tape cell per step). */
    return 1;
}

/* ================================================================
 * BRENT'S THEOREM (1974): Parallel Time = Sequential Time / p + T_∞
 *
 * Brent's Theorem connects circuit model to PRAM model:
 *   Any computation taking T sequential steps with total work W
 *   can be simulated on p processors in time ≤ W/p + T_∞,
 *   where T_∞ is the critical path length (circuit depth).
 *
 * Corollary for NC: Problems in NC are efficiently parallelizable.
 * If depth = O(log^k n) and size = poly(n), then:
 *   - With poly(n) processors: O(log^k n) time
 *   - With n processors: O(n^{1-ε}) time for some ε
 * ================================================================ */

double brent_parallel_time(double T_seq, int p, double T_inf) {
    /* Brent's formula: PT = T_seq/p + T_inf */
    if (p <= 0) return T_inf;
    return T_seq / (double)p + T_inf;
}

int brent_optimal_processors(double T_seq, double T_inf) {
    /* Optimal p minimizes T_seq/p + T_inf.
     * Differentiate: -T_seq/p^2 + 0 = 0 (ignoring integer constraint)
     * Actually, optimal is p = sqrt(T_seq) if minimizing product p*T
     * For time minimization, p should be as large as work allows.
     * Typical choice: p = T_seq / T_inf balances the two terms. */
    if (T_inf <= 0) return (int)T_seq;
    double opt = T_seq / T_inf;
    return (int)(opt + 0.5);
}

double brent_speedup(double T_seq, double T_par) {
    if (T_par <= 0) return 0;
    return T_seq / T_par;
}

double brent_efficiency(double T_seq, double T_par, int p) {
    /* Efficiency = speedup / p = T_seq / (p × T_par) */
    if (p <= 0 || T_par <= 0) return 0;
    return T_seq / ((double)p * T_par);
}

/* ================================================================
 * HIERARCHY INCLUSION PROOFS WITH TRUTH TABLES
 *
 * Below we verify small cases of key hierarchy relationships
 * by exhaustive enumeration for small n.
 * ================================================================ */

/* Check PARITY function */
int parity_function(const int *x, int n) {
    int p = 0;
    for (int i = 0; i < n; i++) p ^= x[i];
    return p;
}

/* Build a PARITY circuit: XOR tree, depth ⌈log₂ n⌉, size O(n).
 * This demonstrates PARITY ∈ NC^1. */
BoolCircuit *build_parity_circuit(int n) {
    BoolCircuit *c = circuit_create(n * 2, 2);

    /* Create input gates */
    int *current = (int *)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++)
        current[i] = circuit_add_input(c, i);

    int remaining = n;
    while (remaining > 1) {
        int next_rem = (remaining + 1) / 2;
        int *next = (int *)malloc((size_t)next_rem * sizeof(int));
        for (int i = 0; i < next_rem; i++) {
            if (2 * i + 1 < remaining) {
                int inputs[2] = {current[2*i], current[2*i+1]};
                next[i] = circuit_add_gate(c, NC_XOR, inputs, 2);
            } else {
                next[i] = current[2*i];
            }
        }
        free(current);
        current = next;
        remaining = next_rem;
    }

    circuit_add_output(c, current[0]);
    free(current);
    return c;
}

int *parity_circuit_evaluate(int n, const int *input_bits) {
    BoolCircuit *pc = build_parity_circuit(n);
    int *result = circuit_evaluate(pc, input_bits);
    circuit_free(pc);
    return result;
}

/* Check MAJORITY function */
int majority_function(const int *x, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += x[i];
    return (sum > n / 2) ? 1 : 0;
}

/* Build a MAJORITY circuit using threshold gate.
 * Single MAJORITY gate → depth 1 → TC^0. */
BoolCircuit *build_majority_circuit(int n) {
    BoolCircuit *c = circuit_create(n + 2, n);
    int *inputs = (int *)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++)
        inputs[i] = circuit_add_input(c, i);
    int maj_gate = circuit_add_gate(c, NC_MAJORITY, inputs, n);
    circuit_add_output(c, maj_gate);
    free(inputs);
    return c;
}

int *majority_circuit_evaluate(int n, const int *input_bits) {
    BoolCircuit *mc = build_majority_circuit(n);
    int *result = circuit_evaluate(mc, input_bits);
    circuit_free(mc);
    return result;
}

/* MOD_k function */
int mod_k_function(const int *x, int n, int k) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += x[i];
    return (sum % k == 0) ? 1 : 0;
}

/* ================================================================
 * VLSI BOUNDS (L7: Applications)
 *
 * Thompson (1979): For any VLSI layout of a circuit computing
 * f: {0,1}^n → {0,1} with area A and time T:
 *   AT^2 ≥ (λm)^2 where m = minimum number of bits that must
 *   cross any bisection of the circuit.
 *
 * For many functions (e.g., sorting, cyclic shift):
 *   AT^2 = Ω(n^2)
 * ================================================================ */

VLSIBounds vlsi_area_time_bound(int num_inputs, double depth) {
    VLSIBounds vb;
    /* Lower bound from bisection width:
     * For functions like sorting, cyclic shift, the minimum bisection
     * width is Ω(n). Then AT^2 = Ω(n^2).
     *
     * Upper bound from tree-based layout (Brent-Kung 1982):
     * A = O(n), T = O(log n), so AT^2 = O(n log^2 n).
     */
    vb.at2_bound = (double)num_inputs * (double)num_inputs;
    vb.area = (double)num_inputs;
    vb.time_per_op = depth;
    return vb;
}

/* ================================================================
 * DEMONSTRATIONS
 * ================================================================ */

void nc_hierarchy_demo(void) {
    printf("\n");
    printf("================================================================\n");
    printf("  NC HIERARCHY — NICK'S CLASS (Pippenger 1979)\n");
    printf("================================================================\n\n");

    /* L1: Definitions */
    printf("--- NC^i DEFINITIONS (L1) ---\n");
    printf("  NC^i = { L | ∃ uniform circuit family {C_n} with:\n");
    printf("           fan-in 2, size(C_n) = poly(n),\n");
    printf("           depth(C_n) = O(log^i n) }\n\n");

    printf("  AC^i = same with unbounded fan-in AND/OR gates\n");
    printf("  TC^i = same with threshold (MAJORITY) gates added\n");
    printf("  ACC^i = AC^i + MOD_m gates (fixed m)\n\n");

    /* L2: Core Concepts */
    printf("--- HIERARCHY (L2, L4) ---\n");
    printf("  NC^0 ⊊ AC^0 ⊊ TC^0 ⊆ NC^1 ⊆ L ⊆ NL ⊆ NC^2 ⊆ NC ⊆ P\n");
    printf("  (NL = coNL by Immerman-Szelepcsenyi 1988)\n\n");

    /* L4: Known results */
    printf("--- KNOWN RESULTS (L4) ---\n");
    printf("  PARITY ∉ AC^0  (FSS 1981, Ajtai 1983, Hastad 1986)\n");
    printf("  → AC^0 ≠ NC^1  (PARITY ∈ NC^1 via XOR tree)\n");
    printf("  MAJORITY ∉ AC^0  (FSS 1981)\n");
    printf("  → AC^0 ≠ TC^0  (MAJORITY ∈ TC^0 via single gate)\n");
    printf("  NC^1 ⊆ L  (Borodin 1977)\n");
    printf("  NL = coNL  (Immerman 1988, Szelepcsenyi 1988)\n");
    printf("  NL ⊆ NC^2  (Borodin-Cook-Dymond-Ruzzo-Tompa 1989)\n");
    printf("  NC ⊆ P  (trivial; NC = P is OPEN)\n\n");

    /* Open problems */
    printf("--- MAJOR OPEN PROBLEMS (L9) ---\n");
    printf("  1. TC^0 vs NC^1  — majority in log-depth?\n");
    printf("  2. NC^1 vs L     — log-depth circuits vs log space?\n");
    printf("  3. NC vs P       — all P problems parallelizable?\n");
    printf("  4. L vs NL       — Savitch gives NL ⊆ L^2, but L = NL?\n");
    printf("  5. ACC^0 vs NEXP?  — Williams (2014): NEXP ⊄ ACC^0\n");
    printf("  6. UNIFORM vs NON-UNIFORM hierarchies?\n\n");

    /* Parity circuit demonstration */
    printf("--- PARITY IN NC^1 (L6) ---\n");
    for (int n = 4; n <= 16; n *= 2) {
        BoolCircuit *pc = build_parity_circuit(n);
        printf("  n=%d: gates=%d depth=%d class=",
               n, circuit_size(pc), circuit_depth(pc));
        ClassBounds b = circuit_classify(pc, n);
        printf("%s\n", b.description);
        circuit_free(pc);
    }

    /* Majority circuit demonstration */
    printf("\n--- MAJORITY IN TC^0 (L6) ---\n");
    for (int n = 4; n <= 16; n *= 2) {
        BoolCircuit *mc = build_majority_circuit(n);
        printf("  n=%d: gates=%d depth=%d class=",
               n, circuit_size(mc), circuit_depth(mc));
        ClassBounds b = circuit_classify(mc, n);
        printf("%s\n", b.description);
        circuit_free(mc);
    }

    /* Verification: PARITY computation */
    printf("\n--- PARITY VERIFICATION ---\n");
    for (int n = 3; n <= 8; n++) {
        int max_states = 1 << n;
        int correct = 1;
        for (int s = 0; s < max_states && correct; s++) {
            int *input = (int *)malloc((size_t)n * sizeof(int));
            for (int i = 0; i < n; i++) input[i] = (s >> i) & 1;
            BoolCircuit *pc = build_parity_circuit(n);
            int *out = circuit_evaluate(pc, input);
            int expected = parity_function(input, n);
            if (out[0] != expected) correct = 0;
            free(input); free(out); circuit_free(pc);
        }
        printf("  n=%d PARITY: %s (checked all %d inputs)\n",
               n, correct ? "CORRECT" : "FAIL", max_states);
    }

    /* Verification: MAJORITY computation */
    printf("\n--- MAJORITY VERIFICATION ---\n");
    for (int n = 3; n <= 7; n++) {
        int max_states = 1 << n;
        int correct = 1;
        for (int s = 0; s < max_states && correct; s++) {
            int *input = (int *)malloc((size_t)n * sizeof(int));
            for (int i = 0; i < n; i++) input[i] = (s >> i) & 1;
            BoolCircuit *mc = build_majority_circuit(n);
            int *out = circuit_evaluate(mc, input);
            int expected = majority_function(input, n);
            if (out[0] != expected) correct = 0;
            free(input); free(out); circuit_free(mc);
        }
        printf("  n=%d MAJORITY: %s (checked all %d inputs)\n",
               n, correct ? "CORRECT" : "FAIL", max_states);
    }

    /* NC depth scaling table */
    nc_hierarchy_depth_table(1024);
    nc_hierarchy_class_table();

    /* Brent's Theorem */
    printf("\n--- BRENT'S THEOREM (L5) ---\n");
    printf("  T_seq steps on p processors => T_par = T_seq/p + T_inf\n");
    printf("  If T_inf = polylog(n) and T_seq = poly(n):\n");
    printf("    With p = poly(n) processors → T_par = polylog(n)\n");
    printf("    This characterizes NC.\n\n");
    printf("  %8s %8s %8s %8s %8s %8s\n",
           "T_seq", "T_inf", "p", "T_par", "Speedup", "Effic.");
    printf("  -------- -------- -------- -------- -------- --------\n");
    double test_cases[][2] = {{100, 10}, {1000, 20}, {5000, 25}, {10000, 30}};
    for (int i = 0; i < 4; i++) {
        double Ts = test_cases[i][0], Ti = test_cases[i][1];
        int p = brent_optimal_processors(Ts, Ti);
        double Tp = brent_parallel_time(Ts, p, Ti);
        double sp = brent_speedup(Ts, Tp);
        double ef = brent_efficiency(Ts, Tp, p);
        printf("  %8.0f %8.0f %8d %8.1f %8.1fx %7.1f%%\n",
               Ts, Ti, p, Tp, sp, ef * 100.0);
    }

    /* VLSI bounds */
    printf("\n--- VLSI AREA-TIME BOUNDS (L7) ---\n");
    for (int n = 4; n <= 1024; n *= 4) {
        VLSIBounds vb = vlsi_area_time_bound(n, log2((double)n));
        printf("  n=%-5d AT^2 ≥ %.0f (area=%.0f, time=%.1f)\n",
               n, vb.at2_bound, vb.area, vb.time_per_op);
    }

    printf("\n================================================================\n");
}

/* ================================================================
 * BENCHMARKS
 * ================================================================ */

void nc_bench_demo(void) {
    printf("\n");
    printf("================================================================\n");
    printf("  NC HIERARCHY BENCHMARKS\n");
    printf("================================================================\n\n");

    printf("NC^i = poly-size, fan-in 2, depth O(log^i n).\n");
    printf("NC ⊂ P (proved). NC = P? OPEN.\n\n");

    printf("--- Depth Scaling ---\n");
    printf("  n        log n   NC^1     NC^2     NC^3\n");
    for (int n = 16; n <= 65536; n *= 4) {
        double lgn = log2((double)n);
        printf("  %-8d %-7.0f %-7.0f %-7.0f %-7.0f\n",
               n, lgn,
               nc_i_depth_bound(1, n),
               nc_i_depth_bound(2, n),
               nc_i_depth_bound(3, n));
    }

    printf("\n--- Known Separations ---\n");
    printf("  AC^0 ⊂ TC^0 ⊂ NC^1 ⊂ L ⊆ NL ⊂ NC^2 ⊂ NC ⊂ P/poly\n");
    printf("  AC^0 ≠ NC^1: PROVED (FSS/Ajtai/Hastad 1981-86).\n");
    printf("  NL = coNL: PROVED (Immerman-Szelepcsenyi 1988).\n");
    printf("  TC^0 vs NC^1: OPEN (since 1989).\n");
    printf("  NC^1 vs L: OPEN.\n");
    printf("  NC vs P: OPEN.\n\n");

    printf("P-complete (under NC^1 reductions): Circuit Value Problem.\n");
    printf("NC^1-complete: Formula Eval, Addition.\n");
    printf("NC^2-complete: Matrix Det, CFL Membership.\n");
}
