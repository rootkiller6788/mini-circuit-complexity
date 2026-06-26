/* csat.c -- Circuit-SAT Complexity: Core Algorithms and Demos
 *
 * Circuit-SAT: given Boolean circuit C, exists x with C(x)=1?
 * NP-complete (Cook 1971). First problem proved NP-complete.
 *
 * Williams' program (2010-2014): SAT algorithms <=> circuit lower bounds.
 *   If C-SAT solvable in 2^n/n^{omega(1)} time => NEXP not in C.
 *   Proved for ACC0 (Williams 2014). Open for P/poly.
 */

#include "csat.h"

/* ============================================================================
 * L1: Core SAT Algorithms - Implementations
 * ============================================================================ */

int csat_brute(BooleanCircuit* c, int n_inputs)
{
    long long total = 1LL << n_inputs;
    if (total > (1LL << 20)) total = 1LL << 20;
    int* input = (int*)calloc((size_t)n_inputs, sizeof(int));
    if (!input) return 0;
    for (long long m = 0; m < total; m++) {
        for (int i = 0; i < n_inputs; i++)
            input[i] = (int)((m >> i) & 1);
        if (circuit_evaluate(c, input)) {
            free(input);
            return 1;
        }
    }
    free(input);
    return 0;
}

int csat_dpll(BooleanCircuit* c, int* assign, int depth, int nv)
{
    if (depth == nv)
        return circuit_evaluate(c, assign);
    assign[depth] = 0;
    if (csat_dpll(c, assign, depth + 1, nv)) return 1;
    assign[depth] = 1;
    if (csat_dpll(c, assign, depth + 1, nv)) return 1;
    assign[depth] = -1;
    return 0;
}

int csat_random(BooleanCircuit* c, int nv, int samples)
{
    int* input = (int*)malloc((size_t)nv * sizeof(int));
    if (!input) return 0;
    for (int s = 0; s < samples; s++) {
        for (int i = 0; i < nv; i++)
            input[i] = rand() & 1;
        if (circuit_evaluate(c, input)) {
            free(input);
            return 1;
        }
    }
    free(input);
    return 0;
}

int csat_dpll_full(BooleanCircuit* c, int* assign, int depth, int nv)
{
    if (depth == nv)
        return circuit_evaluate(c, assign);
    assign[depth] = 0;
    int val0 = circuit_evaluate(c, assign);
    assign[depth] = 1;
    int val1 = circuit_evaluate(c, assign);
    if (val0 == val1) {
        assign[depth] = val0 ? 1 : 0;
        return csat_dpll_full(c, assign, depth + 1, nv);
    }
    if (!val0 && val1) {
        assign[depth] = 1;
        return csat_dpll_full(c, assign, depth + 1, nv);
    }
    if (val0 && !val1) {
        assign[depth] = 0;
        return csat_dpll_full(c, assign, depth + 1, nv);
    }
    assign[depth] = 0;
    if (csat_dpll_full(c, assign, depth + 1, nv)) return 1;
    assign[depth] = 1;
    if (csat_dpll_full(c, assign, depth + 1, nv)) return 1;
    assign[depth] = -1;
    return 0;
}

int csat_random_walk(BooleanCircuit* c, int nv, int max_steps)
{
    int* assign = (int*)malloc((size_t)nv * sizeof(int));
    if (!assign) return 0;
    for (int i = 0; i < nv; i++)
        assign[i] = rand() & 1;
    for (int step = 0; step < max_steps; step++) {
        if (circuit_evaluate(c, assign)) { free(assign); return 1; }
        int flip = rand() % nv;
        assign[flip] ^= 1;
    }
    free(assign);
    return 0;
}

int csat_random_restarts(BooleanCircuit* c, int nv, int restarts, int steps)
{
    for (int r = 0; r < restarts; r++)
        if (csat_random_walk(c, nv, steps)) return 1;
    return 0;
}

