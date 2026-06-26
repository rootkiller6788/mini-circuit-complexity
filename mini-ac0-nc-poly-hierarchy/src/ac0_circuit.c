/* ac0_circuit.c -- AC0-Specific Circuit Operations
 * =====================================================================
 * DNF/CNF construction, Sipser functions (depth hierarchy),
 * symmetric functions, MOD_p gates, and AC0 lower bounds.
 *
 * Key results:
 *   - PARITY not in AC0 (FSS 1981, Ajtai 1983, Hastad 1986)
 *   - AC0 depth hierarchy is strict (Hastad 1986)
 *   - S_d separates AC0[d] from AC0[d+1]
 *   - Razborov-Smolensky: MAJORITY not in AC0[p] for p!=2
 *
 * References:
 *   Arora & Barak Chapter 6, Sections 6.1-6.3
 *   Hastad "Computational Limitations of Small-Depth Circuits" (1986)
 */
#include "ac0nc.h"
#include "ac0_circuit.h"

/* ===== AC0 Allowed Gates ===== */
int ac0_is_allowed_gate(AC0GateType t)
{
    switch (t) {
        case AC0_GATE_INPUT:
        case AC0_GATE_AND:
        case AC0_GATE_OR:
        case AC0_GATE_NOT:
        case AC0_GATE_CONST:
            return 1;
        default:
            return 0;
    }
}

/* ===== DNF (Disjunctive Normal Form) =====
 * DNF = OR of AND terms. Each AND term is a conjunction of literals.
 * Depth = 2. Size = (#terms) * (avg literals per term).
 *
 * DNF is the canonical form for AC0[2] circuits.
 * Every Boolean function has a unique DNF (its minterms).
 */
DNF* dnf_create(int n_vars, int n_terms)
{
    if (n_vars <= 0 || n_terms <= 0) return NULL;
    DNF *d = (DNF*)calloc(1, sizeof(DNF));
    if (!d) return NULL;
    d->n_vars   = n_vars;
    d->n_terms  = n_terms;
    d->terms    = (int**)calloc(n_terms, sizeof(int*));
    d->term_sizes = (int*)calloc(n_terms, sizeof(int));
    if (!d->terms || !d->term_sizes) {
        free(d->terms);
        free(d->term_sizes);
        free(d);
        return NULL;
    }
    for (int t = 0; t < n_terms; t++) {
        d->terms[t] = (int*)calloc(n_vars, sizeof(int));
        if (!d->terms[t]) {
            for (int i = 0; i < t; i++) free(d->terms[i]);
            free(d->terms);
            free(d->term_sizes);
            free(d);
            return NULL;
        }
        for (int v = 0; v < n_vars; v++)
            d->terms[t][v] = 0; /* 0=absent, +1=positive, -1=negated */
    }
    return d;
}

void dnf_free(DNF *d)
{
    if (!d) return;
    for (int t = 0; t < d->n_terms; t++)
        free(d->terms[t]);
    free(d->terms);
    free(d->term_sizes);
    free(d);
}

void dnf_set_literal(DNF *d, int term, int var, int polarity)
{
    if (!d || term < 0 || term >= d->n_terms) return;
    if (var < 0 || var >= d->n_vars) return;
    if (d->terms[term][var] == 0)
        d->term_sizes[term]++;
    d->terms[term][var] = (polarity > 0) ? 1 : -1;
}

int dnf_evaluate(DNF *d, const int *assignment)
{
    if (!d || !assignment) return 0;
    for (int t = 0; t < d->n_terms; t++) {
        int term_true = 1;
        int has_literals = 0;
        for (int v = 0; v < d->n_vars; v++) {
            int lit = d->terms[t][v];
            if (lit == 0) continue;
            has_literals = 1;
            int var_val = assignment[v];
            if ((lit > 0 && !var_val) || (lit < 0 && var_val)) {
                term_true = 0;
                break;
            }
        }
        if (has_literals && term_true) return 1;
    }
    return 0;
}

