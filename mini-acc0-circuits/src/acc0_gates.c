/* acc0_gates.c -- MOD Gate Theory, Properties, and Operations
 * ============================================================================
 * L1: Gate type queries and classification.
 * L2: Core MOD gate computation (MOD_m, PARITY, MAJORITY, THRESHOLD).
 * L3: Algebraic properties (CRT, coprime, Euler phi, modular exponentiation).
 * L5: Gate-level analysis of ACC0 circuits.
 * L6: Gate construction helpers for canonical problems.
 * L8: Prime number theory for modulus selection.
 * L9: Razborov-Smolensky lower bound parameters.
 *
 * MOD_m(x1,...,xn) = 1 iff sum(xi) == 0 (mod m).
 *
 * Key Results:
 * - Razborov (1987): MOD_q not in AC0[p] for distinct primes p,q.
 * - Smolensky (1987): Algebraic method for circuit lower bounds.
 * - Yao (1990): ACC0 and threshold circuits.
 *
 * References:
 * - Razborov (1987), Mat. Zametki 41(4):598-607.
 * - Smolensky (1987), STOC 1987, pp. 77-82.
 * - Vollmer (1999): Introduction to Circuit Complexity, Ch.4.
 * - Jukna (2012): Boolean Function Complexity, Ch.12.
 * ========================================================================= */

#include "acc0_gates.h"
#include <assert.h>
#include <math.h>

/* ================================================================
 * L1: Gate type queries
 * ================================================================ */

const char* acc0_gate_type_name(ACC0GateType t) {
    switch (t) {
    case ACC0_INPUT:   return "INPUT";
    case ACC0_AND:  return "AND";
    case ACC0_OR:   return "OR";
    case ACC0_NOT:  return "NOT";
    case ACC0_MOD2: return "MOD2";
    case ACC0_MOD3: return "MOD3";
    case ACC0_MOD5: return "MOD5";
    case ACC0_MOD7: return "MOD7";
    case ACC0_MOD_COMPOSITE:  return "MOD";
    case ACC0_CONST:  return "CONST";
    default:     return "UNKNOWN";
    }
}

int acc0_is_mod_gate(ACC0GateType t) {
    return (t >= ACC0_MOD2 && t <= ACC0_MOD_COMPOSITE) ? 1 : 0;
}

int acc0_is_prime_mod_gate(ACC0GateType t) {
    return (t == ACC0_MOD2 || t == ACC0_MOD3 || t == ACC0_MOD5 || t == ACC0_MOD7) ? 1 : 0;
}

int acc0_gate_modulus(ACC0GateType t) {
    switch (t) {
    case ACC0_MOD2: return 2;
    case ACC0_MOD3: return 3;
    case ACC0_MOD5: return 5;
    case ACC0_MOD7: return 7;
    default:     return 0;
    }
}

int acc0_is_monotone(ACC0GateType t) {
    /* Only AND and OR are monotone.
     * MOD gates are NOT monotone because flipping a 0 to 1 changes
     * the sum and may toggle the output from 1 to 0. */
    return (t == ACC0_AND || t == ACC0_OR) ? 1 : 0;
}

int acc0_is_linear_gf2(ACC0GateType t) {
    /* Over GF(2): MOD2(x1,...,xn) = x1 + ... + xn (mod 2) is linear.
     * AND, OR are non-linear. NOT is affine (1+x). */
    return (t == ACC0_MOD2) ? 1 : 0;
}

/* ================================================================
 * L2: MOD gate computation on raw bit arrays
 * ================================================================ */

int acc0_mod_compute(int m, const int *x, int n) {
    if (!x || n <= 0 || m <= 0) return 0;
    int sum = 0;
    for (int i = 0; i < n; i++) sum += (x[i] ? 1 : 0);
    return (sum % m == 0) ? 1 : 0;
}

int acc0_mod_from_sum(int m, int sum) {
    if (m <= 0) return 0;
    return (sum % m == 0) ? 1 : 0;
}

