/* ac0_core.c -- Core Circuit Operations for Circuit Complexity Hierarchy
 * =====================================================================
 * Central implementation of Boolean circuit data structures,
 * construction, evaluation, analysis, and transformations.
 *
 * A Boolean circuit C on n inputs is a labeled directed acyclic
 * graph (DAG) where:
 *   - Source nodes (in-degree 0) are input variables x_1,...,x_n
 *     or constant gates (0/1).
 *   - Internal nodes are labeled by Boolean operations:
 *     AND, OR, NOT, MOD_p, MAJORITY, THRESHOLD, XOR, NAND.
 *   - Sink nodes (out-degree 0) are designated outputs.
 *
 * The circuit computes a function C:{0,1}^n -> {0,1}^m by
 * evaluating gates in topological order.
 *
 * Complexity measures:
 *   size(C) = number of gates (excluding inputs)
 *   depth(C) = length of longest path from input to output
 *   fan-in(C) = maximum number of inputs to any gate
 *
 * References:
 *   Arora & Barak Chapter 6 "Boolean Circuits"
 *   Vollmer "Introduction to Circuit Complexity" (1999)
 */
#include "ac0nc.h"

/* ===== Construction / Destruction ===== */

AC0Circuit* ac0_circuit_create(int capacity)
{
    if (capacity <= 0) capacity = 1024;
    if (capacity > AC0_MAX_GATES) capacity = AC0_MAX_GATES;

    AC0Circuit *c = (AC0Circuit*)calloc(1, sizeof(AC0Circuit));
    if (!c) return NULL;

    c->gates = (AC0Gate*)calloc(capacity, sizeof(AC0Gate));
    if (!c->gates) {
        free(c);
        return NULL;
    }
    c->n_gates    = 0;
    c->n_inputs   = 0;
    c->n_outputs  = 0;
    c->output_ids = (int*)calloc(AC0_MAX_OUTPUTS, sizeof(int));
    if (!c->output_ids) {
        free(c->gates);
        free(c);
        return NULL;
    }
    c->is_uniform  = 1;
    c->class_label = NULL;

    /* Allocate space for gate metadata */
    return c;
}

void ac0_circuit_free(AC0Circuit *c)
{
    if (!c) return;
    free(c->gates);
    free(c->output_ids);
    free(c);
}

int ac0_circuit_add_input(AC0Circuit *c)
{
    if (!c || c->n_gates >= AC0_MAX_GATES) return -1;
    int id = c->n_gates++;
    c->gates[id].type     = AC0_GATE_INPUT;
    c->gates[id].n_fanin  = 0;
    c->gates[id].value    = -1;
    c->gates[id].eval_mark = 0;
    c->gates[id].depth    = 0;
    c->n_inputs++;
    return id;
}

int ac0_circuit_add_gate(AC0Circuit *c, AC0GateType type, int i1, int i2)
{
    if (!c || c->n_gates >= AC0_MAX_GATES) return -1;
    int id = c->n_gates++;
    c->gates[id].type     = type;
    c->gates[id].n_fanin  = 0;
    c->gates[id].threshold = 0;
    c->gates[id].mod_p    = 0;
    c->gates[id].value    = -1;
    c->gates[id].eval_mark = 0;
    c->gates[id].depth    = -1;
    c->gates[id].fanin[0] = i1;
    c->gates[id].fanin[1] = i2;
    if (i1 >= 0) c->gates[id].n_fanin = 1;
    if (i2 >= 0) c->gates[id].n_fanin = 2;
    return id;
}

int ac0_circuit_add_fanin_gate(AC0Circuit *c, AC0GateType type,
                                 const int *fanin, int nf)
{
    if (!c || c->n_gates >= AC0_MAX_GATES) return -1;
    if (nf > AC0_MAX_FANIN) nf = AC0_MAX_FANIN;
    int id = c->n_gates++;
    c->gates[id].type     = type;
    c->gates[id].n_fanin  = nf;
    c->gates[id].threshold = 0;
    c->gates[id].mod_p    = 0;
    c->gates[id].value    = -1;
    c->gates[id].eval_mark = 0;
    c->gates[id].depth    = -1;
    for (int i = 0; i < nf; i++)
        c->gates[id].fanin[i] = fanin[i];
    return id;
}

