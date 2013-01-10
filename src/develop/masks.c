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

int _circle_get_area(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_form_t form, int *width, int *height, int *posx, int *posy)
{  
  //we get the cicle values
  dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form.points)->data);
  
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

int _circle_get_mask(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_form_t form, float **buffer, int *width, int *height, int *posx, int *posy)
{
  //we get the area
  if (!_circle_get_area(module,pipe,wd,ht,form,width,height,posx,posy)) return 0;
  
  //we get the cicle values
  dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form.points)->data);
  
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

int dt_masks_get_points(dt_develop_t *dev, dt_masks_form_t form, float **points, int *points_count, float dx, float dy)
{
  if (form.type == DT_MASKS_CIRCLE)
  {
    dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form.points)->data);
    return _circle_get_points(dev,circle->center[0]-dx, circle->center[1]-dy, circle->radius, points, points_count);
  }
  return 0;
}

int dt_masks_circle_get_border(dt_develop_t *dev, dt_masks_form_t form, float **border, int *border_count, float dx, float dy)
{
  if (form.type == DT_MASKS_CIRCLE)
  {
    dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(form.points)->data);
    return _circle_get_points(dev,circle->center[0]-dx, circle->center[1]-dy, circle->radius + circle->border, border, border_count); 
  }
  return 0;
}

int dt_masks_get_area(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_form_t form, int *width, int *height, int *posx, int *posy)
{
  if (form.type == DT_MASKS_CIRCLE)
  {
    return _circle_get_area(module,pipe,wd,ht,form,width,height,posx,posy);
  }
  return 0;  
}

int dt_masks_get_mask(dt_iop_module_t *module, dt_dev_pixelpipe_t *pipe, int wd, int ht, dt_masks_form_t form, float **buffer, int *width, int *height, int *posx, int *posy)
{
  if (form.type == DT_MASKS_CIRCLE)
  {
    return _circle_get_mask(module,pipe,wd,ht,form,buffer,width,height,posx,posy);
  }
  return 0; 
}

int dt_masks_get_blob(GList *forms, float **blob, int *blob_size)
{
  //first, we need to get the size of the blob to allocate it
  int nb = 0;
  int bs = 1;
  GList *fs = g_list_first(forms);
  while (fs)
  {
    dt_masks_form_t *f = (dt_masks_form_t *) (fs->data);
    bs += 2;
    if (f->type == DT_MASKS_CIRCLE) bs += 4;
    nb++;
    fs = g_list_next(fs);
  }
  *blob = malloc(bs*sizeof(float));
  
  //and then we go thought all forms and add them to the blob
  (*blob)[0] = (float) nb;
  GList *fs = g_list_first(forms);
  nb=0;
  while (fs)
  {
    dt_masks_form_t *f = (dt_masks_form_t *) (fs->data);
    (*blob)[nb++] = (float) f->type;
    (*blob)[nb++] = (float) f->formid;
    if (f->type == DT_MASKS_CIRCLE)
    {
      dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *) (g_list_first(f->points)->data);
      (*blob)[nb++] = circle->center[0];
      (*blob)[nb++] = circle->center[1];
      (*blob)[nb++] = circle->radius;
      (*blob)[nb++] = circle->border;
    }
    fs = g_list_next(fs);
  }
  
  *blob_size = bs;
  
  return 1;
}

int dt_masks_init_from_blob(GList **forms, float *blob, int blob_size)
{
  if (blob_size == 0) return 0;
  //we read the number of forms
  float nb = blob[0];
  int pos=0;
  //and we go throught the forms
  // /!\ here we assume we don't get a corrupt buffer... may need some check !
  for (int i=0; i<nb; i++)
  {
    dt_masks_form_t f;
    f.type = (dt_masks_type_t) blob[pos++];
    f.formid = blob[pos++];
    if (f.type == DT_MASKS_CIRCLE)
    {
      dt_masks_point_circle_t circle;
      circle.center[0] = blob[pos++];
      circle.center[1] = blob[pos++];
      circle.radius = blob[pos++];
      circle.border = blob[pos++];
      f.points = g_list_append(f.points,&circle);
    }
    *forms = g_list_append(*forms,&f);
  }  
  return 1;
}

void dt_masks_read_forms(dt_develop_t *dev)
{
  if(dev->image_storage.id <= 0) return;

  sqlite3_stmt *stmt;
  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db),
      "select imgid, formid, form, version, points, points_count from masks where imgid = ?1", -1, &stmt, NULL);
  DT_DEBUG_SQLITE3_BIND_INT(stmt, 1, dev->image_storage.id);
  
  while(sqlite3_step(stmt) == SQLITE_ROW)
  {
    // db record:
    // 0-img, 1-formid, 2-form_type, 3-version, 4-points, 5-points_count
    
    //we get the values
    dt_masks_form_t *form = (dt_masks_form_t *)malloc(sizeof(dt_masks_form_t));
    form->formid = sqlite3_column_int(stmt, 1);
    form->type = sqlite3_column_int(stmt, 2);
    form->version = sqlite3_column_int(stmt, 3);
    int nb_points = sqlite3_column_int(stmt, 5);
    
    //and now we "read" the blob
    if (form->type == DT_MASKS_CIRCLE)
    {
      dt_masks_point_circle_t *circle = (dt_masks_point_circle_t *)malloc(sizeof(dt_masks_point_circle_t));
      memcpy(circle, sqlite3_column_blob(stmt, 4), sizeof(dt_masks_point_circle_t));
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

void dt_masks_write_forms(dt_develop_t *dev)
{
  //we first erase all masks for the image present in the db
  sqlite3_stmt *stmt;
  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db), "delete from masks where imgid = ?1", -1, &stmt, NULL);
  DT_DEBUG_SQLITE3_BIND_INT(stmt, 1, dev->image_storage.id);
  sqlite3_step(stmt);
  sqlite3_finalize (stmt);
  
  //and now we write each forms
  GList *forms = g_list_first(dev->forms);
  while (forms)
  {
    dt_masks_form_t *form = (dt_masks_form_t *) forms->data;

    DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db), "insert into masks (imgid, formid, form, version, points, points_count) values (?1, ?2, ?3, ?4, ?5, ?6)", -1, &stmt, NULL);
    DT_DEBUG_SQLITE3_BIND_INT(stmt, 1, dev->image_storage.id);
    DT_DEBUG_SQLITE3_BIND_INT(stmt, 2, form->formid);
    DT_DEBUG_SQLITE3_BIND_INT(stmt, 3, form->type);
    DT_DEBUG_SQLITE3_BIND_INT(stmt, 4, form->version);
    if (form->type == DT_MASKS_CIRCLE)
    {
      DT_DEBUG_SQLITE3_BIND_BLOB(stmt, 5, form->points, sizeof(dt_masks_point_circle_t), SQLITE_TRANSIENT);
      DT_DEBUG_SQLITE3_BIND_INT(stmt, 6, 1);
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

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;
