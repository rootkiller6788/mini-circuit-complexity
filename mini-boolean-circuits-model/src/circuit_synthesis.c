/* circuit_synthesis.c -- Logic Synthesis and Technology Mapping
 *
 * Implements Boolean circuit synthesis from truth tables via multiple
 * methods: DNF/CNF, Quine-McCluskey exact minimization, Reed-Muller
 * (ESOP), and Shannon decomposition. Also includes technology mapping
 * to standard bases and advanced heuristic synthesis.
 *
 * Theorem (Quine-McCluskey 1952/1956): The QM algorithm finds the
 *   exact minimum sum-of-products form for a Boolean function.
 * Theorem (Reed-Muller 1954): Every Boolean function has a unique
 *   representation as XOR of AND terms (algebraic normal form).
 * Theorem (Shannon 1949): f(x1,...,xn) = (~x1 & f(0,x2,...,xn)) |
 *   (x1 & f(1,x2,...,xn)), enabling recursive decomposition.
 *
 * References: De Micheli (1994); Brayton et al. (1990); Jukna (2012) Ch.4.
 */

#include "circuit_synthesis.h"
#include "circuit_analysis.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>


/* ===================================================================
 * L5: Truth Table Synthesis - DNF, CNF
 * =================================================================== */

BooleanCircuit* circuit_synthesize_dnf(const int* truth, int n_vars) {
    assert(truth && n_vars > 0 && n_vars <= 16);
    long long total = 1LL << n_vars;
    int n_sat = 0;
    for (long long t = 0; t < total; t++) if (truth[t]) n_sat++;
    int cap = n_vars + n_sat * (n_vars * 2 + 2) + 10;
    BooleanCircuit* c = circuit_create(cap);
    int* in_ids = (int*)malloc((size_t)n_vars * sizeof(int));
    assert(in_ids);
    for (int j = 0; j < n_vars; j++) in_ids[j] = circuit_add_gate(c, INPUT, -1, -1);
    int* term_ids = (int*)malloc((size_t)(n_sat + 1) * sizeof(int));
    assert(term_ids);
    int n_terms = 0;
    for (long long t = 0; t < total; t++) {
        if (!truth[t]) continue;
        int* lits = (int*)malloc((size_t)n_vars * sizeof(int));
        assert(lits);
        int nl = 0;
        for (int j = 0; j < n_vars; j++) {
            if ((t >> j) & 1) lits[nl++] = in_ids[j];
            else { int ng = circuit_add_gate(c, NOT, in_ids[j], -1); lits[nl++] = ng; }
        }
        int term = lits[0];
        for (int j = 1; j < nl; j++) term = circuit_add_gate(c, AND, term, lits[j]);
        term_ids[n_terms++] = term;
        free(lits);
    }
    int out;
    if (n_terms == 0) out = circuit_add_gate(c, CONST0, -1, -1);
    else if (n_terms == 1) out = term_ids[0];
    else { out = term_ids[0]; for (int i = 1; i < n_terms; i++) out = circuit_add_gate(c, OR, out, term_ids[i]); }
    circuit_set_output(c, &out, 1);
    free(term_ids); free(in_ids);
    return c;
}

BooleanCircuit* circuit_synthesize_cnf(const int* truth, int n_vars) {
    assert(truth && n_vars > 0 && n_vars <= 16);
    long long total = 1LL << n_vars;
    int n_unsat = 0;
    for (long long t = 0; t < total; t++) if (!truth[t]) n_unsat++;
    int cap = n_vars + n_unsat * (n_vars * 2 + 2) + 10;
    BooleanCircuit* c = circuit_create(cap);
    int* in_ids = (int*)malloc((size_t)n_vars * sizeof(int));
    assert(in_ids);
    for (int j = 0; j < n_vars; j++) in_ids[j] = circuit_add_gate(c, INPUT, -1, -1);
    int* clause_ids = (int*)malloc((size_t)(n_unsat + 1) * sizeof(int));
    assert(clause_ids);
    int n_clauses = 0;
    for (long long t = 0; t < total; t++) {
        if (truth[t]) continue;
        int* lits = (int*)malloc((size_t)n_vars * sizeof(int));
        assert(lits);
        int nl = 0;
        for (int j = 0; j < n_vars; j++) {
            if (!((t >> j) & 1)) lits[nl++] = in_ids[j];
            else { int ng = circuit_add_gate(c, NOT, in_ids[j], -1); lits[nl++] = ng; }
        }
        int clause = lits[0];
        for (int j = 1; j < nl; j++) clause = circuit_add_gate(c, OR, clause, lits[j]);
        clause_ids[n_clauses++] = clause;
        free(lits);
    }
    int out;
    if (n_clauses == 0) out = circuit_add_gate(c, CONST1, -1, -1);
    else if (n_clauses == 1) out = clause_ids[0];
    else { out = clause_ids[0]; for (int i = 1; i < n_clauses; i++) out = circuit_add_gate(c, AND, out, clause_ids[i]); }
    circuit_set_output(c, &out, 1);
    free(clause_ids); free(in_ids);
    return c;
}