double csat_williams_speedup(int n, int k)
{
    double baseline = pow(2.0, (double)n);
    double divisor  = pow((double)n, (double)k);
    if (divisor < 1.0) return baseline;
    return baseline / divisor;
}

BooleanCircuit* circuit_parse(const char* spec)
{
    if (!spec) return NULL;
    int ng = 0, ni = 0;
    for (const char* p = spec; *p; p++) {
        if (*p == 'I') ni++;
        if (*p == 'A' || *p == 'O' || *p == 'N' || *p == 'X') ng++;
    }
    int cap = ni + ng + 10;
    BooleanCircuit* c = circuit_create(cap);
    if (!c) return NULL;
    int* gate_ids = (int*)malloc((size_t)(ni + ng) * sizeof(int));
    if (!gate_ids) { circuit_free(c); return NULL; }
    int gidx = 0;
    for (const char* p = spec; *p; p++) {
        int gid = -1;
        switch (*p) {
            case 'I': gid = circuit_add_gate(c, INPUT, -1, -1); break;
            case 'A':
                gid = circuit_add_gate(c, AND,
                    (gidx >= 2) ? gate_ids[gidx-1] : -1,
                    (gidx >= 2) ? gate_ids[gidx-2] : -1);
                break;
            case 'O':
                gid = circuit_add_gate(c, OR,
                    (gidx >= 2) ? gate_ids[gidx-1] : -1,
                    (gidx >= 2) ? gate_ids[gidx-2] : -1);
                break;
            case 'N':
                gid = circuit_add_gate(c, NOT,
                    (gidx >= 1) ? gate_ids[gidx-1] : -1, -1);
                break;
            case 'X':
                gid = circuit_add_gate(c, XOR,
                    (gidx >= 2) ? gate_ids[gidx-1] : -1,
                    (gidx >= 2) ? gate_ids[gidx-2] : -1);
                break;
            default: break;
        }
        if (gid >= 0) gate_ids[gidx++] = gid;
    }
    if (gidx > 0) {
        int out = gate_ids[gidx - 1];
        circuit_set_output(c, &out, 1);
    }
    free(gate_ids);
    return c;
}

/* ============================================================================
 * Demo Functions
 * ============================================================================ */

void csat_demo(void)
{
    printf("\n================================================================\n");
    printf("  CIRCUIT-SAT COMPLEXITY\n");
    printf("================================================================\n\n");
    printf("Circuit-SAT: Given circuit C, exists x with C(x)=1?\n");
    printf("This is NP-complete (Cook 1971).\n\n");

    int n = 8;
    printf("--- Brute-Force Circuit-SAT ---\n");
    BooleanCircuit* parity = circuit_parity(n);
    printf("PARITY(%d): size=%d, SAT=%s\n", n, circuit_size(parity),
           csat_brute(parity, n) ? "YES" : "NO");
    circuit_free(parity);

    BooleanCircuit* maj = circuit_majority(n);
    printf("MAJORITY(%d): size=%d, SAT=%s\n", n, circuit_size(maj),
           csat_brute(maj, n) ? "YES" : "NO");
    circuit_free(maj);

    BooleanCircuit* or_all = circuit_create(2 * n);
    int* ins = (int*)malloc((size_t)n * sizeof(int));
    if (ins && or_all) {
        for (int i = 0; i < n; i++) ins[i] = circuit_add_gate(or_all, INPUT, -1, -1);
        int cur = ins[0];
        for (int i = 1; i < n; i++)
            cur = circuit_add_gate(or_all, OR, cur, ins[i]);
        int out_id = cur;
        circuit_set_output(or_all, &out_id, 1);
        printf("OR_ALL(%d): size=%d, SAT=%s\n", n, circuit_size(or_all),
               csat_brute(or_all, n) ? "YES" : "NO");
    }
    if (or_all) circuit_free(or_all);
    free(ins);

    printf("\n--- DPLL Circuit-SAT Timing ---\n");
    for (int nn = 4; nn <= 10; nn += 2) {
        BooleanCircuit* c = circuit_parity(nn);
        int* assign = (int*)calloc((size_t)nn, sizeof(int));
        if (!assign || !c) { if (c) circuit_free(c); free(assign); continue; }
        clock_t t = clock();
        int sat = csat_dpll(c, assign, 0, nn);
        double ms = 1000.0 * (clock() - t) / CLOCKS_PER_SEC;
        printf("  PARITY(%d): SAT=%s (%.3f ms)\n", nn, sat ? "YES" : "NO", ms);
        circuit_free(c);
        free(assign);
    }

    printf("\n--- Williams' Program ---\n");
    printf("If Circuit-SAT solvable in 2^n / n^k time:\n");
    for (int nn = 10; nn <= 30; nn += 10) {
        double base = pow(2.0, (double)nn);
        for (int k = 1; k <= 3; k++) {
            double imp = csat_williams_speedup(nn, k);
            printf("  n=%d, k=%d: 2^n=%.0f -> improved=%.0f (%.0fx faster)\n",
                   nn, k, base, imp, base / imp);
        }
    }
    printf("\nWilliams (2010): ANY non-trivial Circuit-SAT algorithm\n");
    printf("implies NEXP not in ACC0 (breakthrough!).\n");
    printf("This connects ALGORITHMS to LOWER BOUNDS.\n");
}

