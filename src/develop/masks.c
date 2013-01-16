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
#include "develop/imageop.h"
#include "develop/blend.h"
#include "control/control.h"
#include "control/conf.h"
#include "develop/masks.h"
#include "common/debug.h"

int _circle_get_points(dt_develop_t *dev, float x, float y, float radius, float **points, int *points_count)
{
  float wd = dev->preview_pipe->iwidth;
  float ht = dev->preview_pipe->iheight;

  //how many points do we need ?
  float r = radius*MIN(wd,ht);
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

int _circle_get_area(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_form_t *form, int *width, int *height, int *posx, int *posy)
{  
  //we get the cicle values
  dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form->points)->data);
  
  float r = (circle->radius + circle->border)*MIN(wd,ht);
  int l = (int) (2.0*M_PI*r);
  //buffer allocations
  float *points = malloc(2*(l+1)*sizeof(float)); 
  
  //now we set the points
  points[0] = circle->center[0]*wd;
  points[1] = circle->center[1]*ht;
  for (int i=1; i<l+1; i++)
  {
    float alpha = (i-1)*2.0*M_PI/(float) l;
    points[i*2] = points[0] + r*cosf(alpha);
    points[i*2+1] = points[1] + r*sinf(alpha);
  }
  
  //and we transform them with all distorted modules
  if (!dt_dev_distort_transform_plus(module->dev,pipe,0,module->priority,points,l+1)) return 0;
  
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
  *width = (xmax-xmin);
  *height = (ymax-ymin);
  return 1;
}

int _circle_get_mask(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_form_t *form, float **buffer, int *width, int *height, int *posx, int *posy)
{
  //we get the area
  if (!_circle_get_area(module,pipe,wd,ht,form,width,height,posx,posy)) return 0;
  
  //we get the cicle values
  dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form->points)->data);
  
  //we create a buffer of points with all points in the area
  int w = *width, h = *height;
  float *points = malloc(w*h*2*sizeof(float));
  for (int i=0; i<h; i++)
    for (int j=0; j<w; j++)
    {
      points[(i*w+j)*2] = (j+(*posx));
      points[(i*w+j)*2+1] = (i+(*posy));
    }
    
  //we back transform all this points
  dt_dev_distort_backtransform_plus(module->dev,pipe,0,module->priority,points,w*h);
  
  //we allocate the buffer
  *buffer = malloc(w*h*sizeof(float));
  
  //we populate the buffer
  float center[2] = {circle->center[0]*wd, circle->center[1]*ht};
  float radius2 = circle->radius*MIN(wd,ht)*circle->radius*MIN(wd,ht);
  float total2 = (circle->radius+circle->border)*MIN(wd,ht)*(circle->radius+circle->border)*MIN(wd,ht);
  for (int i=0; i<h; i++)
    for (int j=0; j<w; j++)
    {
      float x = points[(i*w+j)*2];
      float y = points[(i*w+j)*2+1];
      float l2 = (x-center[0])*(x-center[0]) + (y-center[1])*(y-center[1]);
      if (l2<radius2) (*buffer)[i*w+j] = 1.0f;
      else if (l2 < total2)
      {
        float f = (total2-l2)/(total2-radius2);
        (*buffer)[i*w+j] = f*f;
      }
      else (*buffer)[i*w+j] = 0.0f;
    }
  free(points);
  return 1;
}

int dt_masks_get_points(dt_develop_t *dev, dt_masks_form_t *form, float **points, int *points_count, float dx, float dy)
{
  if (form->type == DT_MASKS_CIRCLE)
  {
    dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form->points)->data);
    return _circle_get_points(dev,circle->center[0]-dx, circle->center[1]-dy, circle->radius, points, points_count);
  }
  return 0;
}

int dt_masks_get_border(dt_develop_t *dev, dt_masks_form_t *form, float **border, int *border_count, float dx, float dy)
{
  if (form->type == DT_MASKS_CIRCLE)
  {
    dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form->points)->data);
    return _circle_get_points(dev,circle->center[0]-dx, circle->center[1]-dy, circle->radius + circle->border, border, border_count); 
  }
  return 0;
}

int dt_masks_get_area(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_form_t *form, int *width, int *height, int *posx, int *posy)
{
  if (form->type == DT_MASKS_CIRCLE)
  {
    return _circle_get_area(module,pipe,wd,ht,form,width,height,posx,posy);
  }
  return 0;  
}

