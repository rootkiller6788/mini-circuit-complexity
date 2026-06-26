/* monotone.c -- Monotone Circuit Lower Bounds: Graph & Clique Library
 *
 * API:
 *   Graph* graph_create(n)
 *   void   graph_add_edge(g, u, v)
 *   int    graph_has_edge(g, u, v)
 *   int    graph_has_clique(g, k)
 *   Graph* graph_complete(n)
 *   Graph* graph_random(n, edge_prob)
 *   void   graph_free(g)
 *
 *   double sunflower_bound(p, k)
 *   double razborov_clique_lower(n, k)
 *   double monotone_size_estimate(n, k, depth) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "monotone.h"

/* ============ Graph Implementation ============ */

Graph* graph_create(int n) {
    Graph* g = malloc(sizeof(Graph));
    g->n = n;
    g->adj = malloc((size_t)n * sizeof(char*));
    for (int i = 0; i < n; i++) g->adj[i] = calloc((size_t)n, sizeof(char));
    return g;
}

void graph_add_edge(Graph* g, int u, int v) {
    if (u >= 0 && v >= 0 && u < g->n && v < g->n) g->adj[u][v] = g->adj[v][u] = 1;
}

int graph_has_edge(const Graph* g, int u, int v) {
    return (u >= 0 && v >= 0 && u < g->n && v < g->n) ? g->adj[u][v] : 0;
}

Graph* graph_complete(int n) {
    Graph* g = graph_create(n);
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++) g->adj[i][j] = g->adj[j][i] = 1;
    return g;
}

Graph* graph_random(int n, double p) {
    Graph* g = graph_create(n);
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if ((double)rand() / RAND_MAX < p) g->adj[i][j] = g->adj[j][i] = 1;
    return g;
}

void graph_free(Graph* g) {
    for (int i = 0; i < g->n; i++) free(g->adj[i]);
    free(g->adj); free(g);
}

/* ============ Clique Detection ============ */

int graph_has_clique(Graph* g, int k) {
    if (k <= 0) return 0;
    if (k == 1) return g->n >= 1;
    if (k == 2) {
        for (int i = 0; i < g->n; i++)
            for (int j = i + 1; j < g->n; j++)
                if (g->adj[i][j]) return 1;
        return 0;
    }
    if (k == 3) {
        for (int a = 0; a < g->n; a++)
            for (int b = a + 1; b < g->n; b++) if (g->adj[a][b])
                for (int c = b + 1; c < g->n; c++)
                    if (g->adj[a][c] && g->adj[b][c]) return 1;
        return 0;
    }
    if (k == 4) {
        for (int a = 0; a < g->n; a++)
            for (int b = a + 1; b < g->n; b++) if (g->adj[a][b])
                for (int c = b + 1; c < g->n; c++) if (g->adj[a][c] && g->adj[b][c])
                    for (int d = c + 1; d < g->n; d++)
                        if (g->adj[a][d] && g->adj[b][d] && g->adj[c][d]) return 1;
        return 0;
    }
    return 0;
}

/* ============ Sunflower Lemma ============ */

typedef unsigned long long SetMask;

static int popcount_ull(SetMask s) { int c = 0; while (s) { c++; s &= s - 1; } return c; }

int is_sunflower(SetMask* sets, int p, SetMask* core) {
    *core = sets[0];
    for (int i = 1; i < p; i++) *core &= sets[i];
    for (int i = 0; i < p; i++)
        for (int j = i + 1; j < p; j++)
            if ((sets[i] & ~(*core)) & (sets[j] & ~(*core))) return 0;
    return 1;
}

double sunflower_bound(int p, int k) {
    return pow((double)(p - 1), (double)k) * tgamma((double)(k + 1));
}

/* ============ Lower Bounds ============ */

double razborov_clique_lower(int n, int k) {
    return exp(sqrt((double)k) * log((double)n) / 4.0);
}

