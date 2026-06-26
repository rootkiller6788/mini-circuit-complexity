/* complete_problems.c — Complete Problems for Circuit Classes
 * =====================================================================
 * Each circuit class has canonical complete problems:
 *
 *   AC0:    No known natural complete problem
 *   TC0:    MAJORITY, SORTING, MULTIPLICATION, DIVISION
 *   NC1:    Boolean Formula Evaluation (BFE, Buss 1987)
 *   L:      Undirected graph reachability (Reingold 2008)
 *   NL:     Directed graph reachability
 *   NC2:    Matrix determinant (Cook 1985)
 *   P:      Circuit Value Problem (CVP, Ladner 1975)
 *   NP:     SAT
 *   PSPACE: TQBF (True Quantified Boolean Formulas)
 *   P/poly: None (not recursively presentable)
 *
 * This file implements CVP and BFE.
 *
 * References:
 *   Ladner (1975) "The Circuit Value Problem is Log Space Complete for P"
 *   Buss (1987) "The Boolean Formula Value Problem Is in ALOGTIME"
 *   Cook (1985) "A Taxonomy of Problems with Fast Parallel Algorithms"
 */
#include "ac0nc.h"

/* ===== Circuit Value Problem (CVP) =====
 * Input: Boolean circuit C (as list of gates) and assignment x.
 * Output: C(x)
 *
 * CVP is P-complete under logspace reductions (Ladner 1975).
 * If CVP in NC, then P = NC.
 * If CVP in L, then P = L.
 *
 * The gate types:
 *   0 = INPUT (value from input assignment)
 *   1 = AND (value = left AND right)
 *   2 = OR  (value = left OR right)
 *   3 = NOT (value = NOT left) */

CVPInstance* cvp_create(int n_nodes)
{
    if (n_nodes <= 0) return NULL;
    CVPInstance *p = (CVPInstance*)calloc(1, sizeof(CVPInstance));
    if (!p) return NULL;
    p->nodes = (CVPNode*)calloc(n_nodes, sizeof(CVPNode));
    if (!p->nodes) { free(p); return NULL; }
    p->n_nodes = n_nodes;
    p->n_inputs = 0;
    return p;
}

void cvp_free(CVPInstance *p)
{
    if (!p) return;
    free(p->nodes);
    free(p);
}

int cvp_evaluate(CVPInstance *p, const int *inputs)
{
    if (!p || !inputs) return 0;

    /* Evaluate gates in topological order (gates are ordered
     * such that inputs to a gate appear before it). */
    for (int i = 0; i < p->n_nodes; i++) {
        CVPNode *n = &p->nodes[i];
        switch (n->type) {
            case 0: /* INPUT */
                n->value = inputs[n->left];
                break;
            case 1: /* AND */
                n->value = p->nodes[n->left].value
                         & p->nodes[n->right].value;
                break;
            case 2: /* OR */
                n->value = p->nodes[n->left].value
                         | p->nodes[n->right].value;
                break;
            case 3: /* NOT */
                n->value = !p->nodes[n->left].value;
                break;
            default:
                n->value = 0;
        }
    }
    return p->nodes[p->n_nodes - 1].value;
}

int cvp_is_p_complete(CVPInstance *p)
{
    /* Any CVP instance represents a P-complete problem.
     * The decision version: given (C, x, j), is the j-th output 1?
     * This is P-complete because any P-time TM can be encoded as
     * a polynomial-size circuit (via tableau method). */
    return (p != NULL);
}

/* ===== Boolean Formula Evaluation (BFE) =====
 * A formula is a circuit where each gate has fan-out <= 1.
 * BFE is NC1-complete under ALOGTIME reductions (Buss 1987).
 *
 * The formula is given as a tree where:
 *   type=0: variable (index gives variable)
 *   type=1: AND of left and right children
 *   type=2: OR
 *   type=3: NOT of left child */

Formula* formula_create(int n_nodes)
{
    if (n_nodes <= 0) return NULL;
    Formula *f = (Formula*)calloc(1, sizeof(Formula));
    if (!f) return NULL;
    f->nodes = (FormulaNode*)calloc(n_nodes, sizeof(FormulaNode));
    if (!f->nodes) { free(f); return NULL; }
    f->n_nodes = n_nodes;
    return f;
}

void formula_free(Formula *f)
{
    if (!f) return;
    free(f->nodes);
    free(f);
}