int dt_masks_get_mask(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_form_t *form, float **buffer, int *width, int *height, int *posx, int *posy)
{
  if (form->type == DT_MASKS_CIRCLE)
  {
    return _circle_get_mask(module,pipe,wd,ht,form,buffer,width,height,posx,posy);
  }
  return 0; 
}

dt_masks_point_circle_t *dt_masks_get_circle(dt_masks_form_t *form)
{
  if (form->type == DT_MASKS_CIRCLE) return (dt_masks_point_circle_t *) (g_list_first(form->points)->data);
  return NULL;
}

dt_masks_form_t *dt_masks_create(dt_masks_type_t type)
{
  dt_masks_form_t *form = (dt_masks_form_t *)malloc(sizeof(dt_masks_form_t));
  form->type = type;
  
  struct timeval tv;
  gettimeofday(&tv,NULL);
  form->formid = tv.tv_sec*1000000.0+tv.tv_usec;
  
  form->points = NULL;
  
  return form;
}

dt_masks_form_t *dt_masks_get_from_id(dt_develop_t *dev, double id)
{
  GList *forms = g_list_first(dev->forms);
  while (forms)
  {
    dt_masks_form_t *form = (dt_masks_form_t *) forms->data;
    if (form->formid == id) return form;
    forms = g_list_next(forms);
  }
  return NULL;
}


void dt_masks_read_forms(dt_develop_t *dev)
{
  if(dev->image_storage.id <= 0) return;

  sqlite3_stmt *stmt;
  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db),
      "select imgid, formid, form, name, version, points, points_count from mask where imgid = ?1", -1, &stmt, NULL);
  DT_DEBUG_SQLITE3_BIND_INT(stmt, 1, dev->image_storage.id);
  
  while(sqlite3_step(stmt) == SQLITE_ROW)
  {
    // db record:
    // 0-img, 1-formid, 2-form_type, 3-name, 4-version, 5-points, 6-points_count
    
    //we get the values
    dt_masks_form_t *form = (dt_masks_form_t *)malloc(sizeof(dt_masks_form_t));
    form->formid = sqlite3_column_double(stmt, 1);
    form->type = sqlite3_column_int(stmt, 2);
    const char *name = (const char *)sqlite3_column_text(stmt, 3);
    snprintf(form->name,128,"%s",name);
    form->version = sqlite3_column_int(stmt, 4);
    form->points = NULL;
    //int nb_points = sqlite3_column_int(stmt, 6);
    
    //and now we "read" the blob
    if (form->type == DT_MASKS_CIRCLE)
    {
      dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *)malloc(sizeof(dt_masks_point_circle_t));
      memcpy(circle, sqlite3_column_blob(stmt, 5), sizeof(dt_masks_point_circle_t));
      form->points = g_list_append(form->points,circle);
    }
    else if(form->type == DT_MASKS_BEZIER)
    {
      //TODO
    }
    
    //and we can add the form to the list
    dev->forms = g_list_append(dev->forms,form);
  }
  
  sqlite3_finalize (stmt);  
}

void dt_masks_write_form(dt_masks_form_t *form, dt_develop_t *dev)
{
  //we first erase all masks for the image present in the db
  sqlite3_stmt *stmt;
  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db), "delete from mask where imgid = ?1 and formid = ?2", -1, &stmt, NULL);
  DT_DEBUG_SQLITE3_BIND_INT(stmt, 1, dev->image_storage.id);
  DT_DEBUG_SQLITE3_BIND_DOUBLE(stmt, 2, form->formid);
  sqlite3_step(stmt);
  sqlite3_finalize (stmt);
  
  //and we write the form
  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db), "insert into mask (imgid, formid, form, name, version, points, points_count) values (?1, ?2, ?3, ?4, ?5, ?6, ?7)", -1, &stmt, NULL);
  DT_DEBUG_SQLITE3_BIND_INT(stmt, 1, dev->image_storage.id);
  DT_DEBUG_SQLITE3_BIND_DOUBLE(stmt, 2, form->formid);
  DT_DEBUG_SQLITE3_BIND_INT(stmt, 3, form->type);
  DT_DEBUG_SQLITE3_BIND_TEXT(stmt, 4, form->name, strlen(form->name), SQLITE_TRANSIENT);
  DT_DEBUG_SQLITE3_BIND_INT(stmt, 5, form->version);
  if (form->type == DT_MASKS_CIRCLE)
  {
    dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form->points)->data);
    DT_DEBUG_SQLITE3_BIND_BLOB(stmt, 6, circle, sizeof(dt_masks_point_circle_t), SQLITE_TRANSIENT);
    DT_DEBUG_SQLITE3_BIND_INT(stmt, 7, 1);
  }
  else if (form->type == DT_MASKS_BEZIER)
  {
    //TODO
  }
  
  sqlite3_step (stmt);
  sqlite3_finalize (stmt);
}

