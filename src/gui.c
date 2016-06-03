#include "gui.h"
#include "imgproc.h"
#include "global_constants.h"
#include "classes.h"
#include <string.h>
#include <stdlib.h>
#include "hopfield_recovery.h"

#define MOUSE_DOWN 4
#define MOUSE_UP 8
#define CLEAR_WHITE(pixbuf) (gdk_pixbuf_fill(pixbuf, 0xffffffff))
#define NOISE_POWER 0.15

static GtkBuilder *builder;
static classes_container *container;
static double *w_matr;

static int mouse_flag = MOUSE_UP;

struct point
{
  int x,y;
} start_pos = {0,0};

struct rect
{
  int x,y;
  int width, height;
};

static void
tree_view_clear_records (GtkTreeView *t_view)
{
  GtkTreeStore *store;
  GtkTreeIter root;

  store = GTK_TREE_STORE(gtk_tree_view_get_model(t_view));
  gtk_tree_store_clear(store);
  gtk_tree_store_append(store, &root, NULL);
  gtk_tree_store_set(store, &root, 0, "Сгенерированные классы", -1);
}

static void
tree_view_append_record (GtkTreeView *t_view, char *name)
{
  GtkTreeStore *store;
  GtkTreeIter root;
  GtkTreeIter new_elem_iter;

  store = GTK_TREE_STORE(gtk_tree_view_get_model(t_view));
  gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &root, "0");
  gtk_tree_store_append(store, &new_elem_iter, &root);
  gtk_tree_store_set(store, &new_elem_iter, 0, name, -1);
}

static void
show_message_box(GtkBuilder *builder,
                 const gchar *msg,
                 GtkMessageType type)
{
  GtkWidget *parent, *dialog;

  parent = GTK_WIDGET(gtk_builder_get_object(builder,
                                             "mainwindow"));
  dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  type,
                                  GTK_BUTTONS_CLOSE,
                                  msg, NULL);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

static char*
get_class_name_from_user (GtkBuilder *builder)
{
  GtkWidget *window, *dialog, *label, *entry;
  GtkWidget *hbox, *content_area;
  GtkDialogFlags flags;
  int response, input_length;
  char *class_name;
  const char *input_text;

  window = GTK_WIDGET(gtk_builder_get_object(builder, "mainwindow"));
  dialog = gtk_dialog_new_with_buttons("Введите имя класса",
                                       GTK_WINDOW(window),
                                       GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
                                       "Отмена", GTK_RESPONSE_CANCEL,
                                       "ОК", GTK_RESPONSE_ACCEPT,
                                       NULL);
  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  entry = gtk_entry_new();
  label = gtk_label_new("Имя класса:");
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 15);
  gtk_box_pack_end(GTK_BOX(hbox), entry, TRUE, TRUE, 15);
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
  gtk_container_add(GTK_CONTAINER(content_area), hbox);

  gtk_widget_show_all(content_area);
  response = gtk_dialog_run(GTK_DIALOG(dialog));
  class_name = (char*)malloc(MAX_CLASS_NAME_LENGTH);
  if (response == GTK_RESPONSE_ACCEPT)
    {
      input_text = gtk_entry_get_text(GTK_ENTRY(entry));
      if ((input_length = strlen(input_text)) == 0)
        sprintf(class_name, "Класс%i", container->n_of_classes);
      else
        memcpy(class_name, input_text, input_length + 1);
    }
  gtk_widget_destroy(dialog);

  return class_name;
}

static void
on_class_activated (GtkTreeView *t_view,
                    GtkTreePath *path,
                    GtkTreeViewColumn *column,
                    gpointer data)
{
  int path_depth, class_index;
  int *indeces;
  GtkBuilder *builder;
  GtkImage *f_image, *s_image;
  GdkPixbuf *class_init_img, *clear_img;

  indeces = gtk_tree_path_get_indices_with_depth (path, &path_depth);
  if (path_depth == 2)
    {
      class_index = indeces[path_depth - 1];
      builder = GTK_BUILDER(data);
      f_image = GTK_IMAGE(gtk_builder_get_object(builder, "imagefirst"));
      class_init_img = container->classes[class_index].init_image;
      gtk_image_set_from_pixbuf (f_image, class_init_img);

      s_image = GTK_IMAGE(gtk_builder_get_object(builder, "imagesecond"));
      clear_img = gdk_pixbuf_copy(gtk_image_get_pixbuf (s_image));
      CLEAR_WHITE (clear_img);
      gtk_image_set_from_pixbuf (s_image, clear_img);
      g_object_unref (clear_img);
    }

}

