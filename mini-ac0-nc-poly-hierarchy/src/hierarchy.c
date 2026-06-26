/* hierarchy.c — Class Inclusion Matrix and Hierarchy Analysis
 * =====================================================================
 * Implements the full complexity class inclusion matrix,
 * known separations, and open problems for circuit classes.
 *
 * The landscape:
 *   AC0 < AC0[p] < TC0 <= NC1 <= L <= NL <= NC2 <= ... <= NC <= P <= P/poly
 *                                        |
 *                                   NL=coNL (IS 1988)
 *
 *   BPP <= P/poly (Adleman 1978)
 *   BPP <= Sigma2 (Sipser-Gacs-Lautemann 1983)
 *   NEXP not subset of ACC0 (Williams 2014)
 *
 * Open:
 *   TC0 vs NC1?  NC vs P?  P/poly vs NP?
 *
 * References:
 *   Arora & Barak Appendix A
 *   Complexity Zoo (complexityzoo.net)
 */
#include "ac0nc.h"
#include "circuit_classes.h"

/* ===== Class Names ===== */
const char* complexity_class_names[CLASS_COUNT] = {
    [CLASS_AC0]         = "AC0",
    [CLASS_AC0_MOD2]    = "AC0[2]",
    [CLASS_AC0_MOD3]    = "AC0[3]",
    [CLASS_AC0_MODP]    = "AC0[p]",
    [CLASS_TC0]         = "TC0",
    [CLASS_NC0]         = "NC0",
    [CLASS_NC1]         = "NC1",
    [CLASS_NC2]         = "NC2",
    [CLASS_NC]          = "NC",
    [CLASS_L]           = "L",
    [CLASS_NL]          = "NL",
    [CLASS_P]           = "P",
    [CLASS_NP]          = "NP",
    [CLASS_PH]          = "PH",
    [CLASS_PSPACE]      = "PSPACE",
    [CLASS_BPP]         = "BPP",
    [CLASS_PPOLY]       = "P/poly",
};

/* ===== Inclusion Matrix =====
 * inclusion_matrix[a][b]:
 *   1 = a subset b is KNOWN
 *   0 = a subset b is OPEN
 *  -1 = a NOT subset b is KNOWN (separation) */
