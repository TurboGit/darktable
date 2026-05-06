/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.

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

#include "control/signal.h"
#include "control/control.h"
#include "dtgtk/button.h"
#include "gui/gtk.h"
#include "libs/lib.h"
#include "libs/lib_api.h"

DT_MODULE(1)

typedef struct dt_lib_log_history_t
{
  GtkWidget *button;
  GtkWidget *popover;
  GtkWidget *text_view;
  GtkTextBuffer *text_buffer;
  GtkTextMark *end_mark;
} dt_lib_log_history_t;

static void _populate_text_buffer(dt_lib_module_t *self);
static void _log_redraw_callback(gpointer instance, dt_lib_module_t *self);
static gboolean _button_press_release(GtkWidget *button,
                                      GdkEventButton *event,
                                      dt_lib_module_t *self);

const char *name(dt_lib_module_t *self)
{
  return N_("log history");
}

dt_view_type_flags_t views(dt_lib_module_t *self)
{
  return DT_VIEW_ALL;
}

uint32_t container(dt_lib_module_t *self)
{
  return DT_UI_CONTAINER_PANEL_CENTER_BOTTOM_RIGHT;
}

int expandable(dt_lib_module_t *self)
{
  return 0;
}

int position(const dt_lib_module_t *self)
{
  return 1000;
}

static void _populate_text_buffer(dt_lib_module_t *self)
{
  dt_lib_log_history_t *d = self->data;
  if(!d || !d->text_buffer) return;

  char *msgs[DT_CTL_LOG_HISTORY_SIZE];
  char *timestamps[DT_CTL_LOG_HISTORY_SIZE];

  const int count = dt_control_log_history_get_entries(msgs, timestamps, DT_CTL_LOG_HISTORY_SIZE);

  gtk_text_buffer_set_text(d->text_buffer, "", -1);

  if(count == 0) return;

  GtkTextIter iter;
  gtk_text_buffer_get_end_iter(d->text_buffer, &iter);
  gtk_text_buffer_move_mark(d->text_buffer, d->end_mark, &iter);

  for(int i = 0; i < count; i++)
  {
    gchar *line = g_strdup_printf("[%s] %s\n", timestamps[i], msgs[i]);
    gtk_text_buffer_insert(d->text_buffer, &iter, line, -1);
    g_free(line);
    gtk_text_buffer_get_end_iter(d->text_buffer, &iter);
    gtk_text_buffer_move_mark(d->text_buffer, d->end_mark, &iter);
  }

  gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(d->text_view), d->end_mark, 0.0, FALSE, 0.0, 0.0);
}

static void _log_redraw_callback(gpointer instance, dt_lib_module_t *self)
{
  dt_lib_log_history_t *d = self->data;
  if(d->popover && gtk_widget_is_visible(d->popover))
    _populate_text_buffer(self);
}

static gboolean _button_press_release(GtkWidget *button,
                                      GdkEventButton *event,
                                      dt_lib_module_t *self)
{
  static guint start_time = 0;

  int delay = 0;
  g_object_get(gtk_settings_get_default(), "gtk-long-press-time", &delay, NULL);

  if((event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) ||
     (event->type == GDK_BUTTON_RELEASE && event->time - start_time > delay))
  {
    dt_lib_log_history_t *d = self->data;
    if(gtk_widget_is_visible(d->popover))
      gtk_popover_popdown(GTK_POPOVER(d->popover));
    else
    {
      _populate_text_buffer(self);
      gtk_popover_popup(GTK_POPOVER(d->popover));
    }
    return TRUE;
  }
  else
  {
    start_time = event->time;
    return FALSE;
  }
}

void gui_init(dt_lib_module_t *self)
{
  dt_lib_log_history_t *d = g_malloc0(sizeof(dt_lib_log_history_t));
  self->data = d;

  self->widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  d->button = dtgtk_button_new(dtgtk_cairo_paint_messages, CPF_NONE, NULL);
  gtk_widget_set_tooltip_text(d->button, _("view log history\n(right-click to open)"));

  d->popover = gtk_popover_new(d->button);
  gtk_widget_set_name(d->popover, "log-history-popover");
  gtk_popover_set_position(GTK_POPOVER(d->popover), GTK_POS_TOP);
  gtk_container_set_border_width(GTK_CONTAINER(d->popover), 0);
  gtk_widget_set_size_request(d->popover,
                              DT_PIXEL_APPLY_DPI(500),
                              DT_PIXEL_APPLY_DPI(300));

  d->text_buffer = gtk_text_buffer_new(NULL);
  d->text_view = gtk_text_view_new_with_buffer(d->text_buffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(d->text_view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(d->text_view), FALSE);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(d->text_view), 6);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(d->text_view), 6);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(d->text_view), 4);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(d->text_view), 4);
  gtk_widget_set_name(d->text_view, "log-history-text");

  GtkTextIter end_iter;
  gtk_text_buffer_get_end_iter(d->text_buffer, &end_iter);
  d->end_mark = gtk_text_buffer_create_mark(d->text_buffer, NULL, &end_iter, TRUE);

  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrolled), d->text_view);
  gtk_widget_set_name(scrolled, "log-history-scrolled");
  gtk_widget_set_margin_top(scrolled, 0);
  gtk_widget_set_margin_bottom(scrolled, 0);
  gtk_widget_set_margin_start(scrolled, 0);
  gtk_widget_set_margin_end(scrolled, 0);
  gtk_widget_set_hexpand(d->text_view, TRUE);
  gtk_widget_set_vexpand(d->text_view, TRUE);

  gtk_container_add(GTK_CONTAINER(d->popover), scrolled);
  gtk_widget_set_hexpand(scrolled, TRUE);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_widget_show_all(scrolled);

  g_signal_connect(G_OBJECT(d->button), "button-press-event",
                   G_CALLBACK(_button_press_release), self);
  g_signal_connect(G_OBJECT(d->button), "button-release-event",
                   G_CALLBACK(_button_press_release), self);

  gtk_box_pack_start(GTK_BOX(self->widget), d->button, FALSE, FALSE, 0);

  DT_CONTROL_SIGNAL_CONNECT(DT_SIGNAL_CONTROL_LOG_REDRAW,
                            G_CALLBACK(_log_redraw_callback), self);
}

void gui_cleanup(dt_lib_module_t *self)
{
  dt_lib_log_history_t *d = self->data;

  DT_CONTROL_SIGNAL_DISCONNECT(G_CALLBACK(_log_redraw_callback), self);

  if(d->popover)
    gtk_widget_destroy(d->popover);

  g_free(d);
  self->data = NULL;
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
