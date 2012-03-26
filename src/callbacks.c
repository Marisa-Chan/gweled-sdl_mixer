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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "main.h"
#include "sound.h"
#include "sge_core.h"
#include "board_engine.h"
#include "graphic_engine.h"

extern gint gi_game_running;
extern gint gi_score;
extern gboolean g_do_not_score;

extern GtkBuilder *gweled_xml;
extern GtkWidget *g_main_window;
extern GtkWidget *g_pref_window;
extern GtkWidget *g_score_window;
extern GtkWidget *g_drawing_area;
extern GdkPixmap *g_buffer_pixmap;
extern GtkWidget *g_menu_pause;
extern GtkWidget *g_pref_music_button;
extern GtkWidget *g_pref_sounds_button;

extern gint gi_gem_clicked;
extern gint gi_x_click;
extern gint gi_y_click;

extern gint gi_gem_dragged;
extern gint gi_x_drag;
extern gint gi_y_drag;

extern guint board_engine_id;
extern GweledPrefs prefs;

static gint gi_dragging = 0;

static gboolean game_mode_changed = FALSE;

void
on_new1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *box;
	gint response;

	if (gi_game_running) {
		box = gtk_message_dialog_new (GTK_WINDOW (g_main_window),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_MESSAGE_QUESTION,
					      GTK_BUTTONS_YES_NO,
					      _("Do you really want to abort this game?"));

		gtk_dialog_set_default_response (GTK_DIALOG (box),
						 GTK_RESPONSE_NO);
		response = gtk_dialog_run (GTK_DIALOG (box));
		gtk_widget_destroy (box);

		if (response != GTK_RESPONSE_YES)
			return;
	}

    gweled_stop_game ();

    welcome_screen_visibility (TRUE);
}

void
on_scores1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
 	show_hiscores (0, FALSE);
}

void
on_pause_activate (GtkMenuItem *menuitem, gpointer user_data)
{

    if(!gi_game_running) return;

    if(board_get_pause() == TRUE )
	    board_set_pause(FALSE);
    else
	    board_set_pause(TRUE);
}

gboolean
on_window_unmap_event (GtkWidget *widget,
                       GdkEvent  *event,
                       gpointer   user_data)
{
    if(gi_game_running && prefs.game_mode == TIMED_MODE && board_get_pause() == FALSE )
        board_set_pause(TRUE);

    return FALSE;
}

gboolean
on_window_focus_out_event (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
    if( gi_game_running && prefs.game_mode == TIMED_MODE && board_get_pause() == FALSE )
        board_set_pause(TRUE);

    return FALSE;
}

void
on_quit1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
    GtkWidget *box;
    gint response;

    if (gi_game_running) {
        box = gtk_message_dialog_new (GTK_WINDOW (g_main_window),
                          GTK_DIALOG_DESTROY_WITH_PARENT,
                          GTK_MESSAGE_QUESTION,
                          GTK_BUTTONS_YES_NO,
                          _("Do you want to save the current game?"));

        gtk_dialog_set_default_response (GTK_DIALOG (box),
                         GTK_RESPONSE_NO);
        response = gtk_dialog_run (GTK_DIALOG (box));
        gtk_widget_destroy (box);

        if (response == GTK_RESPONSE_YES)
            save_current_game();
        else {
            gchar *filename;

            filename = g_strconcat(g_get_user_config_dir(), "/gweled.saved-game", NULL);
            unlink(filename);
        }
    }

	gtk_main_quit ();
}

void
on_preferences1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gtk_widget_show (g_pref_window);
}
void
on_about1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	const gchar *authors[] = {
	    "Sebastien Delestaing <sebdelestaing@free.fr>",
	    "Daniele Napolitano <dnax88@gmail.com>",
	    NULL
	};

	const gchar *translator_credits = _("translator-credits");

    gtk_show_about_dialog (GTK_WINDOW(g_main_window),
             "authors", authors,
		     "translator-credits", g_strcmp0("translator-credits", translator_credits) ? translator_credits : NULL,
             "comments", _("A puzzle game with gems"),
             "copyright", "Copyright © 2003-2005 Sebastien Delestaing\nCopyright © 2010 Daniele Napolitano",
             "version", VERSION,
             "website", "http://gweled.org",
		     "logo-icon-name", "gweled",
             NULL);
}

gboolean
drawing_area_expose_event_cb (GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
	gdk_draw_drawable (GDK_DRAWABLE (widget->window),
			   widget->style->fg_gc[gtk_widget_get_state (widget)],
			   g_buffer_pixmap, event->area.x, event->area.y,
			   event->area.x, event->area.y, event->area.width,
			   event->area.height);

	return FALSE;
}

