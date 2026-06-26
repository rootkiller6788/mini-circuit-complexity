/* cnf_tools.c -- CNF Representation and Manipulation
 *
 * CNF (Conjunctive Normal Form): conjunction of clauses,
 * each clause is a disjunction of literals.
 *
 * SAT in CNF form is the canonical NP-complete problem.
 * The phase transition for random 3-SAT occurs at
 * clause-to-variable ratio ~ 4.26.
 *
 * References:
 *   Cook (1971) "The Complexity of Theorem-Proving Procedures"
 *   Mitchell et al. (1992) "Hard and Easy Distributions of SAT Problems"
 *   Biere et al. (2009) "Handbook of Satisfiability"
 */

#include "csat.h"

/* ============================================================================
 * L2: CNF Data Structure Operations
 * ============================================================================ */

CNFInstance* cnf_create(int nv)
{
    if (nv <= 0) return NULL;
    CNFInstance* cnf = (CNFInstance*)calloc(1, sizeof(CNFInstance));
    if (!cnf) return NULL;
    cnf->nv = nv;
    cnf->nc = 0;
    cnf->cap = 16;  /* initial clause capacity */
    cnf->clauses = (int**)calloc((size_t)cnf->cap, sizeof(int*));
    cnf->sizes   = (int*)calloc((size_t)cnf->cap, sizeof(int));
    if (!cnf->clauses || !cnf->sizes) {
        free(cnf->clauses); free(cnf->sizes); free(cnf);
        return NULL;
    }
    return cnf;
}

void cnf_add_clause(CNFInstance* cnf, const int* clause, int size)
{
    if (!cnf || !clause || size <= 0) return;
    /* Grow capacity if needed */
    if (cnf->nc >= cnf->cap) {
        int newcap = cnf->cap * 2;
        int** newc = (int**)realloc(cnf->clauses, (size_t)newcap * sizeof(int*));
        int*  news = (int*)realloc(cnf->sizes, (size_t)newcap * sizeof(int));
        if (!newc || !news) { free(newc); free(news); return; }
        cnf->clauses = newc;
        cnf->sizes   = news;
        cnf->cap     = newcap;
    }
    /* Allocate and copy clause */
    int* cl = (int*)malloc((size_t)(size + 1) * sizeof(int));
    if (!cl) return;
    for (int i = 0; i < size; i++) cl[i] = clause[i];
    cl[size] = 0;  /* terminator */
    cnf->clauses[cnf->nc] = cl;
    cnf->sizes[cnf->nc]   = size;
    cnf->nc++;
}

void cnf_free(CNFInstance* cnf)
{
    if (!cnf) return;
    for (int i = 0; i < cnf->nc; i++)
        free(cnf->clauses[i]);
    free(cnf->clauses);
    free(cnf->sizes);
    free(cnf);
}

int cnf_evaluate(const CNFInstance* cnf, const int* assign)
{
    if (!cnf || !assign) return 0;
    for (int i = 0; i < cnf->nc; i++) {
        int clause_sat = 0;
        int* cl = cnf->clauses[i];
        for (int j = 0; j < cnf->sizes[i]; j++) {
            int lit = cl[j];
            int var = (lit > 0) ? lit : -lit;
            int val = assign[var];  /* 0 or 1 */
            if ((lit > 0 && val == 1) || (lit < 0 && val == 0)) {
                clause_sat = 1;
                break;
            }
        }
        if (!clause_sat) return 0;  /* clause falsified */
    }
    return 1;  /* all clauses satisfied */
}

void cnf_print_dimacs(const CNFInstance* cnf, FILE* f)
{
    if (!cnf || !f) return;
    fprintf(f, "p cnf %d %d\n", cnf->nv, cnf->nc);
    for (int i = 0; i < cnf->nc; i++) {
        for (int j = 0; j < cnf->sizes[i]; j++)
            fprintf(f, "%d ", cnf->clauses[i][j]);
        fprintf(f, "0\n");
    }
}

CNFInstance* cnf_parse_dimacs(const char* filename)
{
    if (!filename) return NULL;
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;
    CNFInstance* cnf = NULL;
    char line[4096];
    int nv = -1, nc = -1;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == 'c') continue;
        if (line[0] == 'p') {
            if (sscanf(line, "p cnf %d %d", &nv, &nc) == 2) {
                cnf = cnf_create(nv);
                if (!cnf) { fclose(f); return NULL; }
            }
            continue;
        }
        if (!cnf) continue;
        int literals[256];
        int nl = 0;
        char* tok = strtok(line, " \t\n");
        while (tok && nl < 256) {
            int lit = atoi(tok);
            if (lit == 0) break;
            literals[nl++] = lit;
            tok = strtok(NULL, " \t\n");
        }
        if (nl > 0)
            cnf_add_clause(cnf, literals, nl);
    }
    fclose(f);
    return cnf;
}