const int inclusion_matrix[CLASS_COUNT][CLASS_COUNT] = {
    /* Row 0: AC0 */
    [CLASS_AC0] = {
        [CLASS_AC0] = 1, [CLASS_AC0_MOD2] = 1, [CLASS_AC0_MOD3] = 1,
        [CLASS_AC0_MODP] = 1, [CLASS_TC0] = 1, [CLASS_NC0] = 0,
        [CLASS_NC1] = 1, [CLASS_NC2] = 1, [CLASS_NC] = 1,
        [CLASS_L] = 1, [CLASS_NL] = 1, [CLASS_P] = 1,
        [CLASS_NP] = 1, [CLASS_PH] = 1, [CLASS_PSPACE] = 1,
        [CLASS_BPP] = 0, [CLASS_PPOLY] = 1,
    },
    /* Row 1: AC0[2] */
    [CLASS_AC0_MOD2] = {
        [CLASS_TC0] = 0, [CLASS_NC1] = 1, [CLASS_NC2] = 1,
        [CLASS_NC] = 1, [CLASS_L] = 1, [CLASS_NL] = 1,
        [CLASS_P] = 1, [CLASS_NP] = 1, [CLASS_PH] = 1,
        [CLASS_PSPACE] = 1, [CLASS_PPOLY] = 1,
    },
    /* Row 3: TC0 */
    [CLASS_TC0] = {
        [CLASS_TC0] = 1, [CLASS_NC1] = 1, [CLASS_NC2] = 1,
        [CLASS_NC] = 1, [CLASS_L] = 1, [CLASS_NL] = 1,
        [CLASS_P] = 1, [CLASS_NP] = 1, [CLASS_PH] = 1,
        [CLASS_PSPACE] = 1, [CLASS_PPOLY] = 1,
    },
    /* Row 4: NC1 */
    [CLASS_NC1] = {
        [CLASS_NC1] = 1, [CLASS_L] = 1, [CLASS_NL] = 1,
        [CLASS_NC2] = 1, [CLASS_NC] = 1, [CLASS_P] = 1,
        [CLASS_NP] = 1, [CLASS_PH] = 1, [CLASS_PSPACE] = 1,
        [CLASS_PPOLY] = 1,
    },
    /* Row 5: L */
    [CLASS_L] = {
        [CLASS_L] = 1, [CLASS_NL] = 1, [CLASS_NC2] = 1,
        [CLASS_NC] = 1, [CLASS_P] = 1, [CLASS_NP] = 1,
        [CLASS_PH] = 1, [CLASS_PSPACE] = 1, [CLASS_PPOLY] = 1,
    },
    /* Row 6: NL */
    [CLASS_NL] = {
        [CLASS_L] = 0, [CLASS_NL] = 1, [CLASS_NC2] = 1,
        [CLASS_NC] = 1, [CLASS_P] = 1, [CLASS_NP] = 1,
        [CLASS_PH] = 1, [CLASS_PSPACE] = 1, [CLASS_PPOLY] = 1,
    },
    /* Row 7: NC2 */
    [CLASS_NC2] = {
        [CLASS_NC2] = 1, [CLASS_NC] = 1, [CLASS_P] = 1,
        [CLASS_NP] = 1, [CLASS_PH] = 1, [CLASS_PSPACE] = 1,
        [CLASS_PPOLY] = 1,
    },
    /* Row 8: NC */
    [CLASS_NC] = {
        [CLASS_NC] = 1, [CLASS_P] = 1, [CLASS_NP] = 1,
        [CLASS_PH] = 1, [CLASS_PSPACE] = 1, [CLASS_PPOLY] = 1,
    },
    /* Row 9: P */
    [CLASS_P] = {
        [CLASS_NP] = 1, [CLASS_PH] = 1, [CLASS_PSPACE] = 1,
        [CLASS_PPOLY] = 1,
    },
    /* Row 10: NP */
    [CLASS_NP] = {
        [CLASS_PH] = 1, [CLASS_PSPACE] = 1, [CLASS_PPOLY] = 0,
    },
    /* Row 12: BPP */
    [CLASS_BPP] = {
        [CLASS_BPP] = 1, [CLASS_P] = 0, [CLASS_NP] = 0,
        [CLASS_PH] = 1, [CLASS_PSPACE] = 1, [CLASS_PPOLY] = 1,
    },
};

/* ===== Known Separations ===== */
const SeparationResult known_separations[] = {
    {CLASS_AC0, CLASS_NC1, "PARITY not in AC0",
     "Furst-Saxe-Sipser/Ajtai/Hastad", 1981, "Godel Prize 1994"},
    {CLASS_AC0_MOD2, CLASS_NC1, "MAJORITY not in AC0[2]",
     "Razborov", 1987, "Nevanlinna Prize 1990"},
    {CLASS_AC0_MOD3, CLASS_NC1, "MAJORITY not in AC0[p] for p!=2",
     "Razborov-Smolensky", 1987, "Nevanlinna Prize 1990"},
    {CLASS_NC1, CLASS_L, "L != NC1",
     "Space hierarchy", 0, ""},
    {CLASS_L, CLASS_PSPACE, "L != PSPACE",
     "Space hierarchy (Stearns-Hartmanis-Lewis)", 1965, "Turing Award 1993"},
    {CLASS_P, CLASS_EXP, "P != EXP",
     "Time hierarchy", 1965, "Turing Award 1993"},
    {CLASS_ACC0, CLASS_NEXP, "NEXP not in ACC0",
     "Williams", 2014, "Breakthrough Prize"},
};
const int n_known_separations = 7;

/* ===== Open Problems ===== */
const OpenProblem major_open_problems[] = {
    {CLASS_TC0, CLASS_NC1, "TC0 vs NC1", 1989,
     "Equivalent to: is MAJORITY in NC1?"},
    {CLASS_NC, CLASS_P, "NC vs P", 1975,
     "Equivalent to: is CVP in NC?"},
    {CLASS_P, CLASS_NP, "P vs NP", 1971,
     "Millennium Prize Problem #1"},
    {CLASS_NP, CLASS_PPOLY, "NP vs P/poly", 1982,
     "If NP in P/poly, PH collapses (Karp-Lipton)"},
    {CLASS_BPP, CLASS_P, "BPP vs P", 1980,
     "Can randomness be eliminated?"},
    {CLASS_ACC0, CLASS_NEXP, "ACC0 vs NEXP", 2014,
     "NEXP not in ACC0, but is ACC0 in NEXP?"},
};
const int n_open_problems = 6;