void dt_masks_write_forms(dt_develop_t *dev)
{
  //we first erase all masks for the image present in the db
  sqlite3_stmt *stmt;
  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db), "delete from mask where imgid = ?1", -1, &stmt, NULL);
  DT_DEBUG_SQLITE3_BIND_INT(stmt, 1, dev->image_storage.id);
  sqlite3_step(stmt);
  sqlite3_finalize (stmt);
  
  //and now we write each forms
  GList *forms = g_list_first(dev->forms);
  while (forms)
  {
    dt_masks_form_t *form = (dt_masks_form_t *) forms->data;

    DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db), "insert into mask (imgid, formid, form, name, version, points, points_count) values (?1, ?2, ?3, ?4, ?5, ?6, ?7)", -1, &stmt, NULL);
    DT_DEBUG_SQLITE3_BIND_INT(stmt, 1, dev->image_storage.id);
    DT_DEBUG_SQLITE3_BIND_DOUBLE(stmt, 2, form->formid);
    DT_DEBUG_SQLITE3_BIND_INT(stmt, 3, form->type);
    DT_DEBUG_SQLITE3_BIND_TEXT(stmt, 4, form->name, strlen(form->name), SQLITE_TRANSIENT);
    DT_DEBUG_SQLITE3_BIND_INT(stmt, 5, form->version);
    if (form->type == DT_MASKS_CIRCLE)
    {
      DT_DEBUG_SQLITE3_BIND_BLOB(stmt, 6, form->points, sizeof(dt_masks_point_circle_t), SQLITE_TRANSIENT);
      DT_DEBUG_SQLITE3_BIND_INT(stmt, 7, 1);
    }
    else if (form->type == DT_MASKS_BEZIER)
    {
      //TODO
    }
    
    sqlite3_step (stmt);
    sqlite3_finalize (stmt);
    forms = g_list_next(forms);
  }  
}


static void _gui_form_create(dt_iop_module_t *module, dt_masks_form_t *form, dt_masks_form_gui_t *gui)
{
  gui->pipe_hash = gui->formid = gui->points_count = gui->border_count = 0;
  
  if (dt_masks_get_points(module->dev,form, &gui->points, &gui->points_count,0,0))
  {
    if (dt_masks_get_border(module->dev,form, &gui->border, &gui->border_count,0,0))
    {
      gui->pipe_hash = module->dev->preview_pipe->backbuf_hash;
      gui->formid = form->formid;
    }
  }
}
static void _gui_form_remove(dt_iop_module_t *module, dt_masks_form_t *form, dt_masks_form_gui_t *gui)
{
  gui->pipe_hash = gui->formid = gui->points_count = gui->border_count = 0;
  if (gui->points) free(gui->points);
  gui->points = NULL;
  if (gui->border) free(gui->border);
  gui->border = NULL;
}


static void _gui_form_update_border(dt_iop_module_t *module, dt_masks_form_t *form, dt_masks_form_gui_t *gui)
{
  float *border;
  int border_count;
  
  if (dt_masks_get_border(module->dev,form, &border, &border_count,0,0))
  {
    if (gui->border) free(gui->border);
    gui->border = border;
    gui->border_count = border_count;
  }
}

static int _gui_form_test_create(dt_iop_module_t *module, dt_masks_form_t *form, dt_masks_form_gui_t *gui)
{
  //we test if the image has changed
  if (gui->pipe_hash > 0)
  {
    if (gui->pipe_hash != module->dev->preview_pipe->backbuf_hash)
    {
      _gui_form_remove(module,form,gui);
    }
  }
  
  //we create the spots if needed
  if (gui->pipe_hash == 0)
  {
    _gui_form_create(module,form,gui);
  }
  return 1;
}

