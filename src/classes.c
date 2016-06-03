#include "classes.h"
#include "global_constants.h"
#include <stdlib.h>
#include "imgproc.h"
#include <string.h>

void
init_container (classes_container **container)
{
  (*container) = (classes_container*)malloc(sizeof(classes_container));
  (*container)->n_of_classes = 0;
  (*container)->classes = (class_instance*)malloc(sizeof(class_instance) * CLASSES_MAX_COUNT);
  (*container)->classes_capacity = CLASSES_MAX_COUNT;
}

void
free_container (classes_container **container)
{
  if ((*container) != NULL)
    {
      if ((*container)->n_of_classes > 0)
        {
          for (int i = 0; i < (*container)->n_of_classes; ++i)
            {
              free ((*container)->classes[i].name);
              free ((*container)->classes[i].vector);
              g_object_unref ((*container)->classes[i].init_image);
            }
        }
      free ((*container)->classes);
      free ((*container));
    }
}

void
add_class_to_container (classes_container *container, GdkPixbuf *image, char *name)
{
  double **train, **test, *vector;
  int n_of_classes;
  GdkPixbuf *resized, *noised;
  int name_length;

  g_assert (container != NULL);

  n_of_classes = container->n_of_classes;

  resized = resize (image, WIDTH_STEP, HEIGHT_STEP, LESSER, FALSE);
  container->classes[n_of_classes].vector = get_vector_from_pixbuf (resized, -1, 1);

  name_length = strlen(name) + 1;
  container->classes[n_of_classes].name = (char*)malloc(name_length);
  memcpy (container->classes[n_of_classes].name, name, name_length);

  container->classes[n_of_classes].init_image = gdk_pixbuf_copy(image);

  container->n_of_classes++;
  g_object_unref (resized);
}

void
clear_container (classes_container *container)
{
  int n_of_classes;

  g_assert (container != NULL);

  n_of_classes = container->n_of_classes;
  for (int i = 0; i < n_of_classes; ++i)
    {
      free (container->classes[i].name);
      free (container->classes[i].vector);
      g_object_unref (container->classes[i].init_image);
    }
  container->n_of_classes = 0;
}

