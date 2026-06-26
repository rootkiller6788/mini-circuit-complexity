/* barrier_theorem.c -- Razborov-Rudich Theorem Implementation
 *
 * L4: Formal implementation of the Razborov-Rudich Theorem (1997).
 *
 * The theorem: If one-way functions exist, then no natural proof can
 * show that SAT notin P/poly (or more generally, that any NP problem is
 * not in P/poly).
 *
 * Proof structure (7 steps):
 *   1. Assume OWF exist (standard cryptographic assumption).
 *   2. Let P be a natural property that separates NP from P/poly.
 *      - P constructive, P large, P useful against P/poly.
 *   3. OWF => PRF exist (HILL 1999).
 *      - PRF family {F_k} is in P/poly and indistinguishable from random.
 *   4. P is large => random function R has P(R) >= 2^{-O(n)}.
 *   5. PRF indistinguishable from random => |Pr[P(F_k)] - Pr[P(R)]| is negligible.
 *      => Pr[P(F_k)] >= 2^{-O(n)} - negl(n) > 0.
 *   6. => There exists k such that P(F_k) holds.
 *   7. P useful => F_k notin P/poly. But F_k in P/poly. CONTRADICTION.
 *
 * Corollary: To prove NP notin P/poly, one must use non-natural proof
 * techniques, OR prove that OWF do not exist.
 *
 * References:
 *   Razborov-Rudich (1997): JCSS 55(1):24-35
 *   Arora-Barak (2009), Chapter 23.3
 *   HILL (1999): FOCS 1999
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "barrier_theorem.h"
#include "shannon.h"

/* ========================================================================
 * Proof Steps
 * ======================================================================== */

static BarrierProofStep RR_PROOF_STEPS[] = {
    {1, "Assume OWF exist",
     "Let F = {f_k} be a family of one-way functions.",
     1, 0, {2, 3}, 2},
    {2, "OWF => PRF exist (HILL 1999)",
     "There exists a PRF family {G_k} where G_k: {0,1}^n -> {0,1} "
     "is poly-time and indistinguishable from random.",
     0, 1, {4, 6}, 2},
    {3, "Assume P is a natural property separating NP from P/poly",
     "P is C-constructive, C-large, and useful against P/poly. "
     "P(SAT_n) is true for sufficiently large n.",
     1, 0, {4, 5}, 2},
    {4, "P large => random function R has P with prob >= 2^{-O(n)}",
     "Let n be large. Pr_R[P(R)] >= 1/2^{c*n} for some constant c.",
     0, 1, {5}, 1},
    {5, "PRF indistinguishable from random => Pr[P(G_k)] >= 2^{-O(n)} - negl(n)",
     "If |Pr[P(G_k)] - Pr[P(R)]| were non-negligible, P would distinguish "
     "PRF from random, violating PRF security.",
     0, 1, {6}, 1},
    {6, "There exists k such that P(G_k) holds",
     "Since Pr[P(G_k)] > 0 (for sufficiently large n), some key k works.",
     0, 1, {7}, 1},
    {7, "P useful => G_k notin P/poly. But G_k in P/poly. CONTRADICTION.",
     "P(f) => f requires super-poly circuits (usefulness). "
     "But G_k is polynomial-time computable => poly-size circuits. "
     "Contradiction! Therefore assumption (1) or (3) must be false.",
     0, 1, {0}, 0}
};

int rr_proof_step_count(void) {
    return 7;
}

const BarrierProofStep *rr_proof_get_step(int step_number) {
    if (step_number < 1 || step_number > 7) return NULL;
    return &RR_PROOF_STEPS[step_number - 1];
}

