#ifndef THREE_BARRIERS_H
#define THREE_BARRIERS_H
#include "natural_core.h"

/*
 * three_barriers.h -- The Three Barriers to P vs NP
 *
 * L1: Definitions
 *   - Relativization Barrier (BGS 1975): Diagonalization alone cannot resolve
 *     P vs NP because there exist oracles making it go either way.
 *   - Natural Proofs Barrier (RR 1997): If OWF exist, no natural proof can
 *     separate NP from P/poly.
 *   - Algebrization Barrier (AW 2009): Even algebraic extensions of
 *     relativization techniques cannot resolve P vs NP.
 *
 * L2: Core Concepts
 *   - Oracle Turing machines: TM with access to an oracle A
 *   - Relativizing proof: proof that holds relative to any oracle
 *   - Extension of oracles to low-degree polynomial extensions
 *   - Algebrizing proof: proof that holds relative to algebraic oracles
 *
 * L4: Fundamental Laws
 *   - Baker-Gill-Solovay Theorem (1975): Exist oracles A,B such that
 *     P^A = NP^A and P^B != NP^B.
 *   - Razborov-Rudich Theorem (1997): See barrier_theorem.h
 *   - Aaronson-Wigderson Theorem (2009): Any proof technique that
 *     algebrizes cannot prove P != NP (or P = NP).
 *
 * L7: Applications
 *   - Understanding why P vs NP remains open after 50+ years
 *   - Guiding research toward barrier-bypassing approaches:
 *     GCT, meta-complexity, interactive proofs, proof complexity
 *
 * L8: Advanced Topics
 *   - MIP* = RE (2020): Quantum interactive proofs bypass barriers
 *   - Geometric Complexity Theory (Mulmuley-Sohoni 2001)
 *   - Minimum Circuit Size Problem (MCSP) meta-complexity
 *   - Proof complexity lower bounds
 *
 * References:
 *   Baker-Gill-Solovay (1975): SIAM J. Comput. 4(4):431-442
 *   Razborov-Rudich (1997): JCSS 55(1):24-35
 *   Aaronson-Wigderson (2009): J. ACM 56(5):1-59
 */

/* ========================================================================
 * Barrier Types
 * ======================================================================== */

typedef enum {
    BARRIER_RELATIVIZATION = 0,
    BARRIER_NATURAL_PROOFS = 1,
    BARRIER_ALGEBRIZATION  = 2,
    BARRIER_NUM            = 3
} BarrierType;

/* ========================================================================
 * Oracle Model (for Relativization)
 * ======================================================================== */

/*
 * Oracle: A set of strings (language) that a TM can query.
 *
 * In relativization: P^A = languages decidable in poly time with oracle A.
 * NP^A = languages verifiable in poly time with oracle A.
 *
 * Key fact: There exist oracles A, B such that:
 *   P^A = NP^A  (collapsing oracle)
 *   P^B != NP^B (separating oracle)
 *
 * Therefore: any proof that "relativizes" (holds for all oracles)
 * cannot resolve P vs NP, because it would give contradictory results
 * for different oracles.
 */
typedef struct {
    int     n_strings;       /* number of strings in the oracle */
    char  **strings;         /* strings in the oracle */
    int    *lengths;         /* lengths of each string */
    int     capacity;
    char    name[64];

    /* For simulation: represent oracle as a subset of {0,1}^* up to
     * a certain length bound. For bounded-length queries, this is exact. */
    int     max_query_length;

    /* For algebraic oracles: field and polynomial extension */
    int     is_algebraic;     /* 1 if this is an algebraic oracle */
    int     field_prime;      /* prime field size (for GF(p)) */
} Oracle;

/*
 * oracle_create: Create an empty oracle with given name and max query length.
 */
Oracle *oracle_create(const char *name, int max_query_len);

/*
 * oracle_free: Deallocate an oracle.
 */
void oracle_free(Oracle *oracle);

/*
 * oracle_add: Add a string to the oracle.
 * Returns 1 if added, 0 if already present.
 */
int oracle_add(Oracle *oracle, const char *str);

/*
 * oracle_query: Check if a string is in the oracle.
 * Returns 1 if yes, 0 if no.
 */
int oracle_query(const Oracle *oracle, const char *str);

/*
 * oracle_create_bgs_A: Create the BGS oracle A where P^A = NP^A.
 *   A is a PSPACE-complete set (e.g., TQBF).
 *   Then: P^A = NP^A = PSPACE.
 */