/* ===== Queries ===== */
int is_subset_known(ComplexityClass a, ComplexityClass b)
{
    if (a < 0 || a >= CLASS_COUNT || b < 0 || b >= CLASS_COUNT)
        return 0;
    return inclusion_matrix[a][b] == 1;
}

int is_separation_known(ComplexityClass a, ComplexityClass b)
{
    if (a < 0 || a >= CLASS_COUNT || b < 0 || b >= CLASS_COUNT)
        return 0;
    return inclusion_matrix[a][b] == -1;
}

int is_open_question(ComplexityClass a, ComplexityClass b)
{
    if (a < 0 || a >= CLASS_COUNT || b < 0 || b >= CLASS_COUNT)
        return 0;
    return inclusion_matrix[a][b] == 0;
}

void print_class_info(ComplexityClass cls)
{
    if (cls < 0 || cls >= CLASS_COUNT) return;
    printf("Class: %s\n", complexity_class_names[cls]);

    printf("  Known subsets: ");
    for (int i = 0; i < CLASS_COUNT; i++)
        if (i != cls && inclusion_matrix[i][cls] == 1)
            printf("%s ", complexity_class_names[i]);
    printf("\n");

    printf("  Known supersets: ");
    for (int i = 0; i < CLASS_COUNT; i++)
        if (i != cls && inclusion_matrix[cls][i] == 1)
            printf("%s ", complexity_class_names[i]);
    printf("\n");

    printf("  Known separations from: ");
    int any = 0;
    for (int i = 0; i < CLASS_COUNT; i++)
        if (inclusion_matrix[i][cls] == -1) {
            printf("%s ", complexity_class_names[i]);
            any = 1;
        }
    if (!any) printf("none");
    printf("\n");
}

void print_inclusion_chain(void)
{
    printf("Inclusion Chain:\n");
    printf("  AC0 < AC0[p] <= TC0 <= NC1 <= L <= NL <= NC2 <= ... <= NC <= P\n");
    printf("  P <= BPP <= P/poly\n");
    printf("  P <= NP <= PH <= PSPACE\n");
}

/* Uniformity names */
const char* uniformity_name(UniformityLevel u)
{
    switch (u) {
        case UNIFORM_NONE:     return "non-uniform";
        case UNIFORM_P:        return "P-uniform";
        case UNIFORM_L:        return "L-uniform (logspace-uniform)";
        case UNIFORM_DLOGTIME: return "DLOGTIME-uniform";
    }
    return "unknown";
}

/* ===== Landscape Printers ===== */

typedef struct { const char* from; const char* to; const char* proof; int year; } Inclusion;

void complexity_landscape_print(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  COMPLEXITY CLASS LANDSCAPE\n");
    printf("  Circuit Complexity and Beyond\n");
    printf("==========================================================\n\n");

    printf("Classes:\n");
    for (int i = 0; i < CLASS_COUNT; i++)
        printf("  %-12s\n", complexity_class_names[i]);

    printf("\n");
    print_inclusion_chain();
    printf("\n");
}

void hierarchy_diagram_print(void) {
    printf("\n");
    printf("                    NP <= PH <= PSPACE\n");
    printf("                     ^         ^\n");
    printf("                     |         |\n");
    printf("  AC0 < TC0 <= NC1 <= P <= P/poly\n");
    printf("          ^       ^      ^\n");
    printf("          |       |      |\n");
    printf("        open     open   open\n");
    printf("      TC0 vs   NC vs  P/poly\n");
    printf("       NC1      P     vs NP\n");
    printf("\n");
}