void csat_core_algorithms_demo(void)
{
    printf("\n================================================================\n");
    printf("  CIRCUIT-SAT: Algorithm Portfolio\n");
    printf("================================================================\n\n");
    int n = 10;
    printf("--- Brute Force (2^%d assignments) ---\n", n);
    BooleanCircuit* par = circuit_parity(n);
    if (!par) return;
    clock_t t = clock();
    int sat = csat_brute(par, n);
    double ms = 1000.0 * (clock() - t) / CLOCKS_PER_SEC;
    printf("  PARITY(%d) SAT=%s (%.1f ms)\n", n, sat ? "YES" : "NO", ms);

    printf("\n--- DPLL (exponential worst-case) ---\n");
    int* assign = (int*)calloc((size_t)n, sizeof(int));
    if (assign) {
        t = clock();
        sat = csat_dpll(par, assign, 0, n);
        ms = 1000.0 * (clock() - t) / CLOCKS_PER_SEC;
        printf("  PARITY(%d) SAT=%s (%.1f ms)\n", n, sat ? "YES" : "NO", ms);
        free(assign);
    }
    circuit_free(par);

    printf("\n--- Random Sampling (heuristic) ---\n");
    BooleanCircuit* maj = circuit_majority(n);
    if (maj) {
        t = clock();
        sat = csat_random(maj, n, 1000);
        ms = 1000.0 * (clock() - t) / CLOCKS_PER_SEC;
        printf("  MAJORITY(%d) SAT=%s (%.1f ms, 1000 samples)\n",
               n, sat ? "YES" : "NO", ms);
        circuit_free(maj);
    }

    printf("\n--- Williams Connection ---\n");
    for (int k = 1; k <= 3; k++) {
        double base = pow(2.0, (double)n);
        double imp  = csat_williams_speedup(n, k);
        printf("  2^%d/n^%d: %.0f -> %.0f (%.0fx faster)\n",
               n, k, base, imp, base / imp);
    }
    printf("Any non-trivial SAT algorithm -> circuit lower bounds (Williams).\n");
}

void csat_parse_demo(void)
{
    printf("\n===== Circuit Parser =====\n\n");
    printf("Format: I=INPUT A=AND O=OR N=NOT X=XOR. Sequential gate list.\n\n");
    const char* specs[] = {"IIX", "IIA", "IIN", "IIO"};
    const char* names[] = {"XOR(x0,x1)", "AND(x0,x1)", "NOT(x0)", "OR(x0,x1)"};
    int inp[2] = {1, 0};
    for (int i = 0; i < 4; i++) {
        BooleanCircuit* c = circuit_parse(specs[i]);
        if (!c) continue;
        printf("  %s: eval(1,0)=%d\n", names[i], circuit_evaluate(c, inp));
        circuit_free(c);
    }
    printf("\nParser builds DAG from gate sequence. Used for importing benchmarks.\n");
}

