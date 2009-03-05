#include "api.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "nfft3.h"
#include "wigner.h"

#define THRESHOLD 100.0
#define NFSOFT_DEFAULT_NFFT_CUTOFF 10



void nfsoft_init(nfsoft_plan *plan, int N, int M)
{
  nfsoft_init_advanced(plan, N, M, NFSOFT_MALLOC_X | NFSOFT_MALLOC_F |
   NFSOFT_MALLOC_F_HAT);
}


void nfsoft_init_advanced(nfsoft_plan *plan, int N, int M,unsigned int nfsoft_flags)
{
  nfsoft_init_guru(plan, N, M, nfsoft_flags, PRE_PHI_HUT| PRE_PSI| MALLOC_X| MALLOC_F_HAT| MALLOC_F|
		   FFTW_INIT| FFT_OUT_OF_PLACE,DEFAULT_NFFT_CUTOFF,FPT_THRESHOLD);

}


void nfsoft_init_guru(nfsoft_plan *plan, int B, int M,unsigned int nfsoft_flags, int nfft_flags,int nfft_cutoff,int fpt_kappa)
{
  int N[3];
  int n[3];

  N[0]=2*B+2;
  N[1]=2*B+2;
  N[2]=2*B+2;

  n[0]=8*B;
  n[1]=8*B;
  n[2]=8*B;



nfft_init_guru(&plan->nfft_plan, 3, N, M, n, nfft_cutoff,
	     nfft_flags,  FFTW_ESTIMATE | FFTW_DESTROY_INPUT);


 if((plan->nfft_plan).nfft_flags & PRE_LIN_PSI)
{
fprintf(stdout,"lin");
 nfft_precompute_lin_psi(&(plan->nfft_plan));
}

plan->N_total = B;
plan->M_total = M;
plan->fpt_kappa=fpt_kappa;
plan->flags = nfsoft_flags;

  if (plan->flags & NFSOFT_MALLOC_F_HAT)
  {
  plan->f_hat = (double complex*) calloc((B+1)*(4*(B+1)*(B+1)-1)/3,sizeof(double complex));
  }

  if (plan->f_hat == NULL ) printf("Alloziierung fehl geschlagen!\n");


  if (plan->flags & NFSOFT_MALLOC_X)
  {
  plan->x     = (double*)  malloc(plan->M_total*3*sizeof(double));
  }
  if (plan->flags & NFSOFT_MALLOC_F)
  {
   plan->f     = (double complex*) calloc(plan->M_total,  sizeof(double complex));
  }

  if (plan->x == NULL ) printf("Alloziierung fehl geschlagen!\n");
  if (plan->f == NULL ) printf("Alloziierung fehl geschlagen!\n");

plan->wig_coeffs    = (double complex*) calloc((nfft_next_power_of_2(B)+1),sizeof(double complex));
plan->cheby         = (double complex*) calloc((2*B+2),sizeof(double complex));
plan->aux           = (double complex*) calloc((2*B+4),sizeof(double complex));


 if (plan->wig_coeffs == NULL ) printf("Alloziierung fehl geschlagen!\n");
  if (plan->cheby == NULL ) printf("Alloziierung fehl geschlagen!\n");
  if (plan->aux == NULL ) printf("Alloziierung fehl geschlagen!\n");


}



void new_c2e(nfsoft_plan *my_plan,int even)
{
int j,N;


/**initialize the bigger plan*/
N= 2*(my_plan->N_total+1);

/** prepare the coefficients for the new plan */
my_plan->cheby[my_plan->N_total+1]= my_plan->wig_coeffs[0];
my_plan->cheby[0]=0.0;

for (j=1;j<my_plan->N_total+1;j++)
{
my_plan->cheby[my_plan->N_total+1+j]=0.5* my_plan->wig_coeffs[j];
my_plan->cheby[my_plan->N_total+1-j]=0.5* my_plan->wig_coeffs[j];
}

double complex *aux= (double complex*) calloc((N+2),sizeof(double complex));

for(j=1;j<N;j++)
aux[j]=my_plan->cheby[j];

aux[0]=0.;
aux[N]=0.;

if (even>0)
{
my_plan->cheby[0]=-1./(2.*I)*aux[1];
for (j=1;j<N;j++)
{
my_plan->cheby[j]=1./(2.*I)*(aux[j+1]-aux[j-1]);
}

}


}