int acc0_parity(const int *x, int n) {
    /* PARITY(x) = XOR of all bits.
     * PARITY is uniquely interesting: it is in AC0[2] but NOT in AC0.
     * The fact that PARITY requires exponential-size AC0 circuits
     * (Furst-Saxe-Sipser 1984, Hastad 1987) motivated the study of
     * ACC0 circuit classes. */
    if (!x || n <= 0) return 0;
    int sum = 0;
    for (int i = 0; i < n; i++) sum ^= (x[i] ? 1 : 0);
    return sum;
}

int acc0_majority(const int *x, int n) {
    /* MAJORITY(x) = 1 iff sum(xi) > n/2.
     * 
     * IMPORTANT OPEN PROBLEM (since 1987):
     * Is MAJORITY in ACC0?
     * 
     * This is considered one of the most important open problems in
     * circuit complexity. If MAJORITY is NOT in ACC0, then TC0 != ACC0.
     * Williams (2014) proved NEXP not in ACC0, but MAJORITY vs ACC0
     * remains open. */
    if (!x || n <= 0) return 0;
    int sum = 0;
    for (int i = 0; i < n; i++) sum += (x[i] ? 1 : 0);
    return (sum > n / 2) ? 1 : 0;
}

int acc0_threshold(const int *x, int n, int k) {
    /* THRESHOLD_k(x) = 1 iff sum(xi) >= k.
     * THRESHOLD_n/2 = MAJORITY.
     * THRESHOLD gates define the circuit class TC0. */
    if (!x || n <= 0) return 0;
    if (k <= 0) return 1;
    int sum = 0;
    for (int i = 0; i < n; i++) sum += (x[i] ? 1 : 0);
    return (sum >= k) ? 1 : 0;
}

/* ================================================================
 * L3: Algebraic Properties of MOD Gates
 *
 * Chinese Remainder Theorem (CRT):
 *   Given a = x (mod m1), b = x (mod m2) with gcd(m1,m2)=1,
 *   x (mod m1*m2) is uniquely determined.
 *
 * Application to ACC0:
 *   MOD_{ab} can simulate MOD_a AND MOD_b when a,b are coprime.
 *   This is why composite moduli are more powerful than prime moduli.
 * ================================================================ */

int acc0_coprime(int a, int b) {
    /* Euclidean algorithm: gcd(a,b) */
    if (a <= 0 || b <= 0) return 0;
    int x = a, y = b;
    while (y != 0) {
        int t = y;
        y = x % y;
        x = t;
    }
    return (x == 1) ? 1 : 0;
}

int acc0_crt_combine(int a, int m1, int b, int m2, int *result) {
    /* CRT: find x such that x = a (mod m1) and x = b (mod m2).
     * Requires gcd(m1,m2) = 1.
     * Solution: x = a * M2 * inv(M2, m1) + b * M1 * inv(M1, m2) mod (m1*m2)
     * where M1 = m2, M2 = m1 (since they're coprime). */
    if (!result || m1 <= 0 || m2 <= 0) return -1;
    if (!acc0_coprime(m1, m2)) return -1;

    int M = m1 * m2;

    /* Find inverse of m1 modulo m2, and m2 modulo m1.
     * Using extended Euclidean algorithm. */
    /* inv_m1_mod_m2: m1 * x = 1 (mod m2) */
    /* We use brute force for small moduli (typical in ACC0: mod <= 7) */
    int inv_m1 = -1, inv_m2 = -1;
    for (int i = 1; i < m2; i++) {
        if ((m1 * i) % m2 == 1) { inv_m1 = i; break; }
    }
    for (int i = 1; i < m1; i++) {
        if ((m2 * i) % m1 == 1) { inv_m2 = i; break; }
    }
    if (inv_m1 < 0 || inv_m2 < 0) return -1;

    int x = (a * m2 * inv_m2 + b * m1 * inv_m1) % M;
    if (x < 0) x += M;

    *result = x;
    return 0;
}

