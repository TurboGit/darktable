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
#include "develop/masks.h"

int _circle_get_points(dt_develop_t *dev, float x, float y, float radius, float **points, int *points_count)
{
  float wd = dev->preview_pipe->iwidth;
  float ht = dev->preview_pipe->iheight;

  //how many points do we need ?
  float r = radius*wd;
  int l = (int) (2.0*M_PI*r);
  
  //buffer allocations
  *points = malloc(2*(l+1)*sizeof(float));
  *points_count = l+1;  
  
  //now we set the points
  (*points)[0] = x*wd;
  (*points)[1] = y*ht;
  for (int i=1; i<l+1; i++)
  {
    float alpha = (i-1)*2.0*M_PI/(float) l;
    (*points)[i*2] = (*points)[0] + r*cosf(alpha);
    (*points)[i*2+1] = (*points)[1] + r*sinf(alpha);
  }
  
  //and we transform them with all distorted modules
  if (dt_dev_distort_transform(dev,*points,l+1)) return 1;
  
  //if we failed, then free all and return
  free(*points);
  *points = NULL;
  *points_count = 0;
  return 0;  
}

int dt_masks_circle_get_points(dt_develop_t *dev, dt_masks_circle_t circle, float **points, int *points_count)
{
  return _circle_get_points(dev,circle.center[0], circle.center[1], circle.radius, points, points_count); 
}

int dt_masks_circle_get_border(dt_develop_t *dev, dt_masks_circle_t circle, float **border, int *border_count)
{
  return _circle_get_points(dev,circle.center[0], circle.center[1], circle.radius + circle.border, border, border_count);   
}

int dt_masks_circle_get_area(dt_iop_module_t *module, dt_masks_circle_t circle, int *width, int *height, int *posx, int *posy)
{
  float wd = module->dev->pipe->iwidth;
  float ht = module->dev->pipe->iheight;
  
  float r = (circle.radius + circle.border)*wd;
  int l = (int) (2.0*M_PI*r);
  
  //buffer allocations
  float *points = malloc(2*(l+1)*sizeof(float)); 
  
  //now we set the points
  points[0] = circle.center[0]*wd;
  points[1] = circle.center[1]*ht;
  for (int i=1; i<l+1; i++)
  {
    float alpha = (i-1)*2.0*M_PI/(float) l;
    points[i*2] = points[0] + r*cosf(alpha);
    points[i*2+1] = points[1] + r*sinf(alpha);
  }
  
  //and we transform them with all distorted modules
  if (dt_dev_distort_transform_plus(module->dev,0,module->priority,0,points,l+1)) return 0;
  
  //now we search min and max
  float xmin, xmax, ymin, ymax;
  xmin = ymin = FLT_MAX;
  xmax = ymax = FLT_MIN;
  for (int i=1; i < l+1; i++)
  {
    xmin = fminf(points[i*2],xmin);
    xmax = fmaxf(points[i*2],xmax);
    ymin = fminf(points[i*2+1],ymin);
    ymax = fmaxf(points[i*2+1],ymax);
  }
  
  //and we set values
  *posx = xmin;
  *posy = ymin;
  *width = xmax-xmin;
  *height = ymax-ymin;
  return 1;
}

int dt_masks_circle_get_mask(dt_iop_module_t *module, dt_masks_circle_t circle, float **buffer, int *width, int *height, int *posx, int *posy)
{
  //we get the area
  dt_masks_circle_get_area(module,circle,width,height,posx,posy);
  
  //we create a buffer of points with all points in the area
  int w = *width, h = *height;
  float *points = malloc(w*h*2*sizeof(float));
  for (int i=0; i<h; i++)
    for (int j=0; j<w; j++)
    {
      points[(i*w+j)*2] = j+(*posx);
      points[(i*w+j)*2+1] = i+(*posy);
    }
    
  //we back transform all this points
  dt_dev_distort_backtransform_plus(module->dev,0,module->priority,0,points,w*h);
  
  //we allocate the buffer
  *buffer = malloc(w*h*sizeof(float));
  
  //we populate the buffer
  float wd = module->dev->pipe->iwidth;
  float ht = module->dev->pipe->iheight;
  float center[2] = {circle.center[0]*wd, circle.center[1]*ht};
  float radius2 = circle.radius*wd*circle.radius*wd;
  float total2 = (circle.radius+circle.border)*wd*(circle.radius+circle.border)*wd;
  for (int i=0; i<h; i++)
    for (int j=0; j<w; j++)
    {
      float x = points[(i*w+j)*2];
      float y = points[(i*w+j)*2+1];
      float l2 = (x-center[0])*(x-center[0]) + (y-center[1])*(y-center[1]);
      if (l2<radius2) (*buffer)[i*w+j] = 1.0f;
      else if (l2 < total2)
      {
        (*buffer)[i*w+j] = (total2-l2)/(total2-radius2);
      }
      else (*buffer)[i*w+j] = 0.0f;
    }
  free(points);
  return 1;
}

/*
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

  dt_iop_mask_t *p = module->mask;
  dt_iop_mask_gui_t *g = module->mask_gui;
  
  //create the buffer if needed
  if (g->points_count < 7)
  {
    if (!_create_buffer(module)) return 0;
  }
  
  //if the buffer is created, we draw it
  if (g->points_count < 7) return 0;
  
  double dashed[] = {4.0, 2.0};
  dashed[0] /= zoom_scale;
  dashed[1] /= zoom_scale;
  cairo_set_dash(cr, dashed, 0, 0);
  cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
  cairo_set_source_rgba(cr, .3, .3, .3, .8);
  
  if (g->selected || g->dragging) cairo_set_line_width(cr, 5.0/zoom_scale);
  else cairo_set_line_width(cr, 3.0/zoom_scale);
  if (g->dragging)
  {
    src_x = p->spot[i].xc*wd;
    src_y = p->spot[i].yc*ht;
    cairo_arc (cr, src_x, src_y, 10.0, 0, 2.0*M_PI);
  }
  else
  {
    cairo_move_to(cr,gspt.source[2],gspt.source[3]);
    for (int i=2; i<gspt.pts_count; i++)
    {
      cairo_line_to(cr,gspt.source[i*2],gspt.source[i*2+1]);
    }
    cairo_line_to(cr,gspt.source[2],gspt.source[3]);
    src_x = gspt.source[0];
    src_y = gspt.source[1];
  }
  cairo_stroke_preserve(cr);
  if(i == g->selected || i == g->dragging) cairo_set_line_width(cr, 2.0/zoom_scale);
  else                                     cairo_set_line_width(cr, 1.0/zoom_scale);
  cairo_set_source_rgba(cr, .8, .8, .8, .8);
  cairo_stroke(cr);
}
*/

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;
