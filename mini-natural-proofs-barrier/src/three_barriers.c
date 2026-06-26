/* three_barriers.c -- The Three Barriers to P vs NP
 *
 * L8: Advanced Topics
 *   - Relativization Barrier (Baker-Gill-Solovay 1975)
 *   - Natural Proofs Barrier (Razborov-Rudich 1997)
 *   - Algebrization Barrier (Aaronson-Wigderson 2009)
 *
 * Any proof of P != NP (or P = NP) must bypass ALL THREE barriers.
 * This is why P vs NP has remained open since Cook (1971).
 *
 * L7: Applications — understanding the fundamental limits of
 *     mathematical proof techniques.
 *
 * Overview of the three barriers:
 *
 * 1. RELATIVIZATION (BGS 1975):
 *    Technique: Provide an oracle (black-box) that a proof "relativizes"
 *    to, meaning the proof still works if we equip all machines with
 *    that oracle.
 *    Result: There exist oracles A, B such that P^A = NP^A and P^B != NP^B.
 *    => Any proof that relativizes cannot resolve P vs NP, because it
 *       would give contradictory results for different oracles.
 *    Examples: Diagonalization (Turing 1936), simulation arguments
 *       all relativize.
 *    What bypasses: Interactive proofs results (IP = PSPACE, Shamir 1992)
 *       do NOT relativize.
 *
 * 2. NATURAL PROOFS (RR 1997):
 *    Technique: A "natural" proof uses a property P that is constructive,
 *    large, and useful against a circuit class.
 *    Result: If OWF exist, no natural proof can separate NP from P/poly.
 *    All known circuit lower bounds are natural.
 *    What bypasses: Geometric Complexity Theory (GCT), meta-complexity,
 *       proof complexity lower bounds.
 *
 * 3. ALGEBRIZATION (AW 2009):
 *    Technique: Extension of relativization where the oracle is a
 *    Boolean function, but queries can be made to a low-degree polynomial
 *    extension of the oracle over a finite field.
 *    Result: Any proof that algebrizes cannot resolve P vs NP.
 *    Both IP=PSPACE and PCP Theorem algebrize, yet they use non-relativizing
 *    techniques (arithmetization). AW shows arithmetization itself is
 *    not enough — any proof technique that algebrizes cannot resolve P vs NP.
 *    What bypasses: GCT (does not algebrize), possibly interactive proofs
 *       with MIP* = RE.
 *
 * References:
 *   BGS (1975): SIAM J. Comput. 4(4):431-442
 *   RR (1997): JCSS 55(1):24-35
 *   AW (2009): J. ACM 56(5):1-59
 *   JNVWY (2020): "MIP* = RE"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "three_barriers.h"

/* ========================================================================
 * Oracle Implementation
 * ======================================================================== */

Oracle *oracle_create(const char *name, int max_query_len) {
    Oracle *o = (Oracle *)malloc(sizeof(Oracle));
    if (!o) return NULL;
    o->n_strings = 0;
    o->strings = NULL;
    o->lengths = NULL;
    o->capacity = 0;
    if (name) strncpy(o->name, name, 63);
    else o->name[0] = '\0';
    o->max_query_length = max_query_len;
    o->is_algebraic = 0;
    o->field_prime = 0;
    return o;
}

void oracle_free(Oracle *oracle) {
    if (!oracle) return;
    for (int i = 0; i < oracle->n_strings; i++) free(oracle->strings[i]);
    free(oracle->strings);
    free(oracle->lengths);
    free(oracle);
}

