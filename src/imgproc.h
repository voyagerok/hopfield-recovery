#ifndef IMGPROC_H
#define IMGPROC_H

#define CHANNEL_DEPTH 8
#define N_CHANNELS_RGB 3
#define N_CHANNELS_RGBA 4
#define N_CHANNELS_GRAY 1


#include <gtk/gtk.h>

#define LESSER 10
#define GREATER 20

#define BIN_THERSHOLD 180

#define P_WIDTH(image) (gdk_pixbuf_get_width(image))
#define P_HEIGHT(image) (gdk_pixbuf_get_height(image))
#define P_STRIDE(image) (gdk_pixbuf_get_rowstride(image))
#define P_PIXELS(image) (gdk_pixbuf_get_pixels(image))
#define P_PIXELS_READ(image) (gdk_pixbuf_read_pixels(image))
#define P_N_CHANNELS(image) (gdk_pixbuf_get_n_channels(image))

GdkPixbuf*
noise (GdkPixbuf *image, int w_step, int h_step, double power);

GdkPixbuf*
resize (const GdkPixbuf *image, int width_step, int height_step, int mode, int need_binarization);

double*
get_vector_from_pixbuf (const GdkPixbuf *image, int min, int max);

GdkPixbuf*
get_pixbuf_from_vector (const double *vector, int width, int height, int min, int max);

#endif // IMGPROC_H