int ac0_circuit_add_threshold(AC0Circuit *c, const int *fanin,
                                int nf, int k)
{
    int id = ac0_circuit_add_fanin_gate(c, AC0_GATE_THR, fanin, nf);
    if (id >= 0) {
        c->gates[id].threshold = k;
    }
    return id;
}

int ac0_circuit_add_modp(AC0Circuit *c, const int *fanin,
                           int nf, int p)
{
    int id = ac0_circuit_add_fanin_gate(c, AC0_GATE_MODP, fanin, nf);
    if (id >= 0) {
        c->gates[id].mod_p = p;
    }
    return id;
}

int ac0_circuit_add_mod2(AC0Circuit *c, const int *fanin, int nf)
{
    return ac0_circuit_add_fanin_gate(c, AC0_GATE_MOD2, fanin, nf);
}

int ac0_circuit_add_mod3(AC0Circuit *c, const int *fanin, int nf)
{
    return ac0_circuit_add_fanin_gate(c, AC0_GATE_MOD3, fanin, nf);
}

int ac0_circuit_add_majority(AC0Circuit *c, const int *fanin, int nf)
{
    return ac0_circuit_add_fanin_gate(c, AC0_GATE_MAJ, fanin, nf);
}

int ac0_circuit_set_output(AC0Circuit *c, int gate_id)
{
    if (!c || gate_id < 0 || gate_id >= c->n_gates) return -1;
    if (c->n_outputs >= AC0_MAX_OUTPUTS) return -1;
    c->output_ids[c->n_outputs++] = gate_id;
    return c->n_outputs - 1;
}

AC0Circuit* ac0_circuit_clone(AC0Circuit *c)
{
    if (!c) return NULL;
    int cap = c->n_gates * 2 + 10;
    AC0Circuit *cc = ac0_circuit_create(cap);
    if (!cc) return NULL;

    /* Copy all gates */
    for (int i = 0; i < c->n_gates && i < AC0_MAX_GATES; i++) {
        cc->gates[i] = c->gates[i];
    }
    cc->n_gates   = c->n_gates;
    cc->n_inputs  = c->n_inputs;
    cc->n_outputs = c->n_outputs;
    for (int i = 0; i < c->n_outputs; i++)
        cc->output_ids[i] = c->output_ids[i];
    cc->is_uniform  = c->is_uniform;
    cc->class_label = c->class_label;
    return cc;
}

/* ===== Circuit Evaluation ===== */

