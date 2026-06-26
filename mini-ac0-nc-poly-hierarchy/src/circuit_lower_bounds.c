/* circuit_lower_bounds.c — Lower Bound Methods in Circuit Complexity
 * =====================================================================
 * Three main methods for proving circuit lower bounds:
 *
 * 1. Shannon Counting (1949):
 *    Most Boolean functions need exponential-size circuits.
 *    #circuits of size S << #functions
 *
 * 2. Gate Elimination:
 *    Remove one gate at a time, analyze how many functions survive.
 *
 * 3. Hastad Switching Lemma (1986):
 *    Random restriction converts depth-d AC0 circuit to
 *    shallow decision tree with high probability.
 *
 * 4. Razborov's Approximation Method (1989):
 *    For monotone circuits: approximate with small DNF/CNF.
 *    Proved CLIQUE requires exponential monotone circuit size.
 *
 * 5. Natural Proofs Barrier (Razborov-Rudich 1997):
 *    Most known lower bound proofs are "natural" and cannot
 *    separate P from NP under standard cryptographic assumptions.
 *
 * References:
 *   Shannon (1949) "The Synthesis of Two-Terminal Switching Circuits"
 *   Hastad (1986) "Computational Limitations of Small-Depth Circuits"
 *   Razborov (1989) "On the Method of Approximation"
 *   Razborov & Rudich (1997) "Natural Proofs"
 */
#include "ac0nc.h"
#include <math.h>

/* ===== Shannon Counting =====
 * The most fundamental lower bound: count how many distinct
 * Boolean functions exist vs how many circuits of given size.
 *
 * Number of Boolean functions on n inputs: 2^{2^n}
 * Number of circuits of size S: ~ (S * (n+S)^2)^S
 *
 * For S = o(2^n / n), only a negligible fraction of functions
 * have circuits of size S.
 *
 * Corollary: almost all functions require circuits of size
 * at least Omega(2^n / n). This is the BEST known lower bound
 * for general circuits (no known explicit function with
 * superlinear circuit lower bound!). */

double shannon_circuit_count(int n_inputs, int size)
{
    /* Upper bound on number of circuits with n_inputs and given size.
     * Each gate: type (4 choices), 2 inputs (from n+size choices),
     * so <= (4 * (n_inputs + size)^2)^size circuits total.
     *
     * Returns log10 of this bound for numerical stability. */
    if (n_inputs <= 0 || size <= 0) return 1.0;
    double choices_per_gate = 4.0 * (n_inputs + size) * (n_inputs + size);
    return pow(choices_per_gate, (double)size);
}

double shannon_lower_bound(int n, double epsilon)
{
    /* Shannon (1949): to compute a 1-epsilon fraction of all
     * Boolean functions on n inputs, need size at least
     * (1-epsilon) * 2^n / n.
     *
     * For epsilon=0.5, most functions need size ~ 2^n / (2n). */
    if (n <= 0) return 0;
    double total_functions = pow(2.0, pow(2.0, (double)n));
    double needed = total_functions * (1.0 - epsilon);
    /* Solve for minimal S where #circuits(S) >= needed */
    double S = 1.0;
    while (shannon_circuit_count(n, (int)S) < needed && S < 1e10)
        S *= 1.1;
    return S;
}

void shannon_counting_demo(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  SHANNON COUNTING ARGUMENT (1949)\n");
    printf("  Almost all functions require exponential circuits\n");
    printf("==========================================================\n\n");

    printf("Counting argument:\n");
    printf("  Total Boolean functions on n inputs: 2^{2^n}\n");
    printf("  Total circuits of size S: ~ (O(S*(n+S)^2))^S\n\n");

    printf("  %-6s %-20s %-12s\n", "n", "#Functions", "#Circuits(S=n^3)");
    printf("  %-6s %-20s %-12s\n", "----", "-----------", "----------------");
    for (int n = 2; n <= 8; n += 2) {
        double S = n * n * n;
        double nf = pow(2.0, pow(2.0, (double)n));
        double nc = shannon_circuit_count(n, (int)S);
        printf("  %-6d %-20.4e %-12.4e\n", n, nf, nc);
    }

    printf("\n  Conclusion: Most functions need size >= 2^n / n\n");
    printf("  BUT: No explicit function known with superlinear lower bound!\n");
    printf("  The best explicit lower bound is ~ 3n - o(n) (Blum 1984).\n");
    printf("  This is the circuit complexity GAP. Natural proofs explain why.\n");
}

