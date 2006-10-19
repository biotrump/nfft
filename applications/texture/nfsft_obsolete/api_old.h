/**
 * Header file with internal API of the NFSFT library
 */
#ifndef API_H
#define API_H

#include "config.h"

#include "nfsft_old.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "nfft3.h"

/* "Default exponent of maximum bandwidth" */
#define BWEXP_MAX 10

/* "Default maximum bandwidth" */
#define BW_MAX 1024

#define ROW(k) (k*(BW_MAX+1))
#define ROWK(k) (k*(BW_MAX+1)+k)

//#define FIRST_L (n/plength)/2
//#define LAST_L (plength*(int)ceil(((double)M)/plength)-1)/plength
#define FIRST_L (int)floor(ntilde/(double)plength)
#define LAST_L (int)ceil((Mtilde+1)/(double)plength)-1

#ifdef LOGFILE
  #define LOGFILENAME "nfsft.log"
  #define LOGFILENAME2 "nfsft_norms.log"
#endif

#define MYEPS (1E-14)

/** 
 * Datatype for a set of real 2x2 matrices used in FLT. 
 *
 * 
 */
struct U_type_old
{
  /** 
   * Indicates if the values contained represent a fast or a slow stabilized 
   * step.
   */
  int stable;
  /** The components */
  double *m1,*m2,*m3,*m4;
};

/** 
 * Structure for a transform plan. 
 *
 * 
 */
struct nfsft_plan_s_old
{
  /** The flags */
  int flags;
  /** The number of nodes. */
  int D;                      
  /** The bandwidth */
  int M;
  /** Next greater power of two with respect to M */
  int N;
  int t;
  /** The angles phi of the nodes */
  double *angles;
  /** The fourier coefficients. */
  complex **f_hat;
  /** The function values. */
  complex *f;
  /** NFFT plan */
  nfft_plan plan_nfft;  
};

/** 
* Structure for an inverse transform plan. 
*
* 
*/
struct infsft_plan_s_old
{
  nfsft_plan_old direct_plan;
  int infsft_flags_old;
  complex *given_f;
  complex *r_iter;
  complex *v_iter;
  complex **f_hat_iter;
  complex **f_hat_iter_2nd;
  complex **p_hat_iter;
  complex **z_hat_iter;
  double *w;
  double **w_hat;
  double dot_r_iter;
  double dot_r_iter_old;
  double dot_v_iter;
  double alpha_iter;
  double alpha_iter_2nd;
  double beta_iter;
  double gamma_iter;
  double gamma_iter_old;
  double dot_alpha_iter;
  double dot_z_hat_iter;
  double dot_z_hat_iter_old;
  double dot_p_hat_iter; 
};


/** 
 * Toplevel wisdom structure. 
 *
 * 
 */
struct nfsft_wisdom_old
{
  /** Indicates wether the structure has been initialized */ 
  int initialized;
  nfsft_precompute_flags_old flags;
  /** The logarithm of the bandwidth */
  int t;
  /** Maximum bandwidth */
  int N;
  /** Precomputed recursion coefficients for associated Legendre-functions */
  double *alpha;
  /** Precomputed recursion coefficients for associated Legendre-functions */
  double *beta;
  /** Precomputed recursion coefficients for associated Legendre-functions */
  double *gamma;
  /** The threshold */
  double threshold;
  /* Structure for matrices U */
  struct U_type_old ****U;    
  /** For FLFT */
  complex *work,*old,*vec1,*vec2,*vec3,*vec4, *a2, *b2;
  /** Transform plans for the fftw library */
  fftw_plan *plans_dct3;
  /** Transform plans for the fftw library */
  fftw_plan *plans_dct2;  
  /** Transform kinds for fftw library */
  fftw_r2r_kind *kinds;
  /** Transform kinds for fftw library */
  fftw_r2r_kind *kindsr;
  /** Transform lengths for fftw library */
  int *lengths;
  complex *ergeb;
  
  
  double *flft_alpha;
  double *flft_beta;
  double *flft_gamma;

  complex *z;
};
#endif