static void
on_clear (GtkButton *button,
                           gpointer data)
{
  GtkBuilder *builder;
  GtkImage *image;
  GdkPixbuf *pbuf, *clear_img;

  builder = GTK_BUILDER(data);

  image = GTK_IMAGE(gtk_builder_get_object(builder, "imagefirst"));
  pbuf = gtk_image_get_pixbuf(image);
  clear_img = gdk_pixbuf_copy(pbuf);
  CLEAR_WHITE(clear_img);
  gtk_image_set_from_pixbuf(image, clear_img);

  image = GTK_IMAGE(gtk_builder_get_object(builder, "imagesecond"));
  pbuf = gtk_image_get_pixbuf(image);
  clear_img = gdk_pixbuf_copy(pbuf);
  CLEAR_WHITE(clear_img);
  gtk_image_set_from_pixbuf(image, clear_img);

  g_object_unref(clear_img);
}

static void
on_recover (GtkButton *button,
                           gpointer data)
{
  int n_of_classes;
  GtkBuilder *builder;
  GtkImage *f_image, *s_image;
  GdkPixbuf *pf_image, *ps_image;

  n_of_classes = container->n_of_classes;
  builder = GTK_BUILDER(data);
  if (n_of_classes < 2)
    {
      show_message_box (builder, "Ошибка: не стоило очищать классы", GTK_MESSAGE_ERROR);
      return;
    }
  else if (w_matr == NULL)
    {
      show_message_box (builder, "Ошибка: систему необходимо обучить", GTK_MESSAGE_ERROR);
      return;
    }

  f_image = GTK_IMAGE(gtk_builder_get_object(builder, "imagefirst"));
  s_image = GTK_IMAGE(gtk_builder_get_object(builder, "imagesecond"));

  pf_image = gtk_image_get_pixbuf (f_image);
  ps_image = recover (pf_image, w_matr);

  gtk_image_set_from_pixbuf (s_image, ps_image);

  g_object_unref (ps_image);
}

static void
get_noise (GSimpleAction *action,
                 GVariant *variant,
                 gpointer data)
{
  GtkBuilder *builder;
  GtkImage *image;
  GdkPixbuf *input;
  GdkPixbuf *resized, *orig, *noised;

  srand (time(NULL));

  builder = GTK_BUILDER(data);
  image = GTK_IMAGE(gtk_builder_get_object(builder, "imagefirst"));
  input = gtk_image_get_pixbuf(image);

  resized = resize (input, WIDTH_STEP, HEIGHT_STEP, LESSER, FALSE);
  noised = noise (resized, 1, 1, NOISE_POWER);
  orig = resize (noised, WIDTH_STEP, HEIGHT_STEP, GREATER, FALSE);

  gtk_image_set_from_pixbuf(image, orig);

  g_object_unref(resized);
  g_object_unref(noised);
  g_object_unref(orig);
}

static void
on_save (GSimpleAction *action,
                 GVariant *variant,
                 gpointer data)
{
  GtkWidget *dialog, *image, *window;
  GdkPixbuf *image_for_save;
  GtkFileChooser *chooser;
  GtkFileFilter *img_filter, *all_filter;
  int response;
  gchar *fname;

  img_filter = gtk_file_filter_new ();
  all_filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (img_filter, "Image Files");
  gtk_file_filter_set_name (all_filter, "All Files");
  gtk_file_filter_add_pattern (img_filter, "*.jpg");
  gtk_file_filter_add_pattern (all_filter, "*");

  image = GTK_WIDGET(data);
  window = gtk_widget_get_toplevel(image);
  dialog = gtk_file_chooser_dialog_new("Сохранить изображение",
                                       GTK_WINDOW(window),
                                       GTK_FILE_CHOOSER_ACTION_SAVE,
                                       "Отмена", GTK_RESPONSE_CANCEL,
                                       "Сохранить", GTK_RESPONSE_ACCEPT,
                                       NULL);
  chooser = GTK_FILE_CHOOSER(dialog);
  gtk_file_chooser_set_current_folder_uri(chooser, g_get_home_dir());
  gtk_file_chooser_set_current_name(chooser, "Безымянный");
  gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);
  gtk_file_chooser_add_filter(chooser, img_filter);
  gtk_file_chooser_add_filter(chooser, all_filter);

  response = gtk_dialog_run(GTK_DIALOG(dialog));
  if (response == GTK_RESPONSE_ACCEPT)
    {
      image_for_save = gtk_image_get_pixbuf(GTK_IMAGE(image));
      fname = gtk_file_chooser_get_filename(chooser);
      gdk_pixbuf_save(image_for_save, fname, "jpeg", NULL, NULL);
      g_free(fname);
    }
  gtk_widget_destroy(dialog);
  g_object_unref(img_filter);
  g_object_unref(all_filter);
}