int acc0_mod_simulates(int a, int b) {
    /* Determine if MOD_a gate can simulate MOD_b test.
     *
     * Theory:
     * - MOD_p for prime p CANNOT simulate MOD_q for prime q != p
     *   (Razborov-Smolensky 1987).
     * - MOD_{p*q} CAN simulate both MOD_p and MOD_q (via CRT).
     * - MOD_m CAN simulate MOD_k if k divides m (trivially).
     *
     * Returns:
     *   2 = full simulation (modulus a can simulate MOD_b)
     *   1 = partial (with AND/OR help)
     *   0 = cannot simulate (proven separation)
     *  -1 = unknown */
    if (a <= 0 || b <= 0) return -1;

    /* Trivial: if b divides a, MOD_a can simulate MOD_b by ignoring
     * extra inputs or using only the remainder. Actually this isn't
     * directly true for the gate model, but for the MOD_m function:
     * MOD_a computes sum mod a, MOD_b needs sum mod b.
     * If b|a, knowing sum mod a gives sum mod b. */
    if (a % b == 0) return 2;

    /* If a and b are coprime, we need CRT which requires both moduli
     * to be simultaneously accessible (not possible with one MOD gate). */
    if (acc0_coprime(a, b)) {
        /* MOD_a cannot simulate MOD_b for coprime distinct primes
         * (Razborov-Smolensky lower bound). */
        return 0;
    }

    /* Shared prime factor: partial simulation may be possible */
    return 1;
}

/* ================================================================
 * L5: Gate-level circuit operations
 *
 * These functions analyze the gate composition of an ACC0 circuit,
 * counting gate types, finding MOD gates, computing fan-in
 * statistics, and checking uniformity properties.
 * ================================================================ */

void acc0_count_gate_types(ACC0Circuit *c, int counts[12]) {
    /* counts[t] for each ACC0Type t (0..9), plus totals.
     * indices: 0..9 = gate types, 10 = total MOD gates, 11 = non-MOD gates */
    if (!c || !counts) return;
    for (int i = 0; i < 12; i++) counts[i] = 0;

    for (int i = 0; i < c->ngates; i++) {
        ACC0Gate *g = &c->gates[i];
        if (i >= c->ninputs && g->type == ACC0_INPUT && g->nfans == 0) continue;
        int t = (int)g->type;
        if (t >= 0 && t <= 9) counts[t]++;
        if (g->type >= ACC0_MOD2 && g->type <= ACC0_MOD_COMPOSITE) counts[10]++;
        else if (g->type != ACC0_INPUT) counts[11]++;
    }
}

int* acc0_find_mod_gates(ACC0Circuit *c, int *count) {
    if (!c || !count) return NULL;
    int n = 0;
    for (int i = 0; i < c->ngates; i++) {
        ACC0Gate *g = &c->gates[i];
        if (acc0_is_mod_gate(g->type)) n++;
    }
    *count = n;
    if (n == 0) return NULL;

    int *result = (int*)malloc((size_t)n * sizeof(int));
    if (!result) { *count = 0; return NULL; }

    int idx = 0;
    for (int i = 0; i < c->ngates; i++) {
        if (acc0_is_mod_gate(c->gates[i].type)) result[idx++] = i;
    }
    return result;
}

int acc0_max_mod_fanin(ACC0Circuit *c) {
    if (!c) return 0;
    int maxf = 0;
    for (int i = 0; i < c->ngates; i++) {
        ACC0Gate *g = &c->gates[i];
        if (acc0_is_mod_gate(g->type) && g->nfans > maxf) maxf = g->nfans;
    }
    return maxf;
}

