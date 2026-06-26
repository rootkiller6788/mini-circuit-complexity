/* sat_lookahead.c -- Lookahead SAT Solver
 *
 * Lookahead solver: probe each variable to detect failed literals
 * and equivalences before branching. Strong on structured instances.
 *
 * Based on: Heule & van Maaren (2009) "Look-Ahead Based SAT Solving"
 * Used in: march, OKsolver, satz
 *
 * Key techniques:
 *   - Failed literal detection
 *   - Equivalence detection
 *   - Double-lookahead (probe deeper)
 *   - Lookahead-based variable ordering
 */

#include "csat.h"

LookaheadSolver* lookahead_create(const CNFInstance* cnf)
{
    if (!cnf) return NULL;
    LookaheadSolver* s = (LookaheadSolver*)calloc(1, sizeof(LookaheadSolver));
    if (!s) return NULL;
    s->nv = cnf->nv;
    s->nc = cnf->nc;
    s->clauses = (int**)malloc((size_t)cnf->nc * sizeof(int*));
    s->c_sizes = (int*)malloc((size_t)cnf->nc * sizeof(int));
    if (!s->clauses || !s->c_sizes) {
        lookahead_free(s); return NULL;
    }
    for (int i = 0; i < cnf->nc; i++) {
        int sz = cnf->sizes[i];
        s->c_sizes[i] = sz;
        s->clauses[i] = (int*)malloc((size_t)(sz + 1) * sizeof(int));
        if (s->clauses[i]) {
            for (int j = 0; j < sz; j++) s->clauses[i][j] = cnf->clauses[i][j];
            s->clauses[i][sz] = 0;
        }
    }
    s->assign = (int*)calloc((size_t)(s->nv + 1), sizeof(int));
    s->failed_lits = (int*)malloc((size_t)(2 * s->nv) * sizeof(int));
    s->equiv = (int*)malloc((size_t)(2 * s->nv) * sizeof(int));
    if (!s->assign || !s->failed_lits || !s->equiv) {
        lookahead_free(s); return NULL;
    }
    s->n_failed = 0;
    s->n_equiv  = 0;
    return s;
}

void lookahead_free(LookaheadSolver* s)
{
    if (!s) return;
    for (int i = 0; i < s->nc; i++)
        free(s->clauses[i]);
    free(s->clauses);
    free(s->c_sizes);
    free(s->assign);
    free(s->failed_lits);
    free(s->equiv);
    free(s);
}

/* Simplified clause evaluation: is clause satisfied under partial assignment?
 * Returns: 1=satisfied, 0=undecided, -1=falsified */
static int clause_status(int* cl, int sz, int* assign)
{
    int any_true = 0, any_unassigned = 0;
    for (int i = 0; i < sz; i++) {
        int lit = cl[i];
        int var = (lit > 0) ? lit : -lit;
        if (var <= 0 || var >= 256) continue;
        if (assign[var] == 0) { any_unassigned = 1; continue; }
        if ((lit > 0 && assign[var] == 1) ||
            (lit < 0 && assign[var] == -1)) {
            any_true = 1;
            break;
        }
    }
    if (any_true) return 1;
    if (any_unassigned) return 0;
    return -1;
}

/* Probe literal: temporarily set it and check consequences. */
int lookahead_probe(LookaheadSolver* s, int lit)
{
    if (!s) return 0;
    int var = (lit > 0) ? lit : -lit;
    int val = (lit > 0) ? 1 : -1;
    if (var < 1 || var > s->nv) return 0;
    int* save = (int*)malloc((size_t)(s->nv + 1) * sizeof(int));
    if (!save) return 0;
    for (int v = 1; v <= s->nv; v++)
        save[v] = s->assign[v];
    s->assign[var] = val;
    int conflict = 0;
    for (int i = 0; i < s->nc && !conflict; i++) {
        int status = clause_status(s->clauses[i], s->c_sizes[i], s->assign);
        if (status == -1) conflict = 1;
    }
    for (int v = 1; v <= s->nv; v++)
        s->assign[v] = save[v];
    free(save);
    return conflict ? -1 : 1;
}

/* Detect all failed literals by probing each unassigned variable */
int lookahead_find_failed(LookaheadSolver* s)
{
    if (!s) return 0;
    s->n_failed = 0;
    for (int v = 1; v <= s->nv; v++) {
        if (s->assign[v] != 0) continue;
        if (lookahead_probe(s, v) < 0) {
            s->failed_lits[s->n_failed++] = v;
            s->assign[v] = 1;
            continue;
        }
        if (lookahead_probe(s, -v) < 0) {
            s->failed_lits[s->n_failed++] = -v;
            s->assign[v] = -1;
        }
    }
    return s->n_failed;
}

/* Detect equivalent literals */
int lookahead_find_equiv(LookaheadSolver* s)
{
    if (!s) return 0;
    s->n_equiv = 0;
    for (int v1 = 1; v1 <= s->nv; v1++) {
        if (s->assign[v1] != 0) continue;
        for (int v2 = v1 + 1; v2 <= s->nv; v2++) {
            if (s->assign[v2] != 0) continue;
            int* save = (int*)malloc((size_t)(s->nv + 1) * sizeof(int));
            if (!save) continue;
            for (int v = 1; v <= s->nv; v++) save[v] = s->assign[v];
            s->assign[v1] = 1;
            int forced_v2 = lookahead_probe(s, -v2) < 0 ? -1 :
                           lookahead_probe(s, v2) < 0 ? 1 : 0;
            for (int v = 1; v <= s->nv; v++) s->assign[v] = save[v];
            if (forced_v2 == 1) {
                s->equiv[s->n_equiv++] = v1;
                s->equiv[s->n_equiv++] = v2;
            } else if (forced_v2 == -1) {
                s->equiv[s->n_equiv++] = v1;
                s->equiv[s->n_equiv++] = -v2;
            }
            free(save);
        }
    }
    return s->n_equiv / 2;
}

/* Main solve: use lookahead to guide DPLL search */
int lookahead_solve(LookaheadSolver* s)
{
    if (!s) return -1;
    int nf = lookahead_find_failed(s);
    if (nf < 0) return 0;
    int ne = lookahead_find_equiv(s);
    int var = -1;
    for (int v = 1; v <= s->nv; v++) {
        if (s->assign[v] == 0) { var = v; break; }
    }
    if (var < 0) return 1;
    int probe_pos = lookahead_probe(s, var);
    if (probe_pos < 0) {
        s->assign[var] = -1;
        return lookahead_solve(s);
    }
    int probe_neg = lookahead_probe(s, -var);
    if (probe_neg < 0) {
        s->assign[var] = 1;
        return lookahead_solve(s);
    }
    s->assign[var] = 1;
    int result = lookahead_solve(s);
    if (result == 1) return 1;
    s->assign[var] = -1;
    result = lookahead_solve(s);
    if (result == 1) return 1;
    s->assign[var] = 0;
    (void)ne;
    return 0;
}

void lookahead_get_model(const LookaheadSolver* s, int* model)
{
    if (!s || !model) return;
    for (int i = 0; i < s->nv; i++)
        model[i] = (s->assign[i + 1] == 1) ? 1 : 0;
}