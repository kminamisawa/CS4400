#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following student struct 
 */
student_t student = {
  "Koji Minamisawa",     /* Full name */
  "k.minamisawa@utah.edu",  /* Email address */

};

/******************************************************
 * PINWHEEL KERNEL
 *
 * Your different versions of the pinwheel kernel go here
 ******************************************************/

/* 
 * naive_pinwheel - The naive baseline version of pinwheel 
 */
char naive_pinwheel_descr[] = "naive_pinwheel: baseline implementation";
void naive_pinwheel(pixel *src, pixel *dest)
{
  int qi, qj, i, j;

  /* qi & qj are column and row of quadrant
     i & j are column and row within quadrant */

  /* Loop over 4 quadrants: */
  for (qi = 0; qi < 2; qi++)
    for (qj = 0; qj < 2; qj++)
      /* Loop within quadrant: */
      for (i = 0; i < src->dim/2; i++)
        for (j = 0; j < src->dim/2; j++) {
          int s_idx = RIDX((qj * src->dim/2) + i,
                           j + (qi * src->dim/2), src->dim);
          int d_idx = RIDX((qj * src->dim/2) + src->dim/2 - 1 - j,
                           i + (qi * src->dim/2), src->dim);
          dest[d_idx].red = (src[s_idx].red
                             + src[s_idx].green
                             + src[s_idx].blue) / 3;
          dest[d_idx].green = (src[s_idx].red
                               + src[s_idx].green
                               + src[s_idx].blue) / 3;
          dest[d_idx].blue = (src[s_idx].red
                              + src[s_idx].green
                              + src[s_idx].blue) / 3;
        }
}

