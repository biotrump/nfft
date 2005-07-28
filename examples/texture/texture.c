#include "texture.h"
#include <stdlib.h>

/**
 * Convert a non-flat index to a flat index.
 * \arg l the frequence
 * \arg m ranges from -l to l
 * \arg n ranges from -l to l
 */
int flatIndex(int l, int m, int n) {
	return m + n + ((l*(5 + 6*l + 4*l*l + 6*m)) / 3);
}

/**
 * Carries out the transform.
 */
void texture_trafo(texture_plan *ths) {
}

void texture_init(texture_plan *ths, int N,	int N1, int N2) {
	texture_init_advanced(ths, N, N1, N2);
}

void texture_init_advanced(texture_plan *ths, int N, int N1, int N2) {
	ths->N_total = flatIndex(N, N, N) + 1;
	ths->f_hat = (complex*) malloc(sizeof(complex) * ths->N_total);
	ths->M_total = N1 * N2;
	ths->f = (complex*) malloc(sizeof(complex) * ths->M_total);
	
	ths->h_phi = (double*) malloc(sizeof(double) * N1);
	ths->h_theta = (double*) malloc(sizeof(double) * N1);
	ths->r_phi = (double**) malloc(sizeof(double) * N1 * N2);
	ths->r_theta = (double**) malloc(sizeof(double) * N1 * N2);
}

/**
 * Frees all memory allocated by texture_init.
 */
void texture_finalize(texture_plan *ths) {
	free(ths->f_hat);
	free(ths->f);
	free(ths->h_phi);
	free(ths->h_theta);
	free(ths->r_phi);
	free(ths->r_theta);
}

/**
 * The adjoint version of the transform.
 */
void texture_adjoint(texture_plan *ths) {
}