static void _gui_form_save_creation(dt_iop_module_t *module, dt_masks_form_t *form, dt_masks_form_gui_t *gui)
{
  module->dev->forms = g_list_append(module->dev->forms,form);
  gui->creation = FALSE;
  
  //update params
  int forms_count = module->blend_params->forms_count;
  module->blend_params->forms[forms_count] = form->formid;
  module->blend_params->forms_state[forms_count] = DT_BLEND_FORM_SHOW | DT_BLEND_FORM_USE;
  if (form->type == DT_MASKS_CIRCLE) snprintf(form->name,128,"mask circle #%d",forms_count);
  dt_masks_write_form(form,module->dev);
  
  //update gui
  dt_iop_gui_blend_data_t *bd = (dt_iop_gui_blend_data_t*)module->blend_data;
  bd->form_label[forms_count] = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(bd->form_label[forms_count]), gtk_label_new(form->name));
  gtk_widget_show_all(bd->form_label[forms_count]);
  g_object_set_data(G_OBJECT(bd->form_label[forms_count]), "form", GUINT_TO_POINTER(forms_count));
  gtk_box_pack_end(GTK_BOX(bd->form_box), bd->form_label[forms_count], TRUE, TRUE,0);
  g_signal_connect(G_OBJECT(bd->form_label[forms_count]), "button-press-event", G_CALLBACK(dt_iop_gui_blend_setform_callback), module);
  
  module->blend_params->forms_count++;
  
  //show the form if needed
  module->dev->form_gui->formid = form->formid;
}

