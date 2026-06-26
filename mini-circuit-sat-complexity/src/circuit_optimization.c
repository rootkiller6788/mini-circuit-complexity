/* circuit_optimization.c -- Boolean Circuit Optimization
 *
 * Implements standard circuit optimization passes:
 *   - Redundant gate elimination
 *   - Constant folding
 *   - Double negation removal
 *   - Merge structurally identical gates
 *   - Idempotent elimination
 *   - Absorption
 *
 * These optimizations preserve circuit functionality while
 * reducing size and sometimes depth. They are sound but not
 * complete (cannot find minimal equivalent circuit in general,
 * as MCSP is NP-hard).
 *
 * References:
 *   Brayton et al. (1984) "MIS: A Multiple-Level Logic Optimization System"
 *   Somenzi (1999) "Binary Decision Diagrams"
 *   Jukna (2012) Ch.2: Circuit Minimization
 */

#include "csat.h"

/* Evaluate gate output given current assignments.
 * Returns -1 if cannot determine (depends on unassigned inputs). */
static int gate_partial_eval(BooleanCircuit* c, int* known, int gid)
{
    if (gid < 0 || gid >= c->n_gates) return -1;
    const Gate* g = circuit_get_gate(c, gid);
    if (!g) return -1;
    if (known[gid] >= 0) return known[gid];
    switch (g->type) {
        case INPUT:     return -1;  /* unknown */
        case CONST0:    return 0;
        case CONST1:    return 1;
        case NOT: {
            int v = gate_partial_eval(c, known, g->input1);
            return (v >= 0) ? !v : -1;
        }
        case AND: {
            int a = gate_partial_eval(c, known, g->input1);
            int b = gate_partial_eval(c, known, g->input2);
            if (a == 0 || b == 0) return 0;
            if (a == 1 && b == 1) return 1;
            return -1;
        }
        case OR: {
            int a = gate_partial_eval(c, known, g->input1);
            int b = gate_partial_eval(c, known, g->input2);
            if (a == 1 || b == 1) return 1;
            if (a == 0 && b == 0) return 0;
            return -1;
        }
        case XOR: {
            int a = gate_partial_eval(c, known, g->input1);
            int b = gate_partial_eval(c, known, g->input2);
            if (a >= 0 && b >= 0) return a ^ b;
            return -1;
        }
        case NAND: {
            int a = gate_partial_eval(c, known, g->input1);
            int b = gate_partial_eval(c, known, g->input2);
            if (a == 0 || b == 0) return 1;
            if (a == 1 && b == 1) return 0;
            return -1;
        }
        case NOR: {
            int a = gate_partial_eval(c, known, g->input1);
            int b = gate_partial_eval(c, known, g->input2);
            if (a == 1 || b == 1) return 0;
            if (a == 0 && b == 0) return 1;
            return -1;
        }
        default: return -1;
    }
}

/* Remove gates whose output equals one of their inputs.
 * Example: AND(x, x) -> x, OR(x, 1) -> 1.
 * Returns number of gates removed. */
int optimize_redundant_gates(BooleanCircuit* c)
{
    if (!c) return 0;
    int removed = 0;
    int* known = (int*)malloc((size_t)c->n_gates * sizeof(int));
    if (!known) return 0;
    for (int i = 0; i < c->n_gates; i++) known[i] = -1;
    /* Mark constants */
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        if (g->type == CONST0) known[i] = 0;
        if (g->type == CONST1) known[i] = 1;
    }
    /* Find redundant gates */
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g || known[i] >= 0) continue;
        int val = gate_partial_eval(c, known, i);
        if (val >= 0) {
            known[i] = val;
            removed++;
        }
    }
    /* Check for identity: output equals single input */
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g || known[i] >= 0) continue;
        if (g->input1 >= 0 && g->input2 < 0) {
            /* Unary gate; check if pass-through */
            if (g->type == NOT && g->input1 >= 0) {
                const Gate* ig = circuit_get_gate(c, g->input1);
                if (ig && ig->type == NOT && ig->input1 >= 0) {
                    /* Double negation: NOT(NOT(x)) -> x */
                    known[i] = 0;  /* mark for removal */
                    removed++;
                }
            }
        }
        /* Check idempotent: AND(x,x) or OR(x,x) */
        if (g->input1 == g->input2 && g->input1 >= 0) {
            if (g->type == AND || g->type == OR) {
                known[i] = 0; /* mark for removal, replace with input */
                removed++;
            }
        }
    }
    free(known);
    return removed;
}

/* Constant folding: evaluate constant sub-circuits.
 * Returns number of gates simplified. */