int formula_evaluate(Formula *f, int node_id, const int *assign)
{
    if (!f || !assign || node_id < 0 || node_id >= f->n_nodes)
        return 0;

    FormulaNode *n = &f->nodes[node_id];
    switch (n->type) {
        case 0: /* VAR */
            return assign[n->index];
        case 1: /* AND */
            return formula_evaluate(f, n->left, assign)
                 & formula_evaluate(f, n->right, assign);
        case 2: /* OR */
            return formula_evaluate(f, n->left, assign)
                 | formula_evaluate(f, n->right, assign);
        case 3: /* NOT */
            return !formula_evaluate(f, n->left, assign);
        default:
            return 0;
    }
}

/* Generate a random balanced formula with given depth and n_vars */
Formula* formula_random(int n_vars, int depth)
{
    if (n_vars <= 0 || depth <= 0) return NULL;
    int n_nodes = (1 << (depth + 1));
    Formula *f = formula_create(n_nodes);
    if (!f) return NULL;

    /* Build a balanced AND-OR tree */
    int next_id = 0;

    /* Leaves: depth 0 = variables */
    int n_leaves = 1 << depth;
    for (int i = 0; i < n_leaves; i++) {
        f->nodes[next_id].type  = 0;
        f->nodes[next_id].index = i % n_vars;
        next_id++;
    }

    /* Internal nodes: alternating AND/OR */
    int prev_start = 0;
    int prev_count = n_leaves;
    int alt = 1;

    while (prev_count > 1) {
        int new_start = next_id;
        for (int i = 0; i < prev_count; i += 2) {
            f->nodes[next_id].type  = alt ? 1 : 2; /* AND or OR */
            f->nodes[next_id].left  = prev_start + i;
            f->nodes[next_id].right = (i+1 < prev_count) ? prev_start + i + 1 : prev_start + i;
            next_id++;
        }
        prev_start = new_start;
        prev_count = (prev_count + 1) / 2;
        alt = 1 - alt;
    }

    f->n_nodes = next_id;
    return f;
}

/* ===== CVP Demo ===== */
void cvp_demo(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  CIRCUIT VALUE PROBLEM (CVP): P-COMPLETE\n");
    printf("  Ladner 1975\n");
    printf("==========================================================\n\n");

    printf("CVP: Given Boolean circuit C and input x, compute C(x).\n");
    printf("CVP is P-complete under logspace reductions.\n");
    printf("If CVP in NC, then P = NC (all P problems parallelizable).\n");
    printf("If CVP in L, then P = L.\n\n");

    /* Build example: AND(OR(x0,x1), x2) */
    CVPInstance *cvp = cvp_create(5);
    cvp->nodes[0] = (CVPNode){0, 0, -1, 0}; /* x0 */
    cvp->nodes[1] = (CVPNode){0, 1, -1, 0}; /* x1 */
    cvp->nodes[2] = (CVPNode){0, 2, -1, 0}; /* x2 */
    cvp->nodes[3] = (CVPNode){2, 0,  1, 0}; /* OR(x0,x1) */
    cvp->nodes[4] = (CVPNode){1, 3,  2, 0}; /* AND(OR(x0,x1),x2) */

    int test_inputs[] = {1, 0, 1};
    int result = cvp_evaluate(cvp, test_inputs);
    printf("  Example: C = AND(OR(x0, x1), x2)\n");
    printf("  Input:  x=(1, 0, 1)\n");
    printf("  C(x) = AND(OR(1,0), 1) = AND(1,1) = %d\n", result);

    printf("\n  Truth table:\n");
    printf("  x2 x1 x0 | C(x)\n");
    printf("  ---------+-----\n");
    for (int r = 0; r < 8; r++) {
        int inp[] = {r&1, (r>>1)&1, (r>>2)&1};
        printf("   %d  %d  %d |  %d\n", inp[2], inp[1], inp[0],
               cvp_evaluate(cvp, inp));
    }
    cvp_free(cvp);

    printf("\n  Significance: CVP is the canonical P-complete problem.\n");
    printf("  Every P-time computation can be encoded as CVP.\n");
}

