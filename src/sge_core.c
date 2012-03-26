/* Gweled - Seb's Graphic Engine (SGE)
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

/* Levels:
 * 0 -> board background
 * 1 -> gems
 * 2 -> cursor
 * 3 -> text
 */

#include <gtk/gtk.h>
#include <math.h>
#include "sge_core.h"

#define ACCELERATION	1.0
#define SGE_TIMER_INTERVAL  20

// LOCAL FUNCTIONS
void invalidate_objects_above (T_SGEObject * object);
void invalidate_background_beneath (T_SGEObject * object);

// LOCAL VARS
static GRand *g_rand_generator;
static guint g_main_loop_id;
static GList *g_object_list;
static GdkPixmap *g_pixmap_buffer;
static GdkPixbuf **g_pixbufs;
static gint g_width, g_height;
static gint gi_nb_pixbufs;

static GtkWidget *g_drawing_area = NULL;

static gboolean layers_visibility[5] = {TRUE, TRUE, TRUE, TRUE, TRUE};

// helper functions
gint
compare_by_layer (gconstpointer a, gconstpointer b)
{
	return SGE_OBJECT(a)->layer - SGE_OBJECT(b)->layer;
}

// main loop functions
void
draw_object (gpointer object, gpointer user_data)
{
	int x, y;
	GdkGC *gc;
	GdkPixbuf *pixbuffer;


    if((SGE_OBJECT(object)->fadeout && SGE_OBJECT(object)->opacity <= 0)
    || (SGE_OBJECT(object)->zoomout && SGE_OBJECT(object)->zoom <= 0))
    {
	    sge_destroy_object (SGE_OBJECT(object), NULL);
        return;
    }

    if ((int) SGE_OBJECT(object)->needs_drawing
        && layers_visibility[SGE_OBJECT(object)->layer]
        && SGE_OBJECT(object)->opacity > 0) {

		x = (int) SGE_OBJECT(object)->x;
		y = (int) SGE_OBJECT(object)->y;

		if (SGE_OBJECT(object)->pre_rendered) {
			gc = gdk_gc_new (GDK_DRAWABLE (g_pixmap_buffer));
			gdk_draw_drawable (GDK_DRAWABLE (g_pixmap_buffer),
					   gc,
					   GDK_DRAWABLE (SGE_OBJECT(object)->pre_rendered),
					   0, 0, x, y,
					   SGE_OBJECT(object)->width,
					   SGE_OBJECT(object)->height);
			g_object_unref (gc);

		} else {

            if (SGE_OBJECT(object)->opacity < 255
                || SGE_OBJECT(object)->zoom != 1.0)
            {

                pixbuffer = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                        TRUE, 8,
                        SGE_OBJECT(object)->width,
                        SGE_OBJECT(object)->height);

                gdk_pixbuf_fill (pixbuffer, 0x00000000);
                gdk_pixbuf_composite(g_pixbufs[SGE_OBJECT(object)->pixbuf_id],
                        pixbuffer,
                        0,
                        0,
                        SGE_OBJECT(object)->width,
                        SGE_OBJECT(object)->height,
                        0, 0,
                        SGE_OBJECT(object)->zoom,
                        SGE_OBJECT(object)->zoom,
                        GDK_INTERP_NEAREST,
                        SGE_OBJECT(object)->opacity);

                if(SGE_OBJECT(object)->animation)

                    gdk_draw_pixbuf (GDK_DRAWABLE (g_pixmap_buffer),
                            NULL, pixbuffer,
                            ((int)SGE_OBJECT(object)->animation_iter) * SGE_OBJECT(object)->height,
                            0,
                            x, y,
                            SGE_OBJECT(object)->height,
                            SGE_OBJECT(object)->height,
                            GDK_RGB_DITHER_NONE, 0, 0);
                else
                    gdk_draw_pixbuf (GDK_DRAWABLE (g_pixmap_buffer),
                            NULL, pixbuffer,
                            0, 0,
                            x +
                                (SGE_OBJECT(object)->width -
                                 SGE_OBJECT(object)->width *
                                 SGE_OBJECT(object)->zoom) / 2,
                            y +
                                (SGE_OBJECT(object)->height -
                                 SGE_OBJECT(object)->height *
                                 SGE_OBJECT(object)->zoom) / 2,
                            SGE_OBJECT(object)->width * SGE_OBJECT(object)->zoom,
                            SGE_OBJECT(object)->height * SGE_OBJECT(object)->zoom,
                            GDK_RGB_DITHER_NONE, 0, 0);

                g_object_unref(pixbuffer);

            } else if(SGE_OBJECT(object)->blink) {
                pixbuffer = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                        TRUE, 8,
                        SGE_OBJECT(object)->width,
                        SGE_OBJECT(object)->height);

                gdk_pixbuf_fill (pixbuffer, 0x00000000);

                gdk_pixbuf_saturate_and_pixelate(
                            g_pixbufs[SGE_OBJECT(object)->pixbuf_id],
                            pixbuffer,
                            SGE_OBJECT(object)->saturation,
                            FALSE);

                gdk_draw_pixbuf (GDK_DRAWABLE (g_pixmap_buffer),
                        NULL, pixbuffer,
                        0, 0, x, y,
                        SGE_OBJECT(object)->width,
                        SGE_OBJECT(object)->height,
                        GDK_RGB_DITHER_NONE, 0, 0);

               g_object_unref(pixbuffer);

            } else {
                if(SGE_OBJECT(object)->animation) {
                    // width of each sprite of animation is the object height
                    gdk_draw_pixbuf (GDK_DRAWABLE (g_pixmap_buffer),
                            NULL, g_pixbufs[SGE_OBJECT(object)->pixbuf_id],
                            ((int)SGE_OBJECT(object)->animation_iter) * SGE_OBJECT(object)->height,
                            0,
                            x, y,
                            SGE_OBJECT(object)->height,
                            SGE_OBJECT(object)->height,
                            GDK_RGB_DITHER_NONE, 0, 0);

                } else {

                    // draw pixbuf without effects
                    gdk_draw_pixbuf (GDK_DRAWABLE (g_pixmap_buffer),
                            NULL, g_pixbufs[SGE_OBJECT(object)->pixbuf_id],
                            0, 0, x, y,
                            SGE_OBJECT(object)->width,
                            SGE_OBJECT(object)->height,
                            GDK_RGB_DITHER_NONE, 0, 0);
                }
            }
        }
        // request area drawing
        gtk_widget_queue_draw_area (g_drawing_area, x, y,
				SGE_OBJECT(object)->width,
				SGE_OBJECT(object)->height);

		SGE_OBJECT(object)->needs_drawing = 0;

		invalidate_objects_above (SGE_OBJECT(object));
	}

    if(SGE_OBJECT(object)->fadeout) {
        SGE_OBJECT(object)->opacity -= 30;
        invalidate_background_beneath (SGE_OBJECT(object));
    }
    if(SGE_OBJECT(object)->zoomout) {
        SGE_OBJECT(object)->zoom -= 0.20;
        invalidate_background_beneath (SGE_OBJECT(object));
    }
    if(SGE_OBJECT(object)->blink) {
        if(SGE_OBJECT(object)->saturation >= 2.0)
            SGE_OBJECT(object)->blink_increase = FALSE;

        if(SGE_OBJECT(object)->saturation <= 0.4)
            SGE_OBJECT(object)->blink_increase = TRUE;

        if(SGE_OBJECT(object)->blink_increase)
            SGE_OBJECT(object)->saturation += 0.2;
        else
            SGE_OBJECT(object)->saturation -= 0.2;

         invalidate_background_beneath (SGE_OBJECT(object));
    }
    if(SGE_OBJECT(object)->animation) {
        SGE_OBJECT(object)->animation_iter += 0.5;

        if(SGE_OBJECT(object)->animation_iter * SGE_OBJECT(object)->height == SGE_OBJECT(object)->width) {
            if(SGE_OBJECT(object)->animation_repeat)
                SGE_OBJECT(object)->animation_iter = 0;
            else {
                sge_destroy_object (SGE_OBJECT(object), NULL);
                return;
            }
        }
        invalidate_background_beneath (SGE_OBJECT(object));
    }
}