static void
on_create_class (GSimpleAction *action,
                 GVariant *variant,
                 gpointer data)
{
  GtkBuilder *builder;
  GtkImage *image;
  GdkPixbuf *p_image;
  GtkTreeView *t_view;
  char *class_name;

  builder = GTK_BUILDER(data);
  image = GTK_IMAGE(gtk_builder_get_object(builder, "imagefirst"));
  t_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));

  class_name = get_class_name_from_user (builder);

  p_image = gtk_image_get_pixbuf (image);
  add_class_to_container (container, p_image, class_name);
  tree_view_append_record (t_view, class_name);
  gtk_tree_view_expand_all (t_view);

  free (class_name);
}

static void
on_learning (GSimpleAction *action,
                 GVariant *variant,
                 gpointer data)
{
  int n_of_classes;
  GtkBuilder *builder;

  n_of_classes = container->n_of_classes;
  builder = GTK_BUILDER(data);
  if (n_of_classes < 2)
    {
      show_message_box (builder, "Ошибка: для обучения нужно минимум 2 класса", GTK_MESSAGE_ERROR);
      return;
    }
  else
    {
      if (w_matr != NULL)
        free (w_matr);
      w_matr = get_wieght_matrix (container);
    }
}

static void
on_clear_classes (GSimpleAction *action,
                           GVariant *variant,
                           gpointer data)
{
  GtkBuilder *builder;
  GtkTreeView *t_view;

  clear_container(container);
  //clear_neuron (neuron);
  if (w_matr != NULL)
    free (w_matr);
  builder = GTK_BUILDER(data);
  t_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
  tree_view_clear_records(t_view);
}

static GActionEntry builder_entries[] =
{
  {"noise", get_noise,   NULL, NULL, NULL},
//  {"save", on_save,   NULL, NULL, NULL},
  {"create_class", on_create_class,   NULL, NULL, NULL},
  {"learning", on_learning,   NULL, NULL, NULL},
  {"clear_classes", on_clear_classes,   NULL, NULL, NULL}
};

static void
add_action_entries (GtkBuilder *builder)
{
  GtkApplicationWindow *window;

  window = GTK_APPLICATION_WINDOW(gtk_builder_get_object(builder,
                                                         "mainwindow"));
  g_action_map_add_action_entries(G_ACTION_MAP(window),
                                  builder_entries, G_N_ELEMENTS(builder_entries),
                                  builder);
}

static void
setup_menu (GtkBuilder *builder)
{
  GtkWidget *menu_button;
  GMenu *menu;

  menu_button = GTK_WIDGET(gtk_builder_get_object(builder,
                                                  "menubutton"));
  menu = g_menu_new();
  g_menu_append(menu, "Обучить систему", "win.learning");
  g_menu_append(menu, "Создать класс", "win.create_class");
  g_menu_append(menu, "Добавить шум", "win.noise");
  g_menu_append(menu, "Очистить классы", "win.clear_classes");
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button), G_MENU_MODEL(menu));
}


static GdkPixbuf *draw_line (GdkPixbuf *image,
                             struct point *start,
                             struct point *end)
{
  cairo_surface_t *surface;
  int s_width, s_height;
  GdkPixbuf *res;
  cairo_t *context;

  surface = gdk_cairo_surface_create_from_pixbuf(image, 0, NULL);
  context = cairo_create(surface);
  cairo_set_line_width (context, 1.0);
  cairo_set_line_cap(context, CAIRO_LINE_CAP_ROUND);
  cairo_set_source_rgb (context, 0, 0, 0);
  cairo_move_to(context, start->x, start->y);
  cairo_line_to(context, end->x, end->y);
  cairo_stroke(context);
  cairo_destroy(context);