int oracle_add(Oracle *oracle, const char *str) {
    if (!oracle || !str) return 0;

    /* Check if already present */
    for (int i = 0; i < oracle->n_strings; i++) {
        if (strcmp(oracle->strings[i], str) == 0) return 0;
    }

    /* Expand capacity if needed */
    if (oracle->n_strings >= oracle->capacity) {
        int new_cap = (oracle->capacity == 0) ? 16 : oracle->capacity * 2;
        char **new_strs = (char **)realloc(
            oracle->strings, (size_t)new_cap * sizeof(char *));
        if (!new_strs) return 0;
        int *new_lens = (int *)realloc(
            oracle->lengths, (size_t)new_cap * sizeof(int));
        if (!new_lens) {
            oracle->strings = new_strs; /* keep old */
            return 0;
        }
        oracle->strings = new_strs;
        oracle->lengths = new_lens;
        oracle->capacity = new_cap;
    }

    int len = (int)strlen(str);
    oracle->strings[oracle->n_strings] = (char *)malloc(
        (size_t)(len + 1));
    if (!oracle->strings[oracle->n_strings]) return 0;
    strcpy(oracle->strings[oracle->n_strings], str);
    oracle->lengths[oracle->n_strings] = len;
    oracle->n_strings++;
    return 1;
}

int oracle_query(const Oracle *oracle, const char *str) {
    if (!oracle || !str) return 0;
    for (int i = 0; i < oracle->n_strings; i++) {
        if (strcmp(oracle->strings[i], str) == 0) return 1;
    }
    return 0;
}

Oracle *oracle_create_bgs_A(void) {
    /* Oracle A where P^A = NP^A = PSPACE.
     * A is a PSPACE-complete set (TQBF).
     * With oracle A: any PSPACE computation can be done in polynomial time.
     * So P^A = PSPACE, and NP^A = PSPACE as well.
     * Therefore: P^A = NP^A. */
    Oracle *A = oracle_create("BGS_A (PSPACE-complete)", 20);

    /* Add some representative strings for TQBF (simplified).
     * In reality, TQBF = {phi | phi is a true quantified Boolean formula}.
     * We simulate by adding a few strings as examples. */
    oracle_add(A, "TQBF");
    oracle_add(A, "forall_exists");
    oracle_add(A, "qbf_true");

    return A;
}

Oracle *oracle_create_bgs_B(int n) {
    /* Oracle B where P^B != NP^B.
     * Construction: For each polynomial-time machine M_i, we ensure
     * that L_B = {1^n | oracle contains some string of length n} is in NP^B
     * but not in P^B.
     *
     * This is done by diagonalization: for the i-th polynomial-time machine
     * with running time n^i + i, we ensure M_i^B(1^n) fails to decide L_B
     * for some n. */
    Oracle *B = oracle_create("BGS_B (separating oracle)", 20);

    /* Simplified: Add strings to make L_B an NP language.
     * L_B = {1^n | there exists a string y of length n in B}
     * Trivially in NP^B (guess y, query B(y)).
     * Not in P^B by diagonalization (construction ensures all poly-time
     * machines fail on some input). */
    for (int i = 0; i < n; i++) {
        char str[32];
        snprintf(str, 31, "separate_%d", i);
        oracle_add(B, str);
    }

    return B;
}

/* ========================================================================
 * Barrier Information
 * ======================================================================== */

static BarrierInfo BARRIER_INFO[] = {
    {BARRIER_RELATIVIZATION, "Relativization",
     "Baker, Gill, Solovay", 1975,
     "There exist oracles A, B such that P^A = NP^A and P^B != NP^B. "
     "Therefore, any proof technique that 'relativizes' (holds for all "
     "oracles) cannot resolve P vs NP.",
     "Rules out: diagonalization, simulation arguments, most TM-based "
     "separations. Essentially all techniques before interactive proofs.",
     "Interactive proofs (IP=PSPACE does not relativize), "
     "PCP theorem (partially non-relativizing), "
     "Circuit lower bounds (non-relativizing).",
     1},
    {BARRIER_NATURAL_PROOFS, "Natural Proofs",
     "Razborov, Rudich", 1997,
     "If one-way functions exist, no 'natural proof' (constructive + large "
     "+ useful property) can separate NP from P/poly. "
     "All known circuit lower bounds are natural.",
     "Rules out: all known circuit lower bound techniques for P vs NP. "
     "AC0 bounds, monotone bounds, AC0[p] bounds, communication complexity "
     "bounds — all natural, all useless for P vs NP.",
     "GCT (non-constructive), meta-complexity (non-large), "
     "proof complexity (non-constructive), quantum methods.",
     1},
    {BARRIER_ALGEBRIZATION, "Algebrization",
     "Aaronson, Wigderson", 2009,
     "Extension of relativization: oracles can be extended to low-degree "
     "polynomials over finite fields. Any proof that 'algebrizes' (holds "
     "for both the original oracle and its algebraic extension) cannot "
     "resolve P vs NP.",
     "Rules out: arithmetization-based techniques (used in IP=PSPACE, "
     "PCP theorem). Even these breakthroughs don't suffice for P vs NP.",
     "GCT (does not algebrize), quantum interactive proofs (MIP* = RE), "
     "possibly meta-complexity.",
     1}
};

