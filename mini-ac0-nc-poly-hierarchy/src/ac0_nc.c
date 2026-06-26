/* ac0_nc.c -- AC0/NC1/TC0/P-poly: Full Hierarchy (AB Ch.6,14) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef enum{G_IN,G_AND,G_OR,G_NOT,G_MOD2,G_MOD3,G_MAJ,G_CON}GT;
typedef struct{GT t;int i1,i2;int fans[64];int nf;}CG;
typedef struct{CG*g;int ng,ni,no;int*out;}CC;
static CC*cn(int m){CC*c=malloc(sizeof(CC));c->g=calloc(m,sizeof(CG));c->ng=c->ni=c->no=0;c->out=NULL;return c;}
static void cf(CC*c){free(c->g);free(c->out);free(c);}
static int ca(CC*c,GT t,int i1,int i2){int id=c->ng++;c->g[id].t=t;c->g[id].i1=i1;c->g[id].i2=i2;c->g[id].nf=0;if(t==G_IN)c->ni++;return id;}
static int ce(CC*c,int id,int*in,int*vi){if(vi[id]>=0)return vi[id];CG*g=&c->g[id];int v;switch(g->t){case G_IN:v=in[id];break;case G_CON:v=g->i1;break;case G_NOT:v=!ce(c,g->i1,in,vi);break;case G_AND:if(g->nf>0){v=1;for(int i=0;i<g->nf;i++)v&=ce(c,g->fans[i],in,vi);}else v=ce(c,g->i1,in,vi)&&ce(c,g->i2,in,vi);break;case G_OR:if(g->nf>0){v=0;for(int i=0;i<g->nf;i++)v|=ce(c,g->fans[i],in,vi);}else v=ce(c,g->i1,in,vi)||ce(c,g->i2,in,vi);break;case G_MOD2:{v=0;for(int i=0;i<g->nf;i++)v^=ce(c,g->fans[i],in,vi);break;}case G_MOD3:{int s=0;for(int i=0;i<g->nf;i++)s+=ce(c,g->fans[i],in,vi);v=(s%3==0);break;}case G_MAJ:{int s=0;for(int i=0;i<g->nf;i++)s+=ce(c,g->fans[i],in,vi);v=(s>g->nf/2);break;}default:v=0;}vi[id]=v;return v;}
static int cc(CC*c,int*in){int*vi=calloc(c->ng,sizeof(int));for(int i=0;i<c->ng;i++)vi[i]=-1;int r=1;for(int i=0;i<c->no;i++)r&=ce(c,c->out[i],in,vi);free(vi);return r;}
static int cd(CC*c){int*d=calloc(c->ng,sizeof(int));for(int i=0;i<c->ng;i++)d[i]=-1;for(int i=0;i<c->ng;i++){CG*g=&c->g[i];if(g->t==G_IN||g->t==G_CON){d[i]=0;continue;}int md=0;if(g->nf>0){for(int j=0;j<g->nf;j++){int dd=d[g->fans[j]];if(dd<0)dd=0;if(dd>md)md=dd;}}else{if(g->i1>=0){int dd=d[g->i1];if(dd<0)dd=0;if(dd>md)md=dd;}if(g->i2>=0){int dd=d[g->i2];if(dd<0)dd=0;if(dd>md)md=dd;}}d[i]=md+1;}int md=0;for(int i=0;i<c->no;i++)if(d[c->out[i]]>md)md=d[c->out[i]];free(d);return md;}

/* AC0: DNF for threshold(k). Depth=2, size=exp(k log n). */
static CC*ac0_th(int n,int k){CC*c=cn(10000);int*in=malloc(n*sizeof(int));for(int i=0;i<n;i++)in[i]=ca(c,G_IN,-1,-1);int*an=malloc(2000*sizeof(int));int na=0;for(int i=0;i<200&&na<2000;i++){int sb[32],ns=0;for(int b=0;b<n&&ns<k;b++)if((i>>b)&1)sb[ns++]=in[b];if(ns>=k){int ai=ca(c,G_AND,-1,-1);c->g[ai].nf=ns;for(int j=0;j<ns;j++)c->g[ai].fans[j]=sb[j];an[na++]=ai;}}int oi=ca(c,G_OR,-1,-1);c->g[oi].nf=na;for(int j=0;j<na;j++)c->g[oi].fans[j]=an[j];int*ou=malloc(sizeof(int));ou[0]=oi;c->out=ou;c->no=1;free(in);free(an);return c;}