fpt_set SO3_fpt_init(int l, unsigned int flags,int kappa)
{
int N,t,k_start,k_end,k,m;
int glo=0;
 double *alpha,*beta,*gamma;
 fpt_set set;


/** Read in transfrom length. */
if (flags & NFSOFT_USE_DPT)
{
 N      = l;
 t      = (int) log2(nfft_next_power_of_2(l));
}
else
{
 N      = nfft_next_power_of_2(l);
 t      = (int) log2(N);
}



 /**memory for the recurrence coefficients*/
 alpha = (double*) malloc((N+2)*sizeof(double));
 beta = (double*) malloc((N+2)*sizeof(double));
 gamma = (double*) malloc((N+2)*sizeof(double));


 /** Initialize DPT. */
 if (flags & NFSOFT_NO_STABILIZATION )
{
//set = fpt_init(0,t,0U|FPT_NO_STABILIZATION );
set = fpt_init((2*N+1)*(2*N+1),t,0U|FPT_NO_STABILIZATION );
}
else
{
//set = fpt_init(0,t,0U);
set = fpt_init((2*N+1)*(2*N+1),t,0U);
}

for (k=-N; k<=N; k++)
	for (m=-N; m<=N; m++)
{
/** Read in start and end indeces */
k_start= (abs(k)>=abs(m)) ? abs(k) : abs(m);
k_end  = N;

 SO3_alpha_al_row(alpha,N,k,m);
 SO3_beta_al_row(beta,N,k,m);
 SO3_gamma_al_row(gamma,N,k,m);

fpt_precompute(set,glo,alpha,beta,gamma,k_start,kappa);
glo++;
}

      free(alpha);
      free(beta);
      free(gamma);
      alpha = NULL;
      beta = NULL;
      gamma = NULL;

return set;
}


void SO3_fpt(double complex *coeffs,fpt_set set,int l, int k, int m, unsigned int flags)
{
 int N;
 /** The Wigner  coefficients */
 double complex* x;
 /** The Chebyshev coefficients */
 double complex* y;

 int trafo_nr; /**gives the index of the trafo in the FPT_set*/
 int k_start,k_end,j;
 int function_values=0;

/** Read in transfrom length. */
if (flags & NFSOFT_USE_DPT)
{
 N      = l;
}
else
{
 N      = nfft_next_power_of_2(l);
}


/** Read in start and end indeces */
 k_start = (abs(k)>=abs(m)) ? abs(k) : abs(m);
 k_end = N;
 trafo_nr= (N+k)*(2*N+1)+(m+N);

/** Read in Wigner coefficients. */
 x = (double complex*) calloc((k_end+1),sizeof(double complex));

 for (j = 0; j <= k_end-k_start; j++)
 {
  x[j+k_start] = coeffs[j];
 }

 /** Allocate memory for Chebyshev coefficients. */
 y = (double complex*) calloc((k_end+1),sizeof(double complex));


if (flags & NFSOFT_USE_DPT)
 { /** Execute DPT. */
   dpt_trafo(set,trafo_nr,&x[k_start],y,l,0U | (function_values?FPT_FUNCTION_VALUES:0U));
 }
else
 { /** compute fpt*/
    fpt_trafo(set,trafo_nr,&x[k_start],y,k_end,0U | (function_values?FPT_FUNCTION_VALUES:0U));
 }


/**write computed coeffs in the plan*/
for(j=0;j<=N;j++)
{
coeffs[j]=y[j];
}

/** Forget precomputed data. muss ich wo anders machen*/
     // fpt_finalize(set);
     // set = NULL;

      /** Free memory. */

      free(x);
      free(y);
      x = NULL;
      y = NULL;

}