const BarrierInfo *barrier_info_get(BarrierType type) {
    if (type >= BARRIER_NUM) return NULL;
    return &BARRIER_INFO[type];
}

void barrier_info_print(BarrierType type, FILE *fp) {
    if (!fp) fp = stdout;
    if (type >= BARRIER_NUM) return;

    const BarrierInfo *bi = &BARRIER_INFO[type];
    fprintf(fp, "\n========================================\n");
    fprintf(fp, "  BARRIER %d: %s (%s, %d)\n",
            type + 1, bi->name, bi->authors, bi->year);
    fprintf(fp, "========================================\n\n");
    fprintf(fp, "Statement:\n  %s\n\n", bi->statement);
    fprintf(fp, "Consequence:\n  %s\n\n", bi->consequence);
    fprintf(fp, "What can bypass:\n  %s\n\n", bi->bypass_candidates);
}

void barrier_all_print(FILE *fp) {
    if (!fp) fp = stdout;
    fprintf(fp, "\n========================================\n");
    fprintf(fp, "  THE THREE BARRIERS TO P vs NP\n");
    fprintf(fp, "========================================\n");
    fprintf(fp, "\nAny proof that P != NP (or P = NP) must bypass ALL THREE.\n");
    fprintf(fp, "This is why the problem has resisted resolution for 50+ years.\n\n");

    for (int i = 0; i < BARRIER_NUM; i++) {
        const BarrierInfo *bi = &BARRIER_INFO[i];
        fprintf(fp, "%d. %s (%s, %d)\n", i + 1, bi->name, bi->authors, bi->year);
        fprintf(fp, "   %s\n", bi->statement);
        fprintf(fp, "   Bypassable? %s\n\n",
                bi->is_bypassable ? "YES — known techniques exist" : "Unknown");
    }
}

/* ========================================================================
 * ThreeBarriersSim: Barrier Simulation
 * ======================================================================== */

ThreeBarriersSim *barriers_sim_create(int n_inputs, int circuit_size,
                                       int assume_owf) {
    ThreeBarriersSim *sim = (ThreeBarriersSim *)malloc(
        sizeof(ThreeBarriersSim));
    if (!sim) return NULL;

    sim->oracle_A = oracle_create_bgs_A();
    sim->oracle_B = oracle_create_bgs_B(10);
    sim->n_inputs = n_inputs;
    sim->circuit_size = circuit_size;
    sim->natural_prop = shannon_natural_criteria_check(n_inputs, circuit_size);
    sim->relativization_blocks = 0;
    sim->natural_proofs_blocks = 0;
    sim->algebrization_blocks = 0;
    sim->all_barriers_hold = 0;

    (void)assume_owf;  /* parameter reserved for future use */
    return sim;
}

void barriers_sim_free(ThreeBarriersSim *sim) {
    if (!sim) return;
    oracle_free(sim->oracle_A);
    oracle_free(sim->oracle_B);
    free(sim);
}