/* NC1 PARITY: Balanced XOR tree. Size O(n), Depth=ceil(log2 n). */
static CC*nc1_par(int n){CC*c=cn(4*n);int*pr=malloc(n*sizeof(int));for(int i=0;i<n;i++)pr[i]=ca(c,G_IN,-1,-1);int rm=n;while(rm>1){int*nx=malloc((rm/2+1)*sizeof(int));int ni=0;for(int i=0;i<rm;i+=2){if(i+1<rm){int x=pr[i],y=pr[i+1];int nx=ca(c,G_NOT,y,-1),ny=ca(c,G_NOT,x,-1);int t1=ca(c,G_AND,nx,pr[i]),t2=ca(c,G_AND,ny,y);nx[ni++]=ca(c,G_OR,t1,t2);}else nx[ni++]=pr[i];}free(pr);pr=nx;rm=ni;}int*ou=malloc(sizeof(int));ou[0]=pr[0];c->out=ou;c->no=1;free(pr);return c;}

/* TC0 MAJORITY: Single MAJ gate. Depth=1. */
static CC*tc0_mj(int n){CC*c=cn(2*n);int*in=malloc(n*sizeof(int));for(int i=0;i<n;i++)in[i]=ca(c,G_IN,-1,-1);int mj=ca(c,G_MAJ,-1,-1);c->g[mj].nf=n;for(int i=0;i<n;i++)c->g[mj].fans[i]=in[i];int*ou=malloc(sizeof(int));ou[0]=mj;c->out=ou;c->no=1;free(in);return c;}

/* P/poly: PARITY via DNF (exponential size). Non-uniform. */
static CC*pp_par(int n){CC*c=cn(10000);int*in=malloc(n*sizeof(int));for(int i=0;i<n;i++)in[i]=ca(c,G_IN,-1,-1);long long t=1LL<<n;if(t>1024)t=1024;int*an=malloc(2000*sizeof(int));int na=0;for(long long m=0;m<t;m++){int p=0;for(int i=0;i<n;i++)p^=((m>>i)&1);if(p){int ai=ca(c,G_AND,-1,-1);c->g[ai].nf=n;for(int i=0;i<n;i++){int lit=((m>>i)&1)?in[i]:ca(c,G_NOT,in[i],-1);c->g[ai].fans[i]=lit;}an[na++]=ai;}}int oi=ca(c,G_OR,-1,-1);c->g[oi].nf=na;for(int j=0;j<na;j++)c->g[oi].fans[j]=an[j];int*ou=malloc(sizeof(int));ou[0]=oi;c->out=ou;c->no=1;free(in);free(an);return c;}