void csat_random_demo(void)
{
    srand(54321);
    printf("\n=== Randomized Circuit-SAT ===\n\n");
    printf("Random walk: flip random variable, check SAT, repeat.\n");
    printf("Schoning's bound: O((2-2/k)^n) for k-SAT.\n\n");
    for (int n = 4; n <= 10; n += 2) {
        BooleanCircuit* p = circuit_parity(n);
        if (!p) continue;
        int sat = csat_random_restarts(p, n, 50, n * n);
        printf("  PARITY(%d): %s (50 restarts, %d steps each)\n",
               n, sat ? "SAT" : "UNSAT?", n * n);
        circuit_free(p);
    }
    printf("\nRandom walk is fast but incomplete - can miss solutions.\n");
}

void csat_hardness_demo(void)
{
    printf("\n=== CSAT Hardness ===\n\n");
    printf("Circuit-SAT: NP-complete (Cook 1971).\n");
    printf("Restricted cases:\n");
    printf("  DNF: trivial (P). CNF/3SAT: NP-complete.\n");
    printf("  Monotone: NP-complete. Planar: NP-complete.\n");
    printf("  Tree/read-once: P. Horn: P. 2SAT: P.\n");
    printf("  AC0-SAT: NP-complete, but Williams algorithm: 2^{n-n^delta}.\n");
    printf("  Depth-2: still NP-complete (DNF-SAT is NP-complete).\n");
    printf("  Bounded treewidth: FPT. d-DNNF: P.\n");
}

void csat_reduction_demo(void)
{
    printf("\n=== Circuit-SAT to CNF-SAT (Tseitin) ===\n\n");
    printf("Tseitin transformation: O(n) size blowup.\n");
    printf("Each gate => 2-4 clauses with auxiliary variables.\n");
    printf("Equisatisfiable: original circuit SAT iff CNF SAT.\n\n");
    BooleanCircuit* p = circuit_parity(4);
    if (!p) return;
    CNFInstance* cnf = csat_to_cnf(p);
    if (cnf) {
        printf("PARITY(4): %d gates => CNF with %d vars, %d clauses\n",
               circuit_size(p), cnf->nv, cnf->nc);
        printf("Clause-to-variable ratio: %.2f\n", cnf_clause_var_ratio(cnf));
        cnf_free(cnf);
    }
    circuit_free(p);
}

void csat_benchmark_demo(void)
{
    printf("\n===== SAT Algorithm Benchmark =====\n\n");
    printf("%6s %14s %14s\n", "n", "brute(ms)", "DPLL(ms)");
    for (int n = 4; n <= 12; n++) {
        BooleanCircuit* p = circuit_parity(n);
        if (!p) continue;
        clock_t t = clock();
        csat_brute(p, n);
        double bm = 1000.0 * (clock() - t) / CLOCKS_PER_SEC;
        int* as = (int*)calloc((size_t)n, sizeof(int));
        t = clock();
        if (as) { csat_dpll(p, as, 0, n); free(as); }
        double dm = 1000.0 * (clock() - t) / CLOCKS_PER_SEC;
        printf("%6d %13.3f %13.3f\n", n, bm, dm);
        circuit_free(p);
    }
    printf("\nBrute-force: O(2^n). DPLL: O(2^n) worst but faster on average.\n");
    printf("Williams program: 2^n/n^k -> circuit lower bounds.\n");
}

void csat_williams_demo(void)
{
    printf("\n=== Williams Algorithmic Method ===\n\n");
    printf("SAT algorithms <=> circuit lower bounds.\n");
    printf("ACC0-SAT in 2^n/n^k => NEXP not in ACC0 (PROVED 2014).\n");
    printf("General C-SAT in 2^n/n^k => NEXP not in P/poly (OPEN).\n\n");
    for (int n = 10; n <= 30; n += 10) {
        printf("  n=%d: 2^n=%.0e", n, pow(2.0, (double)n));
        for (int k = 1; k <= 3; k++)
            printf(", /n^%d=%.0e", k, csat_williams_speedup(n, k));
        printf("\n");
    }
}