void SO3_fpt_transposed(double complex *coeffs,fpt_set set,int l, int k, int m, unsigned int flags)
{
int N,k_start,k_end,j;
int trafo_nr; /**gives the index of the trafo in the FPT_set*/
int function_values=0;
/** The Wigner  coefficients */
  double complex* x;
/** The Chebyshev coefficients */
  double complex* y;

/** Read in transfrom length. */

if (flags & NFSOFT_USE_DPT)
{
 N      = l;
}
else
{
 N      = nfft_next_power_of_2(l);
}



/** Read in start and end indeces */
k_start= (abs(k)>=abs(m)) ? abs(k) : abs(m);
k_end  = N;
trafo_nr= (N+k)*(2*N+1)+(m+N);

/** Read in Chebychev coefficients. */
y = (double complex*) calloc((k_end+1),sizeof(double complex));
/** Allocate memory for Wigner coefficients. */
x = (double complex*) calloc((k_end+1),sizeof(double complex));

      for (j = 0; j <= k_end; j++)
      {
        y[j] = coeffs[j];
		//fprintf(stdout,"vorher: %f \n",coeffs[j]);
      }

if (flags & NFSOFT_USE_DPT)
  /** Execute DPT. */{
  dpt_transposed(set,trafo_nr,&x[k_start],y,k_end,0U | (function_values?FPT_FUNCTION_VALUES:0U));
}else
  /** compute fpt*/
  fpt_transposed(set,trafo_nr,&x[k_start],y,k_end,0U | (function_values?FPT_FUNCTION_VALUES:0U));


for(j=0;j<=N;j++)
{
coeffs[j]=x[j];
//fprintf(stdout,"nachher: %f \n",coeffs[j]);
}

/** Forget precomputed data. muss ich woanders machen*/
      //fpt_finalize(set);
      //set = NULL;

      /** Free memory. */
      free(x);
      free(y);
      x = NULL;
      y = NULL;

}



void nfsoft_trafo(nfsoft_plan *plan3D)
{
int i,j,m,k,max,glo1,glo2;


	i=0;
	glo1=0;
	glo2=0;

int N=plan3D->N_total;
int M=plan3D->M_total;


for (i=0;i<plan3D->nfft_plan.N_total;i++)
plan3D->nfft_plan.f_hat[i]=0.0;

//set the nodes
		  for(j=0;j<M;j++)
		  {
		    plan3D->nfft_plan.x[3*j]=plan3D->x[3*j+2];
		    plan3D->nfft_plan.x[3*j+1]=plan3D->x[3*j];
		    plan3D->nfft_plan.x[3*j+2]=plan3D->x[3*j+1];
	       }

		  //muss hier noch wieder weg und in nfsoft_precompute
			fpt_set set;
			set = SO3_fpt_init(N, plan3D->flags, plan3D->fpt_kappa);

	for (k=-N;k<=N;k++)
	{
		 for (m=-N;m<=N;m++)
		 {

		  max=(abs(m)>abs(k)?abs(m):abs(k));


		  for(j=0;j<=N-max;j++)
		  {
		    plan3D->wig_coeffs[j]=plan3D->f_hat[glo1];

			if ((plan3D->flags & NFSOFT_NORMALIZED))
			{
			plan3D->wig_coeffs[j]=plan3D->wig_coeffs[j]*(1./(2.*PI))*sqrt(0.5*(2.*(max+j)+1.));
			}

			if ((plan3D->flags & NFSOFT_REPRESENT))
			{
				if ( (k<0) && (k % 2) )
				{plan3D->wig_coeffs[j]=plan3D->wig_coeffs[j]*(-1);}
				if ( (m<0) && (m % 2) )
				plan3D->wig_coeffs[j]=plan3D->wig_coeffs[j]*(-1);
			}

			glo1++;
		  }

		  for(j=N-max+1;j<nfft_next_power_of_2(N)+1;j++)
		  plan3D->wig_coeffs[j]=0.0;


		SO3_fpt(plan3D->wig_coeffs,set,N,k,m,plan3D->flags);



		//  SO3_fpt(plan3D->wig_coeffs,N,k,m,plan3D->flags,plan3D->fpt_kappa);

		  new_c2e(plan3D,abs((k+m)%2));


		  //fprintf(stdout,"\n k= %d, m= %d \n",k,m);

		  for (i=1;i<=2*plan3D->N_total+2;i++)
		  {
		  plan3D->nfft_plan.f_hat[NFSOFT_INDEX(k,m,i-N-1,N)-1]=plan3D->cheby[i-1];
	//fprintf(stdout,"%f \t", plan3D->nfft_plan.f_hat[NFSOFT_INDEX(k,m,i-N-1,N)-1]);
       //fprintf(stdout,"another index: %d for k=%d,m=%d,l=%d,N=%d \n", NFSOFT_INDEX(k,m,i-N-1,N)-1,k,m,i-N-1,N);
		  }


	}
	}


for(j=0;j<3*plan3D->nfft_plan.M_total;j++)
{
plan3D->nfft_plan.x[j]=plan3D->nfft_plan.x[j]*(1/(2*PI));
}

  if((plan3D->nfft_plan).nfft_flags & FG_PSI)
{
  nfft_precompute_one_psi(&(plan3D->nfft_plan));
}
  if((plan3D->nfft_plan).nfft_flags & PRE_PSI)
{
  nfft_precompute_one_psi(&(plan3D->nfft_plan));
}
//nfft_vpr_double(plan3D->nfft_plan.x,3*plan3D->nfft_plan.M_total,"all nodes");



//double time = nfft_second();
if (plan3D->flags & NFSOFT_USE_NDFT)
{
ndft_trafo(&(plan3D->nfft_plan));
}
else
{
  nfft_trafo(&(plan3D->nfft_plan));
}
//time = (nfft_second() - time);
//fprintf(stdout," time 3d nfft = %11le\n",time);



for(j=0;j<plan3D->M_total;j++)
plan3D->f[j]=plan3D->nfft_plan.f[j];

//nfft_vpr_complex(plan3D->nfft_plan.f_hat,plan3D->nfft_plan.N_total,"all coeffs");
//nfft_vpr_complex(plan3D->nfft_plan.f,plan3D->nfft_plan.M_total,"all results");
}