int cnf_pure_literal_elim(CNFInstance* cnf)
{
    if (!cnf) return 0;
    int nv = cnf->nv;
    int* pos_occ = (int*)calloc((size_t)(nv + 1), sizeof(int));
    int* neg_occ = (int*)calloc((size_t)(nv + 1), sizeof(int));
    if (!pos_occ || !neg_occ) { free(pos_occ); free(neg_occ); return 0; }
    for (int i = 0; i < cnf->nc; i++) {
        int* cl = cnf->clauses[i];
        for (int j = 0; j < cnf->sizes[i]; j++) {
            int lit = cl[j];
            if (lit > 0) pos_occ[lit] = 1;
            else         neg_occ[-lit] = 1;
        }
    }
    int eliminated = 0;
    for (int v = 1; v <= nv; v++) {
        if (pos_occ[v] && !neg_occ[v]) eliminated++;
        if (!pos_occ[v] && neg_occ[v]) eliminated++;
    }
    free(pos_occ);
    free(neg_occ);
    return eliminated;
}

int cnf_subsumption_elim(CNFInstance* cnf)
{
    if (!cnf) return 0;
    int removed = 0;
    int* removed_flags = (int*)calloc((size_t)cnf->nc, sizeof(int));
    if (!removed_flags) return 0;
    for (int i = 0; i < cnf->nc; i++) {
        if (removed_flags[i]) continue;
        for (int j = 0; j < cnf->nc; j++) {
            if (i == j || removed_flags[j]) continue;
            int* ci = cnf->clauses[i];
            int si = cnf->sizes[i];
            int* cj = cnf->clauses[j];
            int sj = cnf->sizes[j];
            if (si > sj) continue;
            int subsumes = 1;
            for (int k = 0; k < si && subsumes; k++) {
                int found = 0;
                for (int l = 0; l < sj; l++)
                    if (ci[k] == cj[l]) { found = 1; break; }
                if (!found) subsumes = 0;
            }
            if (subsumes) {
                removed_flags[j] = 1;
                removed++;
            }
        }
    }
    free(removed_flags);
    return removed;
}

int cnf_variable_elim(CNFInstance* cnf, int var)
{
    if (!cnf || var < 1 || var > cnf->nv) return 0;
    int added = 0;
    int* pos_clauses = (int*)malloc((size_t)cnf->nc * sizeof(int));
    int* neg_clauses = (int*)malloc((size_t)cnf->nc * sizeof(int));
    int np = 0, nn = 0;
    if (!pos_clauses || !neg_clauses) { free(pos_clauses); free(neg_clauses); return 0; }
    for (int i = 0; i < cnf->nc; i++) {
        int* cl = cnf->clauses[i];
        for (int j = 0; j < cnf->sizes[i]; j++) {
            if (cl[j] == var)  { pos_clauses[np++] = i; break; }
            if (cl[j] == -var) { neg_clauses[nn++] = i; break; }
        }
    }
    for (int pi = 0; pi < np; pi++) {
        for (int ni = 0; ni < nn; ni++) {
            int* pc = cnf->clauses[pos_clauses[pi]];
            int ps = cnf->sizes[pos_clauses[pi]];
            int* nc_ = cnf->clauses[neg_clauses[ni]];
            int ns = cnf->sizes[neg_clauses[ni]];
            int resolvent[256];
            int nr = 0;
            for (int j = 0; j < ps; j++)
                if (pc[j] != var && pc[j] != -var) resolvent[nr++] = pc[j];
            for (int j = 0; j < ns; j++)
                if (nc_[j] != var && nc_[j] != -var) resolvent[nr++] = nc_[j];
            int uniq[256];
            int nu = 0;
            for (int j = 0; j < nr; j++) {
                int dup = 0;
                for (int k = 0; k < nu; k++)
                    if (uniq[k] == resolvent[j]) { dup = 1; break; }
                if (!dup) uniq[nu++] = resolvent[j];
            }
            int taut = 0;
            for (int j = 0; j < nu && !taut; j++)
                for (int k = 0; k < nu && !taut; k++)
                    if (uniq[j] == -uniq[k]) taut = 1;
            if (!taut && nu > 0) {
                cnf_add_clause(cnf, uniq, nu);
                added++;
            }
        }
    }
    free(pos_clauses);
    free(neg_clauses);
    return added;
}