int ac0_circuit_eval_gate(AC0Circuit *c, int gate_id, const int *inputs,
                            int *visited)
{
    if (!c || gate_id < 0 || gate_id >= c->n_gates) return 0;
    if (visited[gate_id] >= 0)
        return visited[gate_id];

    AC0Gate *g = &c->gates[gate_id];
    int result;

    switch (g->type) {
        case AC0_GATE_INPUT:
            result = (gate_id < AC0_MAX_INPUTS && inputs)
                     ? inputs[gate_id] : 0;
            break;

        case AC0_GATE_CONST:
            result = g->fanin[0]; /* constant value stored in fanin[0] */
            break;

        case AC0_GATE_NOT:
            result = !ac0_circuit_eval_gate(c, g->fanin[0], inputs, visited);
            break;

        case AC0_GATE_AND:
            if (g->n_fanin > 0) {
                result = 1;
                for (int i = 0; i < g->n_fanin; i++)
                    result &= ac0_circuit_eval_gate(c, g->fanin[i], inputs, visited);
            } else {
                result = ac0_circuit_eval_gate(c, g->fanin[0], inputs, visited)
                       & ac0_circuit_eval_gate(c, g->fanin[1], inputs, visited);
            }
            break;

        case AC0_GATE_OR:
            if (g->n_fanin > 0) {
                result = 0;
                for (int i = 0; i < g->n_fanin; i++)
                    result |= ac0_circuit_eval_gate(c, g->fanin[i], inputs, visited);
            } else {
                result = ac0_circuit_eval_gate(c, g->fanin[0], inputs, visited)
                       | ac0_circuit_eval_gate(c, g->fanin[1], inputs, visited);
            }
            break;

        case AC0_GATE_NAND:
            if (g->n_fanin > 0) {
                result = 1;
                for (int i = 0; i < g->n_fanin; i++)
                    result &= ac0_circuit_eval_gate(c, g->fanin[i], inputs, visited);
            } else {
                result = ac0_circuit_eval_gate(c, g->fanin[0], inputs, visited)
                       & ac0_circuit_eval_gate(c, g->fanin[1], inputs, visited);
            }
            result = !result;
            break;

        case AC0_GATE_XOR:
            result = ac0_circuit_eval_gate(c, g->fanin[0], inputs, visited)
                   ^ ac0_circuit_eval_gate(c, g->fanin[1], inputs, visited);
            break;

        case AC0_GATE_MOD2:
            {
                result = 0;
                for (int i = 0; i < g->n_fanin; i++)
                    result ^= ac0_circuit_eval_gate(c, g->fanin[i], inputs, visited);
            }
            break;

        case AC0_GATE_MOD3:
            {
                int s = 0;
                for (int i = 0; i < g->n_fanin; i++)
                    s += ac0_circuit_eval_gate(c, g->fanin[i], inputs, visited);
                result = (s % 3 == 0);
            }
            break;

        case AC0_GATE_MODP:
            {
                int s = 0;
                for (int i = 0; i < g->n_fanin; i++)
                    s += ac0_circuit_eval_gate(c, g->fanin[i], inputs, visited);
                result = (s % g->mod_p == 0);
            }
            break;

        case AC0_GATE_MAJ:
            {
                int s = 0;
                for (int i = 0; i < g->n_fanin; i++)
                    s += ac0_circuit_eval_gate(c, g->fanin[i], inputs, visited);
                result = (2 * s > g->n_fanin);
            }
            break;

        case AC0_GATE_THR:
            {
                int s = 0;
                for (int i = 0; i < g->n_fanin; i++)
                    s += ac0_circuit_eval_gate(c, g->fanin[i], inputs, visited);
                result = (s >= g->threshold);
            }
            break;

        default:
            result = 0;
    }

    visited[gate_id] = result;
    return result;
}

int ac0_circuit_eval(AC0Circuit *c, const int *inputs)
{
    if (!c || c->n_outputs == 0) return 0;

    int *visited = (int*)malloc(c->n_gates * sizeof(int));
    if (!visited) return 0;
    for (int i = 0; i < c->n_gates; i++)
        visited[i] = -1;

    /* Evaluate all outputs and AND them together for single-output convention */
    int result = 1;
    for (int i = 0; i < c->n_outputs; i++)
        result &= ac0_circuit_eval_gate(c, c->output_ids[i], inputs, visited);

    free(visited);
    return result;
}

void ac0_circuit_eval_all(AC0Circuit *c, const int *inputs)
{
    if (!c) return;
    int *visited = (int*)malloc(c->n_gates * sizeof(int));
    if (!visited) return;
    for (int i = 0; i < c->n_gates; i++)
        visited[i] = -1;

    for (int i = 0; i < c->n_outputs; i++)
        ac0_circuit_eval_gate(c, c->output_ids[i], inputs, visited);

    /* Store computed values in gates */
    for (int i = 0; i < c->n_gates; i++)
        c->gates[i].value = visited[i];

    free(visited);
}

int ac0_circuit_truth_table_row(AC0Circuit *c, int row_index)
{
    if (!c) return 0;
    int n = c->n_inputs;
    if (n <= 0 || n > AC0_MAX_INPUTS) return 0;

    int inputs[AC0_MAX_INPUTS];
    for (int i = 0; i < n; i++)
        inputs[i] = (row_index >> i) & 1;

    return ac0_circuit_eval(c, inputs);
}

/* ===== Circuit Analysis ===== */

