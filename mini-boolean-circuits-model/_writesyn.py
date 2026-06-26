import os

code = r"""
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

"""

with open('src/circuit_synthesis.c', 'a', encoding='utf-8') as f:
    f.write(code)
print(f'Appended QM: {len(code)} chars')
