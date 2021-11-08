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
#include <splash.h>

static GtkWidget *splash_win;
static GtkWidget *splash_label;

//GdkRGBA rgba = { 0.5, 0.5, 0.5, 1.0 }; // Warning, this is deprecated and must be replaced !

static gboolean update_splash_label_do(gpointer userdata);

void* splash_main(void* params)
{
  t_args* args = (t_args*) params;

  gtk_init (&(args->argc), &(args->argv));
  splash_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title ((GtkWindow*)splash_win, "A Splash Screen Demo");
  gtk_container_set_border_width (GTK_CONTAINER (splash_win), 0);
  gtk_widget_set_size_request(splash_win, 400, 400);

  GtkCssProvider *provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data(provider,
    ".splash_win_background {background-color: DarkGrey}", -1, NULL);
  GtkStyleContext *context = gtk_widget_get_style_context(splash_win);
  gtk_style_context_add_class(context, "splash_win_background");  gtk_style_context_add_provider(gtk_widget_get_style_context(splash_win),
    GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  //gtk_widget_override_background_color(splash_win, GTK_STATE_FLAG_NORMAL, &rgba);

  gtk_window_set_decorated((GtkWindow*)splash_win, FALSE);
  gtk_window_set_position((GtkWindow*)splash_win,GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_resizable((GtkWindow*)splash_win, FALSE);

  GtkWidget *image = gtk_image_new_from_file("darktable.svg");
  splash_label = gtk_label_new("Start");
  // gtk_label_set_xalign((GtkLabel*)splash_label, 0.0);
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start((GtkBox*)vbox, image, FALSE, FALSE, 0);
  gtk_box_pack_start((GtkBox*)vbox, splash_label, FALSE, FALSE, 0);
  gtk_container_add((GtkContainer*)splash_win, vbox);

  gtk_widget_show_all(splash_win);
  gtk_main ();
  return NULL;
}

void update_splash_label(char* msg)
{
  g_idle_add(update_splash_label_do, msg);
}

static gboolean update_splash_label_do(gpointer userdata)
{
  const gchar *msg = (const gchar*) userdata;
  gtk_label_set_label((GtkLabel*)splash_label, msg);
  return G_SOURCE_REMOVE;
}

void splash_quit(pthread_t thread_id)
{
  g_idle_add(splash_quit_do, msg);
}

void splash_quit(pthread_t tid)
{
  gtk_widget_destroy(splash_win);
  gtk_main_quit();
  pthread_cancel(tid);
}

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
