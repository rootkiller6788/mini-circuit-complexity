/**
 * src/circuit.c — AC0 Circuit Model Implementation
 * ===============================================
 *
 * Implements L1-L6: The AC0 circuit model — constant-depth, unbounded
 * fan-in circuits with AND, OR, NOT gates. This is the model for which
 * the switching lemma proves PARITY requires super-polynomial size.
 *
 * THEORETICAL BACKGROUND:
 *
 *   AC0 (Alternating Circuits depth 0):
 *     - Constant depth d (independent of input size n)
 *     - Unbounded fan-in AND and OR gates
 *     - NOT gates (only on inputs, by De Morgan)
 *     - Polynomial size in n
 *
 *   AC0 HIERARCHY:
 *     AC0 ⊂ AC0[2] ⊂ AC0[3] ⊂ ... ⊂ TC0 ⊂ NC1 ⊆ L ⊆ NL ⊆ ...
 *     where AC0[p] adds MOD_p gates (Razborov-Smolensky 1987)
 *
 *   KEY RESULTS:
 *     - PARITY ∉ AC0 (Furst-Saxe-Sipser 1981, Hastad 1986)
 *     - PARITY ∈ AC0[2] (trivially: XOR = MOD_2)
 *     - MAJORITY ∉ AC0[p] for any prime p (Razborov-Smolensky)
 *     - MAJORITY ∈ TC0 (threshold gates)
 *
 *   DEPTH REDUCTION VIA SWITCHING:
 *     A depth-d AC0 circuit is:
 *       output = AND_of_ORs_of_ANDs_of_... (alternating layers)
 *     Switching lemma converts AND-of-ORs → OR-of-ANDs,
 *     reducing depth by 1 with each round.
 *
 * COURSE MAPPING:
 *   - MIT 6.841: AC0 circuit model, constant-depth circuits
 *   - Stanford CS358: Circuit complexity fundamentals
 *   - Berkeley CS278: AC0 lower bounds
 *   - CMU 15-855: Circuit model and switching lemma
 *   - ETH 263-4650: Boolean circuits and lower bounds
 *
 * REFERENCES:
 *   - Vollmer (1999), "Introduction to Circuit Complexity"
 *   - Arora & Barak (2009), Chapter 6 and 14
 *   - Jukna (2012), "Boolean Function Complexity"
 */

#include "switching.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* ================================================================
 * INTERNAL: Memory management for node arrays
 * ================================================================ */

#define MAX_CIRCUIT_NODES 100000
#define MAX_FANIN        1024

static int next_node_id = 0;

static int get_node_id(void) {
    return next_node_id++;
}

/* ================================================================
 * CIRCUIT CREATION AND DESTRUCTION
 * ================================================================ */

/**
 * circuit_create — Create an empty AC0 circuit.
 *
 * @param n_vars  Number of Boolean input variables
 * @return        Newly allocated Circuit
 *
 * Complexity: O(1)
 */
Circuit* circuit_create(int n_vars) {
    if (n_vars < 1) return NULL;

    Circuit* c = (Circuit*)calloc(1, sizeof(Circuit));
    if (!c) return NULL;

    c->n_vars = n_vars;
    c->n_nodes = 0;
    c->depth = 0;
    c->output = NULL;
    c->all_nodes = (CircuitNode**)calloc(MAX_CIRCUIT_NODES, sizeof(CircuitNode*));
    if (!c->all_nodes) {
        free(c);
        return NULL;
    }
    return c;
}

/* Internal: add a node to the circuit's node list */
static int circuit_register_node(Circuit* c, CircuitNode* node) {
    if (!c || !node || c->n_nodes >= MAX_CIRCUIT_NODES) return -1;
    node->id = c->n_nodes;
    c->all_nodes[c->n_nodes] = node;
    c->n_nodes++;
    return node->id;
}

/* Internal: create a basic node */
static CircuitNode* node_create(GateType type) {
    CircuitNode* node = (CircuitNode*)calloc(1, sizeof(CircuitNode));
    if (!node) return NULL;
    node->type = type;
    node->id = get_node_id();
    node->var = -1;
    node->fanin_count = 0;
    node->fanin = NULL;
    node->depth = 0;
    return node;
}

