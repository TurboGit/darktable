/*
    This file is part of darktable,
    Copyright (C) 2009-2021 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gtk/gtk.h>
#include <unistd.h>
#include "splash.h"
#include <splash-resource.h>

static struct {
  GResource *splash_resource;
  GtkWidget *splash_win;
  GtkWidget *splash_label;
  pthread_t thread_id;
} _dt_splash_t;

static gboolean _dt_update_splash_label_do(gpointer userdata);

static void* _dt_splash_start_do();

void dt_splash_start()
{
  pthread_create(&_dt_splash_t.thread_id, NULL, &_dt_splash_start_do, NULL);
  return;
}

static void* _dt_splash_start_do()
{
  // RESOURCE
  _dt_splash_t.splash_resource = dt_splash_get_resource();
  g_resources_register(_dt_splash_t.splash_resource);

  //WINDOW
  _dt_splash_t.splash_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title ((GtkWindow*)_dt_splash_t.splash_win, "darktable start");
  gtk_container_set_border_width (GTK_CONTAINER (_dt_splash_t.splash_win), 0);
  gtk_widget_set_size_request(_dt_splash_t.splash_win, 400, 400);
  gtk_window_set_decorated((GtkWindow*)_dt_splash_t.splash_win, FALSE);
  gtk_window_set_position((GtkWindow*)_dt_splash_t.splash_win,GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_resizable((GtkWindow*)_dt_splash_t.splash_win, FALSE);
  gtk_window_set_keep_above((GtkWindow*)_dt_splash_t.splash_win,TRUE);
  // IMAGE
  GtkWidget *splash_image = gtk_image_new_from_resource("/darktable/splash-screen/splash.svg");

  // LABEL
  _dt_splash_t.splash_label = gtk_label_new("Start");

  // CSS
  GtkCssProvider *provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource(provider, "/darktable/splash-screen/splash.css");

  GtkStyleContext *win_context = gtk_widget_get_style_context(_dt_splash_t.splash_win);
  gtk_style_context_add_class(win_context, "splash_win");  gtk_style_context_add_provider(gtk_widget_get_style_context(_dt_splash_t.splash_win),
    GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkStyleContext *img_context = gtk_widget_get_style_context(splash_image);
  gtk_style_context_add_class(img_context, "splash_image");  gtk_style_context_add_provider(gtk_widget_get_style_context(splash_image),
    GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkStyleContext *label_context = gtk_widget_get_style_context(_dt_splash_t.splash_label);
  gtk_style_context_add_class(label_context, "splash_label");  gtk_style_context_add_provider(gtk_widget_get_style_context(_dt_splash_t.splash_label),
    GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start((GtkBox*)vbox, splash_image, FALSE, FALSE, 0);
  gtk_box_pack_start((GtkBox*)vbox, _dt_splash_t.splash_label, FALSE, FALSE, 0);
  gtk_container_add((GtkContainer*)_dt_splash_t.splash_win, vbox);
  gtk_widget_show_all(_dt_splash_t.splash_win);

  gtk_main();
  return NULL;
}

void dt_update_splash_label(char* msg)
{
  g_idle_add(_dt_update_splash_label_do, msg);
}

static gboolean _dt_update_splash_label_do(gpointer userdata)
{
  const gchar *msg = (const gchar*) userdata;
  gtk_label_set_label((GtkLabel*)_dt_splash_t.splash_label, msg);
  return G_SOURCE_REMOVE;
}

void dt_splash_quit()
{
  gtk_widget_destroy(_dt_splash_t.splash_win);
  g_resources_unregister(_dt_splash_t.splash_resource);
}

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