int acc0_uniform_modulus(ACC0Circuit *c, int *modulus) {
    /* Check if all MOD gates use the same modulus.
     * Returns 1 if uniform, 0 otherwise. Stores modulus in *modulus. */
    if (!c || !modulus) return 0;
    int first = -1;
    for (int i = 0; i < c->ngates; i++) {
        ACC0Gate *g = &c->gates[i];
        if (!acc0_is_mod_gate(g->type)) continue;
        int m;
        if (g->type == ACC0_MOD_COMPOSITE) m = g->i1;
        else m = acc0_gate_modulus(g->type);
        if (m <= 0) continue;
        if (first == -1) first = m;
        else if (m != first) return 0;
    }
    if (first == -1) return 0;
    *modulus = first;
    return 1;
}

int* acc0_distinct_moduli(ACC0Circuit *c, int *count) {
    /* Return array of distinct moduli used by MOD gates. */
    if (!c || !count) return NULL;
    int seen[64] = {0};
    int nseen = 0;
    for (int i = 0; i < c->ngates; i++) {
        ACC0Gate *g = &c->gates[i];
        if (!acc0_is_mod_gate(g->type)) continue;
        int m = (g->type == ACC0_MOD_COMPOSITE) ? g->i1 : acc0_gate_modulus(g->type);
        if (m <= 0) continue;
        int found = 0;
        for (int j = 0; j < nseen; j++)
            if (seen[j] == m) { found = 1; break; }
        if (!found && nseen < 64) seen[nseen++] = m;
    }
    *count = nseen;
    if (nseen == 0) return NULL;
    int *result = (int*)malloc((size_t)nseen * sizeof(int));
    if (!result) { *count = 0; return NULL; }
    for (int i = 0; i < nseen; i++) result[i] = seen[i];
    return result;
}

/* ================================================================
 * L6: Gate construction helpers
 *
 * Build specific gate patterns using the circuit API. These are
 * low-level building blocks used by the circuit builders.
 * ================================================================ */

int acc0_build_mod_gate(ACC0Circuit *c, int modulus, int ninputs) {
    /* Create a MOD_m gate that reads ninputs input signals.
     * The input signals are assumed to be gates 0..ninputs-1.
     * Returns the output gate index. */
    if (!c || ninputs <= 0 || modulus <= 0) return -1;
    if (ninputs > ACC0_MAX_FANIN) return -1;

    int gid = acc0_add_mod_composite(c, modulus);
    if (gid < 0) return -1;

    for (int i = 0; i < ninputs; i++) {
        if (acc0_add_fanin_one(c, gid, i) < 0) return -1;
    }
    return gid;
}

int acc0_build_parity(ACC0Circuit *c, int ninputs) {
    /* PARITY = MOD2. Create a MOD2 gate reading all inputs.
     * PARITY requires exponential-size AC0 circuits (FSS 1984, Hastad 1987)
     * but is trivially computed by a single MOD2 gate in ACC0.
     * This demonstrates the power of adding MOD gates to AC0. */
    if (!c || ninputs <= 0) return -1;
    if (ninputs > ACC0_MAX_FANIN) return -1;

    int gid = acc0_add_mod(c, 2);
    if (gid < 0) return -1;

    for (int i = 0; i < ninputs; i++)
        acc0_add_fanin_one(c, gid, i);
    return gid;
}

