/*
    This file is part of darktable,
    copyright (c) 2012 aldric renaudin.

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

#ifndef DT_DEVELOP_MASKS_H
#define DT_DEVELOP_MASKS_H

#include "dtgtk/button.h"
#include "dtgtk/icon.h"
#include "dtgtk/tristatebutton.h"
#include "dtgtk/slider.h"
#include "dtgtk/tristatebutton.h"
#include "dtgtk/gradientslider.h"
#include "develop/pixelpipe.h"
#include "common/opencl.h"

typedef enum dt_masks_type_t
{
  DT_MASKS_NONE     = 0,
  DT_MASKS_CIRCLE   = 1,
  DT_MASKS_POLYGON  = 2,
  DT_MASKS_BEZIER   = 3
}
dt_masks_type_t;

/** structure used to define a circle */
typedef struct dt_masks_circle_t
{
  float center[2];
  float radius;
  float border;
  
  //id used to store the form
  long form_id;
}
dt_masks_circle_t;

typedef struct dt_masks_bezier_t
{
  //points used to define the form.
  //ctrl1aX - ctrl1aY - pt1X - pt1Y - ctrl1bX - ctrl1bY - .....
  float points[600];
  //points used to draw the border : 1 point per node
  float border[300];
  
  //nb of points (<=150)
  int points_count;
  
  dt_masks_type_t form;
  
  //id used to store the form
  long form_id;
}
dt_masks_bezier_t;

/*
typedef struct dt_masks_gui_form_t
{
  //all the point needed to draw the form
  float *points;
  int points_count;
  float *border;
  int border_count;
  
  gboolean dragging;
  gboolean selected;

  int selected_pts;
  int selected_seg;
  int dragging_pts;
  int dragging seg;
  
  gboolean ok;
  
  uint64_t pipe_hash;
}
dt_masks_gui_form_t;
*/

int dt_masks_circle_get_points(dt_develop_t *dev, dt_masks_circle_t circle, float **points, int *points_count);
int dt_masks_circle_get_border(dt_develop_t *dev, dt_masks_circle_t circle, float **border, int *border_count);

int dt_masks_circle_get_mask(dt_develop_t *dev, dt_masks_circle_t circle, float **buffer, int *width, int *height);

#if 0
/** initialise masks */
int dt_iop_masks_init(dt_iop_module_t *module);

/** set masks edit mode */
int dt_iop_masks_set_edit(dt_iop_module_t *module,int editing);

/** gui events */
int dt_iop_masks_button_pressed(dt_iop_module_t *module, double x, double y, int which, int type, uint32_t state);
int dt_iop_masks_button_released(struct dt_iop_module_t *module, double x, double y, int which, uint32_t state);
int dt_iop_masks_scrolled(dt_iop_module_t *module, double x, double y, int up, uint32_t state);
int dt_iop_masks_mouse_moved(dt_iop_module_t *module, double x, double y, int which);

/** drawing to canvas routine */
int dt_iop_masks_post_expose(dt_iop_module_t *module, cairo_t *cr, int32_t width, int32_t height, int32_t pointerx, int32_t pointery);

/** used to get the mask buffer, his size and coordinates */
/** mask is store a 1 channel float. values are transparency */
int dt_iop_masks_get_mask();
#endif

#endif
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;