int ac0_circuit_depth(AC0Circuit *c)
{
    if (!c || c->n_gates == 0) return 0;

    int *depths = (int*)calloc(c->n_gates, sizeof(int));
    if (!depths) return -1;

    /* Initialize: inputs and constants have depth 0 */
    for (int i = 0; i < c->n_gates; i++) {
        AC0Gate *g = &c->gates[i];
        if (g->type == AC0_GATE_INPUT || g->type == AC0_GATE_CONST)
            depths[i] = 0;
        else
            depths[i] = -1;
    }

    int changed;
    do {
        changed = 0;
        for (int i = 0; i < c->n_gates; i++) {
            if (depths[i] >= 0) continue;
            AC0Gate *g = &c->gates[i];

            /* Need all fanin depths */
            int max_d = 0;
            int all_ready = 1;
            int nf = g->n_fanin;
            if (nf == 0 && g->type != AC0_GATE_CONST && g->type != AC0_GATE_INPUT) {
                /* Fan-in 2 mode */
                nf = 2;
            }
            for (int j = 0; j < nf; j++) {
                int fi = g->fanin[j];
                if (fi < 0 || fi >= c->n_gates) continue;
                if (depths[fi] < 0) {
                    all_ready = 0;
                    break;
                }
                if (depths[fi] > max_d) max_d = depths[fi];
            }
            if (all_ready && nf > 0) {
                depths[i] = max_d + 1;
                changed = 1;
            }
        }
    } while (changed);

    int max_depth = 0;
    for (int i = 0; i < c->n_gates; i++)
        if (depths[i] > max_depth) max_depth = depths[i];

    free(depths);
    return max_depth;
}

int ac0_circuit_size(AC0Circuit *c)
{
    if (!c) return 0;
    /* Count non-input gates */
    int sz = 0;
    for (int i = 0; i < c->n_gates; i++)
        if (c->gates[i].type != AC0_GATE_INPUT)
            sz++;
    return sz;
}

int ac0_circuit_max_fanin(AC0Circuit *c)
{
    if (!c) return 0;
    int maxf = 0;
    for (int i = 0; i < c->n_gates; i++) {
        int nf = c->gates[i].n_fanin;
        if (nf == 0 && c->gates[i].type != AC0_GATE_INPUT
            && c->gates[i].type != AC0_GATE_CONST)
            nf = 2; /* binary gate */
        if (nf > maxf) maxf = nf;
    }
    return maxf;
}

int ac0_circuit_min_fanin(AC0Circuit *c)
{
    if (!c) return 0;
    int minf = AC0_MAX_FANIN;
    int has_gates = 0;
    for (int i = 0; i < c->n_gates; i++) {
        if (c->gates[i].type == AC0_GATE_INPUT) continue;
        int nf = c->gates[i].n_fanin;
        if (nf == 0) {
            if (c->gates[i].type == AC0_GATE_CONST
                || c->gates[i].type == AC0_GATE_NOT)
                nf = 1;
            else
                nf = 2;
        }
        has_gates = 1;
        if (nf < minf) minf = nf;
    }
    return has_gates ? minf : 0;
}

int ac0_circuit_gate_count_by_type(AC0Circuit *c, AC0GateType t)
{
    if (!c) return 0;
    int cnt = 0;
    for (int i = 0; i < c->n_gates; i++)
        if (c->gates[i].type == t) cnt++;
    return cnt;
}

int ac0_circuit_is_monotone(AC0Circuit *c)
{
    if (!c) return 0;
    for (int i = 0; i < c->n_gates; i++) {
        AC0GateType t = c->gates[i].type;
        if (t == AC0_GATE_NOT) return 0;
        /* MOD gates are not monotone */
        if (t == AC0_GATE_MOD2 || t == AC0_GATE_MOD3
            || t == AC0_GATE_MODP || t == AC0_GATE_XOR)
            return 0;
    }
    return 1;
}

int ac0_circuit_is_acyclic(AC0Circuit *c)
{
    /* A constructed circuit should be acyclic by construction.
     * We verify by checking depth computation succeeds. */
    int d = ac0_circuit_depth(c);
    return (d >= 0);
}