/* ===================================================================
 * L5: Quine-McCluskey Exact Minimization
 *
 * QM Algorithm (Quine 1952, McCluskey 1956):
 *   1. List all minterms where f = 1
 *   2. Group by number of 1-bits
 *   3. Combine adjacent groups: terms differing in one bit => new term
 *   4. Uncombined terms are prime implicants
 *   5. Build covering table: which PIs cover which minterms
 *   6. Find essential PIs (cover some minterm uniquely)
 *   7. Greedy set cover for remaining minterms
 *   8. Build circuit from selected PIs
 * =================================================================== */

static int count_ones(int x) {
    int c = 0;
    while (x) { c += x & 1; x >>= 1; }
    return c;
}

static int diff_one_bit(int a, int b, int* mask) {
    int d = a ^ b;
    if (d == 0) return 0;
    if ((d & (d - 1)) == 0) { *mask = d; return 1; }
    return 0;
}

BooleanCircuit* circuit_synthesize_qm(const int* truth, int n_vars) {
    assert(truth && n_vars > 0 && n_vars <= 8);
    long long total = 1LL << n_vars;
    int* minterms = (int*)malloc((size_t)total * sizeof(int));
    assert(minterms);
    int n_mt = 0;
    for (int t = 0; t < (int)total; t++) if (truth[t]) minterms[n_mt++] = t;

    if (n_mt == 0) {
        BooleanCircuit* c = circuit_create(10);
        int z = circuit_add_gate(c, CONST0, -1, -1);
        circuit_set_output(c, &z, 1);
        free(minterms);
        return c;
    }
    if (n_mt == (int)total) {
        BooleanCircuit* c = circuit_create(10);
        int o = circuit_add_gate(c, CONST1, -1, -1);
        circuit_set_output(c, &o, 1);
        free(minterms);
        return c;
    }

    /* Step 1-2: Group minterms by popcount */
    int max_pi = 1 << n_vars;
    int* groups[16];
    int  gsize[16];
    for (int i = 0; i <= n_vars; i++) {
        groups[i] = (int*)malloc((size_t)max_pi * sizeof(int));
        gsize[i] = 0;
    }
    for (int i = 0; i < n_mt; i++) {
        int ones = count_ones(minterms[i]);
        groups[ones][gsize[ones]++] = minterms[i];
    }

    /* Step 3-4: Iteratively combine to find prime implicants */
    int* imp_terms = (int*)malloc((size_t)(max_pi * 2) * sizeof(int));
    int* imp_masks = (int*)malloc((size_t)(max_pi * 2) * sizeof(int));
    assert(imp_terms && imp_masks);
    int n_imp = 0;
    for (int i = 0; i < n_mt; i++) {
        imp_terms[n_imp] = minterms[i];
        imp_masks[n_imp] = 0;
        n_imp++;
    }

    int* pi_terms = (int*)malloc((size_t)max_pi * sizeof(int));
    int* pi_masks = (int*)malloc((size_t)max_pi * sizeof(int));
    int* pi_used  = (int*)malloc((size_t)max_pi * sizeof(int));
    assert(pi_terms && pi_masks && pi_used);
    int n_pi = 0;

    int* combined = (int*)calloc((size_t)(max_pi * 2), sizeof(int));
    assert(combined);

    for (int iter = 0; iter < n_vars; iter++) {
        int n_new = 0;
        int* new_terms = (int*)malloc((size_t)(max_pi * 2) * sizeof(int));
        int* new_masks = (int*)malloc((size_t)(max_pi * 2) * sizeof(int));
        assert(new_terms && new_masks);

        for (int i = 0; i < n_imp; i++) {
            for (int j = i + 1; j < n_imp; j++) {
                if (imp_masks[i] != imp_masks[j]) continue;
                int mask;
                if (diff_one_bit(imp_terms[i], imp_terms[j], &mask)) {
                    int new_term = imp_terms[i] & ~mask;
                    int new_mask = imp_masks[i] | mask;
                    int dup = 0;
                    for (int k = 0; k < n_new; k++)
                        if (new_terms[k] == new_term && new_masks[k] == new_mask)
                            { dup = 1; break; }
                    if (!dup) {
                        new_terms[n_new] = new_term;
                        new_masks[n_new] = new_mask;
                        n_new++;
                    }
                    combined[i] = 1;
                    combined[j] = 1;
                }
            }
        }

        /* Uncombined become prime implicants */
        for (int i = 0; i < n_imp; i++) {
            if (!combined[i]) {
                int dup = 0;
                for (int k = 0; k < n_pi; k++)
                    if (pi_terms[k] == imp_terms[i] && pi_masks[k] == imp_masks[k])
                        { dup = 1; break; }
                if (!dup) {
                    pi_terms[n_pi] = imp_terms[i];
                    pi_masks[n_pi] = imp_masks[i];
                    pi_used[n_pi] = 0;
                    n_pi++;
                }
            }
        }

        if (n_new == 0) { free(new_terms); free(new_masks); break; }
        free(imp_terms); free(imp_masks);
        imp_terms = new_terms;
        imp_masks = new_masks;
        n_imp = n_new;
        memset(combined, 0, (size_t)(max_pi * 2) * sizeof(int));
    }

    /* Remaining are also prime */
    for (int i = 0; i < n_imp; i++) {
        int dup = 0;
        for (int k = 0; k < n_pi; k++)
            if (pi_terms[k] == imp_terms[i] && pi_masks[k] == imp_masks[k])
                { dup = 1; break; }
        if (!dup) {
            pi_terms[n_pi] = imp_terms[i];
            pi_masks[n_pi] = imp_masks[i];
            pi_used[n_pi] = 0;
            n_pi++;
        }
    }

    /* Step 5-6: Build covering table, find essential PIs */
    int** covers = (int**)malloc((size_t)n_pi * sizeof(int*));
    assert(covers);
    for (int i = 0; i < n_pi; i++) {
        covers[i] = (int*)calloc((size_t)n_mt, sizeof(int));
        assert(covers[i]);
        for (int j = 0; j < n_mt; j++) {
            if ((minterms[j] & ~pi_masks[i]) == pi_terms[i])
                covers[i][j] = 1;
        }
    }

    int* minterm_covered = (int*)calloc((size_t)n_mt, sizeof(int));
    assert(minterm_covered);

    /* Find essential prime implicants */
    for (int j = 0; j < n_mt; j++) {
        int cnt = 0, last = -1;
        for (int i = 0; i < n_pi; i++)
            if (covers[i][j]) { cnt++; last = i; }
        if (cnt == 1 && last >= 0) {
            pi_used[last] = 1;
            for (int k = 0; k < n_mt; k++)
                if (covers[last][k]) minterm_covered[k] = 1;
        }
    }

    /* Step 7: Greedy set cover for remaining minterms */
    for (int j = 0; j < n_mt; j++) {
        if (minterm_covered[j]) continue;
        int best_i = -1, best_cnt = 0;
        for (int i = 0; i < n_pi; i++) {
            if (pi_used[i]) continue;
            int cnt = 0;
            for (int k = 0; k < n_mt; k++)
                if (!minterm_covered[k] && covers[i][k]) cnt++;
            if (cnt > best_cnt) { best_cnt = cnt; best_i = i; }
        }
        if (best_i >= 0) {
            pi_used[best_i] = 1;
            for (int k = 0; k < n_mt; k++)
                if (covers[best_i][k]) minterm_covered[k] = 1;
        } else break;
    }

    /* Step 8: Build circuit from selected prime implicants */
    int cap = n_vars + n_pi * (n_vars * 2 + 2) + 10;
    BooleanCircuit* c = circuit_create(cap);
    int* in_ids = (int*)malloc((size_t)n_vars * sizeof(int));
    assert(in_ids);
    for (int j = 0; j < n_vars; j++) in_ids[j] = circuit_add_gate(c, INPUT, -1, -1);

    int* term_ids = (int*)malloc((size_t)n_pi * sizeof(int));
    assert(term_ids);
    int n_terms = 0;

    for (int i = 0; i < n_pi; i++) {
        if (!pi_used[i]) continue;
        int* lits = (int*)malloc((size_t)n_vars * sizeof(int));
        assert(lits);
        int nl = 0;
        for (int j = 0; j < n_vars; j++) {
            if (pi_masks[i] & (1 << j)) continue;
            if (pi_terms[i] & (1 << j)) lits[nl++] = in_ids[j];
            else { int ng = circuit_add_gate(c, NOT, in_ids[j], -1); lits[nl++] = ng; }
        }
        int term;
        if (nl == 0) term = circuit_add_gate(c, CONST1, -1, -1);
        else if (nl == 1) term = lits[0];
        else { term = lits[0]; for (int j = 1; j < nl; j++) term = circuit_add_gate(c, AND, term, lits[j]); }
        term_ids[n_terms++] = term;
        free(lits);
    }

    int out;
    if (n_terms == 0) out = circuit_add_gate(c, CONST0, -1, -1);
    else if (n_terms == 1) out = term_ids[0];
    else { out = term_ids[0]; for (int i = 1; i < n_terms; i++) out = circuit_add_gate(c, OR, out, term_ids[i]); }
    circuit_set_output(c, &out, 1);

    /* Cleanup */
    free(term_ids); free(in_ids); free(minterm_covered);
    for (int i = 0; i < n_pi; i++) free(covers[i]);
    free(covers); free(pi_terms); free(pi_masks); free(pi_used);
    free(imp_terms); free(imp_masks); free(combined); free(minterms);
    for (int i = 0; i <= n_vars; i++) free(groups[i]);
    return c;
}