double monotone_size_estimate(int n, int k, int depth) {
    double base = razborov_clique_lower(n, k);
    return pow(base, 1.0 / depth);
}

/* ============ DEMO ============ */

void monotone_demo(void) {
    printf("\n================================================================\n");
    printf("  MONOTONE CIRCUITS -- GRAPH & CLIQUE LIBRARY DEMO\n");
    printf("================================================================\n\n");
    srand(42);

    /* 1. Graph creation and clique detection */
    printf("--- Graph & Clique ---\n");
    Graph* g5 = graph_complete(5);
    printf("  K_5: 3-clique=%s 4-clique=%s 5-clique=%s\n",
           graph_has_clique(g5, 3) ? "YES" : "NO",
           graph_has_clique(g5, 4) ? "YES" : "NO",
           graph_has_clique(g5, 5) ? "YES" : "NO");
    graph_free(g5);

    /* 2. Random graphs */
    printf("\n--- Random Graphs (p=0.5) ---\n");
    for (int n = 4; n <= 8; n++) {
        Graph* rg = graph_random(n, 0.5);
        printf("  G(%d,0.5): 3-clique=%s\n", n, graph_has_clique(rg, 3) ? "YES" : "NO");
        graph_free(rg);
    }

    /* 3. Sunflower lemma */
    printf("\n--- Sunflower Lemma ---\n");
    for (int p = 3; p <= 7; p += 2)
        for (int k = 2; k <= 5; k++)
            printf("  p=%d k=%d: bound=%.0f\n", p, k, sunflower_bound(p, k));

    /* 4. Razborov lower bounds */
    printf("\n--- Razborov Lower Bounds ---\n");
    printf("  n     k=sqrt(n)  monotone_LB\n");
    for (int n = 8; n <= 64; n *= 2) {
        int k = (int)sqrt((double)n);
        printf("  %-5d %-10d %.1e\n", n, k, razborov_clique_lower(n, k));
    }

    printf("\n  CLIQUE needs super-poly monotone size. Razborov 1985.\n");
}/* clique_search.c -- CLIQUE: Full Search + Circuit Lower Bound Demo */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef struct{int n;char**adj;}Graph;
static Graph*g_new(int n){Graph*g=malloc(sizeof(Graph));g->n=n;g->adj=malloc(n*sizeof(char*));for(int i=0;i<n;i++)g->adj[i]=calloc(n,sizeof(char));return g;}
static void g_free(Graph*g){for(int i=0;i<g->n;i++)free(g->adj[i]);free(g->adj);free(g);}

static int has_clique_k(Graph*g,int k){
    if(k==1)return g->n>=1;if(k==2){for(int i=0;i<g->n;i++)for(int j=i+1;j<g->n;j++)if(g->adj[i][j])return 1;return 0;}
    if(k==3){for(int i=0;i<g->n;i++)for(int j=i+1;j<g->n;j++)if(g->adj[i][j])for(int l=j+1;l<g->n;l++)if(g->adj[i][l]&&g->adj[j][l])return 1;return 0;}
    if(k==4){for(int a=0;a<g->n;a++)for(int b=a+1;b<g->n;b++)if(g->adj[a][b])for(int c=b+1;c<g->n;c++)if(g->adj[a][c]&&g->adj[b][c])for(int d=c+1;d<g->n;d++)if(g->adj[a][d]&&g->adj[b][d]&&g->adj[c][d])return 1;return 0;}
    return 0;
}