void csat_depth_analysis_demo(void)
{
    printf("\n=== Circuit Depth Analysis ===\n\n");
    int n = 8;
    BooleanCircuit* p = circuit_parity(n);
    if (!p) return;
    printf("PARITY(%d): size=%d, exact depth=%d\n",
           n, circuit_size(p), circuit_depth_exact(p));
    int ands = 0, ors = 0, nots = 0, xors = 0;
    circuit_gate_distribution(p, &ands, &ors, &nots, &xors);
    printf("  Gate distribution: AND=%d OR=%d NOT=%d XOR=%d\n", ands, ors, nots, xors);
    printf("  Is alternating: %s\n", circuit_is_alternating(p) ? "yes" : "no");
    circuit_free(p);
    BooleanCircuit* m = circuit_majority(n);
    if (m) {
        printf("MAJORITY(%d): size=%d, exact depth=%d\n",
               n, circuit_size(m), circuit_depth_exact(m));
        circuit_free(m);
    }
}

void csat_optimization_demo(void)
{
    printf("\n=== Circuit Optimization ===\n\n");
    int n = 6;
    BooleanCircuit* c = circuit_parity(n);
    if (!c) return;
    printf("PARITY(%d) original: size=%d, depth=%d\n",
           n, circuit_size(c), circuit_depth(c));
    int removed = optimize_all_passes(c);
    printf("After optimization: size=%d, depth=%d (removed %d gates)\n",
           circuit_size(c), circuit_depth(c), removed);
    circuit_free(c);
}

void csat_transformations_demo(void)
{
    printf("\n=== Circuit Transformations ===\n\n");
    int n = 4;
    BooleanCircuit* c = circuit_parity(n);
    if (!c) return;
    printf("PARITY(%d) original: size=%d\n", n, circuit_size(c));
    int pushed = circuit_push_negations(c);
    printf("After push-negations: %d gates pushed\n", pushed);
    BooleanCircuit* nand_c = circuit_to_nand_basis(c);
    if (nand_c) {
        printf("NAND-basis: size=%d\n", circuit_size(nand_c));
        circuit_free(nand_c);
    }
    circuit_free(c);
}

void csat_cnf_tools_demo(void)
{
    printf("\n=== CNF Tools ===\n\n");
    CNFInstance* cnf = cnf_create(4);
    if (!cnf) return;
    int c1[] = {1, 2, 3, 0};
    int c2[] = {-1, -2, 0};
    int c3[] = {2, -3, 4, 0};
    cnf_add_clause(cnf, c1, 3);
    cnf_add_clause(cnf, c2, 2);
    cnf_add_clause(cnf, c3, 3);
    printf("Created CNF: %d vars, %d clauses, ratio=%.2f\n",
           cnf->nv, cnf->nc, cnf_clause_var_ratio(cnf));
    cnf_print_dimacs(cnf, stdout);
    int assign[] = {0, 1, 1, 0, 1};
    int sat_cnt = cnf_count_satisfied(cnf, assign);
    printf("Satisfied clauses: %d / %d\n", sat_cnt, cnf->nc);
    int elim = cnf_pure_literal_elim(cnf);
    printf("Pure literal elim: %d variables eliminated\n", elim);
    cnf_free(cnf);
}