void barriers_sim_run(ThreeBarriersSim *sim) {
    if (!sim) return;

    /* 1. Relativization check */
    /* If the proof uses only relativizing techniques (diagonalization),
     * it would work the same way with any oracle. But we know there are
     * oracles making P=NP and oracles making P!=NP. So relativizing
     * proofs give contradictory results and must be invalid. */
    sim->relativization_blocks = 1;

    /* 2. Natural Proofs check */
    /* If the proof uses a natural property (constructive + large + useful)
     * and OWF exist, then it cannot separate NP from P/poly. */
    if (sim->natural_prop.is_natural) {
        sim->natural_proofs_blocks = 1;
    }

    /* 3. Algebrization check */
    /* If the proof algebrizes (works for algebraic oracle extensions),
     * it cannot resolve P vs NP. Most arithmetization-based proofs
     * (IP=PSPACE, PCP) algebrize. */
    sim->algebrization_blocks = 1;  /* Most techniques algebrize */

    /* Aggregate */
    sim->all_barriers_hold = (sim->relativization_blocks &&
                               sim->natural_proofs_blocks &&
                               sim->algebrization_blocks);
}

void barriers_sim_print(const ThreeBarriersSim *sim, FILE *fp) {
    if (!sim) { fprintf(fp, "NULL simulation\n"); return; }
    if (!fp) fp = stdout;

    fprintf(fp, "\n========================================\n");
    fprintf(fp, "  THREE BARRIERS SIMULATION RESULTS\n");
    fprintf(fp, "========================================\n\n");

    fprintf(fp, "Parameters: n=%d, circuit_size=%d\n\n", sim->n_inputs, sim->circuit_size);

    fprintf(fp, "1. RELATIVIZATION Barrier\n");
    fprintf(fp, "   Blocks relativizing proofs: %s\n",
            sim->relativization_blocks ? "YES" : "NO");
    fprintf(fp, "   If YES: diagonalization/simulation cannot resolve P vs NP.\n\n");

    fprintf(fp, "2. NATURAL PROOFS Barrier\n");
    fprintf(fp, "   Natural property? %s\n",
            sim->natural_prop.is_natural ? "YES" : "NO");
    fprintf(fp, "   Blocks natural proofs: %s\n",
            sim->natural_proofs_blocks ? "YES" : "NO");
    fprintf(fp, "   If YES: all known circuit lower bound techniques blocked.\n\n");

    fprintf(fp, "3. ALGEBRIZATION Barrier\n");
    fprintf(fp, "   Blocks algebrizing proofs: %s\n",
            sim->algebrization_blocks ? "YES" : "NO");
    fprintf(fp, "   If YES: arithmetization-based techniques blocked.\n\n");

    fprintf(fp, "========================================\n");
    fprintf(fp, "OVERALL: All three barriers hold: %s\n",
            sim->all_barriers_hold ? "YES" : "SOME BYPASSED");
    if (sim->all_barriers_hold) {
        fprintf(fp, "=> Proof technique is blocked by at least one barrier.\n");
        fprintf(fp, "=> P vs NP cannot be resolved by this approach.\n");
    }
    fprintf(fp, "========================================\n");
}

/* ========================================================================
 * Bypass Candidates
 * ======================================================================== */