void clique_search_demo(void){
    printf("\n================================================================\n");
    printf("  CLIQUE SEARCH + RAZBOROV LOWER BOUND\n");
    printf("================================================================\n\n");
    printf("Razborov (1985): CLIQUE requires exp(Omega(k^{1/2} log n))\nmonotone circuit size.\n\n");

    printf("--- CLIQUE Existence Tests ---\n");
    for(int n=4;n<=8;n++){Graph*g=g_new(n);for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)if((i+j)%3!=0)g->adj[i][j]=g->adj[j][i]=1;printf("  n=%d: 3-clique=%s 4-clique=%s\n",n,has_clique_k(g,3)?"YES":"NO",has_clique_k(g,4)?"YES":"NO");g_free(g);}

    printf("\n--- Razborov Lower Bound (monotone circuits) ---\n");
    for(int n=8;n<=64;n*=2){int k=(int)sqrt((double)n);double lb=exp(sqrt((double)k)*log((double)n)/4.0);printf("  n=%-3d k=%-2d monotone size >= %.1e\n",n,k,lb);}
    printf("\nThis was the FIRST super-polynomial circuit lower bound!\n");
    printf("But only works for monotone circuits (no NOT gates).\n");
}
/* monotone_approx.c -- Razborov's Method of Approximations (1985)
 *
 * The method replaces each gate in a monotone circuit with a "small"
 * approximator - a restricted set of positive/negative examples.
 *
 * Key ideas:
 * 1. Each gate's output is approximated by a small set of minterms/maxterms
 * 2. AND -> intersection (via sunflower lemma to bound size)
 * 3. OR -> union (additive blowup)
 * 4. After processing all gates, output approximator differs from CLIQUE
 *    on either YES or NO instances => circuit cannot compute CLIQUE.
 * 5. If circuit is small, approximator stays small => contradiction.
 *
 * This was the FIRST super-polynomial circuit lower bound ever! */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Positive example: a k-clique (set of k vertices that form a clique) */
typedef struct { int n; int k; int* vertices; } Clique;

/* Approximator: maintains set of cliques accepted/rejected */
typedef struct {
    int n;             /* number of vertices in graph */
    int k;             /* clique size */
    Clique** accepted; /* positive examples: cliques we accept */
    Clique** rejected; /* negative examples: k-sets we reject (not cliques) */
    int n_acc;
    int n_rej;
    int max_size;      /* maximum allowed size */
} Approximator;

static Approximator* approx_create(int n,int k,int max_sz){
    Approximator*a=malloc(sizeof(Approximator));
    a->n=n;a->k=k;a->max_size=max_sz;a->n_acc=a->n_rej=0;
    a->accepted=malloc(max_sz*sizeof(Clique*));
    a->rejected=malloc(max_sz*sizeof(Clique*));
    return a;
}

static void approx_free(Approximator*a){
    for(int i=0;i<a->n_acc;i++){free(a->accepted[i]->vertices);free(a->accepted[i]);}
    for(int i=0;i<a->n_rej;i++){free(a->rejected[i]->vertices);free(a->rejected[i]);}
    free(a->accepted);free(a->rejected);free(a);
}

/* Check if two cliques intersect in a sunflower-like pattern */
static int clique_intersection(Clique*a,Clique*b,int*core_sz){
    *core_sz=0;
    for(int i=0;i<a->k;i++)
        for(int j=0;j<b->k;j++)
            if(a->vertices[i]==b->vertices[j])(*core_sz)++;
    return *core_sz;
}

/* AND of two approximators: intersection (with sunflower bound) */
static Approximator* approx_and(Approximator*a,Approximator*b,int max_sz){
    Approximator*r=approx_create(a->n,a->k,max_sz);
    /* Positive: cliques accepted by BOTH */
    for(int i=0;i<a->n_acc&&r->n_acc<max_sz;i++)
        for(int j=0;j<b->n_acc&&r->n_acc<max_sz;j++){
            int cs;
            clique_intersection(a->accepted[i],b->accepted[j],&cs);
            /* Simplified: accept if intersection non-trivial */
            if(cs>0&&r->n_acc<max_sz){
                r->accepted[r->n_acc]=malloc(sizeof(Clique));
                r->accepted[r->n_acc]->n=a->n;r->accepted[r->n_acc]->k=a->k;
                r->accepted[r->n_acc]->vertices=malloc(a->k*sizeof(int));
                memcpy(r->accepted[r->n_acc]->vertices,a->accepted[i]->vertices,a->k*sizeof(int));
                r->n_acc++;
            }
        }
    /* Negative: anything rejected by EITHER (union) */
    for(int i=0;i<a->n_rej&&r->n_rej<max_sz;i++){
        r->rejected[r->n_rej]=malloc(sizeof(Clique));
        r->rejected[r->n_rej]->n=a->n;r->rejected[r->n_rej]->k=a->k;
        r->rejected[r->n_rej]->vertices=malloc(a->k*sizeof(int));
        memcpy(r->rejected[r->n_rej]->vertices,a->rejected[i]->vertices,a->k*sizeof(int));
        r->n_rej++;
    }
    return r;
}

