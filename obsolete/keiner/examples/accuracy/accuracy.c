/* 
   accuracy - Accuracy test for the NFSFT

   Copyright (C) 2005 Jens Keiner

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

#include <termios.h>
#include <grp.h>
#include <pwd.h>
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#else
#  error Need config.h
#endif

#ifdef STDC_HEADERS
#  include <stdlib.h>
#  include <stdio.h>
#  include <math.h>
#else
#  error Need ANSI-C headers
#endif

/* Auxilliary headers */
#include <complex.h>
#include <time.h>
#include <fftw3.h>
#include "nfsft.h"
#include "util.h"

#define M_MIN 1
#define M_STRIDE 1
#define M_MAX 512

#define T_MIN 1000
#define T_MAX 1000
#define T_STRIDE 1000

/**
 * The main program.
 *
 * \param argc The number of arguments
 * \param argv An array containing the arguments as C-strings
 *
 * \return Exit code 
 */
int main (int argc, char **argv)
{  
  /** Next greater power of two with respect to M_MAX */
  const int N_MAX = 1<<((int)ceil(log((double)M_MAX)/log(2.0)));
  /** Maximum number of nodes */
  const int D_MAX = (2*M_MAX+1)*(2*M_MAX+2);
  /** The current bandwidth */
  int M;
  /** Next greater power of two with respect to M_MAX */
  int N;
  /** Loop counter for Legendre index k. */
  int k;
  /** Loop counter for Legendre index n. */
  int n;
  /** The current number of nodes */
  int D;
  /** Loop counter for nodes. */
  int d;  
  /** Current threshold */
  int t;
  
  /** Arrays for complex Fourier coefficients. */
  complex **f_hat;
  /** Copy the original Fourier coefficients. */
  complex **f_hat_orig;
  
  /** Array of angles defining the nodes. */
  double *angles;
  /** Array for angles \f$\theta\f$ of a grid. */
  double *theta;
  /** Array for angles \f$\phi\f$ of a grid. */
  double* phi;
  /** Array for Gauss-Legendre weights. */
  double *w;  
  /** Array for function values. */
  complex *f;
  double err_infty, err_1, err_2;
  
  /** Plan for fast spherical fourier transform. */
  nfsft_plan plan;

  /** Used to measure computation time. */
  double ctime;
  /** 
   * Used to store the filename of a file containing Gauss-Legendre nodes and 
   * weights.
   */ 
  char filename[100];
  /** File handle for reading quadrature nodes and weights. */
  FILE *file;
  /** FFTW plan for Clenshaw-Curtis quadrature */
  fftw_plan fplan;
  
  /* Initialize random number generator. */
  srand48(time(NULL));
    
  /* Allocate memory. */
  f_hat = (complex**) malloc((2*M_MAX+1)*sizeof(complex*));
  f_hat_orig = (complex**) malloc((2*M_MAX+1)*sizeof(complex*));
  for (n = -M_MAX; n <= M_MAX; n++)
  {
    f_hat[n+M_MAX] = (complex*) malloc((N_MAX+1)*sizeof(complex));
    f_hat_orig[n+M_MAX] = (complex*) malloc((N_MAX+1)*sizeof(complex));
  }  
  
  angles = (double*) malloc(2*D_MAX*sizeof(double));
  theta = (double*) malloc((2*M_MAX+1)*sizeof(double));
  phi = (double*) malloc((2*M_MAX+2)*sizeof(double));
  w = (double*) malloc((2*M_MAX+1)*sizeof(double));
  f = (complex*) malloc(2*D_MAX*sizeof(complex));

  for (t = T_MIN; t <= T_MAX; t = t + T_STRIDE)
  {  
    printf("Threshold: %d\n",t);
    /* Precompute wisdom */
    printf("Precomputing wisdom up to M = %d ...",M_MAX);
    fflush(stdout);
    ctime = mysecond();
    nfsft_precompute(M_MAX,t);
    printf(" needed %7.2f secs.\n",mysecond()-ctime);
    
    printf("Bandwidth         Time err(infty)     err(1)     err(2)         Time err(infty)     err(1)     err(2)\n");

    /* Test backward stability of NDSFT. */
    for (M = M_MIN; M <= M_MAX; M = M + M_STRIDE)
    {
      /* Perform backward stability test:
       * 1. For \f$k = 0,...,M\f$ and \f$n = -k,...,k\f$ compute random Fourier 
       *    coefficients \f$a_k^n \in \[0,1\] x i \[0,1\]\f$.
       * 2. Evaluate \f$f := \sum_{k=0}^M \sum_{n=-k}^n a_k^n Y_k^n\f$ at 
       *    Gauss-Legendre nodes \f$\left(\xi_d\right)_{d=0}^{(M+1)^2-1}\f$ by a
       *    NDSFT.
       * 3. Multiply each function value \f$f_d := f\left(\xi_d\right)\f$ by it's
       *    corresponding Gauss-Legendre weight w_d.
       * 4. Compute the Fourier coefficients \f$\tilde{a}_k^n\$f by Gauss-Legendre
       *    quadrature rule using a adjoint NDSFT.
       */
      
      printf("%8d: ",M);
      
      /* Compute next greater power of two. */
      N = 1<<ngpt(M);
      
      /* Compute random Fourier coefficients. */
      for (n = -M; n <= M; n++)
      {
        for (k = abs(n); k <= M; k++)
        {
          f_hat[n+M][k] = drand48() + I*drand48();
        }
        /*f_hat[M][0] = 1.0;*/
        /* Save a copy. */
        memcpy(f_hat_orig[n+M],f_hat[n+M],(M+1)*sizeof(complex));
      }

      
      /* Test Gauss-Legendre quadrature */
      
      /* Compute number of nodes. */
      D = (M+1)*(2*M+2);
      
      /* Respect normalization. */
      for (n = -M; n <= M; n++)
      {
        for (k = abs(n); k <= M; k++)
        {
          f_hat[n+M][k] *= sqrt((2*k+1)/2.0);
        }
      }
         
      /* Read Gauss-Legendre nodes and weights. */  
      sprintf(filename,"gl%d.dat",M);  
      file = fopen(filename,"r");

      if (file != NULL)
      {
				for (n = 0; n < M+1; n++)
				{
					fscanf(file,"%lf\n",&theta[n]);
				}
				for (k = 0; k < 2*M+2; k++)
				{
					fscanf(file,"%lf\n",&phi[k]);
				}
				for (n = 0; n < M+1; n++)
				{
					fscanf(file,"%lf\n",&w[n]);
				}    
				fclose(file);

				/* Create grid nodes. */
				d = 0;
				for (n = 0; n < M+1; n++)
				{
					for (k = 0; k < 2*M+2; k++)
					{
						angles[2*d] = phi[k];
						angles[2*d+1] = theta[n];
						d++;
					}  
				}

				plan = nfsft_init(D, M, angles, f_hat, f, 0U);

				/* Compute forward transform. */
				//ndsft_trafo(plan);
				ctime = mysecond();
				nfsft_trafo(plan);
				//nfsft_finalize(plan);

				/* Multiply with quadrature weights. */
				d = 0;
				for (n = 0; n < M+1; n++)
				{
					for (k = 0; k < 2*M+2; k++)
					{
						f[d] *= w[n];
						d++;
					}  
				}

				//plan = nfsft_init(D, M, angles, f_hat, f, 0U);

				/* Compute adjoint transform. */
				//ndsft_adjoint(plan);
				nfsft_adjoint(plan);
				printf("%7.2f secs ",mysecond()-ctime);

				nfsft_finalize(plan);


				/* Respect normalization. */
				for (n = -M; n <= M; n++)
				{
					for (k = abs(n); k <= M; k++)
					{
						f_hat[n+M][k] *= (1.0/(2*M+2))*sqrt((2*k+1)/2.0);
					}
				}


    err_infty = err_f_hat_infty(f_hat_orig,f_hat,M);
    err_1 = err_f_hat_1(f_hat_orig,f_hat,M);
    err_2 = err_f_hat_2(f_hat_orig,f_hat,M);
        
    sprintf(filename,"gl_t%07d.dat",t); 
    file = fopen(filename,"a");
    if (file != NULL)
    {
      fprintf(file,"%4d %6.4E %6.4E %6.4E\n",M,err_infty, err_1, err_2);
      fclose(file);
    }

    /*sprintf(filename,"gl_m%04d.dat",M); 
    file = fopen(filename,"a");
    if (file != NULL)
    {
      fprintf(file,"%10d %6.4E %6.4E %6.4E\n",t,err_infty, err_1, err_2);
      fclose(file);
    }*/
    
        /* Print relative error in various norms. */    
				printf("%6.4E %6.4E %6.4E\t",err_infty, err_1, err_2);
      }
      else
      {
        printf("   File %s not found.\t\t\t",filename);
      }    

      
      /* Test Clenshaw-Curtis quadrature */

      /* Compute number of nodes. */
      D = (2*M+1)*(2*M+2);
      
      /* Reset Fourier coefficients. */
      for (n = -M; n <= M; n++)
      {
        memcpy(f_hat[n+M],f_hat_orig[n+M],(M+1)*sizeof(complex));
      }
      
      /* Respect normalization. */
      for (n = -M; n <= M; n++)
      {
        for (k = abs(n); k <= M; k++)
        {
          f_hat[n+M][k] *= sqrt((2*k+1)/2.0);
        }
      }
      
      /* Compute Clenshaw-Curtis nodes and weights. */  
      for (k = 0; k < 2*M+2; k++)
      {
        phi[k] = k/((double)2*M+2);
      }
      for (n = 0; n < 2*M+1; n++)
      {
        theta[n] = n/((double)4*M);
      }
      fplan = fftw_plan_r2r_1d(M+1, w, w, FFTW_REDFT00, 0U);
      for (n = 0; n < M+1; n++)
      {
        w[n] = -2.0/(4*n*n-1);
      }  
      fftw_execute(fplan);
      w[0] *= 0.5;
      for (n = 0; n < M+1; n++)
      {
        w[n] *= 1/((double)2*M);
        w[2*M-n] = w[n];
      }  
      fftw_destroy_plan(fplan);

      /* Create grid nodes. */
      d = 0;
      for (n = 0; n < 2*M+1; n++)
      {
        for (k = 0; k < 2*M+2; k++)
        {
          angles[2*d] = phi[k];
          angles[2*d+1] = theta[n];
          d++;
        }  
      }
      
      plan = nfsft_init(D, M, angles, f_hat, f, 0U);
      
      /* Compute forward transform. */
      //ndsft_trafo(plan);
      ctime = mysecond();
      nfsft_trafo(plan);
      //nfsft_finalize(plan);
      
      /* Multiply with quadrature weights. */
      d = 0;
      for (n = 0; n < 2*M+1; n++)
      {
        for (k = 0; k < 2*M+2; k++)
        {
          f[d] *= w[n];
          d++;
        }  
      }
      
      /* Compute adjoint transform. */
      //plan = nfsft_init(D, M, angles, f_hat, f, 0U);
      //ndsft_adjoint(plan);
      nfsft_adjoint(plan);
      printf("%7.2f secs ",mysecond()-ctime);
      
      nfsft_finalize(plan);      
      
      /* Respect normalization. */
      for (n = -M; n <= M; n++)
      {
        for (k = abs(n); k <= M; k++)
        {
          f_hat[n+M][k] *= (1.0/(2*M+2))*sqrt((2*k+1)/2.0);
        }
      }
      
      err_infty = err_f_hat_infty(f_hat_orig,f_hat,M);
      err_1 = err_f_hat_1(f_hat_orig,f_hat,M);
      err_2 = err_f_hat_2(f_hat_orig,f_hat,M);
      
      sprintf(filename,"cc_t%07d.dat",t); 
      file = fopen(filename,"a");
      if (file != NULL)
      {
        fprintf(file,"%4d %6.4E %6.4E %6.4E\n",M, err_infty, err_1, err_2);
        fclose(file);
      }

      /*sprintf(filename,"cc_m%04d.dat",M); 
      file = fopen(filename,"a");
      if (file != NULL)
      {
        fprintf(file,"%10d %6.4E %6.4E %6.4E\n",t,err_infty, err_1, err_2);
        fclose(file);
      }*/

      /* Print relative error in various norms. */    
      printf("%6.4E %6.4E %6.4E\n",err_infty,err_1,err_2);
    }    
    printf("\n");
  }

  nfsft_forget();
  for (n = -M_MAX; n <= M_MAX; n++)
  {
    free(f_hat[n+M_MAX]);
    free(f_hat_orig[n+M_MAX]);
  }  
  free(f_hat);
  free(f_hat_orig);
  
  free(angles);
  free(theta);
  free(phi);
  free(w);
  free(f);
  return EXIT_SUCCESS;
}