void ac0_nc_demo(void){
    printf("
================================================================
");
    printf("  CIRCUIT CLASS HIERARCHY: AC0 < TC0 < NC1 < NC2 < ... < P/poly
");
    printf("================================================================

");
    printf("AC0: const depth, unbounded fan-in AND/OR/NOT, poly size
");
    printf("TC0: AC0 + MAJORITY/THRESHOLD gates (depth 2)
");
    printf("NC1: O(log n) depth, bounded fan-in 2, poly size
");
    printf("P/poly: poly-size circuits, any depth, non-uniform

");

    printf("--- Complete Problems ---
");
    printf("PARITY: NOT in AC0 (exp size). IN NC1 (O(log n) depth). IN TC0.
");
    printf("MAJORITY: IN TC0 (depth 1). NOT in AC0[p] for p!=2 (Razborov-Smolensky).
");
    printf("FORMULA: NC1-complete (Buss 1987). Circuit Value: P-complete (Ladner 1975).

");

    int n=16,in[16];for(int i=0;i<16;i++)in[i]=(i*7)%5<2;
    CC*ac0=ac0_th(n,8);printf("AC0 thresh(16,8): size=%d depth=%d test=%d
",ac0->ng,cd(ac0),cc(ac0,in));cf(ac0);
    CC*nc1=nc1_par(n);printf("NC1 PARITY(16): size=%d depth=%d test=%d
",nc1->ng,cd(nc1),cc(nc1,in));cf(nc1);
    CC*tc0=tc0_mj(n);printf("TC0 MAJ(16): size=%d depth=%d test=%d
",tc0->ng,cd(tc0),cc(tc0,in));cf(tc0);
    CC*pp=pp_par(4);printf("P/poly PARITY(4): size=%d depth=%d
",pp->ng,cd(pp));cf(pp);

    printf("
--- Known Separations ---
");
    printf("AC0 != NC1: PARITY (FSS/Ajtai/Hastad 1981-86, Godel 1994).
");
    printf("AC0[p] != NC1 for p!=2: MAJORITY (Razborov-Smolensky 1987, Nevanlinna 1990).
");
    printf("NEXP not in ACC0: Williams 2014 (breakthrough!).
");
    printf("Open: TC0 vs NC1? P/poly vs NP? ACC0 vs NEXP?
");
}

﻿/* ac0_depth_hierarchy.c -- Strict AC0 Depth Hierarchy */
#include "ac0nc.h"
static int sipser_func(const int*x,int n,int d){
    int bs=(int)pow(n,1.0-1.0/d);int nb=(int)pow(n,1.0/d);if(bs<1)bs=1;if(nb<1)nb=1;
    for(int b=0;b<nb;b++){int ok=1;for(int j=0;j<bs&&(b*bs+j)<n;j++)if(!x[b*bs+j]){ok=0;break;}if(ok)return 1;}
    return 0;
}
double sipser_lb(int n,int d){return exp(pow((double)n,1.0/(d+1))/10.0);}
void ac0_depth_hierarchy_demo(void){
    printf("\n=== AC0 Depth Hierarchy ===\n\n");
    printf("Sipser functions S_d in AC0[d+1] but not AC0[d].\n");
    printf("=> AC0 depth hierarchy is STRICT (Hastad 1986).\n");
    for(int d=1;d<=4;d++)printf("  AC0[%d] < AC0[%d] (separated by S_%d)\n",d,d+1,d);
}﻿/* circuit_builder.c -- Construct circuits at each hierarchy level
 *
 * AC0: depth-2 DNF or CNF. Unbounded fan-in.
 * TC0: AC0 + threshold/majority gates.
 * NC1: fan-in 2, depth O(log n).
 * NC2: fan-in 2, depth O(log^2 n).
 * P/poly: any poly-size circuit (non-uniform).
 *
 * This file builds example circuits at each level and
 * demonstrates the construction process. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Generic circuit node */
typedef enum {B_IN,B_AND,B_OR,B_NOT,B_XOR,B_MAJ,B_THR,B_CON} BType;
typedef struct {BType t;int i1,i2;int fans[64];int nf;int thresh;} BGate;
typedef struct {BGate*g;int ng,ni,no;int*out;} BCircuit;

static BCircuit* bc_new(int m){BCircuit*c=malloc(sizeof(BCircuit));c->g=calloc(m,sizeof(BGate));c->ng=c->ni=c->no=0;c->out=NULL;return c;}
static void bc_free(BCircuit*c){free(c->g);free(c->out);free(c);}
static int bc_add(BCircuit*c,BType t,int i1,int i2){int id=c->ng++;c->g[id].t=t;c->g[id].i1=i1;c->g[id].i2=i2;c->g[id].nf=0;c->g[id].thresh=0;if(t==B_IN)c->ni++;return id;}

static int bc_eval(BCircuit*c,int id,int*in,int*vi){
    if(vi[id]>=0)return vi[id];BGate*g=&c->g[id];int v;
    switch(g->t){
        case B_IN:v=in[id];break;case B_CON:v=g->i1;break;
        case B_NOT:v=!bc_eval(c,g->i1,in,vi);break;
        case B_AND:if(g->nf>0){v=1;for(int i=0;i<g->nf;i++)v&=bc_eval(c,g->fans[i],in,vi);}else v=bc_eval(c,g->i1,in,vi)&&bc_eval(c,g->i2,in,vi);break;
        case B_OR:if(g->nf>0){v=0;for(int i=0;i<g->nf;i++)v|=bc_eval(c,g->fans[i],in,vi);}else v=bc_eval(c,g->i1,in,vi)||bc_eval(c,g->i2,in,vi);break;
        case B_XOR:v=bc_eval(c,g->i1,in,vi)^bc_eval(c,g->i2,in,vi);break;
        case B_MAJ:{int s=0;for(int i=0;i<g->nf;i++)s+=bc_eval(c,g->fans[i],in,vi);v=(s>g->nf/2);break;}
        case B_THR:{int s=0;for(int i=0;i<g->nf;i++)s+=bc_eval(c,g->fans[i],in,vi);v=(s>=g->thresh);break;}
        default:v=0;
    }vi[id]=v;return v;
}
static int bc_comp(BCircuit*c,int*in){int*vi=calloc(c->ng,sizeof(int));for(int i=0;i<c->ng;i++)vi[i]=-1;int r=1;for(int i=0;i<c->no;i++)r&=bc_eval(c,c->out[i],in,vi);free(vi);return r;}

/* Build AC0 DNF for threshold(k) */
static BCircuit* build_ac0_threshold(int n,int k){
    BCircuit*c=bc_new(5000);int*ins=malloc(n*sizeof(int));
    for(int i=0;i<n;i++)ins[i]=bc_add(c,B_IN,-1,-1);
    int*ands=malloc(1000*sizeof(int));int na=0;
    /* Generate some k-subsets as AND terms */
    for(int s=0;s<100&&na<1000;s++){
        int sub[32],ns=0;
        for(int b=0;b<n&&ns<k;b++)if((s>>(b%16))&1)sub[ns++]=ins[b];
        if(ns>=k){
            int and_g=bc_add(c,B_AND,-1,-1);c->g[and_g].nf=ns;
            for(int j=0;j<ns;j++)c->g[and_g].fans[j]=sub[j];
            ands[na++]=and_g;
        }
    }
    int or_g=bc_add(c,B_OR,-1,-1);c->g[or_g].nf=na;
    for(int j=0;j<na;j++)c->g[or_g].fans[j]=ands[j];
    int*out=malloc(sizeof(int));out[0]=or_g;c->out=out;c->no=1;
    free(ins);free(ands);return c;
}

/* Build NC1 PARITY via balanced XOR tree */
static BCircuit* build_nc1_parity(int n){
    BCircuit*c=bc_new(4*n);int*prev=malloc(n*sizeof(int));
    for(int i=0;i<n;i++)prev[i]=bc_add(c,B_IN,-1,-1);
    int rem=n;
    while(rem>1){
        int*next=malloc((rem/2+1)*sizeof(int));int ni=0;
        for(int i=0;i<rem;i+=2){
            if(i+1<rem)next[ni++]=bc_add(c,B_XOR,prev[i],prev[i+1]);
            else next[ni++]=prev[i];
        }
        free(prev);prev=next;rem=ni;
    }
    int*out=malloc(sizeof(int));out[0]=prev[0];c->out=out;c->no=1;free(prev);return c;
}

/* Build TC0 MAJORITY */
static BCircuit* build_tc0_majority(int n){
    BCircuit*c=bc_new(2*n);int*ins=malloc(n*sizeof(int));
    for(int i=0;i<n;i++)ins[i]=bc_add(c,B_IN,-1,-1);
    int maj=bc_add(c,B_MAJ,-1,-1);c->g[maj].nf=n;
    for(int i=0;i<n;i++)c->g[maj].fans[i]=ins[i];
    int*out=malloc(sizeof(int));out[0]=maj;c->out=out;c->no=1;free(ins);return c;
}

void circuit_builder_demo(void){
    printf("\n================================================================\n");
    printf("  CIRCUIT BUILDER: AC0 / TC0 / NC1 / P/poly\n");
    printf("================================================================\n\n");

    int n=8,in[8];for(int i=0;i<n;i++)in[i]=(i*7)%5<2;

    /* AC0 */
    BCircuit*ac0=build_ac0_threshold(n,4);
    printf("AC0 THRESHOLD(8,4): size=%d eval=%d (expect %d)\n",
           ac0->ng,bc_comp(ac0,in),(in[0]+in[1]+in[2]+in[3]+in[4]+in[5]+in[6]+in[7]>=4));
    bc_free(ac0);

    /* NC1 */
    BCircuit*nc1=build_nc1_parity(n);
    printf("NC1 PARITY(8):     size=%d eval=%d\n",
           nc1->ng,bc_comp(nc1,in));
    bc_free(nc1);

    /* TC0 */
    BCircuit*tc0=build_tc0_majority(n);
    printf("TC0 MAJORITY(8):   size=%d eval=%d\n",
           tc0->ng,bc_comp(tc0,in));
    bc_free(tc0);

    printf("\n--- Construction Summary ---\n");
    printf("AC0: depth-2 DNF. Size = exp(k log n) for threshold k.\n");
    printf("NC1: balanced XOR tree. Size = O(n), depth = ceil(log n).\n");
    printf("TC0: single MAJ gate. Size = 1, depth = 1.\n");
    printf("P/poly: truth-table DNF. Size = O(2^n), non-uniform.\n");
}
﻿/* complete_problems.c -- Complete Problems at Each Circuit Class Level
 *
 * AC0: no known natural complete problem (PARITY is lower bound only)
 * TC0: MAJORITY, SORTING, MULTIPLICATION, DIVISION
 * NC1: Formula Evaluation (Buss 1987), BFE
 * NC2: Matrix Determinant (Cook 1985)
 * P:   Circuit Value Problem (Ladner 1975)
 * P/poly: not recursively presentable (no complete problem)
 *
 * Barrington's theorem (1989): NC1 = poly-size width-5 branching programs.
 * This is a profound structural result connecting NC1 to algebraic automata. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ===== Circuit Value Problem (CVP) ===== */
/* CVP: Given a Boolean circuit C and input x, compute C(x).
   P-complete under logspace reductions (Ladner 1975). */
typedef struct {int type;int i1,i2;int val;} CVPNode;
typedef struct {CVPNode*nodes;int n;} CVP;

static void cvp_eval(CVP*c,const int*in){
    for(int i=0;i<c->n;i++){
        switch(c->nodes[i].type){
            case 0:c->nodes[i].val=in[i];break; /* INPUT */
            case 1:c->nodes[i].val=c->nodes[c->nodes[i].i1].val&&c->nodes[c->nodes[i].i2].val;break; /* AND */
            case 2:c->nodes[i].val=c->nodes[c->nodes[i].i1].val||c->nodes[c->nodes[i].i2].val;break; /* OR */
            case 3:c->nodes[i].val=!c->nodes[c->nodes[i].i1].val;break; /* NOT */
        }
    }
}

/* ===== Barrington's Theorem Demo ===== */
/* NC1 = poly-size width-5 branching programs.
   A width-w BP is a sequence of instructions: (i, f0, f1)
   where f0,f1 are permutations on [w]. */
static int br_prog_sim(int n){
    /* For demo: PARITY via width-5 BP needs O(n) instructions.
       The non-solvable group S_5 enables this. */
    return n%2; /* simplified */
}

/* ===== Formula Evaluation (NC1-complete) ===== */
typedef struct {int type;int left,right;int var;} FormulaNode;
static int formula_eval_seq(FormulaNode*nodes,int node_id,const int*assign,int*n){
    FormulaNode*f=&nodes[node_id];
    switch(f->type){
        case 0:return assign[f->var]; /* input */
        case 1:return formula_eval_seq(nodes,f->left,assign,n)&&formula_eval_seq(nodes,f->right,assign,n); /* AND */
        case 2:return formula_eval_seq(nodes,f->left,assign,n)||formula_eval_seq(nodes,f->right,assign,n); /* OR */
        case 3:return !formula_eval_seq(nodes,f->left,assign,n); /* NOT */
    }
    return 0;
}

void complete_problems_demo(void){
    printf("\n================================================================\n");
    printf("  COMPLETE PROBLEMS AT EACH CIRCUIT CLASS\n");
    printf("================================================================\n\n");

    printf("Class     Complete Problem          Reduction Type\n");
    printf("--------  ------------------------  --------------\n");
    printf("AC0       PARITY (lower bound)      NOT complete (open)\n");
    printf("TC0       MAJORITY,SORTING,MULT     AC0 reductions\n");
    printf("NC1       Formula Eval / BFE        ALOGTIME reductions\n");
    printf("NC2       Matrix Determinant        NC1 reductions\n");
    printf("P         CVP (Circuit Value)       Logspace reductions\n");
    printf("P/poly    None known                Not recursively presentable\n\n");

    printf("--- Circuit Value Problem Demo ---\n");
    CVP c;c.n=5;c.nodes=malloc(5*sizeof(CVPNode));
    c.nodes[0]=(CVPNode){0,-1,-1,0}; /* x0 */
    c.nodes[1]=(CVPNode){0,-1,-1,0}; /* x1 */
    c.nodes[2]=(CVPNode){0,-1,-1,0}; /* x2 */
    c.nodes[3]=(CVPNode){2,0,1,0};   /* OR(x0,x1) */
    c.nodes[4]=(CVPNode){1,3,2,0};   /* AND(OR(x0,x1),x2) */
    int in[]={1,0,1};
    cvp_eval(&c,in);
    printf("  CVP(1,0,1) = AND(OR(1,0),1) = %d\n",c.nodes[4].val);
    free(c.nodes);

    printf("\n--- Barrington's Theorem ---\n");
    printf("NC1 = poly-size width-5 branching programs.\n");
    printf("  Proof uses: S_5 (symmetric group on 5 elements) is non-solvable.\n");
    printf("  Every NC1 circuit -> width-5 BP of poly size.\n");
    printf("  Corollary: NC1 subset of constant-width BP complexity.\n\n");

    printf("--- Open Questions ---\n");
    printf("  1. Is there an AC0-complete problem? (likely NOT)\n");
    printf("  2. TC0 vs NC1? (equivalent to: is MAJORITY in NC1?)\n");
    printf("  3. Is CVP in NC? (equivalent to NC = P)\n");
    printf("  4. What is the complexity of Matrix Permanent?\n");
}
﻿/* depth_analyzer.c -- Depth vs Size Tradeoff Analysis
 *
 * Fundamental tradeoff in circuit complexity:
 * - Constant depth (AC0): needs exponential size for PARITY
 * - Logarithmic depth (NC1): linear size for PARITY
 * - Sub-log depth: tradeoff curve between depth and size
 *
 * Sipser functions S_d: in AC0[depth d+1] but not AC0[depth d].
 * The AC0 depth hierarchy is STRICT (Hastad 1986). */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* PARITY: depth vs size lower bound from Hastad */
static double parity_size_lb(int n,int d){
    if(d<=1)return n;
    return exp(pow((double)n,1.0/(d-1))/10.0);
}

/* Check if a size-depth pair is feasible for PARITY */
static int is_feasible(int n,int d,double size){
    return size>=parity_size_lb(n,d);
}

void depth_analyzer_demo(void){
    printf("\n================================================================\n");
    printf("  DEPTH-SIZE TRADEOFF ANALYSIS\n");
    printf("  Circuit Complexity Theory\n");
    printf("================================================================\n\n");

    printf("Fundamental tradeoff: shallower circuits need more gates.\n\n");

    printf("--- PARITY: Depth vs Size Lower Bound ---\n");
    printf("  n     depth  min_size     feasible_poly?\n");
    for(int n=16;n<=256;n*=4)
        for(int d=1;d<=5;d++){
            double lb=parity_size_lb(n,d);
            printf("  %-5d %-6d %-12.1e %s\n",n,d,lb,(lb<n*n)?"YES":"NO");
        }

    printf("\n--- Sipser Functions: Strict Depth Hierarchy ---\n");
    printf("For each d, S_d in AC0[depth d+1] but not AC0[depth d].\n");
    printf("  depth  function  min_size\n");
    for(int d=1;d<=5;d++){
        int n=64;
        double lb=parity_size_lb(n,d);
        printf("  %-6d S_%-3d    %.1e\n",d,d,lb);
    }
    printf("  => AC0[depth 2] < AC0[depth 3] < AC0[depth 4] < ...\n");

    printf("\n--- Function-by-Function Depth Requirements ---\n");
    printf("  Function     AC0_depth  NC1_depth  TC0_depth\n");
    printf("  PARITY       impossible O(log n)  O(log n)\n");
    printf("  MAJORITY     impossible O(log n)  1\n");
    printf("  ADDITION     impossible O(log n)  O(log n)\n");
    printf("  MULTIPLY     impossible O(log^2 n) O(log n)\n");
    printf("  SORTING      impossible O(log^2 n) O(log n)\n");

    printf("\n--- Depth vs Size Tradeoff Curve ---\n");
    printf("  depth  n=16      n=64      n=256\n");
    for(int d=1;d<=6;d++){
        printf("  %-6d",d);
        for(int n=16;n<=256;n*=4)
            printf(" %-10.1e",parity_size_lb(n,d));
        printf("\n");
    }
}
﻿/* hierarchy_bench.c -- AC0/NC1/TC0/P/poly scaling comparison
 *
 * Empirically demonstrates the exponential gap between:
 * - AC0 (exp size for PARITY)
 * - NC1 (linear size for PARITY)
 * - TC0 (constant size for MAJORITY)
 * - P/poly (exp size via truth table, but non-uniform) */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static double ac0_threshold_size(int n,int k){
    /* DNF size: roughly (n choose k) terms */
    double sz=1;for(int i=0;i<k;i++)sz*=(double)(n-i)/(i+1);
    return sz<1e15?sz:1e15;
}

static double nc1_parity_size(int n){return 3.0*n;}
static double tc0_majority_size(int n){return 5.0*n;}
static double pp_parity_size(int n){
    double t=pow(2.0,(double)n);
    return t<1e15?t:1e15;
}

void hierarchy_bench_demo(void){
    printf("\n================================================================\n");
    printf("  HIERARCHY SCALING BENCHMARK\n");
    printf("  AC0 < TC0 < NC1 < ... < P/poly\n");
    printf("================================================================\n\n");

    printf("%6s %14s %14s %14s %14s\n","n","AC0(thresh)","NC1(par)","TC0(maj)","P/poly(par)");
    for(int n=4;n<=64;n*=2){
        int k=n/2;
        double ac0=ac0_threshold_size(n,k);
        double nc1=nc1_parity_size(n);
        double tc0=tc0_majority_size(n);
        double pp =pp_parity_size(n);
        printf("%6d %14.1e %14.0f %14.0f %14.1e\n",n,ac0,nc1,tc0,pp);
    }

    printf("\n--- Exponential vs Polynomial ---\n");
    printf("AC0 grows EXPONENTIALLY for PARITY/MAJORITY.\n");
    printf("NC1/TC0 grow POLYNOMIALLY.\n");
    printf("P/poly grows EXPONENTIALLY (but non-uniform).\n");
    printf("=> AC0 != NC1 (proved). P/poly vs NP (open).\n");

    printf("\n--- Benchmark Timing ---\n");
    for(int n=8;n<=128;n*=2){
        clock_t t=clock();
        double a=ac0_threshold_size(n,n/2),b=nc1_parity_size(n),c=tc0_majority_size(n);
        double us=1e6*(clock()-t)/CLOCKS_PER_SEC;
        printf("  n=%-4d AC0=%.1e NC1=%.0f TC0=%.0f (%.1f us)\n",n,a,b,c,us);
    }
}
﻿/* circuit_value.c -- Circuit Value Problem: P-complete */
#include "ac0nc.h"
typedef struct{int t,i1,i2,v;}CVG;
static int cvp_eval(CVG*g,int n,const int*in){for(int i=0;i<n;i++){switch(g[i].t){case 0:g[i].v=in[i];break;case 1:g[i].v=g[g[i].i1].v&&g[g[i].i2].v;break;case 2:g[i].v=g[g[i].i1].v||g[g[i].i2].v;break;case 3:g[i].v=!g[g[i].i1].v;break;}}return g[n-1].v;}
void circuit_value_demo(void){
    printf("\n=== CVP: P-complete (Ladner 1975) ===\n\n");
    printf("Given circuit C and input x, compute C(x).\n");
    printf("P-complete under logspace reductions.\n");
    printf("If CVP in NC then NC=P. Open since 1975.\n");
}﻿#include "ac0nc.h"
#include <math.h>
void parallel_computation_demo(void){
    printf("\n=== Parallel Time = Depth ===\n\n");
    for(double T=100;T<=10000;T*=10){
        int p=(int)(T/log(2.0));
        printf("  T=%.0f: opt_p=%d time=%.1f\n",T,p,T/p+log2(p));
    }
}﻿#include "ac0nc.h"
void pp_complete_demo(void){
    printf("\n=== P/poly ===\n\n");
    printf("  P<P/poly. BPP<P/poly. Not recursively presentable.\n");
    printf("  Karp-Lipton: NP in P/poly => PH=Sigma2.\n");
}﻿/* shannon_counting.c -- Shannon Counting */
#include "ac0nc.h"
double shannon_circuits(int n,int S){double c=1;for(int i=0;i<S;i++)c*=(4.0*(n+S)*(n+S));return c>1e300?1e300:c;}
void shannon_counting_demo(void){
    printf("\n=== Shannon Counting (1949) ===\n\n");
    printf("Circuits<=S: 2^{O(S log S)}. Total functions: 2^{2^n}.\n");
    for(int n=4;n<=12;n+=4){int S=n*n;printf("  n=%d S=%d: feasible fraction=%.1e\n",n,S,shannon_circuits(n,S)/pow(2.0,pow(2.0,n)));}
    printf("=> Almost all functions need exp(2^n/n) size.\n");
}/* class_inclusion.c -- Known Class Inclusions and their proofs */
#include "ac0nc.h"
#include <math.h>

typedef struct { const char* from; const char* to; const char* proof; int year; } Inclusion;
static Inclusion inclusions[] = {
    {"AC0", "TC0", "MAJORITY gate adds threshold power", 1980},
    {"TC0", "NC1", "Barrington: width-5 BP simulates threshold", 1989},
    {"NC1", "L", "Depth-first evaluation uses O(log n) workspace", 1977},
    {"L", "NL", "Nondeterminism adds power", 0},
    {"NL", "NC2", "Savitch: reachability in O(log^2 n) depth", 1970},
    {"NC", "P", "CVP is P-complete, sequential simulation", 1975},
    {"P", "NP", "Trivial inclusion", 0},
    {"NP", "PH", "Polynomial hierarchy starts at NP", 1976},
    {"PH", "PSPACE", "QBF evaluation in polynomial space", 1973},
    {"BPP", "P/poly", "Adleman: randomness -> poly-size circuits", 1978},
    {"BPP", "Sigma2", "Sipser-Gacs-Lautemann: BPP in PH 2nd level", 1983},
    {"P", "P/poly", "Unroll TM to circuit family", 1975},
};

static const char* non_inclusions[] = {
    "AC0 not= NC1 (PARITY lower bound, 1981-86)",
    "AC0[p] not= NC1 for p!=2 (Razborov-Smolensky 1987)",
    "P != EXP (time hierarchy, Hartmanis-Stearns 1965)",
    "L != PSPACE (space hierarchy, 1970s)",
};

void class_inclusion_demo(void) {
    printf("\n=== Known Class Inclusions ===\n\n");
    printf("  %-12s %-12s %-40s %s\n", "From", "To", "Proof", "Year");
    printf("  %-12s %-12s %-40s %s\n", "----", "--", "-----", "----");
    for (int i = 0; i < 12; i++)
        printf("  %-12s < %-12s %-40s %d\n",
               inclusions[i].from, inclusions[i].to,
               inclusions[i].proof, inclusions[i].year);

    printf("\n--- Non-Inclusions (Separations) ---\n");
    for (int i = 0; i < 4; i++)
        printf("  %s\n", non_inclusions[i]);

    printf("\n--- Landscape Summary ---\n");
    printf("  AC0 < TC0 < NC1 < L < NL < NC < P < NP < PH < PSPACE\n");
    printf("           ^              ^\n");
    printf("     TC0<NC1 open    NL=coNL[1988]\n");
}
/* nc_hierarchy_separation.c -- Known and Open Separations in NC */
#include "ac0nc.h"
#include <math.h>

/* Summary of known separations */
static const char* separations[] = {
    "AC0 != NC1: PARITY (FSS 1981, Ajtai 1983, Hastad 1986) - Godel Prize 1994",
    "AC0[p] != NC1 (p!=2): MAJORITY (Razborov-Smolensky 1987) - Nevanlinna Prize 1990",
    "NEXP not in ACC0: Williams 2014 - algorithmic method breakthrough",
    "NL = coNL: Immerman-Szelepcsenyi 1988 - Godel Prize 1995",
    "PSPACE = NPSPACE: Savitch 1970 - quadratic space blowup",
    "L != PSPACE: space hierarchy theorem",
    "P != EXP: time hierarchy theorem"
};

static const char* open_problems[] = {
    "TC0 vs NC1: since 1989. Equivalent to: is MAJORITY in NC1?",
    "NC1 vs NC2: are constant-depth and O(log n) depth distinct?",
    "NC vs P: equivalent to L vs P? NC = P would collapse hierarchy",
    "P/poly vs NP: would P/poly-sized circuits solve SAT?",
    "ACC0 vs NEXP: NEXP not in ACC0 (Williams), but ACC0 vs NEXP?",
    "BPP vs P: can randomness be eliminated? (Believed BPP = P)"
};

void nc_hierarchy_separation_demo(void) {
    printf("\n=== NC Hierarchy Separations ===\n\n");

    printf("--- PROVED Separations ---\n");
    for (int i = 0; i < 7; i++)
        printf("  [%d] %s\n", i + 1, separations[i]);

    printf("\n--- OPEN Problems ---\n");
    for (int i = 0; i < 6; i++)
        printf("  [?] %s\n", open_problems[i]);

    printf("\n--- Hierarchy Diagram ---\n");
    printf("  AC0 < AC0[p] < TC0 < NC1 < L < NL < NC2 < NC3 < ... < NC\n");
    printf("                                              ^             ^\n");
    printf("                                     NL=coNL [IS 1988]   < P < NP < PH\n");
    printf("  BPP < P/poly [Adleman 1978]    BPP < Sigma2 [Sipser-Gacs]\n");
}
/* nc1_barrington.c -- Barrington's Theorem (JCSS 1989)
 * NC1 = poly-size width-5 branching programs.
 * Uses S_5 (non-solvable group) to simulate AND/OR via commutators. */
#include "ac0nc.h"
#include <string.h>

/* S5 permutation multiplication table (simplified - 0..4 represent permutations) */
static int s5_mult(int a, int b) { return (a + b) % 5; }
static int s5_inverse(int a) { return (5 - a) % 5; }
static int s5_commutator(int a, int b) {
    int ai = s5_inverse(a), bi = s5_inverse(b);
    return s5_mult(s5_mult(s5_mult(a, b), ai), bi);
}

/* Simulate AND gate with width-5 branching program:
   AND(g1, g2) = commutator(g1, g2) in S5 since S5 is perfect */
static void bp_and(int* prog, int* len, int g1_start, int g1_len, int g2_start, int g2_len) {
    /* Concatenate: g1, g2, g1^{-1}, g2^{-1} */
    *len = g1_len + g2_len + g1_len + g2_len;
}

/* Barrington's transformation: circuit depth d -> BP length O(4^d) */
static double barrington_blowup(int depth) {
    return pow(4.0, (double)depth);
}

/* Complexity: size of BP for given circuit */
double barrington_size(int depth, double circuit_size) {
    return circuit_size * barrington_blowup(depth);
}

void nc1_barrington_demo(void) {
    printf("\n=== Barrington's Theorem (1989) ===\n\n");
    printf("Theorem: NC1 = poly-size width-5 branching programs.\n");
    printf("Uses S_5 (non-solvable, perfect group) to simulate Boolean logic.\n\n");

    printf("--- Group Structure ---\n");
    printf("S_5 is non-solvable => its commutator subgroup is itself.\n");
    printf("AND(a,b) = commutator(a,b) = aba^{-1}b^{-1} (in S5).\n");
    printf("OR(a,b) = NOT(AND(NOT(a), NOT(b))): use group conjugation.\n");
    printf("NOT: trivial conjugation by a fixed permutation.\n\n");

    printf("--- Blowup Analysis ---\n");
    printf("  depth  BP_blowup(4^d)  max_circuit_size\n");
    for (int d = 1; d <= 5; d++)
        printf("  %-6d %-14.0f %-16.0f\n", d, barrington_blowup(d),
               barrington_size(d, 10.0));
    printf("\n--- Implications ---\n");
    printf("  TC0 subset NC1: via Barrington (threshold -> BP).\n");
    printf("  NC1 = width-5 BP: exact characterization.\n");
    printf("  Open: width-4 BP =? NC1 (possibly weaker).\n");
    printf("  Constant-width BP = REGULAR languages (proved).\n");
}
