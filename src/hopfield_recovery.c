#include "global_constants.h"
#include "hopfield_recovery.h"
#include <stdlib.h>
#include "imgproc.h"
#include <string.h>
#include <math.h>

#define SIGN(n) ((n) < 0 ? -1 : 1)
#define EPS 0.000001

double *
get_wieght_matrix (classes_container *container)
{
  int n_of_classes;
  double sum, *w_matrix;
  int index;

  w_matrix = (double*)malloc(sizeof(double) * VEC_SIZE * VEC_SIZE);
  n_of_classes = container->n_of_classes;
  for (int i = 0; i < VEC_SIZE; ++i)
    for (int j = 0; j < VEC_SIZE; ++j)
      {
        sum = 0;
        if (i != j)
          {
            for (int k = 0; k < n_of_classes; ++k)
              sum += container->classes[k].vector[i] *
                  container->classes[k].vector[j];
          }
        index = i * VEC_SIZE + j;
        w_matrix[index] = sum / VEC_SIZE;
      }

  return w_matrix;
}

static gboolean
fuzzy_compare (double *vec1, double *vec2, int size)
{
  for (int i = 0; i < size; ++i)
    if (abs(vec1[i] - vec2[i]) > EPS)
      return FALSE;
  return TRUE;
}

GdkPixbuf*
recover (GdkPixbuf *image, double *weight_matrix)
{
  double *img_vector, backup_vec[VEC_SIZE];
  GdkPixbuf *res_in_orig_size, *recovered, *smaller_sized;
  double sum;

  smaller_sized = resize (image, WIDTH_STEP, HEIGHT_STEP, LESSER, FALSE);
  img_vector = get_vector_from_pixbuf (smaller_sized, -1, 1);

  for (int k = 0; k < ITER_COUNT; ++k)
    {
      memcpy (backup_vec, img_vector, sizeof(double) * VEC_SIZE);
      for (int i = 0; i < VEC_SIZE; ++i)
        {
          sum = 0;
          for (int j = 0; j < VEC_SIZE; ++j)
            {
              int index;
              if (i != j)
                {
                  index = i * VEC_SIZE + j;
                  sum += weight_matrix[index] * img_vector[j];
                }
            }
          img_vector[i] = SIGN(sum);
        }
      if (fuzzy_compare (backup_vec, img_vector, VEC_SIZE))
        break;
    }

  recovered = get_pixbuf_from_vector (img_vector, BLANK_IMG_WIDTH / WIDTH_STEP, BLANK_IMG_HEIGHT / HEIGHT_STEP, -1, 1);
  res_in_orig_size = resize (recovered, WIDTH_STEP, HEIGHT_STEP, GREATER, FALSE);

  free (img_vector);
  g_object_unref (recovered);
  g_object_unref (smaller_sized);

  return res_in_orig_size;
}