void
move_object (gpointer object, gpointer user_data)
{
    if(SGE_OBJECT(object)->y_delay > 0) {
        SGE_OBJECT(object)->y_delay--;
        return;
    }

	SGE_OBJECT(object)->vx += SGE_OBJECT(object)->ax;
	SGE_OBJECT(object)->vy += SGE_OBJECT(object)->ay;

	if (sge_object_is_moving (SGE_OBJECT(object)))
	{
		invalidate_background_beneath (SGE_OBJECT(object));
		SGE_OBJECT(object)->x += SGE_OBJECT(object)->vx;
		SGE_OBJECT(object)->y += SGE_OBJECT(object)->vy;
		SGE_OBJECT(object)->needs_drawing |= 0x02;
	}

	if (SGE_OBJECT(object)->stop_condition)

        if (SGE_OBJECT(object)->stop_condition(SGE_OBJECT(object)))
		{
            if (SGE_OBJECT(object)->stop_callback)
                SGE_OBJECT(object)->stop_callback (object, NULL);

            // needed for scores messages
            if(!SGE_OBJECT(object)->fadeout)
                SGE_OBJECT(object)->vy = 0.0;

		    SGE_OBJECT(object)->vx = 0.0;
            SGE_OBJECT(object)->ax = 0.0;
            SGE_OBJECT(object)->ay = 0.0;
		}
}

