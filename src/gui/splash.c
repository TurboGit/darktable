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
#include <common/dtpthread.h>

static gboolean _dt_update_splash_label_do(gpointer userdata);

static void* _dt_splash_draw();

dt_splash_t *dt_splash_start()
{
  dt_splash_t *s = (dt_splash_t *)malloc(sizeof(dt_splash_t));
  s->close = FALSE;

  dt_pthread_create(&(s->thread_id), _dt_splash_draw, s);
  return s;
}

static gboolean _dt_splash_check(gpointer data)
{
  dt_splash_t *s = (dt_splash_t *)data;
  if (s == NULL)
  {
    fprintf (stderr, "unable to create splash screen\n");
    return FALSE;
  }

/*JPV*/ printf("_dt_splash_check() s->close = %s\n", s->close == FALSE ? "FALSE" : "TRUE");
  if(s->close == TRUE)
  {
/*JPV*/ printf("_dt_splash_check() close window\n");
    gtk_widget_destroy(s->splash_win);
    gtk_main_quit();
    g_resources_unregister(s->splash_resource);
    return TRUE;
  }
  else return FALSE;
}

static void *_dt_splash_draw(void *data)
{
//  #define UNUSED(x)        ((void)(x))
  dt_splash_t *s = (dt_splash_t *)data;

  if (s == NULL) return NULL;
  // RESOURCE
  s->splash_resource = dt_splash_get_resource();
  g_resources_register(s->splash_resource);

  //WINDOW
  s->splash_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title ((GtkWindow*)s->splash_win, "darktable start");
  gtk_container_set_border_width (GTK_CONTAINER(s->splash_win), 0);
  gtk_widget_set_size_request(s->splash_win, 400, 400);
  gtk_window_set_decorated((GtkWindow*)s->splash_win, FALSE);
  gtk_window_set_position((GtkWindow*)s->splash_win,GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_type_hint((GtkWindow*)s->splash_win, GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
  gtk_window_set_resizable((GtkWindow*)s->splash_win, FALSE);
  gtk_window_set_keep_above((GtkWindow*)s->splash_win,TRUE);
  /*
  // to check if the display supports alpha channels, get the visual
  //GdkScreen *screen = gtk_widget_get_screen(s->splash_win);
  //GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
  if(!visual)
  {
    visual = gdk_screen_get_system_visual(screen);
    splashscreen->has_alpha = FALSE;
  }
  else
    splashscreen->has_alpha = TRUE;
  gtk_widget_set_visual(s->splash_win, visual);
  */

  // IMAGE
  GtkWidget *splash_image = gtk_image_new_from_resource("/darktable/splash-screen/splash.svg");

  // LABEL
  s->splash_label = gtk_label_new("Start");

  // CSS
  GtkCssProvider *provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource(provider, "/darktable/splash-screen/splash.css");

  GtkStyleContext *win_context = gtk_widget_get_style_context(s->splash_win);
  gtk_style_context_add_class(win_context, "splash_win");  gtk_style_context_add_provider(gtk_widget_get_style_context(s->splash_win),
    GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkStyleContext *img_context = gtk_widget_get_style_context(splash_image);
  gtk_style_context_add_class(img_context, "splash_image");  gtk_style_context_add_provider(gtk_widget_get_style_context(splash_image),
    GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkStyleContext *label_context = gtk_widget_get_style_context(s->splash_label);
  gtk_style_context_add_class(label_context, "splash_label");  gtk_style_context_add_provider(gtk_widget_get_style_context(s->splash_label),
    GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start((GtkBox*)vbox, splash_image, FALSE, FALSE, 0);
  gtk_box_pack_start((GtkBox*)vbox, s->splash_label, FALSE, FALSE, 0);
  gtk_container_add((GtkContainer*)s->splash_win, vbox);
  gtk_widget_show_all(s->splash_win);

  g_timeout_add(500, _dt_splash_check, data);
  s->close = FALSE ;
/*JPV*/ printf("_dt_splash_start_do() before gtk_main()\n");
  gtk_main();
  return NULL;
}

void dt_update_splash_label(dt_splash_t *s)
{
  g_idle_add((GSourceFunc)_dt_update_splash_label_do, s);
}

static gboolean _dt_update_splash_label_do(gpointer data)
{
  dt_splash_t *s = (dt_splash_t *)data;
  if (s == NULL) return FALSE;
  gtk_label_set_label((GtkLabel*)s->splash_label, s->msg);
  return G_SOURCE_REMOVE;
}

void dt_splash_quit(dt_splash_t *data)
{
  dt_splash_t *s = (dt_splash_t *)data;
  if (s == NULL) return;
  s->close = TRUE;
  //pthread_join(dt_splash->thread_id, NULL);
}

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
