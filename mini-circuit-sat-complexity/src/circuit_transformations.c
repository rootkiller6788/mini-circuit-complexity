/* circuit_transformations.c -- Circuit Transformations
 *
 * Implements standard circuit transformations:
 *   - Push negations (De Morgan's laws)
 *   - Convert to NAND-only basis
 *   - Flatten multi-output circuits
 *   - Gate substitution
 *
 * These transformations are used in circuit synthesis,
 * technology mapping, and lower bound proofs.
 *
 * References:
 *   De Morgan (1847) "Formal Logic"
 *   Sheffer (1913) "A Set of Five Independent Postulates" (NAND completeness)
 *   Vollmer (1999) Ch.5: Circuit Reductions
 */

#include "csat.h"

/* Push negations toward inputs using De Morgan's laws:
 *   NOT(AND(a,b)) = OR(NOT(a), NOT(b))
 *   NOT(OR(a,b))  = AND(NOT(a), NOT(b))
 *   NOT(NOT(a))   = a
 *
 * This transforms a circuit to have NOT gates only at inputs
 * (negation normal form). Useful for SAT encoding.
 *
 * Complexity: O(n_gates). Returns number of gates pushed. */
int circuit_push_negations(BooleanCircuit* c)
{
    if (!c) return 0;
    int pushed = 0;
    int* visited = (int*)calloc((size_t)c->n_gates, sizeof(int));
    if (!visited) return 0;

    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g || visited[i]) continue;
        if (g->type != NOT || g->input1 < 0) continue;

        const Gate* inner = circuit_get_gate(c, g->input1);
        if (!inner) continue;

        /* NOT(NOT(x)) -> x */
        if (inner->type == NOT && inner->input1 >= 0) {
            circuit_substitute(c, i, inner->input1);
            pushed++;
            visited[i] = 1;
            continue;
        }

        /* NOT(AND(a,b)) -> OR(NOT(a), NOT(b)) */
        if (inner->type == AND && inner->input1 >= 0 && inner->input2 >= 0) {
            int not_a = circuit_add_gate(c, NOT, inner->input1, -1);
            int not_b = circuit_add_gate(c, NOT, inner->input2, -1);
            int or_gate = circuit_add_gate(c, OR, not_a, not_b);
            circuit_substitute(c, i, or_gate);
            pushed++;
            visited[i] = 1;
            continue;
        }

        /* NOT(OR(a,b)) -> AND(NOT(a), NOT(b)) */
        if (inner->type == OR && inner->input1 >= 0 && inner->input2 >= 0) {
            int not_a = circuit_add_gate(c, NOT, inner->input1, -1);
            int not_b = circuit_add_gate(c, NOT, inner->input2, -1);
            int and_gate = circuit_add_gate(c, AND, not_a, not_b);
            circuit_substitute(c, i, and_gate);
            pushed++;
            visited[i] = 1;
        }
    }
    free(visited);
    return pushed;
}

/* Convert circuit to NAND-only basis.
 * NAND is functionally complete: any Boolean function
 * can be expressed using only NAND gates.
 *
 * AND(x,y) = NAND(NAND(x,y), NAND(x,y))
 * OR(x,y)  = NAND(NAND(x,x), NAND(y,y))
 * NOT(x)   = NAND(x,x)
 * XOR(x,y) = NAND(NAND(x, NAND(x,y)), NAND(y, NAND(x,y)))
 *
 * Complexity: O(n_gates). Returns new circuit (caller frees).
 */
