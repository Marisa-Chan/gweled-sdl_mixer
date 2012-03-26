/* Gweled
 *
 * Copyright (C) 2003-2005 Sebastien Delestaing <sebastien.delestaing@wanadoo.fr>
 * Copyright (C) 2010 Daniele Napolitano <dnax88@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _SGE_CORE_H_
#define _SGE_CORE_H_

typedef struct s_sge_object
{
	gdouble x;
	gdouble y;
	gdouble vx;
	gdouble vy;
	gdouble ax;
	gdouble ay;
	gint y_delay;
	gint dest_x;
	gint dest_y;
	gint width;
	gint height;
	gint layer;
	gint lifetime;
	gint opacity;
	gboolean fadeout;
	gboolean zoomout;
	gfloat zoom;
	gfloat saturation;
	gboolean blink;
	gboolean blink_increase;
	gboolean animation;
	gfloat animation_iter;
	gboolean animation_repeat;
	gint needs_drawing;
	gint pixbuf_id;
	GdkPixmap *pre_rendered;
	gint (*stop_condition)(struct s_sge_object *);
	GFunc stop_callback;
}T_SGEObject;

#define SGE_OBJECT(obj)  ((T_SGEObject *) obj)

void sge_init(void);
void sge_destroy(void);
void sge_set_drawing_area(GtkWidget *drawing_area, GdkPixmap *pixmap_buffer, gint width, gint height);

gint sge_register_pixbuf(GdkPixbuf *pixbuf, int index);

T_SGEObject *sge_create_object(gint x, gint y, gint layer, gint pixbuf_id);//GdkPixbuf *pixbuf);
void sge_destroy_object(gpointer object, gpointer user_data);
void sge_destroy_all_objects_on_level(int level);
void sge_destroy_all_objects(void);

void sge_object_set_lifetime(T_SGEObject *object, gint seconds);
void sge_object_take_down(T_SGEObject *object);
void sge_object_move_to(T_SGEObject *object, gint dest_x, gint dest_y);
void sge_object_fall_to(T_SGEObject *object, gint dest_y);
void sge_object_fall_to_with_delay (T_SGEObject * object, gint y_pos, gint delay);

gboolean sge_object_is_moving(T_SGEObject *object);
gboolean sge_objects_are_moving(void);
gboolean sge_objects_are_moving_on_layer(int layer);

void sge_set_layer_visibility (int layer, gboolean visibility);

void sge_invalidate_layer(int layer);

void sge_object_set_opacity (T_SGEObject *object, gint alpha);
void sge_object_fadeout (T_SGEObject *object);
void sge_object_zoomout (T_SGEObject *object);

void sge_object_blink_start (T_SGEObject *object);
void sge_object_blink_stop (T_SGEObject *object);

void sge_object_animate (T_SGEObject *object, gboolean repeat);

gboolean sge_object_exists (T_SGEObject *object);

#endif