gboolean
sge_main_loop (gpointer data)
{
	g_list_foreach (g_object_list, draw_object, NULL);
	g_list_foreach (g_object_list, move_object, NULL);

    return TRUE;

}

// creation/destruction
void
sge_init (void)
{
	g_rand_generator = g_rand_new_with_seed (time (NULL));
	g_main_loop_id = g_timeout_add (SGE_TIMER_INTERVAL, sge_main_loop, NULL);
	gi_nb_pixbufs = 0;
	g_pixbufs = NULL;
}

void
scale_object_pos (gpointer object, gpointer user_data)
{

    gdouble ratio;

    ratio = *((gdouble *) user_data);
    SGE_OBJECT(object)->x = SGE_OBJECT(object)->x * ratio;
    SGE_OBJECT(object)->y = SGE_OBJECT(object)->y * ratio;

    SGE_OBJECT(object)->dest_x = (int) rint (SGE_OBJECT(object)->x);

    SGE_OBJECT(object)->dest_y = (int) rint (SGE_OBJECT(object)->y);
    SGE_OBJECT(object)->needs_drawing = -1;

}

void
sge_set_drawing_area (GtkWidget * drawing_area, GdkPixmap * pixmap_buffer,
		      gint width, gint height)
{
	gdouble ratio;

	if (g_drawing_area && width && height) {
		ratio = (gdouble) width / (gdouble) g_width;
		g_list_foreach (g_object_list, scale_object_pos, &ratio);
	}

	if (drawing_area)
		g_drawing_area = drawing_area;
	if (pixmap_buffer)
		g_pixmap_buffer = pixmap_buffer;
	if (width)
		g_width = width;
	if (height)
		g_height = height;
}

void
sge_destroy (void)
{
	int i;

	g_source_remove (g_main_loop_id);
	g_rand_free (g_rand_generator);
	for (i = 0; i < gi_nb_pixbufs; i++)
		g_object_unref (g_pixbufs[i]);
	g_free (g_pixbufs);
}

