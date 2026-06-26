/* circuit_complexity.h -- Circuit Complexity Measures and Complexity Classes
 *
 * Defines key circuit complexity measures and maps circuits to the
 * standard circuit complexity class hierarchy.
 *
 * Circuit Complexity Class Hierarchy:
 *   AC^0 subset TC^0 subset NC^1 subset L/poly subset NL/poly subset
 *   AC^1 subset NC^2 subset ... subset NC subset P/poly
 *
 * Known separations:
 *   AC^0 != NC^1 (PARITY not in AC^0, Furst-Saxe-Sipser 1981, Hastad 1986)
 *   AC^0 subset ACC^0 (Razborov-Smolensky 1987: MOD-p not in AC^0 for prime p!=2)
 *
 * Open problems:
 *   NP subset P/poly? (Karp-Lipton: NP in P/poly => PH collapses)
 *   P = P/poly? (non-uniform P vs uniform P)
 *   NC^1 = TC^0?
 *
 * References:
 *   - Arora & Barak (2009) Ch.6, Ch.14
 *   - Vollmer (1999) Ch.2, Ch.4, Ch.7
 *   - Jukna (2012) Ch.1, Ch.2
 *   - Stanford CS358
 */

#ifndef CIRCUIT_COMPLEXITY_H
#define CIRCUIT_COMPLEXITY_H

#include "circuit.h"
#include <stddef.h>

/* ===================================================================
 * CircuitFamily definition (for class membership)
 * =================================================================== */

struct CircuitFamily {
    BooleanCircuit** circuits;
    int              max_n;
    int*             sizes;
    int*             depths;
    CircuitClass     class_hint;
    char*            name;
    int              uniform;
};

/* ===================================================================
 * L2: Circuit Complexity Measures
 * =================================================================== */

/* Circuit density: edges / binomial(n_gates, 2). */
double circuit_density(const BooleanCircuit* c);

/* Size-depth product: size * depth. */
double circuit_complexity_product(const BooleanCircuit* c);

/* Size-depth tradeoff exponent: log(size) / log(depth). */
double circuit_tradeoff_exponent(const BooleanCircuit* c);

/* Average fan-in of non-input gates. */
double circuit_avg_fanin(const BooleanCircuit* c);

/* Average fan-out of all gates. */
double circuit_avg_fanout(const BooleanCircuit* c);

/* Maximum path width: parallelism potential. */
int circuit_pathwidth(const BooleanCircuit* c);

/* ===================================================================
 * L2: Complexity Class Membership Tests
 * =================================================================== */

CircuitClass circuit_determine_class(const BooleanCircuit* c);

int circuit_is_AC0(const BooleanCircuit* c);
int circuit_is_AC1(const BooleanCircuit* c);
int circuit_is_AC2(const BooleanCircuit* c);
int circuit_is_NC1(const BooleanCircuit* c);
int circuit_is_NC2(const BooleanCircuit* c);
int circuit_is_TC0(const BooleanCircuit* c);
int circuit_is_TC1(const BooleanCircuit* c);
int circuit_is_P_poly(const BooleanCircuit* c);

const char* circuit_class_name(CircuitClass cls);

/* ===================================================================
 * L4: Fundamental Lower Bound Estimates
 * =================================================================== */

/* Shannon lower bound (1949): almost all n-variable Boolean functions
 * require size Omega(2^n / n). */
double shannon_lower_bound(int n);

/* Lupanov upper bound (1958): every n-variable Boolean function
 * has circuits of size O(2^n / n). */
double lupanov_upper_bound(int n);

/* Estimate size needed for depth-d circuit. */
double lupanov_size_depth(int n, int depth);

/* Nechiporuk bound (1966) for functions with independent subfunctions. */
double nechiporuk_bound(int n, int m_subfunctions);

/* Hastad switching lemma (1986): depth-d AC^0 circuits computing
 * PARITY require size exp(n^{1/(d-1)}). */
double hastad_parity_lower_bound(int n, int depth);

/* Razborov-Smolensky (1987): MOD-p requires exponential size in
 * AC^0 with MOD-q gates (p prime !=2, q prime power not divisible by p). */
double razborov_smolensky_bound(int n, int p, int q);

/* Karchmer-Wigderson game (1988): depth = communication complexity. */
int kw_depth_lower_bound(int n, int function_id);

/* ===================================================================
 * L4: Circuit Family Operations
 * =================================================================== */

CircuitFamily* circuit_family_create(BooleanCircuit** circuits, int max_n,
                                      const int* sizes, const int* depths,
                                      int uniform, const char* name);

BooleanCircuit* circuit_family_get(const CircuitFamily* family, int n);

CircuitClass circuit_family_class(const CircuitFamily* family);

int circuit_family_is_uniform(const CircuitFamily* family);

void circuit_family_free(CircuitFamily* family);

void circuit_family_print(const CircuitFamily* family);

/* ===================================================================
 * L6: Known Circuit Families
 * =================================================================== */

CircuitFamily* circuit_family_parity(int max_n);
CircuitFamily* circuit_family_majority(int max_n);
CircuitFamily* circuit_family_adder(int max_n);
CircuitFamily* circuit_family_multiplier(int max_n);
CircuitFamily* circuit_family_clique(int max_n, int k);
CircuitFamily* circuit_family_threshold(int max_n, int k);

/* ===================================================================
 * L5: Circuit Minimization
 * =================================================================== */

BooleanCircuit* circuit_minimize(BooleanCircuit* c);

BooleanCircuit* circuit_constant_propagate(BooleanCircuit* c);

#endif /* CIRCUIT_COMPLEXITY_H */