static BypassCandidate BYPASS_CANDIDATES[] = {
    {"Geometric Complexity Theory (GCT)",
     "Mulmuley, Sohoni", 2001,
     "Use algebraic geometry and representation theory to prove lower bounds "
     "by showing that certain algebraic varieties have exponential dimension.",
     {"Natural Proofs", "Algebrization", "", 0}, 2,
     "Active research. Partial results. Full program is multi-decade.",
     "Circuit lower bounds reduced to showing that permanent polynomial "
     "is not in the orbit closure of determinant."},
    {"Meta-Complexity (MCSP)",
     "Allender, Hirahara, Oliveira, Santhanam, et al.", 2015,
     "Prove MCSP (Minimum Circuit Size Problem) is NP-complete. "
     "If MCSP is NP-complete, P != NP follows as corollary.",
     {"Natural Proofs", "", "", 0}, 1,
     "Active research. Known: MCSP is in NP but NP-completeness open. "
     "Connections to learning theory and cryptography.",
     "MCSP hardness would give non-natural proof because MCSP is not "
     "known to be large in the natural proofs sense."},
    {"Proof Complexity",
     "Cook, Reckhow, Krajicek, Razborov, et al.", 1979,
     "Prove exponential lower bounds for specific proof systems "
     "(Extended Frege, etc.). Non-constructive approach.",
     {"Natural Proofs", "", "", 0}, 1,
     "Active. Resolution lower bounds known. Frege lower bounds open. "
     "Connections to bounded arithmetic.",
     "Proof complexity lower bounds avoid constructiveness: they show "
     "hardness for specific formulas without requiring efficient recognition."},
    {"MIP* = RE (Quantum Interactive Proofs)",
     "Ji, Natarajan, Vidick, Wright, Yuen", 2020,
     "Quantum interactive proofs with entangled provers can decide the "
     "halting problem. Transcends classical barriers.",
     {"Relativization", "Algebrization", "Natural Proofs", 0}, 3,
     "Major breakthrough (2020). Shows quantum techniques are fundamentally "
     "more powerful. Implications for P vs NP still being explored.",
     "MIP* = RE shows that quantum entanglement allows verification of "
     "uncomputable problems, far beyond NP."},
    {"Information-Theoretic Methods",
     "Braverman, Rao, Weinstein, Yehudayoff", 2015,
     "Use information theory and compression arguments for circuit lower "
     "bounds. May avoid natural proofs by not being constructive in the "
     "required sense.",
     {"Natural Proofs", "", "", 0}, 1,
     "Active. Compression-based lower bounds for depth-2, monotone, etc. "
     "Scaling to P/poly remains open.",
     "Information complexity measures are not obviously constructive in "
     "the 2^{O(n)} sense from truth tables."},
};

int bypass_candidates_count(void) {
    return sizeof(BYPASS_CANDIDATES) / sizeof(BYPASS_CANDIDATES[0]);
}

const BypassCandidate *bypass_candidates_get(int i) {
    if (i < 0 || i >= bypass_candidates_count()) return NULL;
    return &BYPASS_CANDIDATES[i];
}

void bypass_candidates_print_all(FILE *fp) {
    if (!fp) fp = stdout;
    fprintf(fp, "\n========================================\n");
    fprintf(fp, "  CANDIDATES TO BYPASS THE BARRIERS\n");
    fprintf(fp, "========================================\n\n");

    int n = bypass_candidates_count();
    for (int i = 0; i < n; i++) {
        BypassCandidate *bc = &BYPASS_CANDIDATES[i];
        fprintf(fp, "%d. %s\n", i + 1, bc->name);
        fprintf(fp, "   Leaders: %s (since %d)\n", bc->leader, bc->year_started);
        fprintf(fp, "   Approach: %s\n", bc->approach);
        fprintf(fp, "   Bypasses: ");
        for (int j = 0; j < bc->n_bypasses; j++) {
            fprintf(fp, "%s%s", bc->bypasses[j],
                    (j + 1 < bc->n_bypasses) ? ", " : "");
        }
        fprintf(fp, "\n");
        fprintf(fp, "   Status: %s\n", bc->status);
        fprintf(fp, "   Key insight: %s\n\n", bc->key_insight);
    }
}

/* ========================================================================
 * Demonstration Functions
 * ======================================================================== */