// pixbuf management
void
pixbuf_update_notify (gpointer item, gpointer data)
{
	int pixbuf_id;
	T_SGEObject *object;

	object = (T_SGEObject *) item;
	pixbuf_id = *((int *) data);

	if (object->pixbuf_id == pixbuf_id)
	{
        object->width = gdk_pixbuf_get_width (g_pixbufs[pixbuf_id]);
        object->height = gdk_pixbuf_get_height (g_pixbufs[pixbuf_id]);

		if (object->pre_rendered)
		{
			g_object_unref (G_OBJECT (object->pre_rendered));
			object->pre_rendered =
			    gdk_pixmap_new (g_drawing_area->window,
					    object->width, object->height,
					    -1);
			gdk_draw_pixbuf (GDK_DRAWABLE
					 (object->pre_rendered), NULL,
					 g_pixbufs[pixbuf_id], 0, 0, 0, 0,
					 object->width, object->height,
					 GDK_RGB_DITHER_NONE, 0, 0);
		}
	}
}

gint
sge_register_pixbuf (GdkPixbuf * pixbuf, int index)
{
	int i;

	if (index == -1) {
		for (i = 0; i < gi_nb_pixbufs; i++)
			if (g_pixbufs[i] == 0)
				break;
		if (i == gi_nb_pixbufs) {
			gi_nb_pixbufs++;
			g_pixbufs =
			    (GdkPixbuf **) g_realloc (g_pixbufs,
						      sizeof (GdkPixbuf *)
						      * gi_nb_pixbufs);
		}
		g_pixbufs[i] = pixbuf;
	} else {
		i = index;
		g_object_unref (g_pixbufs[i]);
		g_pixbufs[i] = pixbuf;
		g_list_foreach (g_object_list, pixbuf_update_notify,
				(void *) &i);
	}

	return i;
}

// object/layer handling functions
gboolean
objects_intersect (T_SGEObject * object1, T_SGEObject * object2)
{
	GdkRectangle rect1, rect2, dest;

	rect1.x = object1->x;
	rect1.y = object1->y;
	rect1.width = object1->width;
	rect1.height = object1->height;

	rect2.x = object2->x;
	rect2.y = object2->y;
	rect2.width = object2->width;
	rect2.height = object2->height;

	return gdk_rectangle_intersect (&rect1, &rect2, &dest);
}

void
invalidate_object_if_above (gpointer item, gpointer data)
{
	T_SGEObject *target_object, *source_object;

	source_object = (T_SGEObject *) data;
	target_object = (T_SGEObject *) item;

	if (source_object != target_object)
		if (source_object->layer < target_object->layer)
			if (objects_intersect
			    (source_object, target_object))
				target_object->needs_drawing = -1;
}

void
invalidate_objects_above (T_SGEObject * object)
{
	g_list_foreach (g_object_list, invalidate_object_if_above,
			(void *) object);
}

void
invalidate_background_if_under_object (gpointer item, gpointer data)
{
	T_SGEObject *target_object, *source_object;

	source_object = (T_SGEObject *) data;
	target_object = (T_SGEObject *) item;

	if (source_object != target_object)
		if (target_object->layer == 0)
			if (objects_intersect
			    (source_object, target_object))
				target_object->needs_drawing = -1;
}

void
invalidate_background_beneath (T_SGEObject * object)
{
	g_list_foreach (g_object_list,
			invalidate_background_if_under_object,
			(void *) object);
}

void
invalidate_if_layer (gpointer item, gpointer data)
{
	T_SGEObject *target_object;

	target_object = (T_SGEObject *) item;

	if (target_object->layer == *((int *) data))
		target_object->needs_drawing = -1;
}

void
sge_invalidate_layer (int layer)
{
	g_list_foreach (g_object_list, invalidate_if_layer,
			(void *) &layer);
}

