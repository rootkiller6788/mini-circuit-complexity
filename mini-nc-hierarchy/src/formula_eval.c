/* formula_eval.c -- Boolean Formula Evaluation: NC1-complete (Buss 1987)
 *
 * A Boolean formula is a tree where:
 * - Leaves: variables x_i or constants 0/1
 * - Internal nodes: AND, OR, NOT
 *
 * Formula Evaluation (BFE): given formula and assignment, compute output.
 * Buss (1987): BFE is NC1-complete under ALOGTIME reductions.
 *
 * The NC1 algorithm: tree contraction (rake-and-compress).
 * Each round removes half the nodes in parallel. Depth = O(log n).
 *
 * Uses FormulaNode from nc.h for type definitions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "nc.h"

/* ---- Constructors (L1: Definitions) ---- */

FormulaNode *formula_leaf(int var_idx) {
    FormulaNode *f = (FormulaNode *)malloc(sizeof(FormulaNode));
    assert(f);
    f->type = FORM_INPUT; f->var_index = var_idx; f->const_value = 0;
    f->left = f->right = f->parent = NULL;
    f->value = -1; f->evaluated = 0; f->node_id = -1;
    f->contract_round = 0;
    return f;
}

FormulaNode *formula_const(int val) {
    FormulaNode *f = (FormulaNode *)malloc(sizeof(FormulaNode));
    assert(f);
    f->type = val ? FORM_CONST_1 : FORM_CONST_0;
    f->var_index = -1; f->const_value = val;
    f->left = f->right = f->parent = NULL;
    f->value = val; f->evaluated = 1; f->node_id = -1;
    f->contract_round = 0;
    return f;
}

FormulaNode *formula_and(FormulaNode *l, FormulaNode *r) {
    FormulaNode *f = (FormulaNode *)malloc(sizeof(FormulaNode));
    assert(f);
    f->type = FORM_AND; f->var_index = -1; f->const_value = 0;
    f->left = l; f->right = r; f->parent = NULL;
    f->value = -1; f->evaluated = 0; f->node_id = -1;
    f->contract_round = 0;
    if (l) l->parent = f;
    if (r) r->parent = f;
    return f;
}

FormulaNode *formula_or(FormulaNode *l, FormulaNode *r) {
    FormulaNode *f = (FormulaNode *)malloc(sizeof(FormulaNode));
    assert(f);
    f->type = FORM_OR; f->var_index = -1; f->const_value = 0;
    f->left = l; f->right = r; f->parent = NULL;
    f->value = -1; f->evaluated = 0; f->node_id = -1;
    f->contract_round = 0;
    if (l) l->parent = f;
    if (r) r->parent = f;
    return f;
}

FormulaNode *formula_not(FormulaNode *c) {
    FormulaNode *f = (FormulaNode *)malloc(sizeof(FormulaNode));
    assert(f);
    f->type = FORM_NOT; f->var_index = -1; f->const_value = 0;
    f->left = c; f->right = NULL; f->parent = NULL;
    f->value = -1; f->evaluated = 0; f->node_id = -1;
    f->contract_round = 0;
    if (c) c->parent = f;
    return f;
}

FormulaNode *formula_implies(FormulaNode *a, FormulaNode *b) {
    /* a → b  =  (NOT a) OR b */
    return formula_or(formula_not(a), b);
}

FormulaNode *formula_xor(FormulaNode *a, FormulaNode *b) {
    /* a XOR b = (a AND NOT b) OR (NOT a AND b) */
    return formula_or(formula_and(a, formula_not(formula_copy(b))),
                      formula_and(formula_not(formula_copy(a)), b));
}

FormulaNode *formula_equiv(FormulaNode *a, FormulaNode *b) {
    /* a ↔ b = (a AND b) OR (NOT a AND NOT b) */
    return formula_or(formula_and(formula_copy(a), formula_copy(b)),
                      formula_and(formula_not(a), formula_not(b)));
}

FormulaNode *formula_majority3(FormulaNode *a, FormulaNode *b, FormulaNode *c) {
    /* MAJ(a,b,c) = (a AND b) OR (b AND c) OR (a AND c) */
    return formula_or(formula_or(formula_and(formula_copy(a), formula_copy(b)),
                                  formula_and(formula_copy(b), formula_copy(c))),
                       formula_and(a, c));
}