/* ===================================================================
 * L5: ESOP Synthesis (Reed-Muller / Algebraic Normal Form)
 * =================================================================== */

BooleanCircuit* circuit_synthesize_esop(const int* truth, int n_vars) {
    assert(truth != NULL);
    assert(n_vars > 0 && n_vars <= 12);
    long long total = 1LL << n_vars;
    int* rm_coeffs = (int*)malloc((size_t)total * sizeof(int));
    assert(rm_coeffs);
    for (long long t = 0; t < total; t++)
        rm_coeffs[t] = truth[t];
    /* Fast Walsh transform in GF(2) for Reed-Muller coefficients */
    for (int step = 0; step < n_vars; step++) {
        int bit = 1 << step;
        for (long long i = 0; i < total; i += bit * 2) {
            for (int j = 0; j < bit; j++) {
                long long idx = i + j;
                rm_coeffs[idx + bit] ^= rm_coeffs[idx];
            }
        }
    }
    int cap = n_vars + (int)total * (n_vars + 2) + 10;
    BooleanCircuit* c = circuit_create(cap);
    int in_ids[16];
    for (int j = 0; j < n_vars; j++)
        in_ids[j] = circuit_add_gate(c, INPUT, -1, -1);
    int* xor_terms = (int*)malloc((size_t)(total + 1) * sizeof(int));
    assert(xor_terms);
    int n_terms = 0;
    for (long long t = 0; t < total; t++) {
        if (!rm_coeffs[t]) continue;
        int* lits = (int*)malloc((size_t)n_vars * sizeof(int));
        assert(lits);
        int nl = 0;
        for (int j = 0; j < n_vars; j++)
            if ((t >> j) & 1) lits[nl++] = in_ids[j];
        int term;
        if (nl == 0) term = circuit_add_gate(c, CONST1, -1, -1);
        else if (nl == 1) term = lits[0];
        else {
            term = lits[0];
            for (int k = 1; k < nl; k++)
                term = circuit_add_gate(c, AND, term, lits[k]);
        }
        xor_terms[n_terms++] = term;
        free(lits);
    }
    int out;
    if (n_terms == 0) out = circuit_add_gate(c, CONST0, -1, -1);
    else if (n_terms == 1) out = xor_terms[0];
    else {
        out = xor_terms[0];
        for (int i = 1; i < n_terms; i++)
            out = circuit_add_gate(c, XOR, out, xor_terms[i]);
    }
    circuit_set_output(c, &out, 1);
    free(xor_terms); free(rm_coeffs);
    return c;
}