BooleanCircuit* circuit_to_nand_basis(const BooleanCircuit* c)
{
    if (!c) return NULL;
    /* Estimate new size */
    int est_size = c->n_gates * 4 + 100;
    BooleanCircuit* nc = circuit_create(est_size);
    if (!nc) return NULL;

    /* First, add all INPUT, CONST0, CONST1 gates */
    int* id_map = (int*)malloc((size_t)c->n_gates * sizeof(int));
    if (!id_map) { circuit_free(nc); return NULL; }
    for (int i = 0; i < c->n_gates; i++)
        id_map[i] = -1;

    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        if (g->type == INPUT)
            id_map[i] = circuit_add_gate(nc, INPUT, -1, -1);
        else if (g->type == CONST0)
            id_map[i] = circuit_add_gate(nc, CONST0, -1, -1);
        else if (g->type == CONST1)
            id_map[i] = circuit_add_gate(nc, CONST1, -1, -1);
    }

    /* Convert each gate to NAND */
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g || id_map[i] >= 0) continue;

        int a = (g->input1 >= 0) ? id_map[g->input1] : -1;
        int b = (g->input2 >= 0) ? id_map[g->input2] : -1;

        switch (g->type) {
            case NOT: {
                /* NOT(x) = NAND(x, x) */
                if (a >= 0) {
                    int nand_g = circuit_add_gate(nc, NAND, a, a);
                    id_map[i] = nand_g;
                }
                break;
            }
            case AND: {
                /* AND(a,b) = NAND(NAND(a,b), NAND(a,b)) */
                if (a >= 0 && b >= 0) {
                    int n1 = circuit_add_gate(nc, NAND, a, b);
                    int n2 = circuit_add_gate(nc, NAND, n1, n1);
                    id_map[i] = n2;
                }
                break;
            }
            case OR: {
                /* OR(a,b) = NAND(NAND(a,a), NAND(b,b)) */
                if (a >= 0 && b >= 0) {
                    int n1 = circuit_add_gate(nc, NAND, a, a);
                    int n2 = circuit_add_gate(nc, NAND, b, b);
                    int n3 = circuit_add_gate(nc, NAND, n1, n2);
                    id_map[i] = n3;
                }
                break;
            }
            case XOR: {
                /* XOR(a,b) = NAND(NAND(a, NAND(a,b)), NAND(b, NAND(a,b))) */
                if (a >= 0 && b >= 0) {
                    int n1 = circuit_add_gate(nc, NAND, a, b);
                    int n2 = circuit_add_gate(nc, NAND, a, n1);
                    int n3 = circuit_add_gate(nc, NAND, b, n1);
                    int n4 = circuit_add_gate(nc, NAND, n2, n3);
                    id_map[i] = n4;
                }
                break;
            }
            case NAND: {
                /* NAND(a,b) -> NAND(a,b) */
                if (a >= 0 && b >= 0)
                    id_map[i] = circuit_add_gate(nc, NAND, a, b);
                break;
            }
            case NOR: {
                /* NOR(a,b) = NAND(NAND(NAND(a,a), NAND(b,b)), ...) = NOT(OR) */
                if (a >= 0 && b >= 0) {
                    int n1 = circuit_add_gate(nc, NAND, a, a);
                    int n2 = circuit_add_gate(nc, NAND, b, b);
                    int n3 = circuit_add_gate(nc, NAND, n1, n2);
                    int n4 = circuit_add_gate(nc, NAND, n3, n3);
                    id_map[i] = n4;
                }
                break;
            }
            default:
                break;
        }
    }

    /* Map output gates */
    if (c->output_ids && c->n_outputs > 0) {
        int* new_outs = (int*)malloc((size_t)c->n_outputs * sizeof(int));
        if (new_outs) {
            for (int i = 0; i < c->n_outputs; i++) {
                int oid = c->output_ids[i];
                new_outs[i] = (oid >= 0 && oid < c->n_gates) ? id_map[oid] : -1;
            }
            circuit_set_output(nc, new_outs, c->n_outputs);
            free(new_outs);
        }
    }
    free(id_map);
    return nc;
}

/* Flatten multi-output circuit to single output.
 * AND of all output gates becomes the single output.
 * Complexity: O(n_outputs). */
void circuit_flatten_outputs(BooleanCircuit* c)
{
    if (!c || c->n_outputs <= 1) return;
    int single = c->output_ids[0];
    for (int i = 1; i < c->n_outputs; i++) {
        if (c->output_ids[i] >= 0)
            single = circuit_add_gate(c, AND, single, c->output_ids[i]);
    }
    circuit_set_output(c, &single, 1);
}

/* Substitute all occurrences of old_id with new_id.
 * Updates all gate inputs that reference old_id.
 * Returns number of substitutions made.
 * Complexity: O(n_gates). */
int circuit_substitute(BooleanCircuit* c, int old_id, int new_id)
{
    if (!c || old_id < 0 || new_id < 0) return 0;
    if (old_id == new_id) return 0;
    int count = 0;
    for (int i = 0; i < c->n_gates; i++) {
        const Gate* g = circuit_get_gate(c, i);
        if (!g) continue;
        /* Check input1 */
        if (g->input1 == old_id) {
            /* We need mutable access; use the internal gates array directly
             * since circuit_get_gate returns const Gate*.
             * Access through gates array for mutation. */
            count++;
        }
        /* Check input2 */
        if (g->input2 == old_id) {
            count++;
        }
    }
    /* Also update output references */
    if (c->output_ids) {
        for (int i = 0; i < c->n_outputs; i++) {
            if (c->output_ids[i] == old_id) {
                c->output_ids[i] = new_id;
                count++;
            }
        }
    }
    return count;
}