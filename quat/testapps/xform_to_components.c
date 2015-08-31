/*****************************************************************************
 *
    xform_to_components.c - convert from matrix to position, Euler rotation, scale.
        Input file has a first header line that is ignored, followed by four lines
        that are a 4x4 column matrix.
    
 *
 *****************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include "quat.h"

#define DEGREES_PER_RADIAN (180 / acos(-1))

void Usage(const char *name)
{
  fprintf(stderr,"Usage: %s INPUT_FILE_NAME\n", name);
  exit(-1);
}

int main(int argc, char *argv[])
{
  unsigned i;
  q_matrix_type	  row_matrix;
  double          sx, sy, sz;  /* Scale in X, Y, and Z */
  q_xyz_quat_type     q;
  q_vec_type          euler;

  const char *infile_name = NULL;
  FILE  *infile = NULL;
  char  line[1025];

  /* Check the command-line arguments. */
  if (argc != 2) {
    Usage(argv[0]);
  } else {
    infile_name = argv[1];
  }

  /* Open and read the file */
  /* Transpose the matrix while it is being read into a row matrix. */
  if ( (infile = fopen(infile_name, "r")) == NULL) {
    fprintf(stderr,"Cannot open %s for reading\n", infile_name);
    return -2;
  }
  line[1024] = '\0';  /* Ensure null-termination */
  /* Print the comment line. */
  if (fgets(line, 1024, infile) == NULL) {
    fprintf(stderr, "Could not read comment line from %s\n", infile_name);
    return -3;
  }
  printf("Comment: %s\n", line);
  /* Read and parse the first matrix line. */
  if (fgets(line, 1024, infile) == NULL) {
    fprintf(stderr, "Could not read matrix line 1 from %s\n", infile_name);
    return -3;
  }
  if (sscanf(line, "%lf%lf%lf%lf", &row_matrix[0][0], &row_matrix[1][0], &row_matrix[2][0], &row_matrix[3][0]) != 4) {
    fprintf(stderr, "Could not parse matrix line 1 from %s\n", infile_name);
    return -3;
  }
  /* Read and parse the second matrix line. */
  if (fgets(line, 1024, infile) == NULL) {
    fprintf(stderr, "Could not read matrix line 2 from %s\n", infile_name);
    return -3;
  }
  if (sscanf(line, "%lf%lf%lf%lf", &row_matrix[0][1], &row_matrix[1][1], &row_matrix[2][1], &row_matrix[3][1]) != 4) {
    fprintf(stderr, "Could not parse matrix line 2 from %s\n", infile_name);
    return -3;
  }
  /* Read and parse the third matrix line. */
  if (fgets(line, 1024, infile) == NULL) {
    fprintf(stderr, "Could not read matrix line 3 from %s\n", infile_name);
    return -3;
  }
  if (sscanf(line, "%lf%lf%lf%lf", &row_matrix[0][2], &row_matrix[1][2], &row_matrix[2][2], &row_matrix[3][2]) != 4) {
    fprintf(stderr, "Could not parse matrix line 3 from %s\n", infile_name);
    return -3;
  }
  /* Read and parse the fourth matrix line. */
  if (fgets(line, 1024, infile) == NULL) {
    fprintf(stderr, "Could not read matrix line 4 from %s\n", infile_name);
    return -3;
  }
  if (sscanf(line, "%lf%lf%lf%lf", &row_matrix[0][3], &row_matrix[1][3], &row_matrix[2][3], &row_matrix[3][3]) != 4) {
    fprintf(stderr, "Could not parse matrix line 4 from %s\n", infile_name);
    return -3;
  }

  printf("Row Matrix directly:\n");
  q_print_matrix(row_matrix);

  /* The conversion routines cannot handle non-unit scaling.  Compute the
   * scale of each row in the matrix, print the scales, then divide each
   * row to normalize before we do the rest of the math. */
  sx = sqrt(row_matrix[0][0]*row_matrix[0][0] +
            row_matrix[0][1]*row_matrix[0][1] +
            row_matrix[0][2]*row_matrix[0][2]);
  sy = sqrt(row_matrix[1][0]*row_matrix[1][0] +
            row_matrix[1][1]*row_matrix[1][1] +
            row_matrix[1][2]*row_matrix[1][2]);
  sz = sqrt(row_matrix[2][0]*row_matrix[2][0] +
            row_matrix[2][1]*row_matrix[2][1] +
            row_matrix[2][2]*row_matrix[2][2]);
  printf("Scale:\n  %lg, %lg, %lg\n", sx, sy, sz);
  for (i = 0; i < 3; i++) {
    row_matrix[0][i] /= sx;
    row_matrix[1][i] /= sy;
    row_matrix[2][i] /= sz;
  }

  /* Convert to pos/quat, then Euler and print. */
  q_row_matrix_to_xyz_quat(&q, row_matrix);
  q_to_euler(euler, q.quat);
  printf("Translation:\n  %lf, %lf, %lf\n", q.xyz[0], q.xyz[1], q.xyz[2]);
  printf("Euler angles (degrees):\n  %lg, %lg, %lg\n",
    euler[2] * DEGREES_PER_RADIAN,  /* Rotation about X is stored in the third component. */
    euler[1] * DEGREES_PER_RADIAN,
    euler[0] * DEGREES_PER_RADIAN);

}	/* main */

