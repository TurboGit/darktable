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
  GtkWidget *drawer;
  GtkWidget *text_view;
  GtkTextBuffer *text_buffer;
  GtkTextMark *end_mark;
  gboolean drawer_visible;
} dt_lib_log_history_t;

static void _populate_text_buffer(dt_lib_module_t *self);
static void _log_redraw_callback(gpointer instance, dt_lib_module_t *self);

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
  if(count == 0) return;

  gtk_text_buffer_set_text(d->text_buffer, "", -1);

  GtkTextIter iter;
  gtk_text_buffer_get_end_iter(d->text_buffer, &iter);

  for(int i = 0; i < count; i++)
  {
    gchar *line = g_strdup_printf("[%s] %s\n", timestamps[i], msgs[i]);
    gtk_text_buffer_insert(d->text_buffer, &iter, line, -1);
    g_free(line);
    gtk_text_buffer_get_end_iter(d->text_buffer, &iter);
  }

  // scroll to end
  gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(d->text_view), d->end_mark, 0.0, FALSE, 0.0, 0.0);
}

static void _log_redraw_callback(gpointer instance, dt_lib_module_t *self)
{
  dt_lib_log_history_t *d = self->data;
  if(d->drawer_visible)
    _populate_text_buffer(self);
}

static void _button_clicked(GtkWidget *widget, gpointer user_data)
{
  dt_lib_module_t *self = user_data;
  dt_lib_log_history_t *d = self->data;

  d->drawer_visible = !d->drawer_visible;
  gtk_widget_set_visible(d->drawer, d->drawer_visible);

  if(d->drawer_visible)
    _populate_text_buffer(self);

  // update button appearance
  dtgtk_button_set_paint(DTGTK_BUTTON(widget), dtgtk_cairo_paint_messages,
                         (d->drawer_visible ? CPF_DIRECTION_DOWN : CPF_NONE), NULL);
  gtk_widget_queue_draw(widget);
}

void gui_init(dt_lib_module_t *self)
{
  dt_lib_log_history_t *d = g_malloc0(sizeof(dt_lib_log_history_t));
  self->data = d;

  // main vertical box: drawer + button
  self->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_name(self->widget, "log-history-container");

  // drawer (scrolled window with text view)
  d->text_buffer = gtk_text_buffer_new(NULL);
  d->text_view = gtk_text_view_new_with_buffer(d->text_buffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(d->text_view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(d->text_view), FALSE);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(d->text_view), 4);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(d->text_view), 4);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(d->text_view), 4);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(d->text_view), 4);
  gtk_widget_set_name(d->text_view, "log-history-text");

  GtkTextIter end_iter;
  gtk_text_buffer_get_end_iter(d->text_buffer, &end_iter);
  d->end_mark = gtk_text_buffer_create_mark(d->text_buffer, NULL, &end_iter, FALSE);

  d->drawer = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(d->drawer),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(d->drawer),
                                             DT_PIXEL_APPLY_DPI(150));
  gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(d->drawer),
                                             DT_PIXEL_APPLY_DPI(400));
  gtk_container_add(GTK_CONTAINER(d->drawer), d->text_view);
  gtk_widget_set_name(d->drawer, "log-history-drawer");
  gtk_widget_set_visible(d->drawer, FALSE);
  gtk_box_pack_start(GTK_BOX(self->widget), d->drawer, TRUE, TRUE, 0);

  // button row
  GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  d->button = dtgtk_button_new(dtgtk_cairo_paint_messages, CPF_NONE, NULL);
  gtk_widget_set_tooltip_text(d->button, _("view log history"));
  g_signal_connect(G_OBJECT(d->button), "clicked",
                   G_CALLBACK(_button_clicked), self);

  gtk_box_pack_start(GTK_BOX(button_box), d->button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(self->widget), button_box, FALSE, FALSE, 0);

  // connect to log redraw signal
  DT_CONTROL_SIGNAL_CONNECT(DT_SIGNAL_CONTROL_LOG_REDRAW,
                            G_CALLBACK(_log_redraw_callback), self);
}

void gui_cleanup(dt_lib_module_t *self)
{
  dt_lib_log_history_t *d = self->data;

  DT_CONTROL_SIGNAL_DISCONNECT(G_CALLBACK(_log_redraw_callback), self);

  g_free(d);
  self->data = NULL;
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