  s_width = cairo_image_surface_get_width(surface);
  s_height = cairo_image_surface_get_height(surface);
  res = gdk_pixbuf_get_from_surface(surface,
                                    0, 0,
                                    s_width,
                                    s_height);
  cairo_surface_destroy(surface);
  return res;
}

static void redraw(GtkImage *image,
                 struct point *start,
                   struct point *end)
{
  GdkPixbuf *src_img, *res_img, *resized, *orig;
  struct point scaled_start, scaled_end;

  scaled_start.x = (start->x / WIDTH_STEP);
  scaled_start.y = (start->y / HEIGHT_STEP);
  scaled_end.x = (end->x / WIDTH_STEP);
  scaled_end.y = (end->y / HEIGHT_STEP);
  src_img = gtk_image_get_pixbuf(image);
  resized = resize (src_img, WIDTH_STEP, HEIGHT_STEP, LESSER, FALSE);
  res_img = draw_line(resized, &scaled_start, &scaled_end);
  orig = resize (res_img, WIDTH_STEP, HEIGHT_STEP, GREATER, TRUE);
  gtk_image_set_from_pixbuf(image, orig);

  g_object_unref(res_img);
  g_object_unref(resized);
  g_object_unref(orig);
}

static void
on_mouse_down(GtkWidget *widget,
              GdkEvent *event,
              gpointer data)
{
  GdkEventButton *btn_event;
  GtkAllocation alloc;
  GtkWidget  *image;
  GtkBuilder *builder;
  struct point center, img_top_left;
  GdkPixbuf *pbuf;
  int img_width, img_height;
  gboolean image_empty;

  btn_event = (GdkEventButton*)event;
  builder = GTK_BUILDER(data);
  image = GTK_WIDGET(gtk_builder_get_object(builder, "imagefirst"));
  image_empty = gtk_image_get_storage_type(GTK_IMAGE(image)) == GTK_IMAGE_EMPTY;
  if(btn_event->button == GDK_BUTTON_PRIMARY && !image_empty)
    {
      pbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));

      mouse_flag = MOUSE_DOWN;
      gtk_widget_get_allocation(image, &alloc);
      center.x = alloc.width / 2;
      center.y = alloc.height / 2;
      img_width = gdk_pixbuf_get_width(pbuf);
      img_height = gdk_pixbuf_get_height(pbuf);
      img_top_left.x = center.x - img_width / 2;
      img_top_left.y = center.y - img_height / 2;

      start_pos.x = btn_event->x - img_top_left.x;
      start_pos.y = btn_event->y - img_top_left.y;
    }
}

static void
on_mouse_move(GtkWidget *widget,
              GdkEvent *event,
              gpointer data)
{
  GdkEventMotion *m_event;
  struct point current_pos;
  GtkBuilder *builder;
  GtkWidget *image;
  GdkPixbuf *pbuf;
  int img_width, img_height;
  struct point center, img_top_left;
  GtkAllocation alloc;

  m_event = (GdkEventMotion*)event;
  builder = GTK_BUILDER(data);
  image = GTK_WIDGET(gtk_builder_get_object(builder, "imagefirst"));
  if(mouse_flag == MOUSE_DOWN)
    {
      pbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));

      gtk_widget_get_allocation(image, &alloc);
      center.x = alloc.width / 2 + alloc.x;
      center.y = alloc.height / 2 + alloc.y;
      img_width = gdk_pixbuf_get_width(pbuf);
      img_height = gdk_pixbuf_get_height(pbuf);
      img_top_left.x = center.x - img_width / 2;
      img_top_left.y = center.y - img_height / 2;

      current_pos.x = m_event->x - img_top_left.x;
      current_pos.y = m_event->y - img_top_left.y;

      current_pos.x  = current_pos.x < 0 ? 0 : current_pos.x;
      current_pos.y  = current_pos.y < 0 ? 0 : current_pos.y;

      redraw(GTK_IMAGE(image), &start_pos, &current_pos);

      start_pos = current_pos;
    }
}