Oracle *oracle_create_bgs_A(void);

/*
 * oracle_create_bgs_B: Create the BGS oracle B where P^B != NP^B.
 *   B is constructed via diagonalization against all poly-time machines.
 *   The construction ensures NP^B contains a language not in P^B.
 */
Oracle *oracle_create_bgs_B(int n);

/* ========================================================================
 * Barrier Descriptions
 * ======================================================================== */

/*
 * BarrierInfo: Information about a specific barrier.
 */
typedef struct {
    BarrierType type;
    const char *name;
    const char *authors;
    int         year;
    const char *statement;         /* formal statement of the barrier */
    const char *consequence;       /* what the barrier rules out */
    const char *bypass_candidates; /* known approaches to bypass */
    int         is_bypassable;     /* 1 if we know how to bypass */
} BarrierInfo;

/*
 * barrier_info_get: Get information for a specific barrier type.
 */
const BarrierInfo *barrier_info_get(BarrierType type);

/*
 * barrier_info_print: Print full information about a barrier.
 */
void barrier_info_print(BarrierType type, FILE *fp);

/*
 * barrier_all_print: Print information about all three barriers.
 */
void barrier_all_print(FILE *fp);

/* ========================================================================
 * Barrier Simulator: Interactive exploration
 * ======================================================================== */

/*
 * ThreeBarriersSim: Simulation state for the three barriers.
 *   Demonstrates why each barrier blocks P vs NP resolution.
 */
typedef struct {
    Oracle *oracle_A;       /* P^A = NP^A oracle */
    Oracle *oracle_B;       /* P^B != NP^B oracle */

    NaturalProperty natural_prop;  /* a test natural property */
    int    n_inputs;
    int    circuit_size;

    /* Results */
    int    relativization_blocks;     /* 1 if relativizing proofs blocked */
    int    natural_proofs_blocks;     /* 1 if natural proofs blocked */
    int    algebrization_blocks;      /* 1 if algebrizing proofs blocked */

    /* Verdict: does P vs NP evade all three? */
    int    all_barriers_hold;
} ThreeBarriersSim;

/*
 * barriers_sim_create: Create a barrier simulation with given parameters.
 */
ThreeBarriersSim *barriers_sim_create(int n_inputs, int circuit_size,
                                       int assume_owf);

/*
 * barriers_sim_free: Deallocate simulation.
 */
void barriers_sim_free(ThreeBarriersSim *sim);

/*
 * barriers_sim_run: Run the full barrier simulation.
 *   Checks all three barriers and determines which ones block the proof.
 */
void barriers_sim_run(ThreeBarriersSim *sim);

/*
 * barriers_sim_print: Print simulation results.
 */
void barriers_sim_print(const ThreeBarriersSim *sim, FILE *fp);

/* ========================================================================
 * Demonstration Functions
 * ======================================================================== */

/*
 * barrier_demo_relativization: Demonstrate the relativization barrier.
 */
void barrier_demo_relativization(void);

/*
 * barrier_demo_natural_proofs: Demonstrate the natural proofs barrier.
 */
void barrier_demo_natural_proofs(void);

/*
 * barrier_demo_algebrization: Demonstrate the algebrization barrier.
 */
void barrier_demo_algebrization(void);

/*
 * barrier_demo_all: Run all three barrier demonstrations.
 */
void barrier_demo_all(void);

/* ========================================================================
 * Bypass Candidates
 * ======================================================================== */

/*
 * BypassCandidate: A research program attempting to bypass the barriers.
 */
typedef struct {
    const char *name;
    const char *leader;          /* principal investigators */
    int         year_started;
    const char *approach;        /* high-level strategy */
    const char *bypasses[3];     /* which barriers it bypasses */
    int         n_bypasses;
    const char *status;          /* current research status */
    const char *key_insight;     /* the key idea */
} BypassCandidate;

/*
 * bypass_candidates_count: Number of known bypass candidates.
 */
int bypass_candidates_count(void);

/*
 * bypass_candidates_get: Get i-th bypass candidate.
 */
const BypassCandidate *bypass_candidates_get(int i);

/*
 * bypass_candidates_print_all: Print all bypass candidates.
 */
void bypass_candidates_print_all(FILE *fp);

#endif /* THREE_BARRIERS_H */