void rr_proof_print_all(FILE *fp) {
    if (!fp) fp = stdout;
    fprintf(fp, "\n========================================\n");
    fprintf(fp, "  RAZBOROV-RUDICH PROOF (7 Steps)\n");
    fprintf(fp, "========================================\n\n");

    for (int i = 0; i < 7; i++) {
        BarrierProofStep *step = &RR_PROOF_STEPS[i];
        fprintf(fp, "Step %d: %s\n", step->step_number, step->description);
        fprintf(fp, "  Formal: %s\n", step->formal_statement);
        fprintf(fp, "  Type: %s\n",
                step->is_assumption ? "ASSUMPTION" : "DERIVED");
        if (step->n_leads > 0) {
            fprintf(fp, "  Leads to steps: ");
            for (int j = 0; j < step->n_leads; j++) {
                fprintf(fp, "%d%s", step->leads_to[j],
                        (j + 1 < step->n_leads) ? ", " : "");
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }
}

/* ========================================================================
 * RRTheorem: Implementation
 * ======================================================================== */

RRTheorem *rr_theorem_create(int owf_exist, int n, int S) {
    RRTheorem *thm = (RRTheorem *)malloc(sizeof(RRTheorem));
    if (!thm) return NULL;

    thm->owf_exist = owf_exist;
    thm->property = shannon_natural_criteria_check(n, S);
    thm->property_natural = thm->property.is_natural;
    thm->property_separates = thm->property_natural;
    thm->prf_exist = 0;
    thm->prf_has_property = 0;
    thm->prf_in_ppoly = 1;  /* PRF is always in P/poly by definition */
    thm->contradiction = 0;
    thm->barrier_holds = 0;
    thm->conclusion[0] = '\0';

    return thm;
}

void rr_theorem_free(RRTheorem *thm) {
    free(thm);
}

int rr_theorem_derive(RRTheorem *thm) {
    if (!thm) return 0;

    /* Reset derived fields */
    thm->prf_exist = 0;
    thm->prf_has_property = 0;
    thm->contradiction = 0;
    thm->barrier_holds = 0;
    thm->conclusion[0] = '\0';

    /* Step 1: Check assumptions */
    if (!thm->property_natural) {
        snprintf(thm->conclusion, 511,
            "Property is NOT natural => barrier does not apply. "
            "A non-natural proof might still separate NP from P/poly.");
        return 0;
    }

    /* Step 2: OWF => PRF */
    if (thm->owf_exist) {
        thm->prf_exist = 1;
    } else {
        snprintf(thm->conclusion, 511,
            "OWF do not exist => PRF do not exist => "
            "the Razborov-Rudich contradiction is avoided. "
            "Natural proofs are not blocked in this scenario.");
        return 0;
    }

    /* Step 3: PRF has the natural property (by largeness + indistinguishability)
     * Since P is large (holds for >= 2^{-O(n)} fraction of functions) and
     * PRF is indistinguishable from random, the PRF must also satisfy P
     * with high probability. */
    if (thm->property.large) {
        thm->prf_has_property = 1;
    }

    /* Step 4: Usefulness => PRF not in P/poly */
    /* If P(f) => f notin SIZE[S(n)], and PRF has P, then PRF notin SIZE[S(n)].
     * But PRF is in SIZE[poly(n)] (by definition). For super-poly S,
     * SIZE[poly(n)] is a subset of SIZE[S(n)] only for very large S.
     *
     * The contradiction requires that S(n) is super-polynomial.
     * If S is polynomial, there is no contradiction (P is not useful
     * against P/poly — it only separates from SIZE[S] subset P/poly). */

    /* Check if S is super-polynomial (grows faster than any polynomial) */
    double log_n = log2((double)(thm->property.target_circuit_size > 0 ?
                                  thm->property.target_circuit_size : 1));
    double n_dbl = 10.0; /* use n=10 as reference */
    int is_super_poly = (log_n > 10.0 * log2(n_dbl));  /* S > n^10 */

    if (thm->prf_has_property && thm->property.useful && is_super_poly) {
        /* CONTRADICTION: PRF is in P/poly but P useful implies PRF notin */
        thm->contradiction = 1;
        thm->barrier_holds = 1;
        snprintf(thm->conclusion, 511,
            "BARRIER HOLDS: Assumption (OWF exist + natural P separates NP "
            "from P/poly) leads to contradiction. "
            "Conclusion: If OWF exist, no natural proof can separate "
            "NP from P/poly. All known lower bounds are natural => "
            "cannot resolve P vs NP.");
    } else if (thm->prf_has_property && thm->property.useful) {
        /* S is polynomial — P is still useful against SIZE[S], but
         * SIZE[S] subset P/poly, so separating NP from SIZE[S] does not
         * separate NP from P/poly. */
        snprintf(thm->conclusion, 511,
            "P is natural and useful against SIZE[S], but S is polynomial. "
            "P separates NP from SIZE[S] (a subclass of P/poly). "
            "The barrier still applies: P cannot separate NP from P/poly "
            "because PRF is in P/poly but not in SIZE[S] only when "
            "S is super-polynomial (contradiction).");
    } else {
        snprintf(thm->conclusion, 511,
            "No contradiction: either PRF does not have P, or P is not "
            "useful enough. The natural proof barrier does not block "
            "this particular property/parameter combination.");
    }

    return thm->contradiction;
}

void rr_theorem_print(const RRTheorem *thm, FILE *fp) {
    if (!thm) { fprintf(fp, "NULL theorem\n"); return; }
    if (!fp) fp = stdout;

    fprintf(fp, "\n========================================\n");
    fprintf(fp, "  RAZBOROV-RUDICH THEOREM\n");
    fprintf(fp, "========================================\n\n");

    fprintf(fp, "Assumptions:\n");
    fprintf(fp, "  OWF exist:       %s\n", thm->owf_exist ? "YES" : "NO");
    fprintf(fp, "  Property natural: %s\n", thm->property_natural ? "YES" : "NO");
    fprintf(fp, "  Property separates: %s\n", thm->property_separates ? "YES" : "NO");
    fprintf(fp, "\n");

    fprintf(fp, "Natural Property:\n");
    fprintf(fp, "  Constructive: %s\n", thm->property.constructive ? "YES" : "NO");
    fprintf(fp, "  Large:        %s\n", thm->property.large ? "YES" : "NO");
    fprintf(fp, "  Useful:       %s\n", thm->property.useful ? "YES" : "NO");
    fprintf(fp, "  Size bound S: %d\n", thm->property.target_circuit_size);
    fprintf(fp, "\n");

    fprintf(fp, "Derived:\n");
    fprintf(fp, "  PRF exist:       %s\n", thm->prf_exist ? "YES" : "NO");
    fprintf(fp, "  PRF has property: %s\n", thm->prf_has_property ? "YES" : "NO");
    fprintf(fp, "  PRF in P/poly:    %s\n", thm->prf_in_ppoly ? "YES" : "NO");
    fprintf(fp, "  Contradiction:   %s\n", thm->contradiction ? "YES" : "NO");
    fprintf(fp, "  Barrier holds:   %s\n", thm->barrier_holds ? "YES" : "NO");
    fprintf(fp, "\n");

    fprintf(fp, "Conclusion:\n");
    fprintf(fp, "  %s\n", thm->conclusion);
    fprintf(fp, "========================================\n\n");
}

int rr_theorem_verify(const RRTheorem *thm) {
    if (!thm) return 0;

    /* Verify the logical chain:
     * 1. If OWF AND P natural AND P useful against P/poly => CONTRADICTION
     * 2. Contradiction implies barrier
     *
     * Each implication must be sound. */

    /* Check (1): OWF => PRF (HILL 1999) — we assume this is true */
    if (thm->owf_exist && !thm->prf_exist) return 0;  /* must derive PRF */

    /* Check (2): PRF + P large => PRF has P (by indistinguishability) */
    if (thm->prf_exist && thm->property.large && !thm->prf_has_property)
        return 0;  /* must derive PRF has P */

    /* Check (3): PRF has P + P useful => PRF notin P/poly */
    if (thm->prf_has_property && thm->property.useful) {
        /* This should imply contradiction with PRF in P/poly */
        if (!thm->barrier_holds) return 0;  /* should have contradiction */
    }

    /* Check (4): Contradiction => barrier holds */
    if (thm->contradiction && !thm->barrier_holds) return 0;

    return 1;  /* All implications verified */
}

/* ========================================================================
 * Known Lower Bounds
 * ======================================================================== */

static KnownLowerBound KNOWN_BOUNDS[] = {
    {"AC0 PARITY lower bound",
     "Furst-Saxe-Sipser (1981), Ajtai (1983), Hastad (1987)",
     "AC0", "PARITY", 1, 1, 1, 1},
    {"AC0 MAJORITY lower bound",
     "Furst-Saxe-Sipser (1981), Hastad (1987)",
     "AC0", "MAJORITY", 1, 1, 1, 1},
    {"Monotone CLIQUE lower bound",
     "Razborov (1985)",
     "Monotone", "CLIQUE", 1, 1, 1, 1},
    {"AC0[p] MOD-q lower bound",
     "Razborov (1987), Smolensky (1987)",
     "AC0[p]", "MOD-q (q != p^k)", 1, 1, 1, 1},
    {"Monotone PERFECT-MATCHING lower bound",
     "Razborov (1985)",
     "Monotone", "PERFECT-MATCHING", 1, 1, 1, 1},
    {"Monotone TARDOS function",
     "Tardos (1988)",
     "Monotone", "Tardos function", 1, 1, 1, 1},
    {"Formula size PARITY lower bound",
     "Khrapchenko (1971)",
     "Formulas", "PARITY", 1, 1, 1, 1},
    {"Communication complexity DISJOINTNESS",
     "Kalyanasundaram-Schnitger (1987), Razborov (1990)",
     "Communication", "DISJOINTNESS", 1, 1, 1, 1}
};

int rr_known_bounds_count(void) {
    return sizeof(KNOWN_BOUNDS) / sizeof(KNOWN_BOUNDS[0]);
}

const KnownLowerBound *rr_known_bounds_get(int i) {
    if (i < 0 || i >= rr_known_bounds_count()) return NULL;
    return &KNOWN_BOUNDS[i];
}

void rr_known_bounds_print_all(FILE *fp) {
    if (!fp) fp = stdout;
    fprintf(fp, "\n========================================\n");
    fprintf(fp, "  ALL KNOWN CIRCUIT LOWER BOUNDS\n");
    fprintf(fp, "  (and their Natural Proof status)\n");
    fprintf(fp, "========================================\n\n");

    fprintf(fp, "%-45s %-15s %-20s %s\n",
            "Result", "Target Class", "Hard Function", "Natural?");
    fprintf(fp, "--------------------------------------------- --------------- "
            "-------------------- --------\n");

    int n = rr_known_bounds_count();
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%-45s %-15s %-20s %s\n",
                KNOWN_BOUNDS[i].name,
                KNOWN_BOUNDS[i].target_class,
                KNOWN_BOUNDS[i].hard_function,
                KNOWN_BOUNDS[i].is_natural ? "NATURAL" : "non-natural");
    }

    fprintf(fp, "\nCONCLUSION: All known circuit lower bounds are NATURAL.\n");
    fprintf(fp, "=> None can resolve P vs NP (assuming OWF exist).\n");
    fprintf(fp, "=> New NON-NATURAL techniques are needed.\n");
}