/* ================================================================
 * ADDING GATES
 * ================================================================ */

/**
 * circuit_add_input — Add an INPUT gate (leaf node) to the circuit.
 *
 * INPUT gates represent the Boolean variables x_0, ..., x_{n_vars-1}.
 * They have no fan-in and depth 0.
 *
 * @param c    Circuit
 * @param var  Variable index (0-based)
 * @return     Pointer to new INPUT gate, or NULL on error
 *
 * Complexity: O(1)
 */
CircuitNode* circuit_add_input(Circuit* c, int var) {
    if (!c || var < 0 || var >= c->n_vars) return NULL;

    CircuitNode* node = node_create(GATE_INPUT);
    if (!node) return NULL;

    node->var = var;
    node->depth = 0;
    circuit_register_node(c, node);
    return node;
}

/**
 * circuit_add_and — Add an AND gate with specified fan-in gates.
 *
 * AND gate: outputs 1 iff ALL fan-in gates evaluate to 1.
 *
 * The depth of the new gate = 1 + max(depth of inputs).
 *
 * @param c        Circuit
 * @param inputs   Array of pointers to fan-in gates
 * @param n_inputs Number of fan-in gates
 * @return         Pointer to new AND gate
 *
 * Complexity: O(n_inputs)
 */
CircuitNode* circuit_add_and(Circuit* c, CircuitNode** inputs, int n_inputs) {
    if (!c || !inputs || n_inputs < 1 || n_inputs > MAX_FANIN) return NULL;

    CircuitNode* node = node_create(GATE_AND);
    if (!node) return NULL;

    node->fanin_count = n_inputs;
    node->fanin = (CircuitNode**)calloc((size_t)n_inputs, sizeof(CircuitNode*));
    if (!node->fanin) {
        free(node);
        return NULL;
    }

    /* Copy fan-in references and compute depth */
    int max_d = 0;
    for (int i = 0; i < n_inputs; i++) {
        node->fanin[i] = inputs[i];
        if (inputs[i] && inputs[i]->depth > max_d) {
            max_d = inputs[i]->depth;
        }
    }
    node->depth = max_d + 1;

    circuit_register_node(c, node);
    return node;
}

/**
 * circuit_add_or — Add an OR gate with specified fan-in gates.
 *
 * OR gate: outputs 1 iff ANY fan-in gate evaluates to 1.
 *
 * Complexity: O(n_inputs)
 */
CircuitNode* circuit_add_or(Circuit* c, CircuitNode** inputs, int n_inputs) {
    if (!c || !inputs || n_inputs < 1 || n_inputs > MAX_FANIN) return NULL;

    CircuitNode* node = node_create(GATE_OR);
    if (!node) return NULL;

    node->fanin_count = n_inputs;
    node->fanin = (CircuitNode**)calloc((size_t)n_inputs, sizeof(CircuitNode*));
    if (!node->fanin) {
        free(node);
        return NULL;
    }

    int max_d = 0;
    for (int i = 0; i < n_inputs; i++) {
        node->fanin[i] = inputs[i];
        if (inputs[i] && inputs[i]->depth > max_d) {
            max_d = inputs[i]->depth;
        }
    }
    node->depth = max_d + 1;

    circuit_register_node(c, node);
    return node;
}

/**
 * circuit_add_not — Add a NOT gate.
 *
 * NOT gate: outputs the negation of its single input.
 *
 * @param c      Circuit
 * @param input  Single fan-in gate
 * @return       Pointer to new NOT gate
 *
 * Complexity: O(1)
 */
CircuitNode* circuit_add_not(Circuit* c, CircuitNode* input) {
    if (!c || !input) return NULL;

    CircuitNode* node = node_create(GATE_NOT);
    if (!node) return NULL;

    node->fanin_count = 1;
    node->fanin = (CircuitNode**)calloc(1, sizeof(CircuitNode*));
    if (!node->fanin) {
        free(node);
        return NULL;
    }
    node->fanin[0] = input;
    node->depth = input->depth + 1;

    circuit_register_node(c, node);
    return node;
}

/**
 * circuit_set_output — Set the output gate of the circuit.
 *
 * The circuit's overall depth is set to the output gate's depth.
 *
 * Complexity: O(1)
 */