int dt_masks_mouse_moved (struct dt_iop_module_t *module, double x, double y, int which)
{
  //dt_masks_form_t *form = module->dev->form_visible;
  dt_masks_form_gui_t *gui = module->dev->form_gui;
  
  float pzx, pzy;
  dt_dev_get_pointer_zoom_pos(module->dev, x, y, &pzx, &pzy);
  pzx += 0.5f;
  pzy += 0.5f;
      
  if (gui->form_dragging)
  {
    gui->posx = pzx*module->dev->preview_pipe->backbuf_width;
    gui->posy = pzy*module->dev->preview_pipe->backbuf_height;
    dt_control_queue_redraw_center();
    return 0;
  }
  else if (!gui->creation)
  {
    dt_masks_set_inside(pzx*module->dev->preview_pipe->backbuf_width,pzy*module->dev->preview_pipe->backbuf_height,gui);
    dt_control_queue_redraw_center();
    if (!gui->selected) return 1;
    return 0;
  }
  return 1;
}
int dt_masks_button_released (struct dt_iop_module_t *module, double x, double y, int which, uint32_t state)
{
  dt_masks_form_t *form = module->dev->form_visible;
  dt_masks_form_gui_t *gui = module->dev->form_gui;
  
  if (form->type == DT_MASKS_CIRCLE)
  {
    if (gui->creation)
    {
      //we create the circle
      dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (malloc(sizeof(dt_masks_point_circle_t)));
      
      //we change the center value
      float pzx, pzy;
      dt_dev_get_pointer_zoom_pos(module->dev, x, y, &pzx, &pzy);
      pzx += 0.5f;
      pzy += 0.5f;
      float wd = module->dev->preview_pipe->backbuf_width;
      float ht = module->dev->preview_pipe->backbuf_height;
      float pts[2] = {pzx*wd,pzy*ht};
      dt_dev_distort_backtransform(module->dev,pts,1);
      circle->center[0] = pts[0]/module->dev->preview_pipe->iwidth;
      circle->center[1] = pts[1]/module->dev->preview_pipe->iheight;
      circle->radius = 0.1f;
      circle->border = 0.05f;
      form->points = g_list_append(form->points,circle);
      
      _gui_form_save_creation(module,form,gui);
      
      //we recreate the form points
      _gui_form_remove(module,form,gui);
      _gui_form_create(module,form,gui);
      
      //we save the move
      dt_dev_add_history_item(darktable.develop, module, TRUE);
      
      return 0;
    }
    else if (gui->form_dragging)
    {
      //we get the circle
      dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form->points)->data);
      
      //we end the form dragging
      gui->form_dragging = FALSE;
      
      //we change the center value
      float pzx, pzy;
      dt_dev_get_pointer_zoom_pos(module->dev, x, y, &pzx, &pzy);
      pzx += 0.5f;
      pzy += 0.5f;
      float wd = module->dev->preview_pipe->backbuf_width;
      float ht = module->dev->preview_pipe->backbuf_height;
      float pts[2] = {pzx*wd+gui->dx,pzy*ht+gui->dy};
      dt_dev_distort_backtransform(module->dev,pts,1);
      circle->center[0] = pts[0]/module->dev->preview_pipe->iwidth;
      circle->center[1] = pts[1]/module->dev->preview_pipe->iheight;
      dt_masks_write_form(form,module->dev);
      //we recreate the form points
      _gui_form_remove(module,form,gui);
      _gui_form_create(module,form,gui);
      
      //we save the move
      dt_dev_add_history_item(darktable.develop, module, TRUE);
      
      return 0;
    }
    return 1;
  }
  
  return 0;
}
int dt_masks_button_pressed (struct dt_iop_module_t *module, double x, double y, int which, int type, uint32_t state)
{
  dt_masks_form_t *form = module->dev->form_visible;
  dt_masks_form_gui_t *gui = module->dev->form_gui;
  
  if (form->type == DT_MASKS_CIRCLE)
  {
    if (gui->selected && !gui->creation)
    {
      //we start the form dragging
      gui->form_dragging = TRUE;
      
      dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form->points)->data);
      float pzx, pzy;
      dt_dev_get_pointer_zoom_pos(module->dev, x, y, &pzx, &pzy);
      gui->posx = (pzx + 0.5f)*module->dev->preview_pipe->backbuf_width;
      gui->posy = (pzy + 0.5f)*module->dev->preview_pipe->backbuf_height;
      gui->dx = circle->center[0]*module->dev->preview_pipe->backbuf_width - gui->posx;
      gui->dy = circle->center[1]*module->dev->preview_pipe->backbuf_height - gui->posy;
      return 0;
    }
    return 1;
  }
  
  return 0;
}
int dt_masks_scrolled (struct dt_iop_module_t *module, double x, double y, int up, uint32_t state)
{
  dt_masks_form_t *form = module->dev->form_visible;
  dt_masks_form_gui_t *gui = module->dev->form_gui;
  
  if (form->type == DT_MASKS_CIRCLE)
  {
    if (gui->selected)
    {
      dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form->points)->data);
      if (gui->border_selected)
      {
        if(up && circle->border > 0.002f) circle->border *= 0.9f;
        else  if(circle->border < 0.1f  ) circle->border *= 1.0f/0.9f;
        dt_masks_write_form(form,module->dev);
        _gui_form_update_border(module,form,gui);
      }
      else
      {
        if(up && circle->radius > 0.002f) circle->radius *= 0.9f;
        else  if(circle->radius < 0.1f  ) circle->radius *= 1.0f/0.9f;
        dt_masks_write_form(form,module->dev);
        _gui_form_remove(module,form,gui);
        _gui_form_create(module,form,gui);
      }
      dt_dev_add_history_item(darktable.develop, module, TRUE);
      return 0;
    }
  }
  return 1;
}
void dt_masks_post_expose (struct dt_iop_module_t *module, cairo_t *cr, int32_t width, int32_t height, int32_t pointerx, int32_t pointery)
{
  dt_develop_t *dev = module->dev;
  dt_masks_form_t *form = module->dev->form_visible;
  dt_masks_form_gui_t *gui = module->dev->form_gui;
  //if it's a spot in creation, nothing to draw
  if (form->type == DT_MASKS_CIRCLE && gui->creation) return;
  
  float wd = dev->preview_pipe->backbuf_width;
  float ht = dev->preview_pipe->backbuf_height;
  if (wd < 1.0 || ht < 1.0) return;
  float pzx, pzy;
  dt_dev_get_pointer_zoom_pos(dev, pointerx, pointery, &pzx, &pzy);
  pzx += 0.5f;
  pzy += 0.5f;
  float zoom_x, zoom_y;
  int32_t zoom, closeup;
  DT_CTL_GET_GLOBAL(zoom_y, dev_zoom_y);
  DT_CTL_GET_GLOBAL(zoom_x, dev_zoom_x);
  DT_CTL_GET_GLOBAL(zoom, dev_zoom);
  DT_CTL_GET_GLOBAL(closeup, dev_closeup);
  float zoom_scale = dt_dev_get_zoom_scale(dev, zoom, closeup ? 2 : 1, 1);
  
  cairo_set_source_rgb(cr, .3, .3, .3);

  cairo_translate(cr, width/2.0, height/2.0f);
  cairo_scale(cr, zoom_scale, zoom_scale);
  cairo_translate(cr, -.5f*wd-zoom_x*wd, -.5f*ht-zoom_y*ht);

  double dashed[] = {4.0, 4.0};
  dashed[0] /= zoom_scale;
  dashed[1] /= zoom_scale;
  int len  = sizeof(dashed) / sizeof(dashed[0]);
  
  cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
  
  //we update the form if needed
  if (!_gui_form_test_create(module,form,gui)) return;
    
  //draw form
  if (gui->points_count > 6)
  { 
    cairo_set_dash(cr, dashed, 0, 0);     
    if(gui->selected || gui->form_dragging) cairo_set_line_width(cr, 5.0/zoom_scale);
    else                                     cairo_set_line_width(cr, 3.0/zoom_scale);
    cairo_set_source_rgba(cr, .3, .3, .3, .8);
    if (gui->form_dragging)
    {
      float dx = gui->posx + gui->dx - gui->points[0], dy = gui->posy + gui->dy - gui->points[1];
      cairo_move_to(cr,gui->points[2]+dx,gui->points[3]+dy);
      for (int i=2; i<gui->points_count; i++)
      {
        cairo_line_to(cr,gui->points[i*2]+dx,gui->points[i*2+1]+dy);
      }
      cairo_line_to(cr,gui->points[2]+dx,gui->points[3]+dy);
    }
    else
    {
      cairo_move_to(cr,gui->points[2],gui->points[3]);
      for (int i=2; i<gui->points_count; i++)
      {
        cairo_line_to(cr,gui->points[i*2],gui->points[i*2+1]);
      }
      cairo_line_to(cr,gui->points[2],gui->points[3]);
    }
    cairo_stroke_preserve(cr);
    if(gui->selected || gui->form_dragging) cairo_set_line_width(cr, 2.0/zoom_scale);
    else                                     cairo_set_line_width(cr, 1.0/zoom_scale);
    cairo_set_source_rgba(cr, .8, .8, .8, .8);
    cairo_stroke(cr);
  }

  //draw border
  if ((!gui->form_dragging) && gui->border_count > 6)
  { 
    cairo_set_dash(cr, dashed, len, 0);     
    if(gui->border_selected) cairo_set_line_width(cr, 2.0/zoom_scale);
    else                     cairo_set_line_width(cr, 1.0/zoom_scale);
    cairo_set_source_rgba(cr, .3, .3, .3, .8);
      
    cairo_move_to(cr,gui->border[2],gui->border[3]);
    for (int i=2; i<gui->border_count; i++)
    {
      cairo_line_to(cr,gui->border[i*2],gui->border[i*2+1]);
    }
    cairo_line_to(cr,gui->border[2],gui->border[3]);

    cairo_stroke_preserve(cr);
    if(gui->border_selected) cairo_set_line_width(cr, 2.0/zoom_scale);
    else                     cairo_set_line_width(cr, 1.0/zoom_scale);
    cairo_set_source_rgba(cr, .8, .8, .8, .8);
    cairo_set_dash(cr, dashed, len, 4);
    cairo_stroke(cr);
  }
}

