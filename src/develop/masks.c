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
#include "control/control.h"
#include "develop/imageop.h"
#include "develop/tiling.h"
#include "common/gaussian.h"
#include "masks.h"

int _create_buffer(dt_iop_module_t *module)
{
  dt_develop_t *dev = module->dev;
  dt_iop_masks_t *p = module->mask;
  dt_iop_masks_gui_t *g = module->mask_gui;
  float wd = dev->preview_pipe->iwidth;
  float ht = dev->preview_pipe->iheight;

  //how many points do we need ?
  float r = p->radius*wd;
  int l = (int) (2.0*M_PI*r);
  
  //buffer allocations
  g->points = malloc(2*(l+1)*sizeof(float));
  g->points_count = l+1;
  
  
  //now we set the points
  g->points[0] = p->center[0]*wd;
  g->points[1] = p->center[1]*ht;
  for (int i=1; i<l+1; i++)
  {
    float alpha = (i-1)*2.0*M_PI/(float) l;
    g->points[i*2] = g->points[0] + r*cosf(alpha);
    g->points[i*2+1] = g->points[1] + r*sinf(alpha);
  }
  return 1;
}
int _remove_buffer(dt_iop_module_t *module)
{
  dt_develop_t *dev = module->dev;
  dt_iop_masks_t *p = module->mask;
  dt_iop_masks_gui_t *g = module->mask_gui;
  g->points_count = 0;
  free(g->points);
  return 1;
}

int _move_buffer(dt_iop_module_t *module, float x, float y)
{
  dt_develop_t *dev = module->dev;
  dt_iop_masks_t *p = module->mask;
  dt_iop_masks_gui_t *g = module->mask_gui;
  float xx = x*dev->preview_pipe->iwidth;
  float yy = y*dev->preview_pipe->iheight;

  for (int i=0; i<l+1; i++)
  {
    g->points[i*2] += xx;
    g->points[i*2+1] += yy;
  }
  return 1;
}

int _update_radius(dt_iop_module_t *module)
{
  _remove_buffer(module);
  
  return _create_buffer(module);
}

int dt_iop_masks_init(dt_iop_module_t *module)
{
  //we create the label if needed
  if (!module->mask_gui->label) module->mask_gui->label = gtk_label_new(_("one circular mask"));
  
  //we set the label visible or not
  if (module->mask->used) gtk_widget_show(module->mask_gui->label);
  else gtk_widget_hide(module->mask_gui->label);
}

int dt_iop_masks_set_edit(dt_iop_module_t *module,int editing)
{
  module->mask_gui->editing = editing;
}


int dt_iop_masks_button_pressed(dt_iop_module_t *module, double x, double y, int which, int type, uint32_t state)
{
  if (which != 1) return 0;
  if (!module->mask_gui->editing) return 0;
  
  dt_iop_mask_t *p = module->mask;
  dt_iop_mask_gui_t *g=module->mask_gui;
  float pzx, pzy;
  dt_dev_get_pointer_zoom_pos(module->dev, x, y, &pzx, &pzy);
  pzx += 0.5f;
  pzy += 0.5f;
      
  //do we need to create a new spot ?
  if (!p->used)
  {
    p->center[0] = pzx;
    p->center[1] = pzy;
    p->radius = 15/module->dev->backbuf_width;
    p->border = p->radius/3.0;
    p->used = 1;
    dt_control_queue_redraw_center();
    return 1;
  }
  else if (g->selected)
  {
    g->dragging = TRUE;
    dt_control_queue_redraw_center();
    return 1;
  }
  return 0;
}

int dt_iop_masks_button_released(struct dt_iop_module_t *module, double x, double y, int which, uint32_t state)
{
  if (which != 1) return 0;
  if (!module->mask_gui->editing) return 0;
  if (!module->mask->used) return 0;
  
  if (g->dragging)
  {
    g->dragging = FALSE;
    dt_control_queue_redraw_center();
    return 1;
  }
  return 0;  
}

int dt_iop_masks_scrolled(dt_iop_module_t *module, double x, double y, int up, uint32_t state)
{
  if (!module->mask_gui->editing) return 0;
  if (!module->mask->used) return 0;
  
  if (g->selected)
  {
    if (up)
    {
      p->radius *= 1.1;
    }
    else p->radius *= 0.9;
    _update_radius(module);
    dt_control_queue_redraw_center();
    return 1;
  }
  return 0;
}

int dt_iop_masks_mouse_moved(dt_iop_module_t *module, double x, double y, int which)
{
  if (which > 1) return 0;
  if (!module->mask_gui->editing) return 0;
  if (!module->mask->used) return 0;
  
  dt_iop_mask_t *p = module->mask;
  dt_iop_mask_gui_t *g=module->mask_gui;
  float pzx, pzy;
  dt_dev_get_pointer_zoom_pos(module->dev, x, y, &pzx, &pzy);
  pzx += 0.5f;
  pzy += 0.5f;
  
  if (g->dragging)
  {
    _move_buffer(module,pzx-p->center[0],pzy-p->center[1]);
    p->center[0] = pzx;
    p->center[1] = pzy;
    dt_control_queue_redraw_center();
    return 1;
  }
  else
  {
    //are we near the spot ?
    if (pzx > p->center[0]-radius-border && pzx < p->center[0]+radius+border && pzy > p->center[1]-radius-border && pzy < p->center[1]+radius+border)
    {
      g->selected = TRUE;
    }
    else g->selected = FALSE;
    dt_control_queue_redraw_center();
    return 1;
  }
  return 0;
}

int dt_iop_masks_post_expose(dt_iop_module_t *module, cairo_t *cr, int32_t width, int32_t height, int32_t pointerx, int32_t pointery)
{
  if (!module->mask_gui->editing) return 0;
  if (!module->mask->used) return 0;
  
  //create the buffer if needed
  if (g->points_count < 7)
  {
    if (!_create_buffer(module)) return 0;
  }
  
  //if the buffer is created, we draw it
  if (g->points_count < 7) return 0;
  
}

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;