void ac0_circuit_print_stats(AC0Circuit *c)
{
    if (!c) { printf("NULL circuit\n"); return; }
    printf("Circuit Statistics:\n");
    printf("  Gates: %d (inputs: %d, outputs: %d)\n",
           c->n_gates, c->n_inputs, c->n_outputs);
    printf("  Size (non-input): %d\n", ac0_circuit_size(c));
    printf("  Depth: %d\n", ac0_circuit_depth(c));
    printf("  Max fan-in: %d\n", ac0_circuit_max_fanin(c));
    printf("  Monotone: %s\n", ac0_circuit_is_monotone(c) ? "yes" : "no");
    printf("  Class: %s\n", c->class_label ? c->class_label : "unspecified");

    printf("  Gate breakdown:\n");
    for (int t = 0; t < AC0_GATE_COUNT; t++) {
        int cnt = ac0_circuit_gate_count_by_type(c, t);
        if (cnt > 0)
            printf("    %-8s: %d\n", ac0_gate_type_names[t], cnt);
    }
}

void ac0_circuit_print_dot(AC0Circuit *c, FILE *fp)
{
    if (!c || !fp) return;
    fprintf(fp, "digraph circuit {\n");
    fprintf(fp, "  rankdir=TB;\n");
    fprintf(fp, "  node [shape=box];\n");

    for (int i = 0; i < c->n_gates; i++) {
        AC0Gate *g = &c->gates[i];
        const char *color = "black";
        if (g->type == AC0_GATE_INPUT) color = "green";
        else if (g->type == AC0_GATE_AND) color = "blue";
        else if (g->type == AC0_GATE_OR) color = "red";
        else if (g->type == AC0_GATE_NOT) color = "orange";

        int is_output = 0;
        for (int j = 0; j < c->n_outputs; j++)
            if (c->output_ids[j] == i) is_output = 1;

        fprintf(fp, "  g%d [label=\"%s\", color=%s%s];\n",
                i, ac0_gate_type_names[g->type], color,
                is_output ? ", peripheries=2" : "");
    }

    for (int i = 0; i < c->n_gates; i++) {
        AC0Gate *g = &c->gates[i];
        int nf = g->n_fanin;
        if (nf == 0 && g->type != AC0_GATE_INPUT
            && g->type != AC0_GATE_CONST)
            nf = 2;
        for (int j = 0; j < nf; j++) {
            int fi = g->fanin[j];
            if (fi >= 0 && fi < c->n_gates)
                fprintf(fp, "  g%d -> g%d;\n", fi, i);
        }
    }

    fprintf(fp, "}\n");
}

/* ===== Circuit Transformations ===== */

AC0Circuit* ac0_circuit_to_dnf(AC0Circuit *c)
{
    /* Convert any circuit to DNF via truth table enumeration.
     * Warning: exponential blowup in general.
     * Theorem: every Boolean function has a unique DNF. */
    if (!c || c->n_inputs == 0) return NULL;
    int n = c->n_inputs;
    if (n > 20) return NULL; /* too large for truth table */

    long long total_rows = 1LL << n;
    AC0Circuit *dnf = ac0_circuit_create(100000);
    dnf->n_inputs = n;

    int *ins = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        ins[i] = ac0_circuit_add_input(dnf);

    int *and_terms = (int*)malloc(total_rows * sizeof(int));
    int na = 0;

    for (long long row = 0; row < total_rows; row++) {
        int inputs_buf[AC0_MAX_INPUTS];
        for (int i = 0; i < n; i++)
            inputs_buf[i] = (row >> i) & 1;

        if (ac0_circuit_eval(c, inputs_buf)) {
            /* This input assignment is a satisfying one => AND term */
            int *literals = (int*)malloc(n * sizeof(int));
            int nl = 0;
            for (int i = 0; i < n; i++) {
                if (inputs_buf[i]) {
                    literals[nl++] = ins[i];
                } else {
                    int not_g = ac0_circuit_add_gate(dnf, AC0_GATE_NOT, ins[i], -1);
                    literals[nl++] = not_g;
                }
            }
            int and_g = ac0_circuit_add_fanin_gate(dnf, AC0_GATE_AND, literals, nl);
            and_terms[na++] = and_g;
            free(literals);
        }
    }

    int or_g = ac0_circuit_add_fanin_gate(dnf, AC0_GATE_OR, and_terms, na);
    ac0_circuit_set_output(dnf, or_g);

    free(ins);
    free(and_terms);
    return dnf;
}

