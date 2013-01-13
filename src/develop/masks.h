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

/**forms types */
typedef enum dt_masks_type_t
{
  DT_MASKS_NONE = 0, // keep first
  DT_MASKS_CIRCLE = 1,
  DT_MASKS_BEZIER = 2

}
dt_masks_type_t;

/** structure used to store 1 point for a circle */
typedef struct dt_masks_point_circle_t
{
  float center[2];
  float radius;
  float border;
}
dt_masks_point_circle_t;

/** structure used to store 1 point for a bezier form */
typedef struct dt_masks_point_bezier_t
{
  float corner[2];
  float ctrl1[2];
  float ctrl2[2];
  float border[2];
}
dt_masks_point_bezier_t;

/** structure used to define a form */
typedef struct dt_masks_form_t
{
  GList *points;
  dt_masks_type_t type;
  
  //id used to store the form
  double formid;
  
  //version of the form
  int version;
}
dt_masks_form_t;

dt_masks_point_circle_t *dt_masks_get_circle(dt_masks_form_t *form);

/** get points in real space with respect of distortion dx and dy are used to eventually move the center of the circle */
int dt_masks_get_points(dt_develop_t *dev, dt_masks_form_t *form, float **points, int *points_count, float dx, float dy);
int dt_masks_get_border(dt_develop_t *dev, dt_masks_form_t *form, float **border, int *border_count, float dx, float dy);

/** get the rectangle which include the form and his border */
int dt_masks_get_area(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_form_t *form, int *width, int *height, int *posx, int *posy);
/** get the transparency mask of the form and his border */
int dt_masks_get_mask(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_form_t *form, float **buffer, int *width, int *height, int *posx, int *posy);

/** we create a completly new form. */
dt_masks_form_t *dt_masks_create(dt_masks_type_t type);
/** retrieve a form with is id */
dt_masks_form_t *dt_masks_get_from_id(dt_develop_t *dev, double id);

/** read the forms from the db */
void dt_masks_read_forms(dt_develop_t *dev);
/** write the forms into the db */
void dt_masks_write_form(dt_masks_form_t *form, dt_develop_t *dev);
void dt_masks_write_forms(dt_develop_t *dev);

#endif
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;