void circuit_set_output(Circuit* c, CircuitNode* output) {
    if (!c || !output) return;
    c->output = output;
    c->depth = output->depth;
}

/* ================================================================
 * CIRCUIT EVALUATION
 * ================================================================ */

/**
 * circuit_eval — Evaluate an AC0 circuit on a given assignment.
 *
 * Recursively evaluates gates bottom-up, using memoization by
 * storing computed values in a temporary array indexed by node id.
 *
 * @param c      Circuit
 * @param assign Assignment array of length n_vars
 * @return       0 or 1 (output value)
 *
 * Complexity: O(n_nodes) per evaluation
 */
int circuit_eval(const Circuit* c, const int* assign) {
    if (!c || !c->output || !assign) return 0;

    /* Memoization table: computed value for each node, -1 = not computed */
    int n = c->n_nodes;
    int* memo = (int*)malloc((size_t)n * sizeof(int));
    if (!memo) return 0;
    for (int i = 0; i < n; i++) memo[i] = -1;

    /* Depth-first evaluation using explicit stack */
    typedef struct { int node_id; int state; } Frame;
    /* state: 0 = not visited children, 1 = children done, computing result */
    Frame* stack = (Frame*)malloc((size_t)(n + 1) * sizeof(Frame));
    int top = 0;
    if (!stack) { free(memo); return 0; }

    stack[top].node_id = c->output->id;
    stack[top].state = 0;
    top++;

    int result = 0;

    while (top > 0) {
        top--;
        Frame frame = stack[top];
        int nid = frame.node_id;
        int state = frame.state;

        if (nid < 0 || nid >= n) continue;

        if (memo[nid] >= 0) continue;  /* already computed */

        CircuitNode* node = c->all_nodes[nid];
        if (!node) continue;

        if (state == 0) {
            /* First visit: push children */
            if (node->type == GATE_INPUT) {
                /* Leaf: compute directly */
                int var = node->var;
                memo[nid] = (var >= 0 && var < c->n_vars) ? assign[var] : 0;
            } else {
                /* Push self with state=1, then push children */
                stack[top].node_id = nid;
                stack[top].state = 1;
                top++;

                /* Push children in reverse order (so first child is on top) */
                for (int i = node->fanin_count - 1; i >= 0; i--) {
                    if (node->fanin[i]) {
                        int child_id = node->fanin[i]->id;
                        if (child_id >= 0 && child_id < n && memo[child_id] < 0) {
                            stack[top].node_id = child_id;
                            stack[top].state = 0;
                            top++;
                        }
                    }
                }
            }
        } else {
            /* Second visit: all children computed, compute result */
            int val;
            switch (node->type) {
                case GATE_AND:
                    val = 1;
                    for (int i = 0; i < node->fanin_count; i++) {
                        if (node->fanin[i]) {
                            int cid = node->fanin[i]->id;
                            int cv = (cid >= 0 && cid < n) ? memo[cid] : 0;
                            if (cv <= 0) { val = 0; break; }
                        }
                    }
                    break;

                case GATE_OR:
                    val = 0;
                    for (int i = 0; i < node->fanin_count; i++) {
                        if (node->fanin[i]) {
                            int cid = node->fanin[i]->id;
                            int cv = (cid >= 0 && cid < n) ? memo[cid] : 0;
                            if (cv > 0) { val = 1; break; }
                        }
                    }
                    break;

                case GATE_NOT:
                    if (node->fanin_count > 0 && node->fanin[0]) {
                        int cid = node->fanin[0]->id;
                        int cv = (cid >= 0 && cid < n) ? memo[cid] : 0;
                        val = (cv > 0) ? 0 : 1;
                    } else {
                        val = 0;
                    }
                    break;

                default:
                    val = 0;
                    break;
            }
            memo[nid] = val;
        }
    }

    if (c->output) {
        int out_id = c->output->id;
        result = (out_id >= 0 && out_id < n) ? memo[out_id] : 0;
    }

    free(stack);
    free(memo);
    return result;
}

/* ================================================================
 * CIRCUIT METRICS
 * ================================================================ */

/**
 * circuit_size — Return the number of gates in the circuit.
 *
 * Complexity: O(1)
 */