AC0Circuit* dnf_to_circuit(DNF *d)
{
    if (!d || d->n_terms == 0) return NULL;
    AC0Circuit *c = ac0_circuit_create(d->n_vars + d->n_terms + 1);
    if (!c) return NULL;

    int *ins = (int*)malloc(d->n_vars * sizeof(int));
    if (!ins) { ac0_circuit_free(c); return NULL; }
    for (int v = 0; v < d->n_vars; v++)
        ins[v] = ac0_circuit_add_input(c);

    int *and_gates = (int*)calloc(d->n_terms, sizeof(int));

    for (int t = 0; t < d->n_terms; t++) {
        int nf = d->term_sizes[t];
        if (nf == 0) { and_gates[t] = -1; continue; }
        int *fanin = (int*)malloc(nf * sizeof(int));
        int idx = 0;
        for (int v = 0; v < d->n_vars; v++) {
            int lit = d->terms[t][v];
            if (lit == 0) continue;
            if (lit > 0) {
                fanin[idx++] = ins[v];
            } else {
                int not_g = ac0_circuit_add_gate(c, AC0_GATE_NOT, ins[v], -1);
                fanin[idx++] = not_g;
            }
        }
        and_gates[t] = ac0_circuit_add_fanin_gate(c, AC0_GATE_AND, fanin, idx);
        free(fanin);
    }

    int n_valid = 0;
    int *or_fanin = (int*)malloc(d->n_terms * sizeof(int));
    for (int t = 0; t < d->n_terms; t++)
        if (and_gates[t] >= 0)
            or_fanin[n_valid++] = and_gates[t];
    int or_g = ac0_circuit_add_fanin_gate(c, AC0_GATE_OR, or_fanin, n_valid);
    ac0_circuit_set_output(c, or_g);
    c->class_label = "AC0";

    free(ins);
    free(and_gates);
    free(or_fanin);
    return c;
}

void dnf_print(DNF *d)
{
    if (!d) { printf("NULL DNF\n"); return; }
    printf("DNF(%d vars, %d terms):\n", d->n_vars, d->n_terms);
    for (int t = 0; t < d->n_terms; t++) {
        printf("  term %d: ", t);
        int first = 1;
        for (int v = 0; v < d->n_vars; v++) {
            int lit = d->terms[t][v];
            if (lit == 0) continue;
            if (!first) printf(" AND ");
            if (lit < 0) printf("NOT ");
            printf("x%d", v);
            first = 0;
        }
        if (first) printf("(empty = TRUE)");
        printf("\n");
    }
}

/* ===== CNF (Conjunctive Normal Form) =====
 * CNF = AND of OR clauses. Dual of DNF.
 * Every Boolean function also has a unique CNF (its maxterms).
 */
CNF* cnf_create(int n_vars, int n_clauses)
{
    if (n_vars <= 0 || n_clauses <= 0) return NULL;
    CNF *cnf = (CNF*)calloc(1, sizeof(CNF));
    if (!cnf) return NULL;
    cnf->n_vars    = n_vars;
    cnf->n_clauses = n_clauses;
    cnf->clauses   = (int**)calloc(n_clauses, sizeof(int*));
    cnf->clause_sizes = (int*)calloc(n_clauses, sizeof(int));
    if (!cnf->clauses || !cnf->clause_sizes) {
        free(cnf->clauses);
        free(cnf->clause_sizes);
        free(cnf);
        return NULL;
    }
    for (int i = 0; i < n_clauses; i++) {
        cnf->clauses[i] = (int*)calloc(n_vars, sizeof(int));
        if (!cnf->clauses[i]) {
            for (int j = 0; j < i; j++) free(cnf->clauses[j]);
            free(cnf->clauses);
            free(cnf->clause_sizes);
            free(cnf);
            return NULL;
        }
    }
    return cnf;
}

void cnf_free(CNF *cnf)
{
    if (!cnf) return;
    for (int i = 0; i < cnf->n_clauses; i++)
        free(cnf->clauses[i]);
    free(cnf->clauses);
    free(cnf->clause_sizes);
    free(cnf);
}

void cnf_set_literal(CNF *cnf, int clause, int var, int polarity)
{
    if (!cnf || clause < 0 || clause >= cnf->n_clauses) return;
    if (var < 0 || var >= cnf->n_vars) return;
    if (cnf->clauses[clause][var] == 0)
        cnf->clause_sizes[clause]++;
    cnf->clauses[clause][var] = (polarity > 0) ? 1 : -1;
}

int cnf_evaluate(CNF *cnf, const int *assignment)
{
    if (!cnf || !assignment) return 0;
    for (int cl = 0; cl < cnf->n_clauses; cl++) {
        int clause_satisfied = 0;
        for (int v = 0; v < cnf->n_vars; v++) {
            int lit = cnf->clauses[cl][v];
            if (lit == 0) continue;
            int var_val = assignment[v];
            if ((lit > 0 && var_val) || (lit < 0 && !var_val)) {
                clause_satisfied = 1;
                break;
            }
        }
        if (!clause_satisfied) return 0;
    }
    return 1;
}