void barrier_demo_relativization(void) {
    printf("\n================================================================\n");
    printf("  BARRIER 1: RELATIVIZATION (Baker-Gill-Solovay 1975)\n");
    printf("================================================================\n\n");

    printf("DEFINITION: A proof 'relativizes' if it holds when all machines\n");
    printf("are given access to the same oracle (black-box).\n\n");

    printf("THEOREM: There exist oracles A, B such that:\n");
    printf("  P^A = NP^A  (collapsing oracle)\n");
    printf("  P^B != NP^B (separating oracle)\n\n");

    printf("WHY THIS IS A BARRIER:\n");
    printf("  If a proof that P = NP relativizes, it would also prove P^B = NP^B,\n");
    printf("  which is FALSE. So the proof must be wrong.\n");
    printf("  If a proof that P != NP relativizes, it would also prove P^A != NP^A,\n");
    printf("  which is FALSE. So the proof must be wrong.\n\n");

    printf("EXAMPLES OF RELATIVIZING TECHNIQUES:\n");
    printf("  - Diagonalization (Turing 1936)\n");
    printf("  - Simulation arguments\n");
    printf("  - Clocking constructions\n\n");

    printf("EXAMPLES OF NON-RELATIVIZING TECHNIQUES:\n");
    printf("  - Interactive proofs: IP = PSPACE (Shamir 1992)\n");
    printf("  - PCP Theorem (Arora-Safra, Arora-Lund-Motwani-Sudan-Szegedy 1998)\n");
    printf("  - Circuit lower bounds (non-uniform model)\n\n");

    printf("WHAT BYPASSES THIS BARRIER:\n");
    printf("  Any technique that uses the code of Turing machines\n");
    printf("  rather than just their input-output behavior.\n");
}

void barrier_demo_natural_proofs(void) {
    printf("\n================================================================\n");
    printf("  BARRIER 2: NATURAL PROOFS (Razborov-Rudich 1997)\n");
    printf("================================================================\n\n");

    printf("DEFINITION: A 'natural proof' uses a property P that is:\n");
    printf("  1. CONSTRUCTIVE: Decidable in 2^{O(n)} time from truth table.\n");
    printf("  2. LARGE: Holds for >= 2^{-O(n)} fraction of functions.\n");
    printf("  3. USEFUL: Implies circuit lower bound.\n\n");

    printf("THEOREM: If one-way functions exist, no natural proof can\n");
    printf("show that SAT notin P/poly.\n\n");

    printf("WHY THIS IS A BARRIER:\n");
    printf("  ALL known circuit lower bounds are NATURAL:\n");
    printf("  - AC0 != AC0[p] (FSS/Ajtai/Hastad) — natural\n");
    printf("  - Monotone lower bounds (Razborov) — natural\n");
    printf("  - AC0[p] lower bounds (Razborov-Smolensky) — natural\n");
    printf("  - Communication complexity bounds — natural\n\n");
    printf("  => Existing techniques cannot prove P != NP (if OWF exist).\n");
    printf("  => New NON-NATURAL techniques are needed.\n\n");

    printf("PRF CONTRADICTION:\n");
    printf("  OWF => PRF => PRF looks random => PRF satisfies natural P\n");
    printf("  => P useful => PRF is hard. But PRF is easy. CONTRADICTION!\n\n");

    printf("WHAT BYPASSES THIS BARRIER:\n");
    printf("  - GCT (non-constructive: uses algebraic geometry)\n");
    printf("  - Meta-complexity (non-large: targets MCSP not random fns)\n");
    printf("  - Proof complexity (non-constructive: hard instances)\n");
}