gboolean
drawing_area_button_event_cb (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	static int x_press = -1;
	static int y_press = -1;
	static int x_release = -1;
	static int y_release = -1;

    // resume game on click
    if(gi_game_running && board_get_pause() == TRUE && event->type == GDK_BUTTON_RELEASE ) {
        board_set_pause(FALSE);
        // not handle this click
        return FALSE;
    }

    // in pause mode don't accept events
    if(board_get_pause())
        return FALSE;

    // only handle left button
    if(event->button != 1)
        return FALSE;

    respawn_board_engine_loop();

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		x_press = event->x / prefs.tile_size;
		y_press = event->y / prefs.tile_size;
		if (gi_game_running) {
			gi_x_click = x_press;
			gi_y_click = y_press;
			gi_gem_clicked = -1;
			gi_dragging = -1;

			if(prefs.sounds_on)
			    sound_play_sample(CLICK_EVENT);
		}
		break;

	case GDK_BUTTON_RELEASE:
		gi_dragging = 0;
		gi_gem_dragged = 0;
		x_release = event->x / prefs.tile_size;
		y_release = event->y / prefs.tile_size;
		if( (x_release < 0) ||
			(x_release > BOARD_WIDTH - 1) ||
			(y_release < 0) ||
			(y_release > BOARD_HEIGHT - 1) )
			break;
		if( (x_press != x_release) ||
			(y_press != y_release))
		{
			if (gi_game_running) {
				gi_x_click = x_release;
				gi_y_click = y_release;
				gi_gem_clicked = -1;
			}
		}
		break;

	default:
		break;
	}

	return FALSE;
}

gboolean
drawing_area_motion_event_cb (GtkWidget * widget, GdkEventMotion * event, gpointer user_data)
{
	if (gi_dragging) {
		gi_x_drag = event->x / prefs.tile_size;
		gi_y_drag = event->y / prefs.tile_size;
		gi_gem_dragged = -1;
	}

	return FALSE;
}

gboolean
on_app_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
    on_quit1_activate (NULL, NULL);

	return FALSE;
}

gboolean
on_preferencesDialog_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	gtk_widget_hide (widget);
	return TRUE;
}

void
on_closebutton1_clicked (GtkButton * button, gpointer user_data)
{
	save_preferences();

    // unpause the game if running
    if(gi_game_running)
	    board_set_pause(FALSE);

	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

void on_buttonNormal_clicked (GtkButton * button, gpointer user_data)
{
    prefs.game_mode = NORMAL_MODE;
    welcome_screen_visibility(FALSE);

    gweled_draw_board ();
    gweled_start_new_game ();
    respawn_board_engine_loop();
    gtk_widget_set_sensitive(g_menu_pause, TRUE);
}

void on_buttonTimed_clicked (GtkButton * button, gpointer user_data)
{
    prefs.game_mode = TIMED_MODE;
    welcome_screen_visibility(FALSE);

    gweled_draw_board ();
    gweled_start_new_game ();
    respawn_board_engine_loop();
    gtk_widget_set_sensitive(g_menu_pause, TRUE);
}

void on_buttonEndless_clicked (GtkButton * button, gpointer user_data)
{
    prefs.game_mode = ENDLESS_MODE;
    welcome_screen_visibility(FALSE);

    gweled_draw_board ();
    gweled_start_new_game ();
    respawn_board_engine_loop();
    gtk_widget_set_sensitive(g_menu_pause, TRUE);
}

void
on_smallRadiobutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		prefs.tile_size = 32;

		gweled_set_objects_size (prefs.tile_size);
	}

	if(!gi_game_running)
        welcome_screen_visibility(TRUE);
}

void
on_mediumRadiobutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		prefs.tile_size = 48;

		gweled_set_objects_size (prefs.tile_size);
	}

	if(!gi_game_running)
        welcome_screen_visibility(TRUE);
}

void
on_largeRadiobutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		prefs.tile_size = 64;

		gweled_set_objects_size (prefs.tile_size);
	}

    if(!gi_game_running)
        welcome_screen_visibility(TRUE);
}

void
on_music_checkbutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
	    prefs.music_on = TRUE;
	    if(sound_get_enabled() == FALSE) {
	        sound_init();
		    if(sound_get_enabled() == FALSE) {
	            gtk_widget_set_sensitive(g_pref_music_button, FALSE);
	            gtk_widget_set_sensitive(g_pref_sounds_button, FALSE);
	        }
		}
		sound_music_play();
	} else {
	    prefs.music_on = FALSE;
		sound_music_stop();
		if(prefs.sounds_on == FALSE)
		    sound_destroy();
    }
}

void
on_sounds_checkbutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		prefs.sounds_on = TRUE;
		if(sound_get_enabled() == FALSE) {
	        sound_init();
		    if(sound_get_enabled() == FALSE) {
	            gtk_widget_set_sensitive(g_pref_music_button, FALSE);
	            gtk_widget_set_sensitive(g_pref_sounds_button, FALSE);
	        }
		}
		sound_load_samples();
	} else {
		prefs.sounds_on = FALSE;
		sound_unload_samples();
		if(prefs.music_on == FALSE)
		    sound_destroy();
    }
}
