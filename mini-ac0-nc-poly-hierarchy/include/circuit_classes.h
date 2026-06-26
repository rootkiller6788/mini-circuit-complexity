/* circuit_classes.h — Class Comparisons and Hierarchy Navigation
 * =====================================================================
 * Utility header for comparing circuit complexity classes,
 * checking known inclusions, separations, and open problems.
 *
 * The Landscape (as of 2026):
 *
 *   AC⁰ ⊂ AC⁰[p] ⊂ TC⁰ ⊆ NC¹ ⊆ L ⊆ NL ⊆ NC² ⊆ NC³ ⊆ ... ⊆ NC ⊆ P ⊆ NP ⊆ PH ⊆ PSPACE
 *              ↑                 ↑          ↑           ↑
 *      AC⁰[2] ⊊ NC¹       NL=coNL    NCⁱ⊆NCⁱ⁺¹?   NC vs P?
 *      (Razborov-         (Immerman-   (open for    (open)
 *       Smolensky)         Szelepcsenyi i>1)
 *                           1988)
 *
 *   BPP ⊆ P/poly (Adleman 1978), BPP ⊆ Σ₂^p (Sipser-Gács-Lautemann 1983)
 *   RP ⊆ BPP ⊆ ZPP (with randomness, believed BPP = P)
 *   NEXP ⊈ ACC⁰ (Williams 2014)
 *
 * References:
 *   AB Appendix A (Complexity Class Cheat Sheet)
 *   Complexity Zoo (https://complexityzoo.net)
 */
#ifndef CIRCUIT_CLASSES_H
#define CIRCUIT_CLASSES_H

#include "ac0nc.h"

/* ─── Complexity Class Enum ──────────────────────────────────────── */
typedef enum {
    CLASS_AC0,
    CLASS_AC0_MOD2,
    CLASS_AC0_MOD3,
    CLASS_AC0_MODP,
    CLASS_TC0,
    CLASS_NC0,
    CLASS_NC1,
    CLASS_NC2,
    CLASS_NC,
    CLASS_L,
    CLASS_NL,
    CLASS_P,
    CLASS_NP,
    CLASS_PH,
    CLASS_PSPACE,
    CLASS_BPP,
    CLASS_PPOLY,
    CLASS_COUNT
} ComplexityClass;

extern const char* complexity_class_names[CLASS_COUNT];

/* ─── Class Inclusion Matrix ─────────────────────────────────────── */
/* inclusion_matrix[i][j] = 1 if CLASS_i ⊆ CLASS_j is KNOWN
 * inclusion_matrix[i][j] = 0 if CLASS_i ⊆ CLASS_j is OPEN
 * inclusion_matrix[i][j] = -1 if CLASS_i ⊈ CLASS_j is KNOWN */
extern const int inclusion_matrix[CLASS_COUNT][CLASS_COUNT];

/* ─── Known Separation Results ──────────────────────────────────── */
typedef struct {
    ComplexityClass smaller;
    ComplexityClass larger;
    const char      *result_name;
    const char      *authors;
    int              year;
    const char      *award;
} SeparationResult;

extern const SeparationResult known_separations[];
extern const int n_known_separations;

/* ─── Open Problems ─────────────────────────────────────────────── */
typedef struct {
    ComplexityClass class1;
    ComplexityClass class2;
    const char      *problem_name;
    int              year_posed;
    const char      *significance;
} OpenProblem;

extern const OpenProblem major_open_problems[];
extern const int n_open_problems;

/* ─── Hierarchy Queries ──────────────────────────────────────────── */
int  is_subset_known(ComplexityClass a, ComplexityClass b);
int  is_separation_known(ComplexityClass a, ComplexityClass b);
int  is_open_question(ComplexityClass a, ComplexityClass b);
void print_class_info(ComplexityClass cls);
void print_inclusion_chain(void);

/* ─── Uniformity Notions ─────────────────────────────────────────── */
/* Different uniformity conditions for circuit families:
 *   - Non-uniform: no restriction (P/poly)
 *   - P-uniform: C_n constructible in time poly(n)
 *   - L-uniform: C_n constructible in O(log n) space
 *   - DLOGTIME-uniform: C_n constructible in O(log n) time
 *
 * DLOGTIME-uniform AC⁰ = FO (first-order logic with BIT predicate).
 * L-uniform NC¹ = ALOGTIME. */
typedef enum {
    UNIFORM_NONE,
    UNIFORM_P,
    UNIFORM_L,
    UNIFORM_DLOGTIME
} UniformityLevel;

const char* uniformity_name(UniformityLevel u);

/* ─── Landscape Printer ──────────────────────────────────────────── */
void complexity_landscape_print(void);
void hierarchy_diagram_print(void);
void known_separations_print(void);
void open_problems_print(void);

#endif /* CIRCUIT_CLASSES_H */