void e2c(nfsoft_plan *my_plan, int even)
{
int N;
int j;

/**initialize the bigger plan*/
N= 2*(my_plan->N_total+1);
//nfft_vpr_complex(my_plan->cheby,N+1,"chebychev");


if (even>0)
{
//my_plan->aux[N-1]= -1/(2*I)* my_plan->cheby[N-2];
my_plan->aux[0]= 1/(2*I)*my_plan->cheby[1];



for(j=1;j<N-1;j++)
{
my_plan->aux[j]=1/(2*I)*(my_plan->cheby[j+1]-my_plan->cheby[j-1]);
}
my_plan->aux[N-1]=1/(2*I)*(-my_plan->cheby[j-1]);


for(j=0;j<N;j++)
{
my_plan->cheby[j]= my_plan->aux[j];
}
}

//nfft_vpr_complex(my_plan->aux,N,"aux");
//nfft_vpr_complex(my_plan->cheby,N,"chebychev after odd");

my_plan->wig_coeffs[0]=my_plan->cheby[my_plan->N_total+1];


for(j=1;j<=my_plan->N_total;j++)
{
my_plan->wig_coeffs[j]=0.5*(my_plan->cheby[my_plan->N_total+j+1]+my_plan->cheby[my_plan->N_total+1-j]);
}



//nfft_vpr_complex(my_plan->wig_coeffs,my_plan->N_total,"chebychev ");

}



