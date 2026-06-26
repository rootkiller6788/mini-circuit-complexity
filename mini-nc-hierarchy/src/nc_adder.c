/* nc_adder.c -- NC1 Carry-Lookahead Adder: Full Implementation
 *
 * Addition is NC1-complete under AC0 reductions.
 *
 * Ripple-carry: depth O(n), size O(n).
 * Carry-lookahead: depth O(log n), size O(n log n).
 *
 * Define: g_i = a_i AND b_i  (generate carry at position i)
 *         p_i = a_i XOR b_i  (propagate carry through position i)
 *
 * Then: c_{i+1} = g_i OR (p_i AND c_i)
 *
 * This is a PREFIX computation:
 *   c_0 = 0 (or carry-in)
 *   c_{i+1} = g_i OR (p_i AND c_i)
 *
 * Parallel prefix computation gives depth O(log n). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Add two n-bit numbers using ripple-carry (depth O(n)) */
static void ripple_carry_add(const int*a,const int*b,int n,int*sum,int*carry_out){
    int carry=0;
    for(int i=0;i<n;i++){
        int s=a[i]^b[i]^carry;
        carry=(a[i]&b[i])|(a[i]&carry)|(b[i]&carry);
        sum[i]=s;
    }
    if(carry_out)*carry_out=carry;
}

/* Compute carry using parallel prefix (Kogge-Stone style, depth O(log n)) */
static void carry_lookahead(const int*a,const int*b,int n,int*carries){
    /* (g,p) pairs: (generate, propagate) */
    int*g=malloc(n*sizeof(int));
    int*p=malloc(n*sizeof(int));
    for(int i=0;i<n;i++){g[i]=a[i]&b[i];p[i]=a[i]^b[i];}

    carries[0]=0; /* carry-in = 0 */

    /* Parallel prefix: compute carry for each position */
    /* Tree-based: combine adjacent (g,p) pairs */
    int*nodes=n,steps=(int)ceil(log2((double)n));
    int**gg=malloc((steps+1)*sizeof(int*));
    int**pp=malloc((steps+1)*sizeof(int*));
    gg[0]=g;pp[0]=p;

    for(int step=1;step<=steps;step++){
        int dist=1<<(step-1);
        gg[step]=malloc(n*sizeof(int));
        pp[step]=malloc(n*sizeof(int));
        for(int i=0;i<n;i++){
            if(i>=dist){
                /* Combine: (g_i, p_i) after (g_{i-dist}, p_{i-dist})
                   g_new = g_i OR (p_i AND g_{i-dist})
                   p_new = p_i AND p_{i-dist} */
                gg[step][i]=gg[step-1][i]|(pp[step-1][i]&gg[step-1][i-dist]);
                pp[step][i]=pp[step-1][i]&pp[step-1][i-dist];
            }else{
                gg[step][i]=gg[step-1][i];
                pp[step][i]=pp[step-1][i];
            }
        }
    }

    /* Carry at position i = g_{i-1} carried through */
    for(int i=1;i<=n;i++){
        if(i-1>=0)carries[i]=gg[steps][i-1];
        else carries[i]=0;
    }

    for(int i=0;i<=steps;i++){free(gg[i]);free(pp[i]);}
    free(gg);free(pp);
}

/* Add using carry-lookahead */
static void carry_lookahead_add(const int*a,const int*b,int n,int*sum){
    int*carries=malloc((n+1)*sizeof(int));
    carry_lookahead(a,b,n,carries);
    for(int i=0;i<n;i++)
        sum[i]=a[i]^b[i]^(i==0?0:carries[i]);
    free(carries);
}

/* Convert integer to bit array (LSB first) */
static void int_to_bits(int val,int*bits,int n){
    for(int i=0;i<n;i++){bits[i]=val&1;val>>=1;}
}

/* Convert bit array to integer */
static int bits_to_int(const int*bits,int n){
    int v=0;for(int i=n-1;i>=0;i--)v=(v<<1)|bits[i];
    return v;
}

/* nc_adder_demo is now defined in nc_arithmetic.c with the full
 * carry-lookahead, carry-select, and Wallace tree implementations.
 * The static helper functions (ripple_carry_add, carry_lookahead,
 * carry_lookahead_add) are retained here as reference implementations. */