/* ===================================================================
 * L5: Shannon Decomposition Synthesis
 * =================================================================== */

BooleanCircuit* circuit_synthesize_shannon(const int* truth, int n_vars) {
    assert(truth != NULL);
    assert(n_vars > 0 && n_vars <= 10);
    long long total = 1LL << n_vars;
    int cap = (int)total * 8 + 10;
    BooleanCircuit* c = circuit_create(cap);
    int in_ids[16];
    for (int j = 0; j < n_vars; j++)
        in_ids[j] = circuit_add_gate(c, INPUT, -1, -1);
    /* Build MUX tree from leaves up.
       Leaves are the truth table values (CONST0 or CONST1). */
    int* cur = (int*)malloc((size_t)total * sizeof(int));
    assert(cur);
    for (long long t = 0; t < total; t++) {
        cur[t] = truth[t] ?
            circuit_add_gate(c, CONST1, -1, -1) :
            circuit_add_gate(c, CONST0, -1, -1);
    }
    int cur_n = (int)total;
    for (int var = n_vars - 1; var >= 0; var--) {
        int next_n = cur_n / 2;
        int* next_lvl = (int*)malloc((size_t)next_n * sizeof(int));
        assert(next_lvl);
        for (int i = 0; i < next_n; i++) {
            int f0 = cur[2 * i];
            int f1 = cur[2 * i + 1];
            int nv = circuit_add_gate(c, NOT, in_ids[var], -1);
            int t0 = circuit_add_gate(c, AND, nv, f0);
            int t1 = circuit_add_gate(c, AND, in_ids[var], f1);
            next_lvl[i] = circuit_add_gate(c, OR, t0, t1);
        }
        free(cur);
        cur = next_lvl;
        cur_n = next_n;
    }
    circuit_set_output(c, cur, 1);
    free(cur);
    return c;
}
/* ===================================================================
 * L5: Technology Mapping
 *
 * Map circuits between functionally complete bases:
 *   {NAND}  : NAND alone is universal
 *   {NOR}   : NOR alone is universal
 *   {AND, NOT} : minimal basis
 *   Threshold gates : for TC^0 synthesis
 * =================================================================== */