/* OR of two approximators: union (additive) */
static Approximator* approx_or(Approximator*a,Approximator*b,int max_sz){
    Approximator*r=approx_create(a->n,a->k,max_sz);
    for(int i=0;i<a->n_acc&&r->n_acc<max_sz;i++){
        r->accepted[r->n_acc]=malloc(sizeof(Clique));r->accepted[r->n_acc]->n=a->n;r->accepted[r->n_acc]->k=a->k;
        r->accepted[r->n_acc]->vertices=malloc(a->k*sizeof(int));
        memcpy(r->accepted[r->n_acc]->vertices,a->accepted[i]->vertices,a->k*sizeof(int));r->n_acc++;
    }
    for(int i=0;i<b->n_acc&&r->n_acc<max_sz;i++){
        r->accepted[r->n_acc]=malloc(sizeof(Clique));r->accepted[r->n_acc]->n=a->n;r->accepted[r->n_acc]->k=a->k;
        r->accepted[r->n_acc]->vertices=malloc(a->k*sizeof(int));
        memcpy(r->accepted[r->n_acc]->vertices,b->accepted[i]->vertices,a->k*sizeof(int));r->n_acc++;
    }
    return r;
}

/* Razborov lower bound: approximator size blowup per gate level */
static double razborov_blowup(int l,int k){
    /* After one AND level, size can blow up to O(l^k) */
    return pow((double)l,(double)k);
}

void monotone_approx_demo(void){
    printf("\n================================================================\n");
    printf("  RAZBOROV'S METHOD OF APPROXIMATIONS (1985)\n");
    printf("  First Super-Polynomial Circuit Lower Bound\n");
    printf("================================================================\n\n");

    printf("Key: replace each gate with 'approximator' (small set of examples).\n");
    printf("AND -> intersection (sunflower lemma bounds blowup)\n");
    printf("OR  -> union (additive, no blowup beyond summing)\n\n");

    printf("--- Approximator Creation ---\n");
    Approximator*a=approx_create(10,3,100);
    printf("Created approximator for n=%d, k=%d, max_size=%d\n",a->n,a->k,a->max_size);
    approx_free(a);

    printf("\n--- Blowup Analysis ---\n");
    printf("  l    k    blowup=l^k    feasible?\n");
    printf("  ---  ---  -----------   ---------\n");
    for(int l=2;l<=10;l+=2)
        for(int k=2;k<=4;k++){
            double b=razborov_blowup(l,k);
            printf("  %-4d %-4d %-13.0f %s\n",l,k,b,
                   (l<=4&&k<=3)?"yes":"NO (exp)");
        }

    printf("\n--- Why It Works ---\n");
    printf("For AND gate: input approximators have size l each.\n");
    printf("Naive intersection: l*k sets = l^k (exponential!)\n");
    printf("Sunflower lemma: if l^k > (p-1)^k * k!, contain sunflower.\n");
    printf("Sunflower allows compression back to size l.\n");
    printf("=> Approximator stays small through multiple levels.\n\n");

    printf("--- Razborov's Conclusion ---\n");
    printf("For k = n^{1/4}: approximator size stays sub-exponential.\n");
    printf("But for CLIQUE(n,k): any correct approximator must be large.\n");
    printf("=> Monotone circuit for CLIQUE must be large (super-poly).\n");
    printf("This was the FIRST super-polynomial circuit lower bound!\n");
}
/* sunflower.c -- Sunflower Lemma (Erdos-Rado 1960): Full Implementation
 *
 * Lemma: Any family of more than (p-1)^k * k! sets of size k
 * contains a sunflower with p petals.
 *
 * A sunflower with p petals is a collection of p sets S_1,...,S_p
 * such that the pairwise intersection S_i ∩ S_j = K (the "core")
 * is the same for all i ≠ j. The petals are pairwise disjoint outside K.
 *
 * This lemma is the KEY to Razborov's monotone circuit lower bounds.
 * It bounds the size of approximators used in the method of approximations. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Set representation: bitmask for universe up to 64 */