void barrier_demo_algebrization(void) {
    printf("\n================================================================\n");
    printf("  BARRIER 3: ALGEBRIZATION (Aaronson-Wigderson 2009)\n");
    printf("================================================================\n\n");

    printf("DEFINITION: A proof 'algebrizes' if it holds when oracles are\n");
    printf("replaced by their low-degree polynomial extensions over GF(2).\n\n");

    printf("THEOREM: Any proof that algebrizes cannot resolve P vs NP.\n\n");

    printf("WHY THIS IS A BARRIER:\n");
    printf("  IP=PSPACE and the PCP Theorem both use arithmetization\n");
    printf("  (replacing Boolean formulas with polynomials).\n");
    printf("  But this technique ALGEBRIZES, so it cannot go further\n");
    printf("  to prove P != NP.\n\n");

    printf("WHAT ALGEBRIZES:\n");
    printf("  - IP=PSPACE (Shamir 1992)\n");
    printf("  - PCP Theorem\n");
    printf("  - MA_EXP notin P/poly (Buhrman-Fortnow-Thierauf 1998)\n\n");

    printf("WHAT DOES NOT ALGEBRIZE:\n");
    printf("  - GCT (geometric, not algebraic in this sense)\n");
    printf("  - MIP* = RE (quantum entanglement transcends)\n");
    printf("  - Possibly circuit lower bound techniques\n\n");

    printf("KEY INSIGHT:\n");
    printf("  Even the most successful non-relativizing technique\n");
    printf("  (arithmetization) is insufficient for P vs NP.\n");
    printf("  A fundamentally new idea is needed.\n");
}

void barrier_demo_all(void) {
    barrier_demo_relativization();
    barrier_demo_natural_proofs();
    barrier_demo_algebrization();

    printf("\n================================================================\n");
    printf("  SUMMARY: WHY P vs NP IS SO HARD\n");
    printf("================================================================\n\n");

    printf("To resolve P vs NP, a proof must:\n");
    printf("  1. NOT relativize (bypass BGS 1975)\n");
    printf("  2. NOT be a natural proof (bypass RR 1997)\n");
    printf("  3. NOT algebrize (bypass AW 2009)\n\n");

    printf("No known technique satisfies all three simultaneously.\n");
    printf("The leading candidates:\n");
    printf("  - Geometric Complexity Theory (Mulmuley-Sohoni)\n");
    printf("  - Meta-Complexity & MCSP\n");
    printf("  - Quantum Interactive Proofs (MIP* = RE)\n\n");

    printf("This is why P vs NP has remained open since 1971.\n");

    barrier_all_print(stdout);
    bypass_candidates_print_all(stdout);
}

void three_barriers_demo(void) {
    barrier_demo_all();
}

void bypass_candidates_demo(void) {
    printf("\n================================================================\n");
    printf("  BYPASS CANDIDATES: Escaping the Natural Proofs Barrier\n");
    printf("================================================================\n\n");

    bypass_candidates_print_all(stdout);

    printf("Strategy for bypassing Natural Proofs:\n");
    printf("  Option A: Avoid CONSTRUCTIVENESS\n");
    printf("    -> Use properties that are not decidable in 2^{O(n)} time.\n");
    printf("    -> Example: GCT uses algebraic geometry (inherently non-constructive).\n");
    printf("  Option B: Avoid LARGENESS\n");
    printf("    -> Use properties that hold for a tiny fraction of functions.\n");
    printf("    -> Example: Meta-complexity targets MCSP, not random functions.\n");
    printf("  Option C: Prove OWF do NOT exist.\n");
    printf("    -> Would break all cryptography but bypass the barrier.\n");
    printf("    -> Unlikely to be true.\n");
}

void constructive_violation_demo(void) {
    printf("\n=== Constructiveness Analysis ===\n\n");
    printf("Constructiveness: P(f) must be decidable in 2^{O(n)} time.\n\n");

    for (int n = 4; n <= 16; n += 4) {
        printf("  n=%d:\n", n);
        int S_values[] = {n, n * n, n * n * n,
                          (int)pow(2.0, n / 2.0),
                          (int)pow(2.0, n)};
        for (int si = 0; si < 5; si++) {
            int S = S_values[si];
            double cost = shannon_constructiveness_cost(n, S);
            int is_constructive = shannon_is_constructive(n, S);
            printf("    S=%-6d: cost=%.2e (constructive: %s)\n",
                   S, cost, is_constructive ? "YES" : "NO");
        }
        printf("\n");
    }
    printf("Key: For S = poly(n), P is constructive.\n");
    printf("     For S = exp(n), P is NOT constructive => avoids barrier.\n");
    printf("     But then P is not useful against P/poly.\n");
}