BooleanCircuit* circuit_map_to_nand2(const BooleanCircuit* c) {
    assert(c != NULL);
    /* Map to {NAND} basis using identities:
     * NOT(x) = NAND(x,x)
     * AND(a,b) = NAND(NAND(a,b), NAND(a,b))
     * OR(a,b) = NAND(NAND(a,a), NAND(b,b))
     * XOR: 4 NAND gates */
    return circuit_to_nand_only(c);
}

BooleanCircuit* circuit_map_to_nor2(const BooleanCircuit* c) {
    assert(c != NULL);
    /* Map to {NOR} basis using De Morgan duals */
    return circuit_to_nor_only(c);
}

BooleanCircuit* circuit_map_to_and_not(const BooleanCircuit* c) {
    assert(c != NULL);
    int cap = c->n_gates * 3 + 10;
    BooleanCircuit* nc = circuit_create(cap);
    int* mapping = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(mapping);
    for (int i = 0; i < c->n_gates; i++) mapping[i] = -1;
    int* order = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);
    for (int idx = 0; idx < c->n_gates; idx++) {
        int i = order[idx];
        const Gate* g = &c->gates[i];
        int a, b;
        switch (g->type) {
        case INPUT: case CONST0: case CONST1: case AND: case NOT:
            a = (g->input1 >= 0) ? mapping[g->input1] : -1;
            b = (g->input2 >= 0) ? mapping[g->input2] : -1;
            mapping[i] = circuit_add_gate(nc, g->type, a, b);
            break;
        case OR:
            /* OR(a,b) = NOT(AND(NOT(a), NOT(b))) */
            a = mapping[g->input1]; b = mapping[g->input2];
            { int na = circuit_add_gate(nc, NOT, a, -1);
              int nb = circuit_add_gate(nc, NOT, b, -1);
              int and_ab = circuit_add_gate(nc, AND, na, nb);
              mapping[i] = circuit_add_gate(nc, NOT, and_ab, -1); }
            break;
        case XOR:
            /* XOR = AND(OR(a,b), NOT(AND(a,b))) -> need OR */
            a = mapping[g->input1]; b = mapping[g->input2];
            { int na = circuit_add_gate(nc, NOT, a, -1);
              int nb = circuit_add_gate(nc, NOT, b, -1);
              int and_nanb = circuit_add_gate(nc, AND, na, nb);
              int or_ab = circuit_add_gate(nc, NOT, and_nanb, -1);
              int and_ab = circuit_add_gate(nc, AND, a, b);
              int n_and_ab = circuit_add_gate(nc, NOT, and_ab, -1);
              mapping[i] = circuit_add_gate(nc, AND, or_ab, n_and_ab); }
            break;
        case NAND:
            a = mapping[g->input1]; b = mapping[g->input2];
            { int and_ab = circuit_add_gate(nc, AND, a, b);
              mapping[i] = circuit_add_gate(nc, NOT, and_ab, -1); }
            break;
        case NOR:
            a = mapping[g->input1]; b = mapping[g->input2];
            { int na = circuit_add_gate(nc, NOT, a, -1);
              int nb = circuit_add_gate(nc, NOT, b, -1);
              int and_nanb = circuit_add_gate(nc, AND, na, nb);
              mapping[i] = circuit_add_gate(nc, NOT, and_nanb, -1); }
            break;
        default:
            mapping[i] = circuit_add_gate(nc, CONST0, -1, -1);
        }
    }
    int* outs = (int*)malloc((size_t)c->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < c->n_outputs; i++) outs[i] = mapping[c->output_ids[i]];
    circuit_set_output(nc, outs, c->n_outputs);
    free(outs); free(order); free(mapping);
    return nc;
}