void csat_lower_bounds_demo(void)
{
    printf("\n=== Circuit Lower Bounds ===\n\n");
    printf("--- Shannon's Counting Argument (1949) ---\n");
    printf("Almost all Boolean functions require size Omega(2^n / n).\n\n");
    for (int n = 3; n <= 10; n++)
        printf("  n=%d: ShannonLB=%.1f, LupanovUB=%.1f\n",
               n, shannon_bound(n), lupanov_bound(n));
    printf("\n--- Hastad Switching Lemma ---\n");
    printf("PARITY requires EXPONENTIAL size for constant-depth circuits.\n");
    printf("Bound for depth d: size >= 2^{n^{1/(d-1)}}.\n");
    for (int d = 2; d <= 5; d++)
        printf("  depth=%d: min size ~ 2^{%d^{1/%d}} = 2^%.1f\n",
               d, 10, d-1, pow(10.0, 1.0/(d-1)));
}

void csat_mcsp_demo(void)
{
    printf("\n=== MCSP: Minimum Circuit Size Problem ===\n\n");
    printf("MCSP: given truth table, find minimal circuit computing it.\n");
    printf("Meta-complexity: hardness of MCSP => circuit lower bounds.\n");
    printf("(Kabanets & Cai 2000, Allender et al. 2017)\n\n");
    int n = 4;
    BooleanCircuit* c = circuit_parity(n);
    if (!c) return;
    MCSPSolver* s = mcsp_create(c);
    if (!s) { circuit_free(c); return; }
    printf("Original circuit size: %d\n", circuit_size(c));
    mcsp_greedy_minimize(s, 1000);
    mcsp_print_result(s);
    int ess = mcsp_kernelize(s);
    printf("Essential gates: %d\n", ess);
    mcsp_free(s);
    circuit_free(c);
}

void csat_lookahead_demo(void)
{
    printf("\n=== Lookahead SAT Solver ===\n\n");
    printf("Lookahead: probe variables, detect failed literals.\n");
    printf("Based on: Heule & van Maaren (2009).\n");
    CNFInstance* cnf = cnf_create(3);
    if (!cnf) return;
    int c1[] = {1, 2, 0};
    int c2[] = {-1, -2, 0};
    int c3[] = {2, -3, 0};
    cnf_add_clause(cnf, c1, 2);
    cnf_add_clause(cnf, c2, 2);
    cnf_add_clause(cnf, c3, 2);
    printf("CNF: %d vars, %d clauses\n", cnf->nv, cnf->nc);
    LookaheadSolver* la = lookahead_create(cnf);
    if (la) {
        int result = lookahead_solve(la);
        printf("Lookahead result: %s\n", result == 1 ? "SAT" :
               (result == 0 ? "UNSAT" : "UNKNOWN"));
        lookahead_free(la);
    }
    cnf_free(cnf);
}

void csat_cdcl_demo(void)
{
    printf("\n=== CDCL SAT Solver ===\n\n");
    printf("Conflict-Driven Clause Learning (MiniSAT-style).\n");
    printf("Key components: watched literals, 1-UIP, VSIDS, Luby restarts.\n\n");
    CNFInstance* cnf = cnf_create(3);
    if (!cnf) return;
    int c1[] = {1, 2, 0};
    int c2[] = {-1, 3, 0};
    int c3[] = {-2, -3, 0};
    int c4[] = {1, -3, 0};
    cnf_add_clause(cnf, c1, 2);
    cnf_add_clause(cnf, c2, 2);
    cnf_add_clause(cnf, c3, 2);
    cnf_add_clause(cnf, c4, 2);
    CDCLSolver* solver = cdcl_create(cnf->nv);
    if (solver) {
        cdcl_load_cnf(solver, cnf);
        int result = cdcl_solve(solver, 10000);
        printf("CDCL result: %s\n", result == 1 ? "SAT" :
               (result == 0 ? "UNSAT" : "UNKNOWN"));
        if (result == 1) {
            int* model = (int*)malloc((size_t)cnf->nv * sizeof(int));
            if (model) {
                cdcl_get_model(solver, model);
                printf("Model: ");
                for (int i = 0; i < cnf->nv; i++)
                    printf("x%d=%d ", i+1, model[i]);
                printf("\n");
                free(model);
            }
        }
        cdcl_print_stats(solver);
        cdcl_free(solver);
    }
    cnf_free(cnf);
}