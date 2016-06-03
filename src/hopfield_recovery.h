#ifndef HOPFIELD_RECOVERY_H
#define HOPFIELD_RECOVERY_H

#include <gtk/gtk.h>
#include "classes.h"

#define ITER_COUNT 1000

//typedef struct weight_matrix
//{
//  double **matrix;
//  int width, height;
//} w_matrix;

double *
get_wieght_matrix (classes_container *container);

GdkPixbuf*
recover (GdkPixbuf *image, double *weight_matrix);

#endif // HOPFIELD_RECOVERY_H