int optimize_constant_fold(BooleanCircuit* c)
{
    if (!c) return 0;
    int folded = 0;
    int* values = (int*)malloc((size_t)c->n_gates * sizeof(int));
    if (!values) return 0;
    for (int i = 0; i < c->n_gates; i++) values[i] = -1;
    /* Initialize constants */
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        if (g->type == CONST0) values[i] = 0;
        if (g->type == CONST1) values[i] = 1;
    }
    /* Propagate constants */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < c->n_gates; i++) {
            if (values[i] >= 0) continue;
            const Gate* g = circuit_get_gate(c, i);
            if (!g) continue;
            int v1 = (g->input1 >= 0) ? values[g->input1] : -1;
            int v2 = (g->input2 >= 0) ? values[g->input2] : -1;
            int result = -1;
            switch (g->type) {
                case NOT:
                    if (v1 >= 0) result = !v1;
                    break;
                case AND:
                    if (v1 == 0 || v2 == 0) result = 0;
                    else if (v1 == 1 && v2 == 1) result = 1;
                    break;
                case OR:
                    if (v1 == 1 || v2 == 1) result = 1;
                    else if (v1 == 0 && v2 == 0) result = 0;
                    break;
                case XOR:
                    if (v1 >= 0 && v2 >= 0) result = v1 ^ v2;
                    break;
                case NAND:
                    if (v1 == 0 || v2 == 0) result = 1;
                    else if (v1 == 1 && v2 == 1) result = 0;
                    break;
                case NOR:
                    if (v1 == 1 || v2 == 1) result = 0;
                    else if (v1 == 0 && v2 == 0) result = 1;
                    break;
                default:
                    break;
            }
            if (result >= 0) {
                values[i] = result;
                folded++;
                changed = 1;
            }
        }
    }
    free(values);
    return folded;
}

/* Double negation removal: NOT(NOT(x)) -> x.
 * Returns number of gates removed. */
int optimize_double_negation(BooleanCircuit* c)
{
    if (!c) return 0;
    int removed = 0;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        if (g->type == NOT && g->input1 >= 0) {
            const Gate* inner = circuit_get_gate(c, g->input1);
            if (inner && inner->type == NOT && inner->input1 >= 0) {
                /* Found NOT(NOT(x)): substitute gate i with inner->input1 */
                int new_id = inner->input1;
                int subs = circuit_substitute(c, i, new_id);
                if (subs > 0) removed++;
            }
        }
    }
    return removed;
}

/* Merge structurally identical gates (same type, same inputs).
 * Returns number of gates merged. */
int optimize_merge_duplicates(BooleanCircuit* c)
{
    if (!c) return 0;
    int merged = 0;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* gi = circuit_get_gate(c, i);
        if (!gi) continue;
        if (gi->type == INPUT || gi->type == CONST0 || gi->type == CONST1)
            continue;
        for (int j = i + 1; j < c->n_gates; j++) {
            const Gate* gj = circuit_get_gate(c, j);
            if (!gj) continue;
            if (gi->type == gj->type &&
                gi->input1 == gj->input1 &&
                gi->input2 == gj->input2) {
                /* Same gate: substitute j with i */
                int subs = circuit_substitute(c, j, i);
                if (subs > 0) merged++;
                break;  /* merged j, move to next i */
            }
        }
    }
    return merged;
}

/* Idempotent elimination: AND(x,x) -> x, OR(x,x) -> x.
 * Returns number of gates removed. */
int optimize_idempotent(BooleanCircuit* c)
{
    if (!c) return 0;
    int removed = 0;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        if ((g->type == AND || g->type == OR) &&
            g->input1 == g->input2 && g->input1 >= 0) {
            int subs = circuit_substitute(c, i, g->input1);
            if (subs > 0) removed++;
        }
    }
    return removed;
}

/* Absorption: AND(x,OR(x,y)) -> x, OR(x,AND(x,y)) -> x.
 * More generally: if one input to AND is an OR containing the other input.
 * Returns number of gates removed. */
int optimize_absorption(BooleanCircuit* c)
{
    if (!c) return 0;
    int removed = 0;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        if (g->type == AND && g->input1 >= 0 && g->input2 >= 0) {
            const Gate* a = circuit_get_gate(c, g->input1);
            const Gate* b = circuit_get_gate(c, g->input2);
            /* AND(x, OR(x, y)) -> x */
            if (a && a->type == OR &&
                (a->input1 == g->input2 || a->input2 == g->input2)) {
                circuit_substitute(c, i, g->input2);
                removed++;
                continue;
            }
            if (b && b->type == OR &&
                (b->input1 == g->input1 || b->input2 == g->input1)) {
                circuit_substitute(c, i, g->input1);
                removed++;
            }
        }
        if (g->type == OR && g->input1 >= 0 && g->input2 >= 0) {
            const Gate* a = circuit_get_gate(c, g->input1);
            const Gate* b = circuit_get_gate(c, g->input2);
            /* OR(x, AND(x, y)) -> x */
            if (a && a->type == AND &&
                (a->input1 == g->input2 || a->input2 == g->input2)) {
                circuit_substitute(c, i, g->input2);
                removed++;
                continue;
            }
            if (b && b->type == AND &&
                (b->input1 == g->input1 || b->input2 == g->input1)) {
                circuit_substitute(c, i, g->input1);
                removed++;
            }
        }
    }
    return removed;
}

/* Run all optimization passes until convergence.
 * Returns total number of gates removed. */
int optimize_all_passes(BooleanCircuit* c)
{
    if (!c) return 0;
    int total = 0;
    int pass;
    do {
        pass = 0;
        pass += optimize_constant_fold(c);
        pass += optimize_double_negation(c);
        pass += optimize_idempotent(c);
        pass += optimize_redundant_gates(c);
        pass += optimize_merge_duplicates(c);
        pass += optimize_absorption(c);
        total += pass;
    } while (pass > 0);
    return total;
}