//objects creation/destruction
T_SGEObject *
sge_create_object (gint x, gint y, gint layer, gint pixbuf_id)
{

    T_SGEObject * object;
	object = (T_SGEObject *) g_malloc (sizeof (T_SGEObject));
    object->x = x;
    object->y = y;
    object->vx = 0.0;
    object->vy = 0.0;
    object->ax = 0.0;
    object->ay = 0.0;
    object->pixbuf_id = pixbuf_id;
    object->dest_x = 0;
    object->dest_y = 0;

    object->y_delay = 0;
    object->opacity = 255;
    object->fadeout = FALSE;
    object->zoom = 1.0;
    object->zoomout = FALSE;

    object->saturation = 1.0;
    object->blink = FALSE;
    object->blink_increase = TRUE;

    object->animation = FALSE;
    object->animation_iter = 1;
    object->animation_repeat = TRUE;

    object->stop_condition = NULL;
	object->stop_callback = NULL;
	object->layer = layer;
	object->needs_drawing = 0x01;

	object->width = gdk_pixbuf_get_width (g_pixbufs[pixbuf_id]);
    object->height = gdk_pixbuf_get_height (g_pixbufs[pixbuf_id]);

	if (layer == 0) {
		object->pre_rendered =
		    gdk_pixmap_new (g_drawing_area->window, object->width,
				    object->height, -1);
		gdk_draw_pixbuf (GDK_DRAWABLE (object->pre_rendered), NULL,
				 g_pixbufs[pixbuf_id], 0, 0, 0, 0,
				 object->width, object->height,
				 GDK_RGB_DITHER_NONE, 0, 0);
	} else
		object->pre_rendered = NULL;

    g_object_list = g_list_append (g_object_list, (gpointer) object);
	g_object_list = g_list_sort (g_object_list, compare_by_layer);

	return object;
}

void
sge_destroy_object (gpointer object, gpointer user_data)
{
	invalidate_background_beneath (SGE_OBJECT(object));
	if (SGE_OBJECT(object)->pre_rendered)
		g_object_unref (G_OBJECT(SGE_OBJECT(object)->pre_rendered));
	g_object_list = g_list_remove (g_object_list, object);
	g_free(object);
}

void
sge_destroy_object_on_level (gpointer object, gpointer user_data)
{
    // destroy only objects in the specified level
    if(SGE_OBJECT(object)->layer != GPOINTER_TO_INT(user_data))
        return;

    sge_destroy_object(object, NULL);
}

void
sge_destroy_all_objects (void)
{
	g_list_foreach (g_object_list, sge_destroy_object, NULL);
}

void
sge_destroy_all_objects_on_level (int level)
{
	g_list_foreach (g_object_list, sge_destroy_object_on_level, GINT_TO_POINTER(level));
}

// Stop conditions
int
is_out_of_screen (T_SGEObject * object)
{
	if ((object->x > g_width) ||
	    (object->x < -object->width) ||
	    (object->y > g_height) || (object->y < -object->height))
		return -1;

	return 0;
}

int
has_reached_destination (T_SGEObject * object)
{
    //g_debug("has_reached_destination():\n");
	if (fabs (object->x - object->dest_x) < 1.0 &&
	    fabs (object->y - object->dest_y) < 1.0)
		return -1;

	return 0;
}

int
has_reached_floor (T_SGEObject * object)
{
	if (object->y >= object->dest_y) {
		object->y = object->dest_y;
		return -1;
	}

	return 0;
}

int
has_reached_time_limit (T_SGEObject * object)
{
	if (object->lifetime--)
		return 0;
	return -1;
}

// animations/effects
void
sge_object_rise (T_SGEObject * object)
{
	object->layer++;
	g_object_list = g_list_sort (g_object_list, compare_by_layer);
}

void
sge_object_take_down (T_SGEObject * object)
{
    //g_print("sge_object_take_down():\n");
	object->vx = g_rand_double_range (g_rand_generator, -1.0, 1.0);
    object->vy = g_rand_double_range (g_rand_generator, 0.0, 1.0);
    object->ax = 0.0;
    object->ay = ACCELERATION;
    object->stop_condition = is_out_of_screen;
	object->stop_callback = sge_destroy_object;

	g_object_list = g_list_sort (g_object_list, compare_by_layer);
}

#define NB_STEPS	10