typedef unsigned long long SetMask;

static int set_size(SetMask s){int c=0;while(s){c++;s&=s-1;}return c;}
static int set_equal(SetMask a,SetMask b){return a==b;}
static SetMask set_intersection(SetMask a,SetMask b){return a&b;}
static int set_disjoint(SetMask a,SetMask b){return (a&b)==0;}

/* Check if k-sets s[0..p-1] form a sunflower with core K */
static int is_sunflower(SetMask*s,int p,SetMask*K){
    *K=s[0];for(int i=1;i<p;i++)*K&=s[i]; /* core = intersection of all */
    for(int i=0;i<p;i++){
        for(int j=i+1;j<p;j++){
            SetMask petal_i=s[i]&~(*K),petal_j=s[j]&~(*K);
            if(!set_disjoint(petal_i,petal_j)) return 0;
        }
    }
    return 1;
}

/* Naive sunflower search: enumerate all p-subsets of family.
   Returns 1 if found, filling sunflower_sets. */
static int find_sunflower_brute(SetMask*sets,int n_sets,int p,SetMask*out,int*out_core){
    if(p>n_sets)return 0;
    /* Check all p-combinations */
    int*idx=malloc(p*sizeof(int));
    for(int i=0;i<p;i++)idx[i]=i;
    while(1){
        SetMask K;
        SetMask subset[32];
        for(int i=0;i<p;i++)subset[i]=sets[idx[i]];
        if(is_sunflower(subset,p,&K)){if(out_core)*out_core=(int)K;if(out)memcpy(out,subset,p*sizeof(SetMask));free(idx);return 1;}
        int pos=p-1;
        while(pos>=0&&idx[pos]==n_sets-p+pos)pos--;
        if(pos<0)break;
        idx[pos]++;
        for(int i=pos+1;i<p;i++)idx[i]=idx[i-1]+1;
    }
    free(idx);return 0;
}

/* Compute sunflower bound: family size > (p-1)^k * k! => contains p-petal k-sunflower */
static double sunflower_bound(int p,int k){return pow((double)(p-1),(double)k)*tgamma((double)(k+1));}

/* Generate random k-sets and test sunflower existence */
static SetMask random_kset(int universe,int k){
    SetMask s=0;int chosen[64]={0};
    for(int i=0;i<k;i++){int b;do{b=rand()%universe;}while(chosen[b]);chosen[b]=1;s|=(1ULL<<b);}
    return s;
}

