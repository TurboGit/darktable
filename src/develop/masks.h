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

int dt_masks_circle_get_points(dt_develop_t *dev, dt_masks_circle_t circle, float **points, int *points_count, float dx, float dy);
int dt_masks_circle_get_border(dt_develop_t *dev, dt_masks_circle_t circle, float **border, int *border_count, float dx, float dy);

int dt_masks_circle_get_area(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_circle_t circle, int *width, int *height, int *posx, int *posy);
int dt_masks_circle_get_mask(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_circle_t circle, float **buffer, int *width, int *height, int *posx, int *posy);

#endif
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;