void known_separations_print(void)
{
    printf("\n=== PROVED Separations ===\n\n");
    printf("  %-12s %-12s %-40s %-20s %s\n",
           "Smaller", "Larger", "Result", "Authors", "Year");
    printf("  %-12s %-12s %-40s %-20s %s\n",
           "-------", "------", "------", "-------", "----");
    for (int i = 0; i < n_known_separations; i++) {
        const SeparationResult *s = &known_separations[i];
        printf("  %-12s %-12s %-40s %-20s %d\n",
               complexity_class_names[s->smaller],
               complexity_class_names[s->larger],
               s->result_name,
               s->authors,
               s->year);
    }
}

void open_problems_print(void)
{
    printf("\n=== OPEN Problems ===\n\n");
    for (int i = 0; i < n_open_problems; i++) {
        const OpenProblem *p = &major_open_problems[i];
        printf("  [%d] %s vs %s (since %d)\n",
               i + 1,
               complexity_class_names[p->class1],
               complexity_class_names[p->class2],
               p->year_posed);
        printf("      %s\n\n", p->significance);
    }
}

/* ===== Known Inclusions Print ===== */
void class_inclusion_demo(void)
{
    printf("\n=== Known Class Inclusions ===\n\n");

    Inclusion known[] = {
        {"AC0",    "TC0",   "MAJORITY gate adds threshold power", 1980},
        {"TC0",    "NC1",   "Barrington: width-5 BP simulates threshold", 1989},
        {"NC1",    "L",     "Depth-first evaluation uses O(log n) workspace", 1977},
        {"L",      "NL",    "Nondeterminism adds power (trivial)", 0},
        {"NL",     "NC2",   "Savitch: reachability in O(log^2 n) depth", 1970},
        {"NC",     "P",     "CVP is P-complete; sequential simulation", 1975},
        {"P",      "NP",    "Determinism subset nondeterminism", 0},
        {"NP",     "PH",    "Polynomial hierarchy starts at NP", 1976},
        {"PH",     "PSPACE","QBF evaluation in polynomial space", 1973},
        {"BPP",    "P/poly","Adleman: randomness -> poly-size circuits", 1978},
        {"BPP",    "Sigma2","Sipser-Gacs-Lautemann: BPP in PH 2nd level", 1983},
        {"P",      "P/poly","Unroll TM to circuit family", 1975},
    };
    int n = sizeof(known)/sizeof(known[0]);

    printf("  %-12s < %-12s %-45s %s\n", "From", "To", "Proof/Result", "Year");
    printf("  %-12s   %-12s %-45s %s\n", "----", "--", "-----------", "----");
    for (int i = 0; i < n; i++)
        printf("  %-12s < %-12s %-45s %d\n",
               known[i].from, known[i].to, known[i].proof, known[i].year);
}

void nc_hierarchy_separation_demo(void)
{
    printf("\n=== NC Hierarchy Separations ===\n\n");

    const char *separations[] = {
        "AC0 != NC1: PARITY (FSS 1981, Ajtai 1983, Hastad 1986) - Godel Prize 1994",
        "AC0[p] != NC1 (p!=2): MAJORITY (Razborov-Smolensky 1987) - Nevanlinna 1990",
        "NEXP not in ACC0: Williams 2014 - algorithmic method breakthrough",
        "NL = coNL: Immerman-Szelepcsenyi 1988 - Godel Prize 1995",
        "PSPACE = NPSPACE: Savitch 1970",
        "L != PSPACE: space hierarchy theorem",
        "P != EXP: time hierarchy theorem"
    };
    const char *open[] = {
        "TC0 vs NC1: since 1989. Equivalent to: is MAJORITY in NC1?",
        "NC vs P: equivalent to L vs P?",
        "P/poly vs NP: would P/poly-sized circuits solve SAT?",
        "ACC0 vs NEXP: NEXP not in ACC0 (Williams), but ACC0 vs NEXP?",
        "BPP vs P: can randomness be eliminated? (Believed BPP = P)"
    };

    printf("--- PROVED ---\n");
    for (int i = 0; i < 7; i++)
        printf("  [%d] %s\n", i+1, separations[i]);
    printf("\n--- OPEN ---\n");
    for (int i = 0; i < 5; i++)
        printf("  [?] %s\n", open[i]);
}