int cnf_count_satisfied(const CNFInstance* cnf, const int* assign)
{
    if (!cnf || !assign) return 0;
    int count = 0;
    for (int i = 0; i < cnf->nc; i++) {
        int* cl = cnf->clauses[i];
        for (int j = 0; j < cnf->sizes[i]; j++) {
            int lit = cl[j];
            int var = (lit > 0) ? lit : -lit;
            int val = assign[var];
            if ((lit > 0 && val == 1) || (lit < 0 && val == 0)) {
                count++;
                break;
            }
        }
    }
    return count;
}

double cnf_clause_var_ratio(const CNFInstance* cnf)
{
    if (!cnf || cnf->nv <= 0) return 0;
    return (double)cnf->nc / (double)cnf->nv;
}

/* ============================================================================
 * L3: Tseitin Transformation (Circuit -> CNF)
 * ============================================================================ */

CNFInstance* csat_to_cnf(const BooleanCircuit* c)
{
    if (!c) return NULL;
    int extra_vars = c->n_gates;
    CNFInstance* cnf = cnf_create(c->n_inputs + extra_vars);
    if (!cnf) return NULL;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        if (g->type == INPUT || g->type == CONST0 || g->type == CONST1)
            continue;
        int gv = c->n_inputs + i + 1;  /* auxiliary variable (1-indexed) */
        switch (g->type) {
            case AND: {
                int a = (g->input1 >= 0) ? g->input1 + 1 : 0;
                int b = (g->input2 >= 0) ? g->input2 + 1 : 0;
                if (a <= 0 || b <= 0) break;
                int c1[] = {-gv, a, 0};
                int c2[] = {-gv, b, 0};
                int c3[] = {gv, -a, -b, 0};
                cnf_add_clause(cnf, c1, 2);
                cnf_add_clause(cnf, c2, 2);
                cnf_add_clause(cnf, c3, 3);
                break;
            }
            case OR: {
                int a = (g->input1 >= 0) ? g->input1 + 1 : 0;
                int b = (g->input2 >= 0) ? g->input2 + 1 : 0;
                if (a <= 0 || b <= 0) break;
                int c1[] = {gv, -a, 0};
                int c2[] = {gv, -b, 0};
                int c3[] = {-gv, a, b, 0};
                cnf_add_clause(cnf, c1, 2);
                cnf_add_clause(cnf, c2, 2);
                cnf_add_clause(cnf, c3, 3);
                break;
            }
            case NOT: {
                int a = (g->input1 >= 0) ? g->input1 + 1 : 0;
                if (a <= 0) break;
                int c1[] = {gv, a, 0};
                int c2[] = {-gv, -a, 0};
                cnf_add_clause(cnf, c1, 2);
                cnf_add_clause(cnf, c2, 2);
                break;
            }
            case XOR: {
                int a = (g->input1 >= 0) ? g->input1 + 1 : 0;
                int b = (g->input2 >= 0) ? g->input2 + 1 : 0;
                if (a <= 0 || b <= 0) break;
                int c1[] = {gv, a, b, 0};
                int c2[] = {gv, -a, -b, 0};
                int c3[] = {-gv, a, -b, 0};
                int c4[] = {-gv, -a, b, 0};
                cnf_add_clause(cnf, c1, 3);
                cnf_add_clause(cnf, c2, 3);
                cnf_add_clause(cnf, c3, 3);
                cnf_add_clause(cnf, c4, 3);
                break;
            }
            default: break;
        }
    }
    /* Force output gate to be true */
    if (c->n_outputs > 0 && c->output_ids) {
        int out_gate = c->output_ids[0];
        int out_var = c->n_inputs + out_gate + 1;
        int unit[] = {out_var, 0};
        cnf_add_clause(cnf, unit, 1);
    }
    return cnf;
}

CNFInstance* csat_to_cnf_plaisted(const BooleanCircuit* c)
{
    /* Plaisted-Greenbaum uses polarity-based naming to reduce
     * auxiliary variables. For simplicity, fall back to Tseitin. */
    if (!c) return NULL;
    return csat_to_cnf(c);
}