int circuit_size(const Circuit* c) {
    return c ? c->n_nodes : 0;
}

/**
 * circuit_depth — Return the depth of the circuit.
 *
 * Depth is the length of the longest path from an input to the output.
 * NOT gates count toward depth unless applied only to inputs (by convention).
 *
 * Complexity: O(1)
 */
int circuit_depth(const Circuit* c) {
    return c ? c->depth : 0;
}

/* ================================================================
 * BUILDING CIRCUITS FOR CANONICAL FUNCTIONS
 * ================================================================ */

/**
 * circuit_parity_ac0 — Build an AC0 circuit for PARITY on n variables.
 *
 * Since PARITY ∉ AC0, any constant-depth circuit must have
 * exponential size. This function builds a depth-n tree
 * (XOR tree), which is NOT an AC0 circuit but serves as
 * a demonstration of why AC0 cannot compute PARITY efficiently.
 *
 * For the true AC0 construction: use depth-2 DNF/CNF with 2^{n-1} terms.
 * See circuit_parity_depth2().
 *
 * Strategy: recursive XOR tree
 *   PARITY(x_0,...,x_{n-1}) = x_0 ⊕ PARITY(x_1,...,x_{n-1})
 *
 * If n is large, this uses circuit_parity_depth2() as fallback.
 *
 * Complexity: O(2^n) nodes for the recursive tree
 */
Circuit* circuit_parity_ac0(int n) {
    if (n <= 0) return NULL;
    if (n > 6) {
        /* For larger n, use the depth-2 construction (which is truly AC0
         * but exponential in size — demonstrating the lower bound) */
        return circuit_parity_depth2(n);
    }

    Circuit* c = circuit_create(n);
    if (!c) return NULL;

    /* Build recursively:
     * parity(x_0,...,x_{n-1}) = (x_{n-1} AND NOT parity(x_0,...,x_{n-2}))
     *                        OR (NOT x_{n-1} AND parity(x_0,...,x_{n-2}))
     */

    /* Base case: n=1, PARITY(x_0) = x_0 */
    if (n == 1) {
        CircuitNode* in = circuit_add_input(c, 0);
        circuit_set_output(c, in);
        return c;
    }

    /* Build sub-circuit for PARITY on n-1 variables */
    Circuit* sub = circuit_parity_ac0(n - 1);
    if (!sub) { circuit_free(c); return NULL; }

    /* We need the sub-circuit as a subgraph within c.
     * Since cross-circuit sharing is complex, build from scratch.
     * For n ≤ 6, use truth table approach. */

    circuit_free(c); /* release the empty circuit */
    circuit_free(sub);

    /* Fallback to truth table → DNF → circuit */
    int table_size = 1 << n;
    int* table = (int*)malloc((size_t)table_size * sizeof(int));
    if (!table) return NULL;
    for (int mask = 0; mask < table_size; mask++) {
        int parity = 0;
        for (int i = 0; i < n; i++) parity ^= ((mask >> i) & 1);
        table[mask] = parity;
    }

    /* Build DNF from truth table */
    int sat_count = 0;
    for (int i = 0; i < table_size; i++) if (table[i]) sat_count++;
    DNF* d = dnf_create(n, sat_count, n);
    if (!d) { free(table); return NULL; }
    int term_idx = 0;
    for (int mask = 0; mask < table_size && term_idx < sat_count; mask++) {
        if (table[mask]) {
            for (int v = 0; v < n; v++) {
                dnf_set_literal(d, term_idx, v, v,
                    ((mask >> v) & 1) ? LITERAL_POS : LITERAL_NEG);
            }
            term_idx++;
        }
    }
    free(table);

    Circuit* result = circuit_from_dnf(d);
    dnf_free(d);
    return result;
}

/**
 * circuit_parity_depth2 — Build a depth-2 circuit (DNF) for PARITY.
 *
 * PARITY DNF: 2^{n-1} terms, each of width n.
 * Each term corresponds to an assignment where parity = 1.
 *
 * This is an exponential-size AC0 circuit, demonstrating that
 * while AC0 CAN compute PARITY, it requires exponential size.
 *
 * Complexity: O(2^n) gates, size = 2^{n-1} * n inputs + 1 OR + 2^{n-1} ANDs
 *
 * For n > 10, this becomes impractically large.
 */