void formula_free(FormulaNode *f) {
    if (!f) return;
    formula_free(f->left);
    formula_free(f->right);
    free(f);
}

FormulaNode *formula_copy(const FormulaNode *f) {
    if (!f) return NULL;
    FormulaNode *cp = (FormulaNode *)malloc(sizeof(FormulaNode));
    assert(cp);
    memcpy(cp, f, sizeof(FormulaNode));
    cp->left = formula_copy(f->left);
    cp->right = formula_copy(f->right);
    cp->parent = NULL;
    if (cp->left) cp->left->parent = cp;
    if (cp->right) cp->right->parent = cp;
    return cp;
}

/* ---- Evaluation ---- */

int formula_size(const FormulaNode *f) {
    if (!f) return 0;
    return 1 + formula_size(f->left) + formula_size(f->right);
}

int formula_depth(const FormulaNode *f) {
    if (!f) return 0;
    if (f->type == FORM_INPUT || f->type == FORM_CONST_0
        || f->type == FORM_CONST_1) return 0;
    int dl = formula_depth(f->left), dr = formula_depth(f->right);
    return 1 + (dl > dr ? dl : dr);
}

int formula_num_vars(const FormulaNode *f) {
    if (!f) return 0;
    if (f->type == FORM_INPUT)
        return (f->var_index + 1 > formula_num_vars(f->left) + formula_num_vars(f->right))
               ? f->var_index + 1
               : formula_num_vars(f->left) + formula_num_vars(f->right);
    int l = formula_num_vars(f->left);
    int r = formula_num_vars(f->right);
    return l > r ? l : r;
}

int formula_eval_recursive(const FormulaNode *f, const int *assign) {
    if (!f) return 0;
    if (f->evaluated) return f->value;
    int r = 0;
    switch (f->type) {
        case FORM_INPUT:    r = assign[f->var_index]; break;
        case FORM_CONST_0:  r = 0; break;
        case FORM_CONST_1:  r = 1; break;
        case FORM_NOT:      r = !formula_eval_recursive(f->left, assign); break;
        case FORM_AND:      r = formula_eval_recursive(f->left, assign)
                              && formula_eval_recursive(f->right, assign); break;
        case FORM_OR:       r = formula_eval_recursive(f->left, assign)
                              || formula_eval_recursive(f->right, assign); break;
    }
    ((FormulaNode *)f)->value = r;
    ((FormulaNode *)f)->evaluated = 1;
    return r;
}

/* NC1 tree contraction evaluation.
 * Rake operation: remove leaves and propagate values up.
 * Compress operation: shortcut chains.
 * Each round reduces nodes by constant fraction → O(log n) rounds. */
int formula_eval_nc1(const FormulaNode *f, int num_vars, const int *assign) {
    /* For the sequential implementation: use recursive mode.
     * A true NC1 implementation would use parallel tree contraction,
     * which requires a PRAM or circuit model. */
    return formula_eval_recursive(f, assign);
}

void formula_to_circuit(const FormulaNode *f, BoolCircuit *c) {
    if (!f) return;
    /* Map each formula node to a circuit gate.
     * This creates the circuit that computes the formula. */
    /* Implementation: recursive translation */
    int gate_id;
    switch (f->type) {
        case FORM_INPUT:
            gate_id = circuit_add_input(c, f->var_index);
            break;
        case FORM_CONST_0:
            gate_id = circuit_add_constant(c, 0);
            break;
        case FORM_CONST_1:
            gate_id = circuit_add_constant(c, 1);
            break;
        case FORM_NOT: {
            formula_to_circuit(f->left, c);
            int child = c->num_gates - 1;
            int in[1] = {child};
            gate_id = circuit_add_gate(c, NC_NOT, in, 1);
            break;
        }
        case FORM_AND: {
            formula_to_circuit(f->left, c);
            int l = c->num_gates - 1;
            formula_to_circuit(f->right, c);
            int r = c->num_gates - 1;
            int in[2] = {l, r};
            gate_id = circuit_add_gate(c, NC_AND, in, 2);
            break;
        }
        case FORM_OR: {
            formula_to_circuit(f->left, c);
            int l = c->num_gates - 1;
            formula_to_circuit(f->right, c);
            int r = c->num_gates - 1;
            int in[2] = {l, r};
            gate_id = circuit_add_gate(c, NC_OR, in, 2);
            break;
        }
    }
    /* Mark as output if root */
    if (!f->parent) {
        circuit_add_output(c, gate_id);
    }
}