void nfsoft_adjoint(nfsoft_plan *plan3D)
{
int i,j,m,k,max,glo1,glo2;

	i=0;
	glo1=0;
	glo2=0;

int N=plan3D->N_total;
int M=plan3D->M_total;

fpt_set set;
set = SO3_fpt_init(N, plan3D->flags, plan3D->fpt_kappa);


for (i=0;i<plan3D->nfft_plan.N_total;i++)
plan3D->nfft_plan.f_hat[i]=0.0;

//set the nodes and the samples in the nfft_plan
		  for(j=0;j<M;j++)
		  {
		    plan3D->nfft_plan.x[3*j]=plan3D->x[3*j+2];
		    plan3D->nfft_plan.x[3*j+1]=plan3D->x[3*j];
		    plan3D->nfft_plan.x[3*j+2]=plan3D->x[3*j+1];

		    plan3D->nfft_plan.f[j]=plan3D->f[j];
		  }

for(j=0;j<3*plan3D->nfft_plan.M_total;j++)
{
plan3D->nfft_plan.x[j]=plan3D->nfft_plan.x[j]*(1/(2*PI));
}

nfft_precompute_one_psi(&(plan3D->nfft_plan));

if (plan3D->flags & NFSOFT_USE_NDFT)
{
ndft_adjoint(&(plan3D->nfft_plan));
}
else
{
  nfft_adjoint(&(plan3D->nfft_plan));
}

//nfft_vpr_complex(plan3D->nfft_plan.f_hat,plan3D->nfft_plan.N_total,"all results");

glo1=0;

	for (k=-N;k<=N;k++)
	{
		 for (m=-N;m<=N;m++)
		 {

		  max=(abs(m)>abs(k)?abs(m):abs(k));

		  for (i=1;i<2*plan3D->N_total+3;i++)
		  {
		  plan3D->cheby[i-1]=plan3D->nfft_plan.f_hat[NFSOFT_INDEX(k,m,i-N-1,N)-1];
		  }

	fprintf(stdout,"k=%d,m=%d \n",k,m);
        //nfft_vpr_complex(plan3D->cheby,2*plan3D->N_total+2,"euler");
               e2c(plan3D,abs((k+m)%2));



		//nfft_vpr_complex(plan3D->wig_coeffs,plan3D->N_total+1,"chebys");
		SO3_fpt_transposed(plan3D->wig_coeffs,set,N,k,m,plan3D->flags);
        //nfft_vpr_complex(plan3D->wig_coeffs,plan3D->N_total+1,"wigners");
		//  SO3_fpt_transposed(plan3D->wig_coeffs,N,k,m,plan3D->flags,plan3D->fpt_kappa);




		  for(j=max;j<=N;j++)
		  {
			if ((plan3D->flags & NFSOFT_REPRESENT))
			{
				if ( (k<0) && (k % 2) )
				{
				plan3D->wig_coeffs[j]=-plan3D->wig_coeffs[j];
				}
				if ( (m<0) && (m % 2) )
				plan3D->wig_coeffs[j]=-plan3D->wig_coeffs[j];
			}


            plan3D->f_hat[glo1]=plan3D->wig_coeffs[j];



			if ((plan3D->flags & NFSOFT_NORMALIZED))
			{
			plan3D->f_hat[glo1]=plan3D->f_hat[glo1]*(1/(2.*PI))*sqrt(0.5*(2.*(j)+1.));
			}

			glo1++;
		   }



	}
	}

}



void nfsoft_finalize(nfsoft_plan *plan)
{
  /* Finalise the nfft plan. */
  nfft_finalize(&plan->nfft_plan);
  free(plan->wig_coeffs);
  free(plan->cheby);



  if (plan->flags & NFSOFT_MALLOC_F_HAT)
  {
    //fprintf(stderr,"deallocating f_hat\n");
    free(plan->f_hat);
  }

  /* De-allocate memory for samples, if neccesary. */
  if (plan->flags & NFSOFT_MALLOC_F)
  {
    //fprintf(stderr,"deallocating f\n");
    free(plan->f);
  }

  /* De-allocate memory for nodes, if neccesary. */
  if (plan->flags & NFSOFT_MALLOC_X)
  {
    //fprintf(stderr,"deallocating x\n");
    free(plan->x);
  }
}

int posN(int n,int m, int B)
{
int pos;

if(n> -B) pos=posN(n-1,m,B)+B+1-MAX(abs(m),abs(n-1)); else pos= 0;
//(n > -B? pos=posN(n-1,m,B)+B+1-MAX(abs(m),abs(n-1)): pos= 0)
return pos;
}

