#ifndef BARRIER_THEOREM_H
#define BARRIER_THEOREM_H
#include "natural_core.h"
#include "shannon.h"

/*
 * barrier_theorem.h -- Razborov-Rudich Natural Proofs Barrier Theorem
 *
 * L1: Definitions
 *   - Razborov-Rudich Theorem: No natural proof can show NP notin P/poly
 *     if one-way functions exist.
 *   - Barrier: An obstacle that prevents certain proof techniques from
 *     resolving P vs NP.
 *
 * L2: Core Concepts
 *   - Natural proof: constructive + large + useful property
 *   - PRF contradiction: OWF => PRF => natural property applies to PRF
 *     => PRF has small circuits (contradiction with usefulness)
 *   - All known circuit lower bounds are natural
 *
 * L4: Fundamental Laws
 *   - Razborov-Rudich Theorem (RR97): Formal statement and proof structure
 *   - Relationship to relativization (BGS75) and algebrization (AW09)
 *   - Three barriers constitute the "trifecta" obstructing P vs NP
 *
 * Proof Structure (6 steps):
 *   1. Assume natural property P separates NP from P/poly.
 *   2. P constructive => there exists algorithm A deciding P in time 2^{O(n)}.
 *   3. Assume OWF exist => PRF exist (HILL 1999).
 *   4. PRF family {F_k} is indistinguishable from random for poly-time A.
 *   5. P large => random f has P with high probability.
 *      By indistinguishability, PRF also has P with high probability.
 *   6. P useful => PRF notin P/poly.
 *      But PRF is in P (given key). CONTRADICTION!
 *   7. Therefore: either OWF don't exist, or no natural proof proves P != NP.
 *
 * References:
 *   Razborov-Rudich (1997): JCSS 55(1):24-35
 *   Arora-Barak (2009), Chapter 23
 *   HILL (1999): FOCS 1999
 */

/* ========================================================================
 * Theorem Data Structures
 * ======================================================================== */

/*
 * BarrierProofStep: Represents one step in the Razborov-Rudich proof.
 */
typedef struct {
    int    step_number;
    char   description[256];
    char   formal_statement[512];
    int    is_assumption;   /* 1 if this step is an assumption */
    int    is_derived;      /* 1 if derived from previous steps */
    int    leads_to[8];     /* step numbers this leads to */
    int    n_leads;
} BarrierProofStep;

/*
 * RRTheorem: Full statement of the Razborov-Rudich Theorem.
 */
typedef struct {
    /* Assumptions */
    int owf_exist;             /* One-way functions exist */
    int property_natural;      /* P is constructive + large + useful */
    int property_separates;    /* P separates NP from P/poly */

    /* The property */
    NaturalProperty property;

    /* Derived facts */
    int prf_exist;             /* OWF => PRF */
    int prf_has_property;      /* PRF indistinguishable from random => PRF has P */
    int prf_in_ppoly;          /* PRF is polynomial-time computable => P/poly */
    int contradiction;          /* PRF in P/poly but P useful => PRF notin P/poly */

    /* Verdict */
    int barrier_holds;         /* 1 if the proof shows a genuine barrier */
    char conclusion[512];      /* Human-readable conclusion */
} RRTheorem;

/* ========================================================================
 * Core Theorem Functions
 * ======================================================================== */

/*
 * rr_theorem_create: Initialize an RRTheorem structure with given assumptions.
 *
 * Parameters:
 *   owf_exist: 1 if we assume OWF exist, 0 otherwise
 *   n: number of input variables for the target property
 *   S: circuit size bound for the usefulness criterion
 *
 * Returns a partially filled theorem structure.
 */
RRTheorem *rr_theorem_create(int owf_exist, int n, int S);

/*
 * rr_theorem_free: Deallocate theorem structure.
 */
void rr_theorem_free(RRTheorem *thm);

/*
 * rr_theorem_derive: Derive all consequences from the assumptions.
 *   This runs through the 6-step proof, filling in derived fields.
 *
 * Returns 1 if contradiction is reached (barrier holds), 0 otherwise.
 */
int rr_theorem_derive(RRTheorem *thm);

/*
 * rr_theorem_print: Print the full theorem and proof steps.
 */
void rr_theorem_print(const RRTheorem *thm, FILE *fp);

/*
 * rr_theorem_verify: Verify that the theorem is logically sound.
 *   Checks:
 *     1. If OWF exist => PRF exist (by HILL)
 *     2. If P natural => PRF has P (by largeness + indistinguishability)
 *     3. If P useful => PRF notin P/poly
 *     4. PRF in P (by definition) => CONTRADICTION with (3)
 *
 * Returns 1 if all implications hold, 0 if any step fails.
 */
int rr_theorem_verify(const RRTheorem *thm);

/* ========================================================================
 * Proof Steps (for educational purposes)
 * ======================================================================== */

/*
 * rr_proof_step_count: Return the number of steps in the RR proof (7).
 */
int rr_proof_step_count(void);

/*
 * rr_proof_get_step: Get a specific proof step by number (1-7).
 * Returns NULL if step number is invalid.
 */
const BarrierProofStep *rr_proof_get_step(int step_number);

/*
 * rr_proof_print_all: Print all proof steps in sequence.
 */
void rr_proof_print_all(FILE *fp);

/* ========================================================================
 * Barrier Bypass Analysis
 * ======================================================================== */

/*
 * rr_barrier_bypass_methods: List known approaches that might bypass
 *   the natural proofs barrier.
 *
 * Returns: number of known bypass approaches.
 */
int rr_barrier_bypass_count(void);

/*
 * rr_barrier_bypass_get: Get the name and description of the i-th
 *   bypass approach.
 *
 * Parameters:
 *   i: index (0 to bypass_count - 1)
 *   name: output buffer for name (at least 64 chars)
 *   description: output buffer for description (at least 512 chars)
 * Returns: 1 if valid index, 0 otherwise.
 */
int rr_barrier_bypass_get(int i, char *name, char *description);

/*
 * rr_barrier_bypass_print_all: Print all bypass approaches.
 */
void rr_barrier_bypass_print_all(FILE *fp);

/* ========================================================================
 * "All Known Lower Bounds are Natural" Demonstration
 * ======================================================================== */

/*
 * known_lower_bound: Represents a known circuit lower bound result.
 */
typedef struct {
    const char *name;           /* e.g., "AC0 lower bounds" */
    const char *authors;        /* e.g., "Furst-Saxe-Sipser, Ajtai, Hastad" */
    const char *target_class;   /* e.g., "AC0" */
    const char *hard_function;  /* e.g., "PARITY" */
    int         constructive;   /* whether the proof is constructive */
    int         large;          /* whether the property used is large */
    int         useful;         /* whether the property is useful */
    int         is_natural;     /* ALL known bounds are natural */
} KnownLowerBound;

/*
 * rr_known_bounds_count: Number of known circuit lower bounds.
 */
int rr_known_bounds_count(void);

/*
 * rr_known_bounds_get: Get the i-th known lower bound.
 */
const KnownLowerBound *rr_known_bounds_get(int i);

/*
 * rr_known_bounds_print_all: Print all known lower bounds and their
 *   natural proof status.
 */
void rr_known_bounds_print_all(FILE *fp);

#endif /* BARRIER_THEOREM_H */