void my_pinwheel(pixel *src, pixel *dest)
{
  
  int qi, qj, i, j;

  /* qi & qj are column and row of quadrant
     i & j are column and row within quadrant */

  int localDim = src->dim;
  int half_dim = localDim / 2;
  // int qj_to_half_dim;
  // int qi_to_half_dim;
  // int result = 0;
  // int s_idx;
  // int d_idx;
  // int s_idx_par2;
  // int d_idx_par1;

  /* Loop over 4 quadrants: */
  for (qi = 0; qi < 2; qi++){
    int qi_to_half_dim = qi * half_dim;
    for (qj = 0; qj < 2; qj++){
      int qj_to_half_dim = qj * half_dim;
      /* Loop within quadrant: */
      for (j = 0; j < half_dim; j++){
        int d_idx_par1 = qj_to_half_dim + half_dim - 1 - j;
        int s_idx_par2 = j + (qi_to_half_dim);
        for (i = 0; i < half_dim; i++) {
          int s_idx = RIDX(qj_to_half_dim + i, 
                     s_idx_par2,
                     localDim);
          int d_idx = RIDX(d_idx_par1,
                           i + (qi_to_half_dim),
                           localDim);

          int result =  (src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3; //Grayscale
          dest[d_idx].red = result;
          dest[d_idx].green = result;
          dest[d_idx].blue = result;
        }
      }
    }
  }
}

void my_pinwheel2(pixel *src, pixel *dest)
{
int i, j;

  /* qi & qj are column and row of quadrant
     i & j are column and row within quadrant */

  int localDim = src->dim;
  int half_dim = localDim / 2;

   for (j = 0; j < half_dim; j++){
        int d_idx_par1 = half_dim - 1 - j;
        // int s_idx_par2 = j;
        for (i = 0; i < half_dim; i++) {
          int s_idx = RIDX(0 + i, 
                     j,
                     localDim);
          int d_idx = RIDX(d_idx_par1,
                           i,
                           localDim);

          int result =  (src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3; //Grayscale
          dest[d_idx].red = result;
          dest[d_idx].green = result;
          dest[d_idx].blue = result;
        }
      }

   // int qi_to_half_dim = 0;
   // int qj_to_half_dim = 1;
   for (j = 0; j < half_dim; j++){
        int d_idx_par1 = localDim- 1 - j; //half_dim_+half_dim = local-dim
        // int s_idx_par2 = j;
        for (i = 0; i < half_dim; i++) {
          int s_idx = RIDX(half_dim + i, 
                     j,
                     localDim);
          int d_idx = RIDX(d_idx_par1,
                           i,
                           localDim);

          int result =  (src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3; //Grayscale

          dest[d_idx].red = result;
          dest[d_idx].green = result;
          dest[d_idx].blue = result;
        }
      }

   // int qi_to_half_dim = 1;
   // int qj_to_half_dim = 0;
   for (j = 0; j < half_dim; j++){
        int d_idx_par1 = half_dim - 1 - j;
        int s_idx_par2 = j + half_dim;
        for (i = 0; i < half_dim; i++) {
          int s_idx = RIDX(i, 
                     s_idx_par2,
                     localDim);
          int d_idx = RIDX(d_idx_par1,
                           i + half_dim,
                           localDim);

          int result =  (src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3; //Grayscale
          dest[d_idx].red = result;
          dest[d_idx].green = result;
          dest[d_idx].blue = result;
        }
      }

      // int qi_to_half_dim = 1;
      // int qj_to_half_dim = 1;
      for (j = 0; j < half_dim; j++){
        int d_idx_par1 = localDim - 1 - j;
        int s_idx_par2 = j + half_dim;
        for (i = 0; i < half_dim; i++) {
          int s_idx = RIDX(half_dim + i, 
                     s_idx_par2,
                     localDim);
          int d_idx = RIDX(d_idx_par1,
                           i + half_dim,
                           localDim);

          int result =  (src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3; //Grayscale
          dest[d_idx].red = result;
          dest[d_idx].green = result;
          dest[d_idx].blue = result;
        }
      }
}

void my_pinwheel4(pixel *src, pixel *dest)
{
  int i, j, ii, jj;

  /* qi & qj are column and row of quadrant
     i & j are column and row within quadrant */

  int localDim = src->dim;
  int half_dim = localDim / 2;

  for (j = 0; j < half_dim; j+=16){
    for (i = 0; i < half_dim; i+=16) {
      for(jj = j; jj < j + 16; jj++){
        for(ii = i; ii < i + 16; ii++){
          int s_idx = RIDX(ii, jj, localDim);
          int d_idx = RIDX(half_dim - 1 - jj, ii, localDim);
          int result =  (src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3; //Grayscale
          dest[d_idx].red = result;
          dest[d_idx].green = result;
          dest[d_idx].blue = result;
        }
      }
    }
  }

  for (j = 0; j < half_dim; j+=16){
    for (i = 0; i < half_dim; i+=16) {
      for(jj = j; jj < j + 16; jj++){
        for(ii = i; ii < i + 16; ii++){
          int s_idx = RIDX(half_dim + ii,
                           jj,
                           localDim);
          int d_idx = RIDX(localDim-1-jj,
                           ii,
                           localDim);

          int result =  (src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3; //Grayscale

          dest[d_idx].red = result;
          dest[d_idx].green = result;
          dest[d_idx].blue = result;
        }
      }
    }
  }

  for (j = 0; j < half_dim; j+=16){
    for (i = 0; i < half_dim; i+=16) {
      for(jj = j; jj < j + 16; jj++){
        for(ii = i; ii < i + 16; ii++){
          int s_idx = RIDX(ii,
                           jj+half_dim,
                           localDim);
          int d_idx = RIDX(half_dim - 1 - jj,
                           ii + half_dim,
                           localDim);

          int result =  (src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3; //Grayscale
          dest[d_idx].red = result;
          dest[d_idx].green = result;
          dest[d_idx].blue = result;

        }
      }
    }
  }

  for (j = 0; j < half_dim; j+=16){
    for (i = 0; i < half_dim; i+=16) {
      for(jj = j; jj < j + 16; jj++){
        for(ii = i; ii < i + 16; ii++){
          int s_idx = RIDX(half_dim + ii,
                           jj + half_dim,
                           localDim);
          int d_idx = RIDX(localDim-1-jj,
                           ii + half_dim,
                           localDim);

          int result =  (src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3; //Grayscale
          dest[d_idx].red = result;
          dest[d_idx].green = result;
          dest[d_idx].blue = result;
        }

      }
    }
  }
}


/* 
 * pinwheel - Your current working version of pinwheel
 * IMPORTANT: This is the version you will be graded on
 */
char pinwheel_descr[] = "pinwheel: Current working version";
void pinwheel(pixel *src, pixel *dest)
{
  my_pinwheel4(src, dest);
}

/*********************************************************************
 * register_pinwheel_functions - Register all of your different versions
 *     of the pinwheel kernel with the driver by calling the
 *     add_pinwheel_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_pinwheel_functions() {
  add_pinwheel_function(&pinwheel, pinwheel_descr);
  add_pinwheel_function(&naive_pinwheel, naive_pinwheel_descr);
}


/***************************************************************
 * MOTION KERNEL
 * 
 * Starts with various typedefs and helper functions for the motion
 * function, and you may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct {
  int red;
  int green;
  int blue;
} pixel_sum;

/* 
 * initialize_pixel_sum - Initializes all fields of sum to 0 
 */
static void initialize_pixel_sum(pixel_sum *sum) 
{
  sum->red = sum->green = sum->blue = 0;
}

/* 
 * accumulate_sum - Accumulates field values of p in corresponding 
 * fields of sum 
 */
static void accumulate_weighted_sum(pixel_sum *sum, pixel p, double weight) 
{
  sum->red += (int) p.red * weight;
  sum->green += (int) p.green * weight;
  sum->blue += (int) p.blue * weight;
}

/* 
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel 
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
  current_pixel->red = (unsigned short)sum.red;
  current_pixel->green = (unsigned short)sum.green;
  current_pixel->blue = (unsigned short)sum.blue;
}

/* 
 * weighted_combo - Returns new pixel value at (i,j) 
 */
static pixel weighted_combo(int dim, int i, int j, pixel *src) 
{
  int ii, jj;
  pixel_sum sum;
  pixel current_pixel;
  double weights[3][3] = { { 0.60, 0.03, 0.00 },
                           { 0.03, 0.30, 0.03 },
                           { 0.00, 0.03, 0.10 } };

  initialize_pixel_sum(&sum);
  for (ii=0; ii < 3; ii++)
    for (jj=0; jj < 3; jj++) 
      if ((i + ii < dim) && (j + jj < dim))
        accumulate_weighted_sum(&sum,
                                src[RIDX(i+ii,j+jj,dim)],
                                weights[ii][jj]);
  
  assign_sum_to_pixel(&current_pixel, sum);

  return current_pixel;
}

/******************************************************
 * Your different versions of the motion kernel go here
 ******************************************************/

/*
 * naive_motion - The naive baseline version of motion 
 */
char naive_motion_descr[] = "naive_motion: baseline implementation";
void naive_motion(pixel *src, pixel *dst) 
{
  int i, j;
    
  for (i = 0; i < src->dim; i++)
    for (j = 0; j < src->dim; j++)
      dst[RIDX(i, j, src->dim)] = weighted_combo(src->dim, i, j, src);
}

/*
 * motion - Your current working version of motion. 
 * IMPORTANT: This is the version you will be graded on
 */
char motion_descr[] = "motion: Current working version";
void my_motion(pixel *src, pixel *dst) 
{
// if ((i + ii < local_dim) && (j + jj < dim))
//  accumulate_weighted_sum(&sum,src[RIDX(i+ii,j+jj,dim)],weights[ii][jj]);
//   static void accumulate_weighted_sum(pixel_sum *sum, pixel p, double weight) 
// {
//   sum->red += (int) p.red * weight;
//   sum->green += (int) p.green * weight;
//   sum->blue += (int) p.blue * weight;
// }

int i, j;
// int local_dim = src->dim;
  for(i = 0; i < src->dim; i++){
    for(j = 0; j < src->dim; j++){
      int ii, jj;
      pixel_sum _sum;
      pixel_sum* sum = &_sum;
      sum->red = sum->green = sum->blue = 0;
      pixel current_pixel;
      sum->red = 0;
      sum->green = 0;
      sum->blue = 0;
      ii = 0;
      jj = 0;

      if ((i+ii < src->dim) && (j + jj < src->dim)){
          pixel p = src[RIDX(i+ii,j+jj,src->dim)];
          sum->red += (int) (p.red * 6/10);
          sum->green += (int) (p.green * 6/10);
          sum->blue += (int) (p.blue * 6/10);
      }
      jj++; //=1
      //(0,1)
      if ((i+ii < src->dim) && (j + jj < src->dim)){
          pixel p = src[RIDX(i+ii,j+jj,src->dim)];
          sum->red += (int) (p.red * 3/100);
          sum->green += (int) (p.green * 3/100);
          sum->blue += (int) (p.blue * 3/100);
      }
      ii++; //=1
      jj = 0;
      //(1,0)
      if ((i+ii < src->dim) && (j + jj < src->dim)){
          pixel p = src[RIDX(i+ii,j+jj,src->dim)];
          sum->red += (int) (p.red * 3/100);
          sum->green += (int) (p.green * 3/100);
          sum->blue += (int) (p.blue * 3/100);
      }
      jj++; //1
      //(1,1)
      if ((i+ii < src->dim) && (j + jj < src->dim)){
          pixel p = src[RIDX(i+ii,j+jj,src->dim)];
          sum->red += (int) (p.red * 3/10);
          sum->green += (int) (p.green * 3/10);
          sum->blue += (int) (p.blue  * 3/10);
      }
      jj++; //2
      //(1,2)
      if ((i+ii < src->dim) && (j + jj < src->dim)){
          pixel p = src[RIDX(i+ii,j+jj,src->dim)];
          sum->red += (int) (p.red * 3/100);
          sum->green += (int) (p.green * 3/100);
          sum->blue += (int) (p.blue * 3/100);
      }
      ii++; //2
      jj = 1;
      //(2,0) 
      //(2,1)
      if ((i+ii < src->dim) && (j + jj < src->dim)){
          pixel p = src[RIDX(i+ii,j+jj,src->dim)];
          sum->red += (int) (p.red * 3/100);
          sum->green += (int) (p.green * 3/100);
          sum->blue += (int) (p.blue * 3/100);
      }
      jj++;
      //(2,2)
      if ((i+ii < src->dim) && (j + jj < src->dim)){
          pixel p = src[RIDX(i+ii,j+jj,src->dim)];
          sum->red += (int) (p.red * 1/10);
          sum->green += (int) (p.green * 1/10);
          sum->blue += (int) (p.blue * 1/10);
      }

      current_pixel.red = (unsigned short)sum->red;
      current_pixel.green = (unsigned short)sum->green;
      current_pixel.blue = (unsigned short)sum->blue;
      dst[RIDX(i, j, src->dim)] = current_pixel;

    }
  }
}

void motion(pixel *src, pixel *dst) 
{
  my_motion(src, dst);
}

/********************************************************************* 
 * register_motion_functions - Register all of your different versions
 *     of the motion kernel with the driver by calling the
 *     add_motion_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_motion_functions() {
  add_motion_function(&motion, motion_descr);
  add_motion_function(&naive_motion, naive_motion_descr);
}