/* ---- Formula generators ---- */

FormulaNode *formula_parity(int num_vars) {
    if (num_vars <= 0) return formula_const(0);
    if (num_vars == 1) return formula_leaf(0);
    FormulaNode *result = NULL;
    for (int i = 0; i < num_vars; i++) {
        FormulaNode *leaf = formula_leaf(i);
        if (!result) result = leaf;
        else result = formula_xor(result, leaf);
    }
    return result;
}

FormulaNode *formula_majority(int num_vars) {
    /* MAJ(x1,...,xn) expressed as OR over all size > n/2 subsets.
     * This gives exponential size formula. For demonstration only. */
    if (num_vars <= 2) {
        if (num_vars <= 0) return formula_const(0);
        if (num_vars == 1) return formula_leaf(0);
        return formula_and(formula_leaf(0), formula_leaf(1));
    }
    /* Use recursive majority construction */
    FormulaNode *a = formula_leaf(0);
    FormulaNode *b = formula_leaf(1);
    FormulaNode *c = formula_leaf(2);
    FormulaNode *base = formula_majority3(a, b, c);
    for (int i = 3; i < num_vars; i++) {
        FormulaNode *d = formula_leaf(i);
        /* Recursive: MAJ_n = MAJ3(MAJ_{n-1}, x_i, x_i) */
        base = formula_majority3(base, c, d);
    }
    return base;
}

FormulaNode *formula_threshold(int num_vars, int k) {
    if (k <= 0) return formula_const(1);
    if (k > num_vars) return formula_const(0);
    /* Simple construction: OR over all k-subsets */
    /* Simplified: return balanced formula for k=1 (disjunction) */
    FormulaNode *result = NULL;
    for (int i = 0; i < num_vars; i++) {
        FormulaNode *leaf = formula_leaf(i);
        if (!result) result = leaf;
        else result = formula_or(result, leaf);
    }
    return result;
}

FormulaNode *formula_balance_spira(const FormulaNode *f) {
    /* Spira (1971): Any formula of size s can be rebalanced to depth O(log s).
     * The algorithm finds a gate that has size between s/3 and 2s/3,
     * recursively balances the subformulas, and combines. */
    if (!f) return NULL;
    int sz = formula_size(f);
    if (sz <= 3) return formula_copy(f);

    /* Find splitting node */
    const FormulaNode *cur = f;
    const FormulaNode *split = f;
    while (cur) {
        int lsz = formula_size(cur->left);
        int rsz = formula_size(cur->right);
        if (lsz >= sz/3 && lsz <= 2*sz/3) { split = cur; break; }
        if (rsz >= sz/3 && rsz <= 2*sz/3) { split = cur; break; }
        cur = (lsz > rsz) ? cur->left : cur->right;
    }

    FormulaNode *l = formula_balance_spira(split->left);
    FormulaNode *r = formula_balance_spira(split->right);

    switch (split->type) {
        case FORM_AND: return formula_and(l, r);
        case FORM_OR:  return formula_or(l, r);
        case FORM_NOT: return formula_not(l);
        default:       return formula_copy(split);
    }
}

/* ---- Printing ---- */

static void formula_print(const FormulaNode *f, int depth) {
    if (!f) return;
    for (int i = 0; i < depth; i++) printf("  ");
    switch (f->type) {
        case FORM_INPUT:   printf("x%d\n", f->var_index); break;
        case FORM_CONST_0: printf("0\n"); break;
        case FORM_CONST_1: printf("1\n"); break;
        case FORM_NOT:     printf("NOT\n");
                           formula_print(f->left, depth+1); break;
        case FORM_AND:     printf("AND\n");
                           formula_print(f->left, depth+1);
                           formula_print(f->right, depth+1); break;
        case FORM_OR:      printf("OR\n");
                           formula_print(f->left, depth+1);
                           formula_print(f->right, depth+1); break;
    }
}

/* ---- Demo ---- */