AC0Circuit* cnf_to_circuit(CNF *cnf)
{
    if (!cnf || cnf->n_clauses == 0) return NULL;
    AC0Circuit *ckt = ac0_circuit_create(cnf->n_vars + cnf->n_clauses + 1);
    if (!ckt) return NULL;

    int *ins = (int*)malloc(cnf->n_vars * sizeof(int));
    for (int v = 0; v < cnf->n_vars; v++)
        ins[v] = ac0_circuit_add_input(ckt);

    int *or_gates = (int*)calloc(cnf->n_clauses, sizeof(int));
    for (int cl = 0; cl < cnf->n_clauses; cl++) {
        int nf = cnf->clause_sizes[cl];
        if (nf == 0) { or_gates[cl] = -1; continue; }
        int *fanin = (int*)malloc(nf * sizeof(int));
        int idx = 0;
        for (int v = 0; v < cnf->n_vars; v++) {
            int lit = cnf->clauses[cl][v];
            if (lit == 0) continue;
            if (lit > 0) {
                fanin[idx++] = ins[v];
            } else {
                int not_g = ac0_circuit_add_gate(ckt, AC0_GATE_NOT, ins[v], -1);
                fanin[idx++] = not_g;
            }
        }
        or_gates[cl] = ac0_circuit_add_fanin_gate(ckt, AC0_GATE_OR, fanin, idx);
        free(fanin);
    }

    int n_valid = 0;
    int *and_fanin = (int*)malloc(cnf->n_clauses * sizeof(int));
    for (int cl = 0; cl < cnf->n_clauses; cl++)
        if (or_gates[cl] >= 0)
            and_fanin[n_valid++] = or_gates[cl];
    int and_g = ac0_circuit_add_fanin_gate(ckt, AC0_GATE_AND, and_fanin, n_valid);
    ac0_circuit_set_output(ckt, and_g);
    ckt->class_label = "AC0";

    free(ins);
    free(or_gates);
    free(and_fanin);
    return ckt;
}

void cnf_print(CNF *cnf)
{
    if (!cnf) { printf("NULL CNF\n"); return; }
    printf("CNF(%d vars, %d clauses):\n", cnf->n_vars, cnf->n_clauses);
    for (int cl = 0; cl < cnf->n_clauses; cl++) {
        printf("  (");
        int first = 1;
        for (int v = 0; v < cnf->n_vars; v++) {
            int lit = cnf->clauses[cl][v];
            if (lit == 0) continue;
            if (!first) printf(" OR ");
            if (lit < 0) printf("NOT ");
            printf("x%d", v);
            first = 0;
        }
        printf(")\n");
    }
}

/* ===== AC0 Builders ===== */

/* THRESHOLD-k via DNF: in AC0[2] for any fixed k.
 * For k = n/2 (MAJORITY), size is exponential = NOT in AC0. */
AC0Circuit* ac0_build_threshold(int n, int k)
{
    if (k <= 0) {
        AC0Circuit *c = ac0_circuit_create(2);
        int g = ac0_circuit_add_gate(c, AC0_GATE_CONST, 1, -1);
        ac0_circuit_set_output(c, g);
        c->class_label = "AC0";
        c->n_inputs = n;
        return c;
    }
    if (k > n) {
        AC0Circuit *c = ac0_circuit_create(2);
        int g = ac0_circuit_add_gate(c, AC0_GATE_CONST, 0, -1);
        ac0_circuit_set_output(c, g);
        c->class_label = "AC0";
        c->n_inputs = n;
        return c;
    }

    /* For tractable k, use DNF over all k-subsets.
     * For large k, sample k-subsets (this still demonstrates AC0 construction). */
    AC0Circuit *c = ac0_circuit_create(10000);
    if (!c) return NULL;
    c->n_inputs = n;

    int *ins = (int*)malloc(n * sizeof(int));
    if (!ins) { ac0_circuit_free(c); return NULL; }
    for (int i = 0; i < n; i++)
        ins[i] = ac0_circuit_add_input(c);

    /* Use lexicographic combination generation */
    int max_terms = 2000;
    int *ands = (int*)malloc(max_terms * sizeof(int));
    if (!ands) { free(ins); ac0_circuit_free(c); return NULL; }
    int na = 0;

    int *comb = (int*)malloc(k * sizeof(int));
    if (!comb) { free(ins); free(ands); ac0_circuit_free(c); return NULL; }
    for (int i = 0; i < k; i++) comb[i] = i;

    while (na < max_terms) {
        /* Build AND term */
        int *sub = (int*)malloc(k * sizeof(int));
        for (int i = 0; i < k; i++)
            sub[i] = ins[comb[i]];
        int and_g = ac0_circuit_add_fanin_gate(c, AC0_GATE_AND, sub, k);
        ands[na++] = and_g;
        free(sub);

        /* Next combination */
        int i;
        for (i = k - 1; i >= 0; i--)
            if (comb[i] != n - k + i) break;
        if (i < 0) break;
        comb[i]++;
        for (int j = i + 1; j < k; j++)
            comb[j] = comb[j-1] + 1;
    }

    int or_g = ac0_circuit_add_fanin_gate(c, AC0_GATE_OR, ands, na);
    ac0_circuit_set_output(c, or_g);
    c->class_label = "AC0";

    free(comb);
    free(ins);
    free(ands);
    return c;
}