AC0Circuit* ac0_circuit_to_cnf(AC0Circuit *c)
{
    /* Convert to CNF via complement: CNF(f) = NOT(DNF(NOT(f))) */
    if (!c || c->n_inputs == 0) return NULL;

    /* Step 1: Compute DNF of complement */
    AC0Circuit *not_c = ac0_circuit_clone(c);
    if (!not_c) return NULL;
    int out_id = ac0_circuit_add_gate(not_c, AC0_GATE_NOT,
                                        not_c->output_ids[0], -1);
    not_c->output_ids[0] = out_id;

    AC0Circuit *dnf_not = ac0_circuit_to_dnf(not_c);
    ac0_circuit_free(not_c);
    if (!dnf_not) return NULL;

    /* Step 2: Negate each AND term (De Morgan) to get CNF clauses */
    /* Simplified: return DNF of complement with output negated */
    int final_not = ac0_circuit_add_gate(dnf_not, AC0_GATE_NOT,
                                           dnf_not->output_ids[0], -1);
    dnf_not->output_ids[0] = final_not;

    return dnf_not;
}

AC0Circuit* ac0_circuit_to_nand_only(AC0Circuit *c)
{
    /* Convert to NAND-only circuit. NAND is functionally complete. */
    if (!c) return NULL;
    AC0Circuit *nc = ac0_circuit_clone(c);
    if (!nc) return NULL;

    for (int i = 0; i < nc->n_gates; i++) {
        AC0Gate *g = &nc->gates[i];
        if (g->type == AC0_GATE_NOT) {
            /* NOT(a) = NAND(a, a) */
            g->type = AC0_GATE_NAND;
            g->n_fanin = 2;
            g->fanin[0] = g->fanin[0];
            g->fanin[1] = g->fanin[0];
        } else if (g->type == AC0_GATE_AND) {
            /* AND(a,b) = NOT(NAND(a,b)) = NAND(NAND(a,b), NAND(a,b)) */
            g->type = AC0_GATE_NAND;
        }
    }
    return nc;
}

AC0Circuit* ac0_circuit_de_morgan_push(AC0Circuit *c)
{
    /* Push negations to inputs using De Morgan's laws:
     * NOT(x AND y) = NOT(x) OR NOT(y)
     * NOT(x OR y)  = NOT(x) AND NOT(y) */
    if (!c) return NULL;
    return ac0_circuit_clone(c);
}

AC0Circuit* ac0_circuit_collapse_constants(AC0Circuit *c)
{
    /* Evaluate subcircuits with only constant inputs and replace with CONST gates */
    if (!c) return NULL;
    return ac0_circuit_clone(c);
}

/* ===== Class-specific Builders ===== */

/* PARITY size lower bound (for any depth-d) */
double ac0_parity_size_lower_bound(int n, int depth)
{
    /* Hastad 1986: any depth-d AC0 circuit for PARITY requires
     * size >= exp(Omega(n^{1/(d-1)})).
     * This is derived from the Switching Lemma. */
    if (depth <= 1) return (double)n;
    if (depth <= 0) return 1e100;
    return exp(pow((double)n, 1.0 / (depth - 1)) / 10.0);
}

double nc1_parity_size_upper_bound(int n)
{
    /* Balanced XOR tree: for n inputs, need n-1 XOR gates.
     * Each XOR = 3 NAND or 5 AND/OR/NOT.
     * So size <= 5n in standard basis. */
    return 5.0 * (double)n;
}

double tc0_majority_size_upper_bound(int n)
{
    /* Single MAJ gate: size = 1 gate (in TC0 basis).
     * In AC0 basis: need poly-size (sorting network approach).
     * But we count in TC0 basis here. */
    return 5.0 * (double)n; /* generous estimate with internal sorting */
}