// used for gems swapping
void
sge_object_move_to (T_SGEObject * object, gint dest_x, gint dest_y)
{
    //g_print("sge_object_move_to(): dest_x:%d dest_y:%d\n", dest_x, dest_y);

	object->vx = (dest_x - object->x) / NB_STEPS;
	object->vy = (dest_y - object->y) / NB_STEPS;
	object->dest_x = dest_x;
	object->dest_y = dest_y;
	object->stop_condition = has_reached_destination;
}

void sge_object_fadeout_cb (gpointer object, gpointer user_data)
{
    SGE_OBJECT(object)->fadeout = TRUE;
}

void
sge_object_set_lifetime (T_SGEObject * object, gint seconds)
{
	object->lifetime = (1000 / SGE_TIMER_INTERVAL) * seconds;
	object->stop_condition = has_reached_time_limit;
	object->stop_callback = sge_object_fadeout_cb;
}
void

sge_object_fall_to (T_SGEObject * object, gint y_pos)
{
    //g_print("sge_object_fall_to(): y_pos:%d\n", y_pos);
	if (object->y < y_pos) {
		object->ay = ACCELERATION;
		object->dest_y = y_pos;
		object->stop_condition = has_reached_floor;
	}
}
void
sge_object_fall_to_with_delay (T_SGEObject * object, gint y_pos, gint delay)
{
    //g_print("sge_object_fall_to_with_accel(): y_pos:%d y:%4.1f diff:%4.1f x:%4.1f delay:%4.1f\n", y_pos/48, object->y/48, (y_pos-object->y)/48 +1, object->x/48, 0-object->y/48);

	if (object->y < y_pos) {
		object->ay = ACCELERATION;
		object->dest_y = y_pos;
		object->stop_condition = has_reached_floor;

		object->y_delay = delay;
	}
}

//other useful stuff

gboolean
sge_object_is_moving (T_SGEObject * object)
{
	return ((object->vx != 0.0) || (object->vy != 0.0));
}

gboolean
sge_objects_are_moving (void)
{
	gint i;
	T_SGEObject *object;

	for (i = 0; i < g_list_length (g_object_list); i++) {
		object =
		    (T_SGEObject *) g_list_nth_data (g_object_list, i);
		if (sge_object_is_moving (object))
			return TRUE;
	}
	return FALSE;
}

gboolean
sge_objects_are_moving_on_layer (int layer)
{
	gint i;
	T_SGEObject *object;

	for (i = 0; i < g_list_length (g_object_list); i++) {
		object =
		    (T_SGEObject *) g_list_nth_data (g_object_list, i);
		if (object->layer == layer)
			if (sge_object_is_moving (object))
				return TRUE;
	}
	return FALSE;
}

void sge_set_layer_visibility (int layer, gboolean visibility)
{
    if(layer >= 0 && layer < 5) {
        layers_visibility[layer] = visibility;
        if(layer > 0)
            sge_invalidate_layer(layer-1);
        else
            sge_invalidate_layer(0);
    }
}

void sge_object_set_opacity (T_SGEObject *object, gint opacity)
{

    if(object->pre_rendered && object->opacity == opacity)
        return;

    if (object->pre_rendered)
        g_object_unref (G_OBJECT(object->pre_rendered));

    object->opacity = opacity;

}

// fadeout the object and then destroy it
void sge_object_fadeout (T_SGEObject *object)
{
    object->fadeout = TRUE;
}

// zoomout the object and then destroy it
void sge_object_zoomout (T_SGEObject *object)
{
    object->zoomout = TRUE;
}

void sge_object_blink_start (T_SGEObject *object)
{
    object->blink = TRUE;
    object->blink_increase = TRUE;
}

void sge_object_blink_stop (T_SGEObject *object)
{
    object->blink = FALSE;
}

void sge_object_animate (T_SGEObject *object, gboolean repeat)
{
    object->animation = TRUE;
    object->animation_repeat = repeat;
}

gboolean
sge_object_exists (T_SGEObject *object)
{
    if(g_list_index(g_object_list, object) == -1)
        return FALSE;
    else
        return TRUE;
}