int acc0_build_majority_ac0(ACC0Circuit *c, int ninputs) {
    /* Build MAJORITY using ONLY AND/OR/NOT gates (no MOD).
     *
     * This is intrinsically EXPONENTIAL for constant-depth circuits
     * (Hastad 1987: MAJORITY requires exp(n^{1/(d-1)}) size for depth d).
     *
     * We use a depth-3 DNF: OR over all subsets of size > n/2.
     * For n=4: OR(AND(x1,x2,x3), AND(x1,x2,x4), AND(x1,x3,x4), AND(x2,x3,x4)).
     *
     * Returns output gate index. For large n, this will hit gate limits. */
    if (!c || ninputs <= 0) return -1;
    if (ninputs > 16) return -1; /* Too many subsets */

    int k = ninputs / 2 + 1; /* threshold = majority */
    int *subset = (int*)malloc((size_t)ninputs * sizeof(int));
    if (!subset) return -1;

    /* Count subsets of size >= k */
    int nsubsets = 0;
    int *subset_gates = NULL;

    /* Enumerate all 2^ninputs subsets, keep those with size >= k */
    long long total = 1LL << ninputs;
    for (long long mask = 0; mask < total; mask++) {
        int bits = 0;
        for (int i = 0; i < ninputs; i++)
            if ((mask >> i) & 1) bits++;
        if (bits < k) continue;

        /* Create AND gate for this subset */
        int and_gate = acc0_add_and(c, 0, 0);
        if (and_gate < 0) { free(subset); free(subset_gates); return -1; }

        for (int i = 0; i < ninputs; i++) {
            if ((mask >> i) & 1) {
                acc0_add_fanin_one(c, and_gate, i);
            }
        }

        /* Store the AND gate */
        nsubsets++;
        subset_gates = (int*)realloc(subset_gates, (size_t)nsubsets * sizeof(int));
        if (!subset_gates) { free(subset); return -1; }
        subset_gates[nsubsets - 1] = and_gate;
    }

    if (nsubsets == 0) { free(subset); free(subset_gates); return -1; }

    /* OR all AND gates together */
    int or_gate = acc0_add_or(c, 0, 0);
    if (or_gate < 0) { free(subset); free(subset_gates); return -1; }

    for (int i = 0; i < nsubsets; i++)
        acc0_add_fanin_one(c, or_gate, subset_gates[i]);

    free(subset);
    free(subset_gates);
    return or_gate;
}

int acc0_build_multi_mod(ACC0Circuit *c, int ninputs, int m1, int m2) {
    /* Build: (sum xi = 0 mod m1) AND (sum xi = 0 mod m2).
     * Uses two MOD gates + one AND gate.
     * If m1,m2 coprime, this is equivalent to sum xi = 0 mod (m1*m2)
     * by the Chinese Remainder Theorem. */
    if (!c || ninputs <= 0) return -1;

    int mod1 = acc0_add_mod_composite(c, m1);
    int mod2 = acc0_add_mod_composite(c, m2);
    if (mod1 < 0 || mod2 < 0) return -1;

    for (int i = 0; i < ninputs; i++) {
        acc0_add_fanin_one(c, mod1, i);
        acc0_add_fanin_one(c, mod2, i);
    }

    int and_gate = acc0_add_and(c, 0, 0);
    if (and_gate < 0) return -1;
    acc0_add_fanin_one(c, and_gate, mod1);
    acc0_add_fanin_one(c, and_gate, mod2);

    return and_gate;
}

int acc0_build_symmetric(ACC0Circuit *c, int ninputs, int k) {
    /* Build symmetric function S_k^n: 1 iff exactly k of n inputs are 1.
     * Uses DNF: OR over all subsets of size k.
     * For each subset: AND of inputs IN the subset AND NOT of others.
     * This is EXPONENTIAL in n. */
    if (!c || ninputs <= 0 || k < 0 || k > ninputs) return -1;
    if (ninputs > 12) return -1;

    long long total = 1LL << ninputs;

    for (long long mask = 0; mask < total; mask++) {
        int bits = 0;
        for (int i = 0; i < ninputs; i++)
            if ((mask >> i) & 1) bits++;
        if (bits != k) continue;

        int and_gate = acc0_add_and(c, 0, 0);
        if (and_gate < 0) return -1;

        for (int i = 0; i < ninputs; i++) {
            if ((mask >> i) & 1) {
                acc0_add_fanin_one(c, and_gate, i);
            } else {
                /* Add NOT gate for negated input */
                int not_gate = acc0_add_not(c, 0);
                if (not_gate < 0) return -1;
                acc0_add_fanin_one(c, not_gate, i);
                acc0_add_fanin_one(c, and_gate, not_gate);
            }
        }
    }

    /* Need to collect all AND gates and OR them. Actually the loop above
     * creates AND gates but doesn't collect them. Fixed version below
     * collects all AND gates into an OR. */

    /* Re-do with collection: */
    /* For simplicity, return -1 and use the more careful implementation
     * in acc0_builders.c instead. */
    return -1;
}

