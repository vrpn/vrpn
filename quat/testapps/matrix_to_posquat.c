/*****************************************************************************
 *
    matrix_to_posquat.c - convert from matrix to position and quaternion
    
 *
 *****************************************************************************/

#include <stdio.h>
#include "quat.h"

int main(int argc, char *argv[])
{
  /* Set the matrix values directly.  Ugly but true. */
  q_matrix_type col_matrix = { {-0.999848,  0.002700, -0.017242,  0.001715},
			     {        0, -0.987960, -0.154710, -0.207295},
			     {-0.017452, -0.154687,  0.987809, -0.098239},
			     {        0,         0,         0,         1} };
/*
  q_matrix_type col_matrix = { { 0.999848, -0.002700,  0.017242, -0.001715},
			     {        0,  0.987960,  0.154710,  0.207295},
			     {-0.017452, -0.154687,  0.987809, -0.098239},
			     {        0,         0,         0,         1} };
*/
  q_matrix_type	  row_matrix;
  q_xyz_quat_type     q;
  q_xyz_quat_type     q_inverse;

  /* Transpose the column matrix into a row matrix */
  unsigned i,j;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
	row_matrix[i][j] = col_matrix[j][i];
    }
  }

  printf("Row Matrix directly:\n");
  q_print_matrix(row_matrix);

  /* Convert to pos/quat and print. */
  q_row_matrix_to_xyz_quat(&q, row_matrix);
  printf("XYZ Quat:\n");
  q_vec_print(q.xyz);
  q_print(q.quat);

  /* Invert the result and print that. */
  printf("XYZ Quat inverse:\n");
  q_xyz_quat_invert(&q_inverse, &q);
  q_vec_print(q_inverse.xyz);
  q_print(q_inverse.quat);

}	/* main */