BooleanCircuit* circuit_map_to_nand(const BooleanCircuit* c) {
    assert(c != NULL);
    return circuit_to_nand_only(c);
}

BooleanCircuit* circuit_map_to_threshold(const BooleanCircuit* c) {
    assert(c != NULL);
    /* Map AND to THR(2): THR(2) of {a,b} with threshold=2
     * Map OR to THR(1): THR(1) of {a,b} with threshold=1
     * Map NOT to THR(0) of {a} with threshold 0...actually use MAJ.
     * This is a simplified mapping for TC^0 synthesis. */
    int cap = c->n_gates * 2 + 10;
    BooleanCircuit* tc = circuit_create(cap);
    int* mapping = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(mapping);
    for (int i = 0; i < c->n_gates; i++) mapping[i] = -1;
    int* order = (int*)malloc((size_t)c->n_gates * sizeof(int));
    assert(order);
    circuit_topological_order(c, order);
    for (int idx = 0; idx < c->n_gates; idx++) {
        int i = order[idx];
        const Gate* g = &c->gates[i];
        switch (g->type) {
        case INPUT:
            mapping[i] = circuit_add_gate(tc, INPUT, -1, -1); break;
        case CONST0:
            mapping[i] = circuit_add_gate(tc, CONST0, -1, -1); break;
        case CONST1:
            mapping[i] = circuit_add_gate(tc, CONST1, -1, -1); break;
        case AND: {
            int kids[] = {mapping[g->input1], mapping[g->input2]};
            mapping[i] = circuit_add_maj_gate(tc, MAJ, 2, kids);
            break;
        }
        case OR: {
            int kids[] = {mapping[g->input1], mapping[g->input2]};
            mapping[i] = circuit_add_maj_gate(tc, MAJ, 2, kids);
            break;
        }
        case NOT: {
            int kids[] = {mapping[g->input1]};
            mapping[i] = circuit_add_maj_gate(tc, MAJ, 1, kids);
            mapping[i] = circuit_add_gate(tc, NOT, mapping[i], -1);
            break;
        }
        case XOR: {
            /* XOR via threshold: XOR(a,b) = THR1(a,b) AND NOT(THR2(a,b))
             * simplified: use MAJ gates */
            int a = mapping[g->input1], b = mapping[g->input2];
            int kids_or[] = {a, b};
            int or_g = circuit_add_maj_gate(tc, MAJ, 2, kids_or);
            int kids_and[] = {a, b};
            int and_g = circuit_add_maj_gate(tc, MAJ, 2, kids_and);
            int nand_g = circuit_add_gate(tc, NOT, and_g, -1);
            int kids_xor[] = {or_g, nand_g};
            mapping[i] = circuit_add_maj_gate(tc, MAJ, 2, kids_xor);
            break;
        }
        case NAND: {
            int kids[] = {mapping[g->input1], mapping[g->input2]};
            int and_g = circuit_add_maj_gate(tc, MAJ, 2, kids);
            mapping[i] = circuit_add_gate(tc, NOT, and_g, -1);
            break;
        }
        default:
            mapping[i] = circuit_add_gate(tc, CONST0, -1, -1);
        }
    }
    int* outs = (int*)malloc((size_t)c->n_outputs * sizeof(int));
    assert(outs);
    for (int i = 0; i < c->n_outputs; i++) outs[i] = mapping[c->output_ids[i]];
    circuit_set_output(tc, outs, c->n_outputs);
    free(outs); free(order); free(mapping);
    return tc;
}