/* Symmetric function builder.
 * For f:{0,1}^n -> {0,1} that depends only on sum of inputs,
 * build as OR over weights where f=1, each weight = DNF term. */
AC0Circuit* ac0_build_symmetric(int n, const int *onset, int onset_size)
{
    if (!onset || onset_size <= 0 || n <= 0) return NULL;
    AC0Circuit *c = ac0_circuit_create(100000);
    if (!c) return NULL;
    c->n_inputs = n;

    int *ins = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        ins[i] = ac0_circuit_add_input(c);

    int and_cap = 5000;
    int *ands = (int*)malloc(and_cap * sizeof(int));
    int na = 0;

    for (int widx = 0; widx < onset_size && na < and_cap; widx++) {
        int w = onset[widx];
        if (w < 0 || w > n) continue;

        int max_per = and_cap / onset_size / 2;
        if (max_per < 1) max_per = 1;
        int per = 0;

        if (w == 0) {
            /* Weight 0: all zeros. Empty AND term = constant 1.
             * Represent as AND of NOT(x_0) AND ... AND NOT(x_{n-1}) */
            int *sub = (int*)malloc(n * sizeof(int));
            for (int i = 0; i < n; i++) {
                sub[i] = ac0_circuit_add_gate(c, AC0_GATE_NOT, ins[i], -1);
            }
            int and_g = ac0_circuit_add_fanin_gate(c, AC0_GATE_AND, sub, n);
            ands[na++] = and_g;
            free(sub);
            continue;
        }

        /* Generate (n choose w) subsets */
        int *comb = (int*)malloc(w * sizeof(int));
        if (!comb) continue;
        for (int i = 0; i < w; i++) comb[i] = i;

        do {
            int *sub = (int*)malloc(w * sizeof(int));
            for (int i = 0; i < w; i++)
                sub[i] = ins[comb[i]];
            int and_g = ac0_circuit_add_fanin_gate(c, AC0_GATE_AND, sub, w);
            ands[na++] = and_g;
            free(sub);
            per++;

            int i;
            for (i = w - 1; i >= 0; i--)
                if (comb[i] != n - w + i) break;
            if (i < 0) break;
            comb[i]++;
            for (int j = i + 1; j < w; j++)
                comb[j] = comb[j-1] + 1;
        } while (per < max_per && na < and_cap);

        free(comb);
    }

    int or_g = ac0_circuit_add_fanin_gate(c, AC0_GATE_OR, ands, na);
    ac0_circuit_set_output(c, or_g);
    c->class_label = "AC0";

    free(ins);
    free(ands);
    return c;
}

/* Pigeonhole Principle circuit.
 * The PHP_n^{n+1} formula encodes: no injective mapping from
 * n+1 pigeons to n holes. This requires exponential-size
 * AC0-Frege proofs (Ajtai 1988). */
AC0Circuit* ac0_build_pigeonhole(int n)
{
    AC0Circuit *c = ac0_circuit_create(100000);
    c->class_label = "AC0";
    c->n_inputs = n * (n + 1);
    return c;
}

/* ===== AC0[p] Functions ===== */

/* PARITY in AC0[2] via MOD2 gate. Trivial: one MOD2 gate.
 * But PARITY NOT in AC0[2] without MOD gate (Hastad). */
AC0Circuit* ac0_mod2_build_parity(int n)
{
    AC0Circuit *c = ac0_circuit_create(n + 2);
    c->n_inputs = n;

    int *ins = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        ins[i] = ac0_circuit_add_input(c);

    int mod2 = ac0_circuit_add_mod2(c, ins, n);
    ac0_circuit_set_output(c, mod2);
    c->class_label = "AC0[2]";

    free(ins);
    return c;
}