static void
on_mouse_up(GtkWidget *widget,
            GdkEvent *event,
            gpointer data)
{
  GdkEventButton *b_event;

  b_event = (GdkEventButton*)event;
  if(b_event->button == GDK_BUTTON_PRIMARY)
      mouse_flag = MOUSE_UP;
}

static void
setup_image_widget(GtkBuilder *builder)
{
  GtkWidget *image, *eventbox, *second_image;
  gint events, new_events;
  GdkPixbuf *blank_image;

  image = GTK_WIDGET(gtk_builder_get_object(builder,
                                            "imagefirst"));
  second_image = GTK_WIDGET(gtk_builder_get_object(builder,
                                            "imagesecond"));
  eventbox = GTK_WIDGET(gtk_builder_get_object(builder,
                                               "eventbox"));

  new_events = 0;
  events= gtk_widget_get_events(eventbox);
  if((events & GDK_BUTTON_PRESS_MASK) == 0)
    new_events |= GDK_BUTTON_PRESS_MASK;
  if((events & GDK_BUTTON_RELEASE_MASK) == 0)
    new_events |= GDK_BUTTON_RELEASE_MASK;
  if((events & GDK_POINTER_MOTION_MASK) == 0)
    new_events |= GDK_POINTER_MOTION_MASK;
  gtk_widget_add_events(eventbox, new_events);

  g_signal_connect(eventbox, "button-press-event",
                   G_CALLBACK(on_mouse_down), builder);
  g_signal_connect(eventbox, "button-release-event",
                   G_CALLBACK(on_mouse_up), NULL);
  g_signal_connect(eventbox, "motion-notify-event",
                   G_CALLBACK(on_mouse_move), builder);

  blank_image = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                               FALSE,
                               CHANNEL_DEPTH,
                               BLANK_IMG_WIDTH,
                               BLANK_IMG_HEIGHT);
  CLEAR_WHITE(blank_image);
  gtk_image_set_from_pixbuf(GTK_IMAGE(image), blank_image);
  gtk_image_set_from_pixbuf(GTK_IMAGE(second_image), blank_image);
  g_object_unref(blank_image);
}

static void
on_open_file(GtkFileChooserButton *button,
             GtkBuilder *builder)
{
  const gchar *file_name;
  GtkImage *image;

  image = GTK_IMAGE(gtk_builder_get_object(builder, "imagefirst"));
  file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));
  gtk_image_set_from_file(image, file_name);
}

static void
init_buttons_signals (GtkBuilder *builder)
{
  GtkWidget *clear_button, *recog_button;

  clear_button = GTK_WIDGET(gtk_builder_get_object(builder, "clearbutton"));
  g_signal_connect(GTK_BUTTON(clear_button), "clicked",
                   G_CALLBACK(on_clear), builder);

  recog_button = GTK_WIDGET(gtk_builder_get_object(builder, "recover"));
  g_signal_connect(GTK_BUTTON(recog_button), "clicked",
                   G_CALLBACK(on_recover), builder);
}

static void
setup_tree_view (GtkBuilder *builder)
{
  GtkTreeIter iter;
  GtkTreeStore *treestore;
  GtkWidget *treeview;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview"));
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(NULL, renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

  treestore = gtk_tree_store_new(1, G_TYPE_STRING);
  gtk_tree_store_append(treestore, &iter, NULL);
  gtk_tree_store_set(treestore, &iter, 0, "Сгенерированные классы", -1);
  gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(treestore));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

  g_signal_connect (treeview, "row-activated", G_CALLBACK(on_class_activated), builder);
}

void on_activate(GtkApplication *app,
                 gpointer data)
{
  GtkWidget *window;

  window = GTK_WIDGET(gtk_builder_get_object(builder,
                                             "mainwindow"));
  gtk_application_add_window(app, GTK_WINDOW(window));
  gtk_widget_show_all(window);
}

void on_startup(GtkApplication *app,
                gpointer data)
{
  builder = gtk_builder_new_from_file(ui_path);
  init_container (&container);
  w_matr = NULL;

  init_buttons_signals(builder);
  setup_image_widget(builder);
  add_action_entries(builder);
  setup_menu(builder);
  setup_tree_view (builder);
}

void on_shutdown(GtkApplication *app,
                 gpointer data)
{
  g_object_unref(builder);
  free_container (&container);

  if (w_matr != NULL)
    free (w_matr);
}