/* ===== Gate Elimination =====
 * Idea: start with an optimal circuit, remove one gate, count
 * how many functions become inexpressible.
 *
 * For each gate removal, the number of functions computable by
 * the remaining circuit is limited. If the original computes F,
 * and removing any gate destroys too many functions, then circuit
 * must be large. */

double gate_elimination_bound(int n, int removed_gates)
{
    /* After removing r gates, circuit becomes at most a (size-r) circuit.
     * So number of functions computable <= #circuits(size-r).
     * For the original circuit to compute a specific function,
     * it must have size >= r + something.
     *
     * This yields ~ 3n lower bound for many explicit functions. */
    if (n <= 0) return 0;
    int base_size = 3 * n;
    if (removed_gates >= base_size) return 0;
    int remaining = base_size - removed_gates;
    return shannon_circuit_count(n, remaining);
}

/* ===== Hastad Switching Lemma =====
 * Let f be a k-DNF formula. Apply random restriction where
 * each variable is set to 0/1/* with probability:
 *   - Pr[set to *] = rho (keep variable)
 *   - Pr[set to 0] = (1-rho)/2
 *   - Pr[set to 1] = (1-rho)/2
 *
 * Then with high probability, f|restriction can be represented
 * as a decision tree of low depth.
 *
 * Lemma (Hastad 1986):
 *   Pr[f|rho not representable as depth-t decision tree]
 *     <= (C * k * rho)^t
 *   where C is a universal constant.
 *
 * This lemma is the key tool for proving:
 *   1. PARITY not in AC0
 *   2. AC0 depth hierarchy is strict
 *   3. AC0 cannot compute MAJORITY (weak version) */

double hastad_switching_prob(int n, double rho, int depth)
{
    /* Simplified: probability that random restriction simplifies
     * a depth-depth AC0 circuit to shallow decision tree.
     * Rho = fraction of variables kept unassigned.
     * Higher rho = harder to simplify (more variables remain).
     *
     * The lemma gives: error_prob <= (c * k * rho)^t
     * where k = fan-in, t = target tree depth, c ~ 10. */
    if (n <= 0 || rho <= 0 || rho > 1.0 || depth <= 0) return 1.0;
    double k = n; /* worst-case fan-in = n */
    double t = (double)depth;
    double c = 10.0;
    double prob = pow(c * k * rho, -t);
    return prob > 1.0 ? 1.0 : prob;
}

/* ===== Depth-Size Tradeoff Visualizer ===== */

void depth_analyzer_demo(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  DEPTH-SIZE TRADEOFF ANALYSIS\n");
    printf("==========================================================\n\n");

    printf("Fundamental tradeoff: shallower circuits need more gates.\n\n");
    printf("--- PARITY: Depth vs Size ---\n");
    printf("  %-6s %-6s %-14s %s\n", "n", "depth", "min_size", "feasible?");
    printf("  %-6s %-6s %-14s %s\n", "----", "-----", "--------", "---------");
    for (int n = 16; n <= 128; n *= 2) {
        for (int d = 1; d <= 4; d++) {
            double lb = ac0_parity_size_lower_bound(n, d);
            double feasible = (lb < n * n * n) ? 1 : 0;
            printf("  %-6d %-6d %-14.2e %s\n", n, d, lb,
                   feasible ? "YES" : "NO");
        }
    }

    printf("\n--- Sipser Functions: Strict Depth Hierarchy ---\n");
    printf("  S_d separates AC0[d] from AC0[d+1] (Hastad 1986).\n");
    for (int d = 1; d <= 5; d++)
        printf("  AC0[%d] < AC0[%d] (separated by S_%d, size >= %.2e)\n",
               d, d+1, d, sipser_ac0_lower_bound(64, d));

    printf("\n--- Function-by-Function Depth Requirements ---\n");
    printf("  %-12s %-12s %-12s %-12s\n",
           "Function", "AC0_depth", "NC1_depth", "TC0_depth");
    printf("  %-12s %-12s %-12s %-12s\n",
           "--------", "---------", "---------", "---------");
    printf("  %-12s %-12s %-12s %-12s\n",
           "PARITY", "impossible", "O(log n)", "O(log n)");
    printf("  %-12s %-12s %-12s %-12s\n",
           "MAJORITY", "impossible", "O(log n)", "1");
    printf("  %-12s %-12s %-12s %-12s\n",
           "ADDITION", "impossible", "O(log n)", "O(log n)");
    printf("  %-12s %-12s %-12s %-12s\n",
           "MULTIPLY", "impossible", "O(log^2 n)", "O(log n)");
    printf("  %-12s %-12s %-12s %-12s\n",
           "SORTING", "impossible", "O(log^2 n)", "O(log n)");

    printf("\n--- Depth vs Size Tradeoff Curve (PARITY) ---\n");
    printf("  %-6s %-10s %-10s %-10s\n", "depth", "n=16", "n=64", "n=256");
    for (int d = 1; d <= 5; d++) {
        printf("  %-6d", d);
        for (int n = 16; n <= 256; n *= 4)
            printf(" %-10.2e", ac0_parity_size_lower_bound(n, d));
        printf("\n");
    }
}