void dt_masks_set_inside(float x, int y, dt_masks_form_gui_t *gui)
{
  //we first check if it's inside borders
  int nb = 0;
  int last = -9999;
  for (int i=0; i<gui->border_count; i++)
  {
    int yy = (int) gui->border[i*2+1];
    if (yy != last && yy == y)
    {
      if (gui->border[i*2] > x) nb++;
    }
    last = yy;
  }  
  if (!(nb & 1))
  {
    gui->selected = FALSE;
    gui->border_selected = FALSE;
    return;
  }
  gui->selected = TRUE;
  
  //and we check if it's inside form
  nb = 0;
  last = -9999;
  for (int i=0; i<gui->points_count; i++)
  {
    int yy = (int) gui->points[i*2+1];
    if (yy != last && yy == y)
    {
      if (gui->points[i*2] > x) nb++;
    }
    last = yy;
  }
  gui->border_selected = !(nb & 1);
}

void dt_masks_init_formgui(dt_develop_t *dev)
{
  dev->form_gui->pipe_hash = dev->form_gui->formid = dev->form_gui->points_count = dev->form_gui->border_count = 0;
  if (dev->form_gui->points) free(dev->form_gui->points);
  dev->form_gui->points = NULL;
  if (dev->form_gui->border) free(dev->form_gui->border);
  dev->form_gui->border = NULL;
  dev->form_gui->posx = dev->form_gui->posy = dev->form_gui->dx = dev->form_gui->dy = 0.0f;
  dev->form_gui->selected = dev->form_gui->border_selected = dev->form_gui->form_dragging = FALSE;
  dev->form_gui->point_dragging = -1;
  dev->form_gui->creation = FALSE;
}

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;