Circuit* circuit_parity_depth2(int n) {
    if (n <= 0) return NULL;
    if (n > 10) {
        /* Too large for practical construction.
         * The lower bound says: for n=20, need ~2^19 ≈ 500K terms.
         * We limit to n ≤ 10 for demonstrations. */
        fprintf(stderr, "circuit_parity_depth2: n=%d too large (max 10)\n", n);
        return NULL;
    }

    int table_size = 1 << n;
    int sat_count = 0;
    for (int mask = 0; mask < table_size; mask++) {
        int parity = 0;
        for (int i = 0; i < n; i++) parity ^= ((mask >> i) & 1);
        if (parity) sat_count++;
    }

    Circuit* c = circuit_create(n);
    if (!c) return NULL;

    /* Create all INPUT gates (2n for variable and its negation) */
    CircuitNode** pos_inputs = (CircuitNode**)calloc((size_t)n, sizeof(CircuitNode*));
    CircuitNode** neg_inputs = (CircuitNode**)calloc((size_t)n, sizeof(CircuitNode*));
    if (!pos_inputs || !neg_inputs) {
        free(pos_inputs); free(neg_inputs);
        circuit_free(c); return NULL;
    }

    for (int v = 0; v < n; v++) {
        pos_inputs[v] = circuit_add_input(c, v);
        /* Negated input: add NOT gate after input */
        neg_inputs[v] = circuit_add_not(c, pos_inputs[v]);
    }

    /* Create AND gates (one per satisfying term) */
    CircuitNode** and_gates = (CircuitNode**)calloc((size_t)sat_count, sizeof(CircuitNode*));
    if (!and_gates) {
        free(pos_inputs); free(neg_inputs);
        circuit_free(c); return NULL;
    }

    int and_idx = 0;
    for (int mask = 0; mask < table_size; mask++) {
        int parity = 0;
        for (int i = 0; i < n; i++) parity ^= ((mask >> i) & 1);
        if (!parity) continue;

        /* This assignment makes parity=1.
         * Create AND term: for each variable v,
         * if mask[v]=1, use x_v; if mask[v]=0, use ~x_v */
        CircuitNode** term_inputs = (CircuitNode**)calloc((size_t)n, sizeof(CircuitNode*));
        if (!term_inputs) continue;

        for (int v = 0; v < n; v++) {
            if ((mask >> v) & 1) {
                term_inputs[v] = pos_inputs[v];
            } else {
                term_inputs[v] = neg_inputs[v];
            }
        }
        and_gates[and_idx] = circuit_add_and(c, term_inputs, n);
        free(term_inputs);
        and_idx++;
    }

    /* OR of all AND terms = output */
    CircuitNode* or_gate = circuit_add_or(c, and_gates, sat_count);
    circuit_set_output(c, or_gate);

    free(and_gates);
    free(pos_inputs);
    free(neg_inputs);
    return c;
}

/**
 * circuit_from_dnf — Build an AC0 circuit from a DNF formula.
 *
 * DNF = OR of ANDs:
 *   One AND gate per term (fan-in = term width)
 *   One OR gate over all AND gates (fan-in = n_terms)
 *
 * Depth = 2 (AND level, then OR level, plus INPUT depth 0)
 * Size = n_inputs + n_NOT + n_terms (AND) + 1 (OR)
 *
 * Complexity: O(n_terms * k_alloc)
 */