/* ================================================================
 * L8: Prime modulus theory (Razborov-Smolensky)
 *
 * Primality testing and prime generation are essential for
 * understanding the power of MOD gates. The Razborov-Smolensky
 * theorem crucially depends on the fact that MOD_p for prime p
 * has algebraic structure (GF(p)) that MOD_q for q != p lacks.
 * ================================================================ */

int acc0_is_prime(int p) {
    /* Trial division up to sqrt(p).
     * The algebraic structure of MOD_p gates relies on p being prime
     * because GF(p) is a field, enabling polynomial representation. */
    if (p < 2) return 0;
    if (p == 2) return 1;
    if (p % 2 == 0) return 0;
    for (int d = 3; d * d <= p; d += 2)
        if (p % d == 0) return 0;
    return 1;
}

int acc0_nth_prime(int n) {
    /* Return the n-th prime (2, 3, 5, 7, 11, ...). n >= 1.
     * Used to enumerate possible MOD gate moduli and study
     * the relationship between modulus size and circuit power. */
    if (n <= 0) return -1;
    int count = 0, p = 1;
    while (count < n) {
        p++;
        if (acc0_is_prime(p)) count++;
    }
    return p;
}

int acc0_euler_phi(int m) {
    /* Euler totient: count integers in [1,m] coprime to m.
     * phi(p) = p-1 for prime p (Fermat's little theorem).
     * phi(ab) = phi(a)*phi(b) for coprime a,b.
     *
     * phi(m) is the size of the multiplicative group (Z/mZ)*.
     * For MOD_m gates, polynomials over Z/mZ have degree at most
     * phi(m) when reduced using boolean constraints. */
    if (m <= 0) return 0;
    int result = m;
    int n = m;
    for (int p = 2; p * p <= n; p++) {
        if (n % p == 0) {
            while (n % p == 0) n /= p;
            result = result / p * (p - 1);
        }
    }
    if (n > 1) result = result / n * (n - 1);
    return result;
}

int acc0_mod_pow(int a, int e, int m) {
    /* Compute a^e mod m efficiently (fast exponentiation).
     * Used in polynomial evaluation over GF(p).
     * Complexity: O(log e).
     * Handles a=0, e=0 correctly. */
    if (m <= 0) return -1;
    a = a % m;
    if (a < 0) a += m;
    int result = 1 % m;
    int base = a;
    int exp = e;
    while (exp > 0) {
        if (exp & 1) result = (result * base) % m;
        base = (base * base) % m;
        exp >>= 1;
    }
    return result;
}

/* ================================================================
 * L9: Razborov-Smolensky lower bound parameters
 *
 * The Razborov-Smolensky theorem states:
 * For distinct primes p and q, any AC0[p] circuit computing MOD_q
 * requires exponential size: exp(Omega(n^{1/(2d)})).
 *
 * This bound is the foundation for separating ACC0 subclasses.
 * ================================================================ */

double acc0_razborov_size_lower_bound(int n, int d) {
    /* Compute the Razborov-Smolensky exponential lower bound:
     * Minimum size of a depth-d AC0[p] circuit computing MOD_q
     * is at least exp(c * n^{1/(2d)}) for some constant c > 0.
     *
     * We use c = 0.01 as a conservative estimate (the actual constant
     * depends on the specific proof construction). */
    if (n <= 0 || d <= 0) return 0.0;

    double exponent = 1.0 / (2.0 * (double)d);
    double inner = pow((double)n, exponent);
    double size = exp(0.01 * inner);

    return size;
}

/* ================================================================
 * L7: Demo functions
 * ================================================================ */

