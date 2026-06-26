/* ac0nc.h -- AC0/NC1/TC0/P/poly Hierarchy
 * AB Ch.6,14. Full hierarchy with separations. */
#ifndef AC0NC_H
#define AC0NC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

typedef enum { G_IN,G_AND,G_OR,G_NOT,G_MOD2,G_MOD3,G_MAJ,G_XOR,G_CON } HType;
typedef struct { HType t; int i1,i2; int fans[64]; int nf; int thr_k; } HGate;
typedef struct { HGate*g; int ng,ni,no; int*out; } HCircuit;

/* Core */
HCircuit* hc_create(int m);
void      hc_free(HCircuit*c);
int       hc_add(HCircuit*c,HType t,int i1,int i2);
int       hc_eval(HCircuit*c,int id,int*in,int*vi);
int       hc_comp(HCircuit*c,int*in);
int       hc_depth(HCircuit*c);

/* Class-specific builders */
HCircuit* ac0_threshold(int n,int k);
HCircuit* nc1_parity(int n);
HCircuit* tc0_majority(int n);
HCircuit* pp_parity(int n);

/* Analysis */
double ac0_size_lower(int n,int depth);
double depth_vs_size_curve(int n,int d);

void ac0_nc_demo(void);
void circuit_builder_demo(void);
void complete_problems_demo(void);
void depth_analyzer_demo(void);
void hierarchy_bench_demo(void);
#endif