Circuit* circuit_from_dnf(const DNF* d) {
    if (!d) return NULL;

    int n = d->n_vars;
    int m = d->n_terms;
    int k = d->k_alloc;

    Circuit* c = circuit_create(n);
    if (!c) return NULL;

    /* Create INPUT gates (one per variable) */
    CircuitNode** inputs = (CircuitNode**)calloc((size_t)n, sizeof(CircuitNode*));
    if (!inputs) { circuit_free(c); return NULL; }
    for (int v = 0; v < n; v++) {
        inputs[v] = circuit_add_input(c, v);
    }

    /* Create NOT gates for negated inputs (reuse: one per variable) */
    CircuitNode** not_inputs = (CircuitNode**)calloc((size_t)n, sizeof(CircuitNode*));
    if (!not_inputs) { free(inputs); circuit_free(c); return NULL; }
    for (int v = 0; v < n; v++) {
        not_inputs[v] = circuit_add_not(c, inputs[v]);
    }

    /* Create AND gate for each term */
    CircuitNode** and_gates = (CircuitNode**)calloc((size_t)m, sizeof(CircuitNode*));
    if (!and_gates) {
        free(inputs); free(not_inputs);
        circuit_free(c); return NULL;
    }

    for (int t = 0; t < m; t++) {
        /* Count non-null literals in this term */
        int lit_count = 0;
        for (int l = 0; l < k; l++) {
            if (d->terms[t * k + l] != LITERAL_NULL) lit_count++;
        }

        if (lit_count == 0) {
            /* Empty term = always true → create trivial AND */
            and_gates[t] = circuit_add_and(c, &inputs[0], 1);
            continue;
        }

        CircuitNode** term_fanin = (CircuitNode**)calloc((size_t)lit_count, sizeof(CircuitNode*));
        if (!term_fanin) continue;

        int fi = 0;
        for (int l = 0; l < k; l++) {
            int lit = d->terms[t * k + l];
            if (lit == LITERAL_NULL) continue;

            int var  = LITERAL_VAR(lit);
            int sign = LITERAL_SIGN(lit);
            if (var < 0 || var >= n) continue;

            term_fanin[fi] = (sign == LITERAL_POS) ? inputs[var] : not_inputs[var];
            fi++;
        }
        and_gates[t] = circuit_add_and(c, term_fanin, lit_count);
        free(term_fanin);
    }

    /* OR gate combining all AND terms = output */
    CircuitNode* or_gate = circuit_add_or(c, and_gates, m);
    circuit_set_output(c, or_gate);

    free(and_gates);
    free(not_inputs);
    free(inputs);

    /* Set circuit depth properly */
    if (c->output) c->depth = c->output->depth;

    return c;
}

/**
 * circuit_from_dt — Build an AC0 circuit from a decision tree.
 *
 * Each leaf path becomes an AND term (conjunction of variable assignments).
 * The OR of all paths leading to leaf=1 gives the function.
 *
 * Depth = tree_depth (each path through the tree is an AND chain)
 *
 * Complexity: O(tree_size * tree_depth)
 */
Circuit* circuit_from_dt(const DTNode* tree) {
    if (!tree) return NULL;

    /* For simplicity, extract truth table then DNF then circuit.
     * A decision tree can represent up to 2^D leaves, where D = depth. */
    int tree_d = dt_depth(tree);
    if (tree_d > 15) return NULL;  /* too deep */

    int n_vars_found = 0;
    /* Find max variable index in the tree */
    typedef struct { const DTNode* n; } SI;
    SI stack[512];
    int top = 0;
    stack[top].n = tree; top++;
    while (top > 0) {
        top--;
        const DTNode* node = stack[top].n;
        if (!node) continue;
        if (node->var >= 0) {
            if (node->var + 1 > n_vars_found) n_vars_found = node->var + 1;
            if (node->child0) { stack[top].n = node->child0; top++; }
            if (node->child1) { stack[top].n = node->child1; top++; }
        }
    }
    if (n_vars_found == 0) n_vars_found = 1;

    /* Build truth table */
    int table_size = 1 << n_vars_found;
    int* table = (int*)malloc((size_t)table_size * sizeof(int));
    if (!table) return NULL;

    int* assign = (int*)calloc((size_t)n_vars_found, sizeof(int));
    if (!assign) { free(table); return NULL; }

    for (int mask = 0; mask < table_size; mask++) {
        for (int v = 0; v < n_vars_found; v++) {
            assign[v] = (mask >> v) & 1;
        }
        table[mask] = dt_eval(tree, assign);
    }
    free(assign);

    /* Convert to DNF via BoolFunc */
    BoolFunc bf;
    bf.n_vars = n_vars_found;
    bf.table = table;
    bf.table_size = 1 << n_vars_found;
    bf.name = "dt_func";
    DNF* d = bf_to_dnf(&bf);
    free(table);
    if (!d) return NULL;

    Circuit* result = circuit_from_dnf(d);
    dnf_free(d);
    return result;
}