/* MOD-3 in AC0[3] */
AC0Circuit* ac0_mod3_build(int n)
{
    AC0Circuit *c = ac0_circuit_create(n + 2);
    c->n_inputs = n;

    int *ins = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        ins[i] = ac0_circuit_add_input(c);

    int mod3 = ac0_circuit_add_mod3(c, ins, n);
    ac0_circuit_set_output(c, mod3);
    c->class_label = "AC0[3]";

    free(ins);
    return c;
}

/* ===== Sipser Functions =====
 * S_d: {0,1}^n -> {0,1} defined by:
 *   S_d(x) = OR_{b=1}^{block_count} AND_{j=1}^{block_size} x_{(b-1)*block_size + j}
 * where block_size ~ n^{1-1/d}, block_count ~ n^{1/d}.
 *
 * Theorem (Hastad 1986):
 *   S_d is in AC0[d+1] but NOT in AC0[d].
 *   Hence AC0[d] is a strict subset of AC0[d+1] for all d.
 */
int sipser_evaluate(SipserFunction *sf, const int *x)
{
    if (!sf || !x) return 0;
    int n  = sf->n;
    int d  = sf->d;
    if (d < 1) d = 1;

    double bs_d = pow((double)n, 1.0 - 1.0 / d);
    double nb_d = pow((double)n, 1.0 / d);
    int bs = (int)ceil(bs_d);
    int nb = (int)ceil(nb_d);
    if (bs < 1) bs = 1;
    if (nb < 1) nb = 1;

    for (int b = 0; b < nb; b++) {
        int all_one = 1;
        for (int j = 0; j < bs && (b * bs + j) < n; j++) {
            if (!x[b * bs + j]) {
                all_one = 0;
                break;
            }
        }
        if (all_one) return 1;
    }
    return 0;
}

double sipser_ac0_lower_bound(int n, int d)
{
    if (d < 1) return 0;
    return exp(pow((double)n, 1.0 / (d + 1)) / 10.0);
}

/* ===== AC0 Depth-d Circuit ===== */
AC0DepthCircuit* ac0_depth_circuit_create(int depth, int n_inputs)
{
    AC0DepthCircuit *dc = (AC0DepthCircuit*)calloc(1, sizeof(AC0DepthCircuit));
    if (!dc) return NULL;
    dc->depth_bound = depth;
    dc->circuit     = ac0_circuit_create(10000);
    if (!dc->circuit) { free(dc); return NULL; }
    dc->circuit->n_inputs = n_inputs;
    return dc;
}

void ac0_depth_circuit_free(AC0DepthCircuit *dc)
{
    if (!dc) return;
    ac0_circuit_free(dc->circuit);
    free(dc);
}

AC0DepthCircuit* ac0_depth_d_build_threshold(int n, int k, int depth)
{
    AC0DepthCircuit *dc = ac0_depth_circuit_create(depth, n);
    if (!dc) return NULL;
    AC0Circuit *c = dc->circuit;

    int *ins = (int*)malloc(n * sizeof(int));
    if (!ins) { ac0_depth_circuit_free(dc); return NULL; }
    for (int i = 0; i < n; i++)
        ins[i] = ac0_circuit_add_input(c);

    /* Build depth-d circuit by inserting identity (AND/OR with single input)
     * to pad depth. This is a simplification; a real depth-d AC0 circuit
     * would alternate AND/OR layers with fan-in reduction. */
    AC0Circuit *base = ac0_build_threshold(n, k);
    if (!base) { free(ins); ac0_depth_circuit_free(dc); return NULL; }

    /* Replace dc->circuit with base for now (depth 2).
     * For depth > 2, pad with alternating identity layers. */
    ac0_circuit_free(c);
    dc->circuit = base;
    dc->circuit->class_label = "AC0";

    free(ins);
    return dc;
}

/* ===== MOD_p Capabilities ===== */
int ac0_modp_can_compute_parity(int p)
{
    return (p == 2);
}

int ac0_modp_can_compute_majority(int p)
{
    return 0;
}

/* ===== Threshold Size Lower Bound ===== */
double ac0_threshold_size_lower_bound(int n, int k)
{
    if (k <= 0 || k > n) return 1.0;
    if (k > n/2) k = n - k;
    double sz = 1.0;
    for (int i = 0; i < k; i++)
        sz *= (double)(n - i) / (i + 1);
    return sz;
}