void formula_eval_demo(void) {
    printf("\n================================================================\n");
    printf("  BOOLEAN FORMULA EVALUATION (NC1-complete)\n");
    printf("  Buss 1987\n");
    printf("================================================================\n\n");

    printf("Formula Evaluation: given formula F and assignment x, compute F(x).\n");
    printf("NC1-complete under ALOGTIME reductions.\n\n");

    /* Test 1: AND(OR(x0,x1), NOT(x2)) */
    FormulaNode *f1 = formula_and(formula_or(formula_leaf(0), formula_leaf(1)),
                                   formula_not(formula_leaf(2)));
    printf("--- F1 = AND(OR(x0,x1), NOT(x2)) ---\n");
    printf("size=%d depth=%d\n", formula_size(f1), formula_depth(f1));
    int a1[] = {1,0,0};
    printf("  F1(1,0,0) = %d (expected 1)\n", formula_eval_recursive(f1, a1));
    int a2[] = {0,0,0};
    printf("  F1(0,0,0) = %d (expected 0)\n", formula_eval_recursive(f1, a2));
    formula_free(f1);

    /* Test 2: XOR */
    printf("\n--- F2 = x0 XOR x1 ---\n");
    FormulaNode *f2 = formula_xor(formula_leaf(0), formula_leaf(1));
    printf("size=%d depth=%d\n", formula_size(f2), formula_depth(f2));
    int x00[] = {0,0}; printf("  F2(0,0) = %d (expected 0)\n", formula_eval_recursive(f2, x00));
    int x01[] = {0,1}; printf("  F2(0,1) = %d (expected 1)\n", formula_eval_recursive(f2, x01));
    int x10[] = {1,0}; printf("  F2(1,0) = %d (expected 1)\n", formula_eval_recursive(f2, x10));
    int x11[] = {1,1}; printf("  F2(1,1) = %d (expected 0)\n", formula_eval_recursive(f2, x11));
    formula_free(f2);

    /* Test 3: MAJ3 */
    printf("\n--- F3 = MAJ3(x0,x1,x2) ---\n");
    FormulaNode *f3 = formula_majority3(formula_leaf(0), formula_leaf(1), formula_leaf(2));
    printf("size=%d depth=%d\n", formula_size(f3), formula_depth(f3));
    for (int s = 0; s < 8; s++) {
        int ass[] = {(s>>0)&1, (s>>1)&1, (s>>2)&1};
        printf("  F3(%d,%d,%d) = %d (majority=%d)\n",
               ass[0], ass[1], ass[2],
               formula_eval_recursive(f3, ass),
               (ass[0]+ass[1]+ass[2] > 1) ? 1 : 0);
    }
    formula_free(f3);

    /* Test 4: Balanced tree */
    printf("\n--- Balanced tree (8 leaves) ---\n");
    FormulaNode *trees[8];
    for (int i = 0; i < 8; i++) trees[i] = formula_leaf(i);
    FormulaNode *b8 = formula_and(
        formula_or(formula_and(trees[0], trees[1]),
                   formula_and(trees[2], trees[3])),
        formula_or(formula_and(trees[4], trees[5]),
                   formula_and(trees[6], trees[7])));
    printf("  size=%d depth=%d\n", formula_size(b8), formula_depth(b8));
    int ass8[] = {1,1,0,1,0,0,1,0};
    printf("  Result = %d\n", formula_eval_recursive(b8, ass8));
    formula_free(b8);

    /* Test 5: Circuit conversion */
    printf("\n--- Formula to Circuit Conversion ---\n");
    FormulaNode *fc = formula_and(formula_or(formula_leaf(0), formula_leaf(1)),
                                   formula_not(formula_leaf(2)));
    BoolCircuit *cc = circuit_create(16, 2);
    formula_to_circuit(fc, cc);
    printf("  Circuit: gates=%d depth=%d\n",
           circuit_size(cc), circuit_depth(cc));
    int in3[] = {1, 0, 0};
    int *out = circuit_evaluate(cc, in3);
    printf("  Circuit(1,0,0) = %d (expected 1)\n", out[0]);
    free(out);
    circuit_free(cc);
    formula_free(fc);

    printf("\n--- NC1 Completeness ---\n");
    printf("Theorem (Buss 1987): Formula Evaluation is NC1-complete\n");
    printf("under ALOGTIME reductions.\n\n");
    printf("Meaning:\n");
    printf("  1. Formula Eval in NC1: tree contraction, depth O(log n).\n");
    printf("  2. Every NC1 language reducible to Formula Eval.\n");
    printf("  3. If Formula Eval in AC0, then NC1 = AC0 (false).\n");
}