/* ================================================================
 * CIRCUIT VERIFICATION
 * ================================================================ */

/**
 * circuit_implements — Verify that the circuit computes a given Boolean function.
 *
 * Checks all 2^n_vars assignments exhaustively.
 *
 * @param c  Circuit
 * @param bf Boolean function
 * @return   1 if the circuit implements the function exactly, 0 otherwise
 *
 * Complexity: O(2^n_vars * circuit_size)
 */
int circuit_implements(const Circuit* c, const BoolFunc* bf) {
    if (!c || !bf) return 0;
    if (c->n_vars != bf->n_vars) return 0;

    int n = c->n_vars;
    int table_size = 1 << n;
    int* assign = (int*)calloc((size_t)n, sizeof(int));
    if (!assign) return 0;

    int ok = 1;
    for (int mask = 0; mask < table_size && ok; mask++) {
        for (int v = 0; v < n; v++) {
            assign[v] = (mask >> v) & 1;
        }
        int circuit_val = circuit_eval(c, assign);
        if (circuit_val != bf->table[mask]) {
            ok = 0;
        }
    }
    free(assign);
    return ok;
}

/* ================================================================
 * CIRCUIT DISPLAY
 * ================================================================ */

/**
 * circuit_print — Print circuit structure in human-readable form.
 */
void circuit_print(const Circuit* c) {
    if (!c) { printf("Circuit(NULL)\n"); return; }

    printf("AC0 Circuit:\n");
    printf("  n_vars = %d, n_nodes = %d, depth = %d\n",
           c->n_vars, c->n_nodes, c->depth);

    if (c->output) {
        printf("  Output gate: id=%d, type=", c->output->id);
        switch (c->output->type) {
            case GATE_AND:   printf("AND\n"); break;
            case GATE_OR:    printf("OR\n");  break;
            case GATE_NOT:   printf("NOT\n"); break;
            case GATE_INPUT: printf("INPUT\n"); break;
            default:         printf("?\n");   break;
        }
    } else {
        printf("  Output: (not set)\n");
    }

    /* Print gate statistics */
    int count_input = 0, count_and = 0, count_or = 0, count_not = 0;
    for (int i = 0; i < c->n_nodes; i++) {
        if (!c->all_nodes[i]) continue;
        switch (c->all_nodes[i]->type) {
            case GATE_INPUT: count_input++; break;
            case GATE_AND:   count_and++;   break;
            case GATE_OR:    count_or++;    break;
            case GATE_NOT:   count_not++;   break;
            default: break;
        }
    }
    printf("  Gates: %d INPUT, %d AND, %d OR, %d NOT\n",
           count_input, count_and, count_or, count_not);

    /* Print first few gates for small circuits */
    if (c->n_nodes <= 30) {
        printf("  Gate details:\n");
        for (int i = 0; i < c->n_nodes; i++) {
            CircuitNode* node = c->all_nodes[i];
            if (!node) continue;
            printf("    [%d] ", node->id);
            switch (node->type) {
                case GATE_INPUT: printf("INPUT(x%d)", node->var); break;
                case GATE_AND:   printf("AND(fan_in=%d)", node->fanin_count); break;
                case GATE_OR:    printf("OR(fan_in=%d)", node->fanin_count); break;
                case GATE_NOT:   printf("NOT"); break;
                default:         printf("?"); break;
            }
            printf(" depth=%d\n", node->depth);
        }
    } else {
        printf("  (%d total gates, details omitted)\n", c->n_nodes);
    }
}

/* ================================================================
 * CIRCUIT DESTRUCTION
 * ================================================================ */

/**
 * circuit_free — Free all memory used by a circuit.
 *
 * Complexity: O(n_nodes)
 */
void circuit_free(Circuit* c) {
    if (!c) return;

    /* Free individual nodes */
    for (int i = 0; i < c->n_nodes; i++) {
        if (c->all_nodes[i]) {
            if (c->all_nodes[i]->fanin) {
                free(c->all_nodes[i]->fanin);
            }
            free(c->all_nodes[i]);
        }
    }

    /* Free node array */
    free(c->all_nodes);

    /* Free circuit struct */
    free(c);
}