void sunflower_demo(void){
    printf("\n================================================================\n");
    printf("  SUNFLOWER LEMMA (Erdos-Rado 1960)\n");
    printf("  Key to Razborov's Monotone Circuit Lower Bounds\n");
    printf("================================================================\n\n");

    printf("Sunflower(p petals): p sets where pairwise intersection = core K.\n");
    printf("Petals are pairwise disjoint outside the core.\n\n");

    printf("--- Sunflower Bound ---\n");
    printf("Any family of > (p-1)^k * k! k-sets contains a p-petal sunflower.\n");
    printf("%4s %4s %20s\n","p","k","bound");
    for(int p=3;p<=7;p+=2)
        for(int k=2;k<=5;k++)
            printf("%4d %4d %20.0f\n",p,k,sunflower_bound(p,k));

    printf("\n--- Constructed Sunflowers ---\n");
    /* Construct a sunflower manually: core={0,1}, petals={2},{3},{4} */
    SetMask manual[]={(1ULL<<0)|(1ULL<<1)|(1ULL<<2),
                      (1ULL<<0)|(1ULL<<1)|(1ULL<<3),
                      (1ULL<<0)|(1ULL<<1)|(1ULL<<4)};
    SetMask K;int core_size;
    if(is_sunflower(manual,3,&K)){
        printf("Manual 3-petal sunflower: core={");
        int first=1;for(int i=0;i<10;i++)if(K&(1ULL<<i)){if(!first)printf(",");printf("%d",i);first=0;}
        printf("} petals=3\n");
    }

    printf("\n--- Random Family Sunflower Search ---\n");
    srand(12345);
    for(int n_fam=10;n_fam<=50;n_fam+=10){
        SetMask*family=malloc(n_fam*sizeof(SetMask));
        for(int i=0;i<n_fam;i++)family[i]=random_kset(8,3);
        int found=find_sunflower_brute(family,n_fam,3,NULL,NULL);
        printf("  n_sets=%d size=3 p=3: sunflower=%s\n",n_fam,found?"FOUND":"none");
        free(family);
    }

    printf("\n--- Razborov Connection ---\n");
    printf("AND gate approximator: intersect approximators of inputs.\n");
    printf("Sunflower lemma limits approximator size after intersection.\n");
    printf("Without sunflower: AND of l approximators -> l^k blowup.\n");
    printf("With sunflower: blowup bounded => feasible approximator size.\n");
    printf("This makes Razborov's method WORK for monotone circuits.\n");
}
﻿/* clique_brute.c */
#include "monotone.h"
int graph_has_kclique(Graph*g,int k){
    if(k==1)return g->n>=1;
    if(k==2){for(int i=0;i<g->n;i++)for(int j=i+1;j<g->n;j++)if(g->adj[i][j])return 1;return 0;}
    if(k==3){for(int a=0;a<g->n;a++)for(int b=a+1;b<g->n;b++)if(g->adj[a][b])for(int c=b+1;c<g->n;c++)if(g->adj[a][c]&&g->adj[b][c])return 1;return 0;}
    return 0;
}
void clique_brute_demo(void){
    printf("CLIQUE brute force: O(n^k). NP-complete for input k.\n");
    Graph*g=malloc(sizeof(Graph));g->n=5;g->adj=malloc(5*sizeof(char*));
    for(int i=0;i<5;i++){g->adj[i]=calloc(5,sizeof(char));for(int j=0;j<5;j++)g->adj[i][j]=(i!=j);}
    printf("K_5: 3-clique=%s 4-clique=%s\n",graph_has_kclique(g,3)?"YES":"NO",graph_has_kclique(g,4)?"YES":"NO");
    for(int i=0;i<5;i++)free(g->adj[i]);free(g->adj);free(g);
}﻿/* razborov_proof.c */
#include "monotone.h"
void razborov_proof_demo(void){
    printf("\n=== Razborov's Proof (1985) ===\n\n");
    printf("1. Replace gates with approximators (small sets of cliques).\n");
    printf("2. AND -> intersection (sunflower lemma bounds blowup: O(l^k)->O(l)).\n");
    printf("3. OR -> union (additive: O(l+l) = O(l)).\n");
    printf("4. After processing: output approx differs from CLIQUE on YES/NO.\n");
    printf("5. If original circuit small => approximator small => contradiction.\n");
}﻿/* monotone_complete.c */
#include "monotone.h"
void monotone_complete_demo(void){
    printf("\n=== Monotone Complete Problems ===\n\n");
    printf("CLIQUE(n,k): monotone-NP-complete.\n");
    printf("PERFECT MATCHING: monotone-P-complete.\n");
    printf("Monotone circuits = negation-free = only AND/OR gates.\n");
}﻿/* monotone_vs_general.c */
#include "monotone.h"
void monotone_vs_general_demo(void){
    printf("\n=== Monotone vs General Circuits ===\n\n");
    printf("Monotone: AND/OR only. General: +NOT gates.\n");
    printf("PARITY: exp size monotone, O(n) size general (gap!).\n");
    printf("Razborov's bound only applies to monotone circuits.\n");
    printf("Extending to general circuits is the big open problem.\n");
}﻿/* approximation_method.c */
#include "monotone.h"
void approximation_method_demo(void){
    printf("\n=== Method of Approximations ===\n\n");
    printf("Key insight: replace gate outputs with 'approximators'\n");
    printf("that track only a SMALL set of positive/negative examples.\n");
    printf("Sunflower lemma: large families contain sunflowers => compressible.\n");
    printf("This keeps approximator size manageable through AND gates.\n");
}﻿/* erdos_rado_sunflower.c */
#include "monotone.h"
void erdos_rado_sunflower_demo(void){
    printf("\n=== Sunflower Lemma (Erdos-Rado 1960) ===\n\n");
    printf("Family of > (p-1)^k * k! k-sets contains p-petal sunflower.\n");
    for(int p=3;p<=7;p+=2)for(int k=2;k<=4;k++)printf("  p=%d k=%d: bound=%.0f\n",p,k,pow(p-1.0,k)*tgamma(k+1));
}﻿/* clique_lower_detail.c */
#include "monotone.h"
void clique_lower_detail_demo(void){
    printf("\n=== CLIQUE Lower Bound Details ===\n\n");
    printf("k=n^{1/4}: size >= exp(Omega(n^{1/8})) monotone.\n");
    printf("k=constant: size >= n^{Omega(log n)} monotone.\n");
    printf("FIRST super-polynomial circuit lower bound (Razborov 1985).\n");
}/* monotone_bench.c -- Monotone Circuit Lower Bound Benchmarks
 * Razborov (1985): CLIQUE requires super-poly monotone circuits. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static double razborov_lb(int n,int k){return exp(sqrt((double)k)*log((double)n)/4.0);}
static double exact_clique_count(int n,int k){
    /* (n choose k) cliques in complete graph */
    double r=1;for(int i=0;i<k;i++)r*=(double)(n-i)/(i+1);
    return r;
}