void ac0_parity_lower_bound_demo(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  PARITY LOWER BOUND\n");
    printf("  PARITY not in AC0 (FSS 1981, Ajtai 1983, Hastad 1986)\n");
    printf("==========================================================\n\n");

    printf("Theorem: Any AC0 circuit for PARITY_n requires size\n");
    printf("         >= exp(Omega(n^{1/(d-1)})), where d = depth.\n\n");

    printf("Three independent proofs:\n");
    printf("  1. Furst, Saxe, Sipser (1981): via random restrictions\n");
    printf("  2. Ajtai (1983): combinatorial, Godel Prize 1994\n");
    printf("  3. Hastad (1986): Switching Lemma, Godel Prize 1994\n\n");

    printf("  n=16: depth=2 => size >= %.2e\n",
           ac0_parity_size_lower_bound(16, 2));
    printf("  n=16: depth=3 => size >= %.2e\n",
           ac0_parity_size_lower_bound(16, 3));
    printf("  n=64: depth=2 => size >= %.2e (astronomical!)\n",
           ac0_parity_size_lower_bound(64, 2));
    printf("\n  By contrast, NC1 PARITY (balanced XOR tree):\n");
    printf("  n=64: depth=%d, size=%.0f\n",
           (int)ceil(log2(64.0)), nc1_parity_size_upper_bound(64));
}

void hierarchy_bench_demo(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  HIERARCHY SCALING BENCHMARK\n");
    printf("  AC0 < TC0 < NC1 < ... < P/poly\n");
    printf("==========================================================\n\n");

    printf("%6s %14s %14s %14s %14s\n",
           "n", "AC0(thresh)", "NC1(par)", "TC0(maj)", "P/poly(par)");
    printf("%6s %14s %14s %14s %14s\n",
           "-", "----------", "--------", "--------", "-----------");
    for (int n = 4; n <= 32; n *= 2) {
        int k = n/2;
        double ac0 = ac0_threshold_size_lower_bound(n, k);
        double nc1 = nc1_parity_size_upper_bound(n);
        double tc0 = tc0_majority_size_upper_bound(n);
        double pp  = ppoly_min_size_upper_bound(n);
        printf("%6d %14.2e %14.0f %14.0f %14.2e\n", n, ac0, nc1, tc0, pp);
    }

    printf("\n--- Exponential vs Polynomial ---\n");
    printf("AC0 grows EXPONENTIALLY for PARITY/MAJORITY.\n");
    printf("NC1/TC0 grow POLYNOMIALLY.\n");
    printf("P/poly grows EXPONENTIALLY (but non-uniform).\n");
    printf("=> AC0 != NC1 (proved). P/poly vs NP (open).\n");
}

void non_uniformity_demo(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  NON-UNIFORM COMPUTATION: P/poly\n");
    printf("==========================================================\n\n");

    printf("P/poly = languages decidable by poly-size circuit families.\n");
    printf("No requirement that circuits be uniformly constructible.\n\n");

    printf("Key results:\n");
    printf("  P subset P/poly (trivial: unroll TM to circuit)\n");
    printf("  BPP subset P/poly (Adleman 1978)\n");
    printf("  P/poly is NOT recursively presentable\n");
    printf("  P/poly has NO complete problems\n");
    printf("  Karp-Lipton: NP subset P/poly => PH = Sigma2\n");
    printf("  IKW: NEXP subset P/poly => NEXP = MA\n\n");

    printf("Non-uniformity as advice:\n");
    printf("  L in P/poly iff exists P-language A and advice function\n");
    printf("  alpha(|x|) such that x in L <=> (x, alpha(|x|)) in A.\n");
    printf("  The advice can encode any information about input size.\n");
}