/* ========================================================================
 * Barrier Bypass Methods
 * ======================================================================== */

static const char *BYPASS_NAMES[] = {
    "Geometric Complexity Theory (GCT)",
    "Meta-Complexity (MCSP approach)",
    "Proof Complexity Lower Bounds",
    "Interactive Proofs & MIP* = RE",
    "Algebraic & Topological Methods",
    "Non-Black-Box Techniques"
};

static const char *BYPASS_DESCRIPTIONS[] = {
    "Mulmuley-Sohoni (2001): Use algebraic geometry and representation "
    "theory to prove lower bounds. The approach is inherently non-constructive "
    "(deciding properties via algebraic varieties rather than algorithms), "
    "thereby bypassing the constructiveness criterion of natural proofs.",

    "Recent approach: Prove MCSP (Minimum Circuit Size Problem) is NP-complete. "
    "If MCSP is NP-complete, then P != NP follows. The property used would be "
    "non-large (targeting specific functions rather than random ones), "
    "bypassing the largeness criterion.",

    "Prove exponential lower bounds for specific proof systems (Resolution, "
    "Frege, etc.). These are non-constructive in the natural proofs sense "
    "because they don't provide efficient algorithms for recognizing hard "
    "instances. They bypass the constructiveness criterion.",

    "MIP* = RE (Ji-Natarajan-Vidick-Wright-Yuen 2020): Quantum interactive "
    "proofs with entangled provers can decide the halting problem. "
    "This shows quantum techniques transcend classical barriers. "
    "Potentially bypasses all three barriers simultaneously.",

    "Use algebraic topology, category theory, or homotopy type theory to "
    "characterize complexity classes. These methods are inherently geometric "
    "and may not relativize, algebrize, or be constructive in the required sense.",

    "Barak's non-black-box techniques: Use the code of adversaries rather "
    "than just their input-output behavior. This circumvents the black-box "
    "nature of relativization and algebrization."
};