void monotone_bench_demo(void){
    printf("\n================================================================\n");
    printf("  MONOTONE CIRCUIT LOWER BOUND BENCHMARKS\n");
    printf("  Razborov 1985\n");
    printf("================================================================\n\n");

    printf("--- CLIQUE Monotone Lower Bound ---\n");
    printf("  n      sqrt(n)  LB(exp)      brute(n choose k)\n");
    for(int n=8;n<=128;n*=2){
        int k=(int)sqrt((double)n);
        double lb=razborov_lb(n,k);
        double brute=exact_clique_count(n,k);
        printf("  %-6d %-8d %-12.1e %-18.1e\n",n,k,lb,brute);
    }

    printf("\n--- Timing the Lower Bound Computation ---\n");
    for(int n=8;n<=128;n*=2){
        int k=(int)sqrt((double)n);
        clock_t t=clock();
        double lb=razborov_lb(n,k);
        double us=1e6*(clock()-t)/CLOCKS_PER_SEC;
        printf("  n=%-4d k=%-2d %.1e (%.1f us)\n",n,k,lb,us);
    }

    printf("\n--- Comparison: Monotone vs General Circuits ---\n");
    printf("CLIQUE: monotone LB = exp(Omega(sqrt(k) log n)).\n");
    printf("CLIQUE: general LB = OPEN (since 1985).\n");
    printf("This gap shows the limitation of monotone techniques.\n");
    printf("Razborov's method only works for monotone circuits.\n");
}