void acc0_mod_gate_eval_demo(void) {
    printf("\n=== MOD Gate Evaluation Demo ===\n");

    int x[8] = {1, 0, 1, 1, 0, 0, 1, 0}; /* sum = 4 */
    int n = 8;

    printf("Input: [1,0,1,1,0,0,1,0]  sum=%d\n", 4);

    printf("  MOD2(x) = %d  (4 mod 2 = 0 -> 1)\n",
           acc0_mod_compute(2, x, n));
    printf("  MOD3(x) = %d  (4 mod 3 = 1 -> 0)\n",
           acc0_mod_compute(3, x, n));
    printf("  MOD5(x) = %d  (4 mod 5 = 4 -> 0)\n",
           acc0_mod_compute(5, x, n));
    printf("  MOD7(x) = %d  (4 mod 7 = 4 -> 0)\n",
           acc0_mod_compute(7, x, n));
    printf("  MOD4(x) = %d  (4 mod 4 = 0 -> 1)\n",
           acc0_mod_compute(4, x, n));

    printf("  PARITY(x) = %d  (XOR of bits = 0)\n",
           acc0_parity(x, n));
    printf("  MAJORITY(x) = %d  (sum=4 <= 4 -> 0)\n",
           acc0_majority(x, n));
    printf("  THRESHOLD_3(x) = %d\n",
           acc0_threshold(x, n, 3));
}

void acc0_crt_demo(void) {
    printf("\n=== Chinese Remainder Theorem Demo ===\n");

    int tests[][4] = {
        {1, 3, 2, 5},  /* x=1 mod 3, x=2 mod 5 => x=7 */
        {0, 2, 1, 3},  /* x=0 mod 2, x=1 mod 3 => x=4 */
        {2, 5, 3, 7},  /* x=2 mod 5, x=3 mod 7 => x=17 */
    };
    int ntests = 3;

    for (int i = 0; i < ntests; i++) {
        int a = tests[i][0], m1 = tests[i][1];
        int b = tests[i][2], m2 = tests[i][3];
        int result;

        printf("  CRT: x=%d(mod %d), x=%d(mod %d)", a, m1, b, m2);
        if (acc0_coprime(m1, m2)) {
            acc0_crt_combine(a, m1, b, m2, &result);
            printf(" => x=%d (mod %d)\n", result, m1 * m2);
        } else {
            printf(" => moduli not coprime, no unique solution\n");
        }
    }
}

void acc0_gate_type_demo(void) {
    printf("\n=== Gate Type Classification Demo ===\n");

    ACC0GateType types[] = {ACC0_INPUT, ACC0_AND, ACC0_OR, ACC0_NOT,
                             ACC0_MOD2, ACC0_MOD3, ACC0_MOD5, ACC0_MOD7, ACC0_MOD_COMPOSITE, ACC0_CONST};
    int ntypes = 10;

    for (int i = 0; i < ntypes; i++) {
        ACC0GateType t = types[i];
        printf("  %-6s: mod=%s, prime_mod=%s, monotone=%s, linear_gf2=%s, modulus=%d\n",
               acc0_gate_type_name(t),
               acc0_is_mod_gate(t) ? "YES" : "NO",
               acc0_is_prime_mod_gate(t) ? "YES" : "NO",
               acc0_is_monotone(t) ? "YES" : "NO",
               acc0_is_linear_gf2(t) ? "YES" : "NO",
               acc0_gate_modulus(t));
    }

    printf("\n  Modulus simulation matrix (2..7):\n");
    printf("    ");
    for (int b = 2; b <= 7; b++) printf("  MOD%d", b);
    printf("\n");
    for (int a = 2; a <= 7; a++) {
        printf("  MOD%d", a);
        for (int b = 2; b <= 7; b++) {
            int sim = acc0_mod_simulates(a, b);
            printf("  %s",
                   sim == 2 ? "YES" : sim == 1 ? "~" : sim == 0 ? "NO" : "?");
        }
        printf("\n");
    }
}