int rr_barrier_bypass_count(void) {
    return sizeof(BYPASS_NAMES) / sizeof(BYPASS_NAMES[0]);
}

int rr_barrier_bypass_get(int i, char *name, char *description) {
    if (i < 0 || i >= rr_barrier_bypass_count()) return 0;
    if (name) strncpy(name, BYPASS_NAMES[i], 63);
    if (description) strncpy(description, BYPASS_DESCRIPTIONS[i], 511);
    return 1;
}

void rr_barrier_bypass_print_all(FILE *fp) {
    if (!fp) fp = stdout;
    fprintf(fp, "\n========================================\n");
    fprintf(fp, "  BYPASSING THE NATURAL PROOFS BARRIER\n");
    fprintf(fp, "========================================\n\n");

    int n = rr_barrier_bypass_count();
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%d. %s\n", i + 1, BYPASS_NAMES[i]);
        fprintf(fp, "   %s\n\n", BYPASS_DESCRIPTIONS[i]);
    }
}

/* ========================================================================
 * Demo Functions
 * ======================================================================== */

void razborov_rudich_proof_demo(void) {
    printf("\n================================================================\n");
    printf("  RAZBOROV-RUDICH PROOF (1997)\n");
    printf("================================================================\n\n");

    rr_proof_print_all(stdout);

    /* Demonstrate with concrete parameters */
    printf("\n--- Demonstration with n=10, S=n^2=100, OWF exist ---\n");
    RRTheorem *thm = rr_theorem_create(1, 10, 100);
    rr_theorem_derive(thm);
    rr_theorem_print(thm, stdout);
    rr_theorem_free(thm);

    printf("--- Demonstration with n=10, S=2^n=1024, OWF exist ---\n");
    thm = rr_theorem_create(1, 10, 1024);
    rr_theorem_derive(thm);
    rr_theorem_print(thm, stdout);
    rr_theorem_free(thm);
}

void natural_examples_demo(void) {
    printf("\n================================================================\n");
    printf("  KNOWN NATURAL LOWER BOUNDS\n");
    printf("================================================================\n\n");

    rr_known_bounds_print_all(stdout);
    rr_barrier_bypass_print_all(stdout);
}