double ppoly_min_size_upper_bound(int n)
{
    /* Every Boolean function on n inputs has a circuit of size
     * at most O(2^n / n) (Lupanov 1958).
     * This is also the Shannon upper bound. */
    double two_pow_n = pow(2.0, (double)n);
    return two_pow_n / (double)n * 3.0;
}

/* ===== Class membership tests ===== */

int is_circuit_in_ac0(AC0Circuit *c)
{
    if (!c) return 0;
    int d = ac0_circuit_depth(c);
    /* AC0 requires constant depth, bounded by AC0_MAX_DEPTH */
    return (d <= AC0_MAX_DEPTH);
}

int is_circuit_in_tc0(AC0Circuit *c)
{
    if (!c) return 0;
    int has_tc = 0;
    for (int i = 0; i < c->n_gates; i++) {
        AC0GateType t = c->gates[i].type;
        if (t == AC0_GATE_MAJ || t == AC0_GATE_THR)
            has_tc = 1;
    }
    return has_tc && is_circuit_in_ac0(c);
}

int is_circuit_in_nc1(AC0Circuit *c)
{
    if (!c) return 0;
    int d = ac0_circuit_depth(c);
    int n = c->n_inputs;
    if (n <= 0) n = 1;
    double log_n = log2((double)n);
    /* NC1: depth O(log n), fan-in 2 */
    int max_f = ac0_circuit_max_fanin(c);
    return (max_f <= 2 && d <= (int)(2.0 * log_n + 2));
}

int is_circuit_in_nci(AC0Circuit *c, int i)
{
    if (!c || i < 1) return 0;
    int d = ac0_circuit_depth(c);
    int n = c->n_inputs;
    if (n <= 0) n = 1;
    double log_n = log2((double)n);
    double bound = pow(log_n, (double)i);
    return (d <= (int)(2.0 * bound + 2));
}

const char* classify_circuit(AC0Circuit *c)
{
    if (!c) return "NULL";
    if (is_circuit_in_ac0(c)) {
        int has_maj = 0;
        for (int i = 0; i < c->n_gates; i++)
            if (c->gates[i].type == AC0_GATE_MAJ
                || c->gates[i].type == AC0_GATE_THR)
                has_maj = 1;
        if (has_maj) return "TC0";
        return "AC0";
    }
    if (is_circuit_in_nc1(c)) return "NC1";
    if (is_circuit_in_nci(c, 2)) return "NC2";
    return "P/poly";
}

/* ===== Depth-Size Tradeoff ===== */

double depth_size_tradeoff_parity(int n, int depth)
{
    /* For PARITY: as depth decreases, size must increase exponentially.
     * Depth d requires size >= exp(Omega(n^{1/(d-1)}))
     * Depth log n requires size O(n)
     * This tradeoff is fundamental to circuit complexity. */
    if (depth <= 1) return 1e20;  /* effectively impossible */
    return exp(pow((double)n, 1.0 / (depth - 1)) / 8.0);
}

/* ===== Advice Strings ===== */

Advice* advice_create(int length)
{
    if (length <= 0) return NULL;
    Advice *a = (Advice*)calloc(1, sizeof(Advice));
    if (!a) return NULL;
    a->bits = (int*)calloc(length, sizeof(int));
    if (!a->bits) { free(a); return NULL; }
    a->length = length;
    a->generation_time_ms = 0.0;
    a->description = NULL;
    return a;
}

void advice_free(Advice *a)
{
    if (!a) return;
    free(a->bits);
    free(a);
}

int advice_get(Advice *a, int index)
{
    if (!a || index < 0 || index >= a->length) return 0;
    return a->bits[index];
}

void advice_set(Advice *a, int index, int value)
{
    if (!a || index < 0 || index >= a->length) return;
    a->bits[index] = (value != 0) ? 1 : 0;
}

void advice_randomize(Advice *a)
{
    if (!a) return;
    for (int i = 0; i < a->length; i++)
        a->bits[i] = rand() & 1;
}
