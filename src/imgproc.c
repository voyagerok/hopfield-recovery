#include "imgproc.h"
#include <stdlib.h>
#include "global_constants.h"
#include "random_helper.h"

GdkPixbuf*
noise (GdkPixbuf *image, int w_step, int h_step, double power)
{
  int width, height;
  const guchar *pixels;
  guchar *res_pix;
  int channels, stride, depth;
  int res_stride, rand_val;
  int s_index, d_index;
  GdkPixbuf *res;
  gboolean has_alpha;

  width = gdk_pixbuf_get_width(image);
  height = gdk_pixbuf_get_height(image);
  pixels = gdk_pixbuf_read_pixels(image);
  channels = gdk_pixbuf_get_n_channels(image);
  stride = gdk_pixbuf_get_rowstride(image);
  has_alpha = gdk_pixbuf_get_has_alpha(image);
  depth = gdk_pixbuf_get_bits_per_sample(image);

  res = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                       has_alpha, depth,
                       width, height);
  res_pix = gdk_pixbuf_get_pixels(res);
  res_stride = gdk_pixbuf_get_rowstride(res);

//  srand(time(NULL));

//  int width_step = width / STEP_RATIO;
//  int height_step = height / STEP_RATIO;
  int width_step = w_step;
  int height_step = h_step;

  for(int i = 0; i < height; i += height_step)
    for(int j = 0; j < width; j += width_step)
      {

        //rand_val = rand() % 20;
        rand_val = flip_coin (power);

        for(int k = 0; k < height && k < height_step; ++k)
          for(int l = 0; l < width && l < width_step; ++l)
            {
              s_index = (k + i) * stride + (l + j) * channels;
              d_index = (k + i) * res_stride + (l + j) * channels;

              if(rand_val)
                {
                  res_pix[d_index] = (1 << depth) - pixels[s_index] - 1;
                  res_pix[d_index + 1] = (1 << depth) - pixels[s_index + 2] - 1;
                  res_pix[d_index + 2] = (1 << depth) - pixels[s_index + 2] - 1;
                }
              else
                {
                  res_pix[d_index] = pixels[s_index];
                  res_pix[d_index + 1] = pixels[s_index + 1];
                  res_pix[d_index + 2] = pixels[s_index + 2];
                }
            }


      }
  return res;
}


GdkPixbuf*
resize (const GdkPixbuf *image, int width_step, int height_step, int mode, int need_binarization)
{
  GdkPixbuf *result;
  int width, height, step;
  int new_width, new_height, new_step;
  const guchar *pixels;
  guchar *res_pixels;
  int index, new_index;

  g_assert (mode == GREATER || mode == LESSER);

  width = gdk_pixbuf_get_width(image);
  height = gdk_pixbuf_get_height(image);
  step = gdk_pixbuf_get_rowstride(image);
  pixels = gdk_pixbuf_read_pixels(image);

  if (mode == LESSER)
    {
      new_width = width / width_step;
      new_height = height / height_step;
    }
  else
    {
      new_width = width * width_step;
      new_height = height * height_step;
    }
  result = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, CHANNEL_DEPTH, new_width, new_height);
  new_step = gdk_pixbuf_get_rowstride(result);
  res_pixels = P_PIXELS(result);

  for (int i = 0; i < new_height; ++i)
    for (int j = 0; j < new_width; ++j)
      {
        new_index = i * new_step + j * N_CHANNELS_RGB;
        if (mode == LESSER)
          index = (i * height_step) * step + (j * width_step) * N_CHANNELS_RGB;
        else
          index = (i / height_step) * step + (j / width_step) * N_CHANNELS_RGB;
        if (!need_binarization)
          {
            res_pixels[new_index] = pixels[index];
            res_pixels[new_index + 1] = pixels[index + 1];
            res_pixels[new_index + 2] = pixels[index + 2];
          }
        else
          {
            res_pixels[new_index] = pixels[index] > BIN_THERSHOLD ? 255 : 0;
            res_pixels[new_index + 1] = pixels[index + 1] > BIN_THERSHOLD ? 255 : 0;
            res_pixels[new_index + 2] = pixels[index + 2] > BIN_THERSHOLD ? 255 : 0;
          }
      }
  return result;
}

double*
get_vector_from_pixbuf (const GdkPixbuf *image, int min, int max)
{
  double *vector;
  int width, height, stride, n_channels;
  const guchar *pixels;
  int img_index, vec_index;

  width = P_WIDTH(image);
  height = P_HEIGHT(image);
  stride = P_STRIDE(image);
  pixels = P_PIXELS_READ(image);
  n_channels = P_N_CHANNELS(image);

  g_assert (width * height == VEC_SIZE);

  vector = (double*)malloc(sizeof(double) * width * height);

  for (int i = 0; i < height; ++i)
      for (int j = 0; j < width; ++j)
        {
          img_index = i * stride + j * n_channels;
          vec_index = i * width + j;
          g_assert (pixels[img_index] == 0 || pixels[img_index] == 255);
          vector[vec_index] = (pixels[img_index] == 0) ? min : max;
        }

  return vector;
}

GdkPixbuf*
get_pixbuf_from_vector (const double *vector, int width, int height, int min, int max)
{
  GdkPixbuf *res;
  guchar *pixels;
  int stride, n_channels;
  int p_index, v_index;

  res = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, CHANNEL_DEPTH, width, height);
  pixels = P_PIXELS (res);
  stride = P_STRIDE (res);
  n_channels = P_N_CHANNELS (res);

  for (int i = 0; i < height; ++i)
    for (int j = 0; j < width; ++j)
      {
        int value;

        p_index = i * stride + j * n_channels;
        v_index = i * width + j;
        value = vector[v_index] == min ? 0 : 255;
        pixels[p_index] = pixels[p_index + 1] = pixels[p_index + 2] = value;
      }

  return res;
}