/* ===== BFE Demo ===== */
void bfe_demo(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  BOOLEAN FORMULA EVALUATION (BFE): NC1-COMPLETE\n");
    printf("  Buss 1987\n");
    printf("==========================================================\n\n");

    printf("BFE: Given a Boolean formula F and assignment a, compute F(a).\n");
    printf("A formula is a circuit where each gate has fan-out <= 1.\n");
    printf("BFE is NC1-complete under ALOGTIME reductions.\n\n");

    /* Build balanced AND-OR tree of depth 3 */
    int depth = 3;
    int n_vars = 4;
    Formula *f = formula_random(n_vars, depth);
    if (!f) { printf("  Failed to create formula.\n"); return; }

    printf("  Random formula: depth=%d, %d nodes, %d variables\n",
           depth, f->n_nodes, n_vars);

    /* Print some evaluations */
    printf("\n  Sample evaluations:\n");
    int test_assigns[][4] = {{0,0,0,0}, {0,0,0,1}, {1,1,0,0}, {1,1,1,1}};
    for (int t = 0; t < 4; t++) {
        int result = formula_evaluate(f, f->n_nodes - 1, test_assigns[t]);
        printf("  f(%d,%d,%d,%d) = %d\n",
               test_assigns[t][0], test_assigns[t][1],
               test_assigns[t][2], test_assigns[t][3], result);
    }

    formula_free(f);

    printf("\n  Significance: BFE is the NC1-complete problem.\n");
    printf("  Barrington's theorem: NC1 = width-5 branching programs.\n");
    printf("  BFE width-5 BP construction gives alternative proof.\n");
}

/* ===== Complete Problems Summary ===== */
void complete_problems_demo(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  COMPLETE PROBLEMS AT EACH CIRCUIT CLASS\n");
    printf("==========================================================\n\n");

    printf("Class     Complete Problem          Reduction Type\n");
    printf("--------  ------------------------  --------------\n");
    printf("AC0       None known (PARITY is lower bound, not complete)\n");
    printf("TC0       MAJORITY/SORTING/MULT     AC0 reductions\n");
    printf("NC1       Formula Evaluation / BFE  ALOGTIME reductions\n");
    printf("NC2       Matrix Determinant        NC1 reductions\n");
    printf("L         Undirected reachability   Logspace reductions\n");
    printf("NL        Directed reachability     Logspace reductions\n");
    printf("P         Circuit Value (CVP)       Logspace reductions\n");
    printf("NP        3SAT                      Polynomial reductions\n");
    printf("PSPACE    TQBF (QBF)               Polynomial reductions\n");
    printf("P/poly    None known                Not recursively presentable\n");

    printf("\n--- Open Questions ---\n");
    printf("  1. Is there an AC0-complete problem? (likely NOT)\n");
    printf("  2. TC0 vs NC1? (equivalent to: is MAJORITY in NC1?)\n");
    printf("  3. Is CVP in NC? (equivalent to NC = P)\n");
    printf("  4. What is the complexity of Matrix Permanent?\n");

    cvp_demo();
    bfe_demo();
}

/* ===== Barrington's Theorem Demo ===== */
void nc1_barrington_demo(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  BARRINGTON'S THEOREM (1989)\n");
    printf("  NC1 = poly-size width-5 branching programs\n");
    printf("==========================================================\n\n");

    printf("Theorem: NC1 = PBP5 (poly-size width-5 branching programs).\n");
    printf("  Uses S_5, the symmetric group on 5 elements.\n");
    printf("  S_5 is NON-SOLVABLE (its commutator subgroup is itself).\n\n");

    printf("Construction:\n");
    printf("  Each Boolean gate is simulated by group commutators:\n");
    printf("  AND(a,b) = aba^{-1}b^{-1} (commutator product in S_5)\n");
    printf("  OR = De Morgan: NOT(AND(NOT(a), NOT(b)))\n");
    printf("  NOT = conjugation by fixed 2-cycle\n\n");

    printf("Blowup: each gate -> O(4) BP instructions.\n");
    printf("  Depth-d circuit -> BP of length O(4^d), width 5.\n\n");

    printf("Blowup analysis:\n");
    printf("  depth  BP_length(4^d)\n");
    printf("  -----  --------------\n");
    for (int d = 1; d <= 5; d++)
        printf("  %-6d %.0f\n", d, barrington_blowup(d));

    printf("\nImplications:\n");
    printf("  1. NC1 = width-5 BP (exact characterization)\n");
    printf("  2. TC0 subset NC1 (threshold gates -> BP via Barrington)\n");
    printf("  3. Constant-width BP recognizes REGULAR languages\n");
    printf("  4. Width-4 BP? Strictly weaker than width-5? (open)\n");
}

/* ===== Hierarchy Landscape Demo ===== */
void hierarchy_landscape_demo(void)
{
    complexity_landscape_print();
    hierarchy_diagram_print();
    known_separations_print();
    open_problems_print();
}
