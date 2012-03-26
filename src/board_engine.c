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

/* for memset and strlen */
#include <string.h>
/* for fabs() */
#include <math.h>

#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gi18n-lib.h>

#include "games-scores.h"

#include "main.h"

#include "sge_core.h"
#include "board_engine.h"
#include "graphic_engine.h"
#include "sound.h"

#define FIRST_BONUS_AT	100	// needs tweaking
#define NB_BONUS_GEMS	8	// same
#define TOTAL_STEPS_FOR_TIMER	60	// seconds
#define HINT_TIMEOUT  15	// seconds

void gweled_remove_gems_and_update_score (void);

enum {
	_IDLE,
	_FIRST_GEM_CLICKED,
	_SECOND_GEM_CLICKED,
	_ILLEGAL_MOVE,
	_MARK_ALIGNED_GEMS,
	_BOARD_REFILLING
};

typedef struct s_alignment {
	gint x;
	gint y;
	gint direction;
	gint length;
} T_Alignment;

gint gi_score, gi_current_score, gi_game_running, gi_game_paused;

gint gi_total_gems_removed;
gint gi_score_per_move;

guint hint_timeout;

gint gi_bonus_multiply;
gint gi_previous_bonus_at;
gint gi_next_bonus_at;
gint gi_level;
gint gi_trigger_bonus;
guint g_steps_for_timer;

gint gi_gem_clicked = 0;
gint gi_x_click = 0;
gint gi_y_click = 0;

gint gi_gem_dragged = 0;
gint gi_x_drag = 0;
gint gi_y_drag = 0;

gint gpc_game_board[BOARD_WIDTH][BOARD_HEIGHT];
gint gi_nb_of_tiles[7];

gboolean g_do_not_score;

T_SGEObject *g_gem_objects[BOARD_WIDTH][BOARD_HEIGHT];
unsigned char gpc_bit_n[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

static T_SGEObject *g_hint_object = NULL;

static gint gi_state = _IDLE;

static GList *g_alignment_list;

extern GRand *g_random_generator;
extern GtkWidget *g_progress_bar;
extern GtkWidget *g_score_label;
extern GtkWidget *g_menu_pause;

extern gint gi_gems_pixbuf[7];
extern gint gi_cursor_pixbuf;
extern gint gi_powerglow_pixbuf;
extern gint gi_sparkle_pixbuf;

extern guint board_engine_id;
extern GweledPrefs prefs;

extern GamesScores *highscores;

gchar
get_new_tile (void)
{
	int i;
	int min, max, min_index, max_index, previous_min_index;

	min_index = 0;
	previous_min_index = 0;
	max_index = 0;
	min = gi_nb_of_tiles[0];
	max = gi_nb_of_tiles[0];
	for (i = 0; i < 7; i++) {
		if (gi_nb_of_tiles[i] < min) {
			min = gi_nb_of_tiles[i];
			min_index = i;
			previous_min_index = min_index;
		}
		if (gi_nb_of_tiles[i] > max) {
			max = gi_nb_of_tiles[i];
			max_index = i;
		}
	}

	i = (gint) g_rand_int_range (g_random_generator, 0, 2);

	switch (i) {
	case 0:
		return g_rand_int_range (g_random_generator, 0, 2) ? min_index : previous_min_index;
	default:
		return (max_index + (gchar) g_rand_int_range (g_random_generator, 1, 7)) % 7;
	}
}



gint
gweled_is_part_of_an_alignment (gint x, gint y)
{
	gint i, result;

	result = 0;
	for (i = x - 2; i <= x; i++)
		if (i >= 0 && i + 2 < BOARD_WIDTH)
			if (gpc_bit_n[gpc_game_board[i][y]] &
			     gpc_bit_n[gpc_game_board[i + 1][y]] &
			     gpc_bit_n[gpc_game_board[i + 2][y]]) {
				result |= 1;	// is part of an horizontal alignment
				break;
			}

	for (i = y - 2; i <= y; i++)
		if (i >= 0 && i + 2 < BOARD_HEIGHT)
		if (gpc_bit_n[gpc_game_board[x][i]] &
		     gpc_bit_n[gpc_game_board[x][i + 1]] &
		     gpc_bit_n[gpc_game_board[x][i + 2]]) {
				result |= 2;	// is part of a vertical alignment
				break;
		}

	return result;
}

gboolean
gweled_check_for_moves_left (int *pi, int *pj)
{
	gint i, j;

	for (j = 0; j <= BOARD_HEIGHT - 1; j++)
		for (i = 0; i <= BOARD_WIDTH - 1; i++) {
			if (i > 0) {
				gweled_swap_gems (i - 1, j, i, j);
				if (gweled_is_part_of_an_alignment (i, j)) {
					gweled_swap_gems (i - 1, j, i, j);
					i = i - 1;
					goto move_found;
				}
				gweled_swap_gems (i - 1, j, i, j);
			}
			if (i < 7) {
				gweled_swap_gems (i + 1, j, i, j);
				if (gweled_is_part_of_an_alignment (i, j)) {
					gweled_swap_gems (i + 1, j, i, j);
					i = i + 1;
					goto move_found;
				}
				gweled_swap_gems (i + 1, j, i, j);
			}
			if (j > 0) {
				gweled_swap_gems (i, j - 1, i, j);
				if (gweled_is_part_of_an_alignment (i, j)) {
					gweled_swap_gems (i, j - 1, i, j);
					j = j - 1;
					goto move_found;
				}
				gweled_swap_gems (i, j - 1, i, j);
			}
			if (j < 7) {
				gweled_swap_gems (i, j + 1, i, j);
				if (gweled_is_part_of_an_alignment (i, j)) {
					gweled_swap_gems (i, j + 1, i, j);
					j = j + 1;
					goto move_found;
				}
				gweled_swap_gems (i, j + 1, i, j);
			}
		}
	return FALSE;

move_found:
	if (pi && pj) {
		*pi = i;
		*pj = j;
	}
	return TRUE;
}

void
gweled_swap_gems (gint x1, gint y1, gint x2, gint y2)
{
	gint i;
	T_SGEObject * object;

	object = g_gem_objects[x1][y1];
	g_gem_objects[x1][y1] = g_gem_objects[x2][y2];
	g_gem_objects[x2][y2] = object;
	i = gpc_game_board[x1][y1];
	gpc_game_board[x1][y1] = gpc_game_board[x2][y2];
	gpc_game_board[x2][y2] = i;
}
void
gweled_refill_board (void)
{
	gint i, j, k;
	//g_debug("gweled_refill_board():");

	for (i = 0; i < BOARD_WIDTH; i++)
		for (j = 0; j < BOARD_HEIGHT; j++)
			if (gpc_game_board[i][j] == -1)
			{
				for (k = j; k > 0; k--)
				{
					gpc_game_board[i][k] =
					    gpc_game_board[i][k - 1];
					g_gem_objects[i][k] =
					    g_gem_objects[i][k - 1];
				}
				gpc_game_board[i][0] = get_new_tile ();
				gi_nb_of_tiles[gpc_game_board[i][0]]++;

				// make sure the new tile appears outside of the screen (1st row is special-cased)
				if (j && g_gem_objects[i][1])
					g_gem_objects[i][0] = sge_create_object (i * prefs.tile_size,
											g_gem_objects[i][1]->y - prefs.tile_size,
											1,
											gi_gems_pixbuf[gpc_game_board[i][0]]);
				else
					g_gem_objects[i][0] = sge_create_object (i * prefs.tile_size,
											-prefs.tile_size,
											1,
											gi_gems_pixbuf[gpc_game_board[i][0]]);
			}
}

void
delete_alignment_from_board (gpointer alignment_pointer, gpointer user_data)
{
	gint i, i_total_score;
    gint gi_gems_removed = 0;
	int xsize, ysize, xhotspot, yhotspot, xpos, ypos;
	char *buffer;
	T_Alignment *alignment;
	T_SGEObject *object;

	alignment = (T_Alignment *) alignment_pointer;
    // delete alignment
	if (alignment->direction == 1)	// horizontal
	{
		xhotspot = (alignment->x * prefs.tile_size + alignment->length * prefs.tile_size / 2);
		yhotspot = (alignment->y * prefs.tile_size + prefs.tile_size / 2);
		for (i = alignment->x; i < alignment->x + alignment->length; i++) {
			if (gpc_game_board[i][alignment->y] != -1) {
				gi_gems_removed++;
				gi_nb_of_tiles[gpc_game_board[i][alignment->y]]--;
				gpc_game_board[i][alignment->y] = -1;
			}
		}
	} else {
		xhotspot = (alignment->x * prefs.tile_size + prefs.tile_size / 2);
		yhotspot = (alignment->y * prefs.tile_size + alignment->length * prefs.tile_size / 2);
		for (i = alignment->y; i < alignment->y + alignment->length; i++) {
			if (gpc_game_board[alignment->x][i] != -1) {
				gi_gems_removed++;
				gi_nb_of_tiles[gpc_game_board[alignment->x][i]]--;
				gpc_game_board[alignment->x][i] = -1;
			}
		}
	}
    //compute score
	if (alignment->length == 1) {	//bonus mode
		i_total_score = 10 * g_rand_int_range (g_random_generator, 1, 2);
    }
	else {
		i_total_score = 10 * (gi_bonus_multiply >> 1) * (gi_gems_removed - 2) + gi_score_per_move;
        if(g_do_not_score == TRUE)
            gi_score_per_move = i_total_score;
    }
    if (g_do_not_score == FALSE) {
        gi_total_gems_removed += gi_gems_removed;
        //g_print("Score: %d Gems removed: %d\n", i_total_score, gi_gems_removed);
		gi_score += i_total_score;
        //display score
		buffer = g_strdup_printf ("%d", i_total_score);
		xsize = strlen (buffer) * FONT_WIDTH;
		ysize = FONT_HEIGHT;
		for (i = 0; i < strlen (buffer); i++) {
			xpos = xhotspot - xsize / 2 + i * FONT_WIDTH;
			ypos = yhotspot - ysize / 2;
			object = gweled_draw_character (xpos, ypos, 4, buffer[i]);
			object->vy = -1.0;
			sge_object_set_lifetime (object, 1);	//1s
		}
		g_free (buffer);
	}
}

void
gweled_remove_gems_and_update_score (void)
{
	g_list_foreach (g_alignment_list, delete_alignment_from_board, NULL);
}

void
take_down_alignment (gpointer object, gpointer user_data)
{
	gint i;
	T_Alignment *alignment;

	alignment = (T_Alignment *) object;

	if (alignment->direction == 1)	// horizontal
		for (i = alignment->x; i < alignment->x + alignment->length; i++)
			sge_object_zoomout (g_gem_objects[i][alignment->y]);
	else
		for (i = alignment->y; i < alignment->y + alignment->length; i++)
			sge_object_zoomout (g_gem_objects[alignment->x][i]);
}

void
gweled_take_down_deleted_gems (void)
{
	g_list_foreach (g_alignment_list, take_down_alignment, NULL);
}

void
destroy_alignment (gpointer object, gpointer user_data)
{
	g_alignment_list = g_list_remove (g_alignment_list, object);
}

void
destroy_all_alignments (void)
{
	g_list_foreach (g_alignment_list, destroy_alignment, NULL);
}

void
gweled_delete_gems_for_bonus (void)
{
	gint i;
	T_Alignment * alignment;

	destroy_all_alignments ();
	for (i = 0; i < NB_BONUS_GEMS; i++) {
		alignment = (T_Alignment *) g_malloc (sizeof (T_Alignment));
		alignment->x = g_rand_int_range (g_random_generator, 0, 7);
		alignment->y = g_rand_int_range (g_random_generator, 0, 7);
		alignment->direction = 1;
		alignment->length = 1;
		g_alignment_list = g_list_append (g_alignment_list, (gpointer) alignment);
	}
}


// FIXME!!!
//
// if we have the following pattern:
//
// xxoxoo
//
// and swap the 2 central gems:
//
// xxxooo <- this is counted as 1 alignment of 6
//
// giving a score of 40 appearing in the middle rather than 10 + 40 (combo bonus).
// However the fix implies a significant change in the function below for
// a bug that is unlikely to happen. I will fix it. Just... not now.
gboolean
gweled_check_for_alignments (void)
{
	gint i, j, i_nb_aligned, start_x, start_y;
	T_Alignment *alignment;

	destroy_all_alignments ();

    // make a list of vertical alignments
	i_nb_aligned = 0;

	for (i = 0; i < BOARD_WIDTH; i++) {
		for (j = 0; j < BOARD_HEIGHT; j++)
			if ((gweled_is_part_of_an_alignment (i, j) & 2) == 2) {
				// record the origin of the alignment
				if (i_nb_aligned == 0) {
					start_x = i;
					start_y = j;
				}
				i_nb_aligned++;
			} else {
				// we found one, let's remember it for later use
				if (i_nb_aligned > 2) {
					alignment = (T_Alignment *)g_malloc (sizeof (T_Alignment));
					alignment->x = start_x;
					alignment->y = start_y;
					alignment->direction = 2;
					alignment->length = i_nb_aligned;
					g_alignment_list = g_list_append(g_alignment_list, (gpointer) alignment);
				}
				i_nb_aligned = 0;
			}

		// end of column
		if (i_nb_aligned > 2) {
			alignment = (T_Alignment *)g_malloc (sizeof (T_Alignment));
			alignment->x = start_x;
			alignment->y = start_y;
			alignment->direction = 2;
			alignment->length = i_nb_aligned;
			g_alignment_list = g_list_append (g_alignment_list, (gpointer) alignment);
		}
		i_nb_aligned = 0;
	}

    // make a list of horizontal alignments
	i_nb_aligned = 0;

	for (j = 0; j < BOARD_HEIGHT; j++) {
		for (i = 0; i < BOARD_WIDTH; i++)
			if ((gweled_is_part_of_an_alignment (i, j) & 1) == 1) {
				// record the origin of the alignment
				if (i_nb_aligned == 0) {
					start_x = i;
					start_y = j;
				}
				i_nb_aligned++;
			} else {
				// if we found one, let's remember it for later use
				if (i_nb_aligned > 2) {
					alignment = (T_Alignment *)g_malloc (sizeof (T_Alignment));
					alignment->x = start_x;
					alignment->y = start_y;
					alignment->direction = 1;
					alignment->length = i_nb_aligned;
					g_alignment_list = g_list_append (g_alignment_list, (gpointer) alignment);
				}
				i_nb_aligned = 0;
			}

		// end of row
		if (i_nb_aligned > 2) {
			alignment = (T_Alignment *) g_malloc (sizeof (T_Alignment));
			alignment->x = start_x;
			alignment->y = start_y;
			alignment->direction = 1;
			alignment->length = i_nb_aligned;
			g_alignment_list = g_list_append (g_alignment_list, (gpointer) alignment);
		}
		i_nb_aligned = 0;
	}

	return (g_list_length (g_alignment_list) != 0);
}

void
board_set_pause(gboolean value)
{
    static gchar *last_text;
    gi_game_paused = value;

    if(value == TRUE) {
        gtk_menu_item_set_label(GTK_MENU_ITEM(g_menu_pause), _("_Resume"));
        gweled_draw_message("paused");
        last_text = g_strdup(gtk_progress_bar_get_text(GTK_PROGRESS_BAR(g_progress_bar)));
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_progress_bar), _("Paused"));
        sge_set_layer_visibility(1, FALSE);
        sge_set_layer_visibility(2, FALSE);
        if(hint_timeout) {
            g_source_remove(hint_timeout);
            hint_timeout = 0;
        }
        if(sge_object_exists(g_hint_object)) {
            sge_destroy_object (g_hint_object, NULL);
            g_hint_object = NULL;
        }
    }
    else {
        gtk_menu_item_set_label(GTK_MENU_ITEM(g_menu_pause), _("_Pause"));
        if(last_text != NULL) {
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_progress_bar), last_text);
            g_free(last_text);
            last_text = NULL;
        }
        sge_set_layer_visibility(1, TRUE);
        sge_set_layer_visibility(2, TRUE);
        sge_destroy_all_objects_on_level(3);
        respawn_board_engine_loop();
    }
}

gboolean
board_get_pause()
{
    return gi_game_paused;
}

gboolean
hint_callback (gpointer data)
{
	gint x, y;

	if (gi_game_running) {
		gweled_check_for_moves_left (&x, &y);
		//g_debug("hint_callback(): x:%d, y%d", x, y);
		g_hint_object = sge_create_object (prefs.tile_size * x,
					prefs.tile_size * y,
					2, gi_powerglow_pixbuf);
		sge_object_animate(g_hint_object, TRUE);
		sge_object_set_lifetime (g_hint_object, 2);
	}

	return TRUE;

}

gboolean
board_engine_loop (gpointer data)
{
	static gint x1, y1, x2, y2, time_slice = 0;
	static T_SGEObject *cursor[2] = { NULL, NULL };
	gchar msg_buffer[200];
    GamesScoreValue score;
	gint hiscore_rank;

	time_slice++;

	const gchar* state[] = {"_IDLE", "_FIRST_GEM_CLICKED", "_SECOND_GEM_CLICKED",
	                        "_ILLEGAL_MOVE", "_MARK_ALIGNED_GEMS", "_BOARD_REFILLING"};

    // progressive score
	if(gi_current_score < gi_score)
	{
		gi_current_score += 10;
		g_sprintf (msg_buffer, "<span weight=\"bold\">%06d</span>", gi_current_score);
		gtk_label_set_markup ((GtkLabel *) g_score_label, msg_buffer);
	}

    /* Let's first check if we are in timer mode, and penalize the player if necessary */
	if (prefs.game_mode == TIMED_MODE && gi_game_running && !gi_game_paused  && (time_slice % 10 == 0))
	{
		gi_total_gems_removed -= g_steps_for_timer;
		if (gi_total_gems_removed <= gi_previous_bonus_at) {
			gweled_draw_message ("time's up #");
			gi_game_running = 0;
 			score.plain = gi_score;
 			games_scores_set_category (highscores, "Timed");
 			hiscore_rank = games_scores_add_score (highscores, score);
 			if (show_hiscores (hiscore_rank, TRUE) == GTK_RESPONSE_REJECT)
                gtk_main_quit ();
            else {
                sge_destroy_all_objects ();
	            gweled_draw_board ();
	            gweled_start_new_game ();
            }
			g_do_not_score = FALSE;
			gi_state = _IDLE;
		} else
			gtk_progress_bar_set_fraction ((GtkProgressBar *)
						       g_progress_bar,
						       (float)(gi_total_gems_removed -gi_previous_bonus_at)
						       / (float)(gi_next_bonus_at - gi_previous_bonus_at));
	}

    //g_debug("Current state: %s", state[gi_state]);

    if(hint_timeout && gi_gem_clicked) {
        g_source_remove(hint_timeout);
        hint_timeout = 0;
        if(g_hint_object && sge_object_exists(g_hint_object)) {
            sge_destroy_object (g_hint_object, NULL);
            g_hint_object = NULL;
        } else
            g_hint_object = NULL;
    }

	switch (gi_state) {
	case _IDLE:
		if (gi_gem_clicked) {
			x1 = gi_x_click;
			y1 = gi_y_click;
			gi_state = _FIRST_GEM_CLICKED;
			sge_object_blink_start(g_gem_objects[gi_x_click][gi_y_click]);

			if (cursor[0])
				sge_destroy_object (cursor[0], NULL);
			cursor[0] = sge_create_object (prefs.tile_size * x1,
					prefs.tile_size * y1,
					2, gi_cursor_pixbuf);
			gi_gem_clicked = 0;
			gi_gem_dragged = 0;
		} else

		break;

	case _FIRST_GEM_CLICKED:
		if (gi_gem_clicked) {
			x2 = gi_x_click;
			y2 = gi_y_click;
			gi_gem_clicked = 0;
			if (((x1 == x2) && (fabs (y1 - y2) == 1)) ||
			    ((y1 == y2) && (fabs (x1 - x2) == 1))) {
				// If the player clicks an adjacent gem, try to swap
				if (cursor[1])
					sge_destroy_object (cursor[1], NULL);
				cursor[1] = sge_create_object (prefs.tile_size * x2,
						prefs.tile_size * y2,
						2, gi_cursor_pixbuf);
				sge_object_blink_stop(g_gem_objects[x1][y1]);
				// swap gems
				sge_object_move_to (g_gem_objects[x1][y1],
						x2 * prefs.tile_size,
						y2 * prefs.tile_size);
				sge_object_move_to (g_gem_objects[x2][y2],
						x1 * prefs.tile_size,
						y1 * prefs.tile_size);
				// swap cursors
				sge_object_move_to (cursor[0],
						x2 * prefs.tile_size,
						y2 * prefs.tile_size);
				sge_object_move_to (cursor[1],
						x1 * prefs.tile_size,
						y1 * prefs.tile_size);
				if(prefs.sounds_on == TRUE)
				    sound_play_sample(SWAP_EVENT);
				gi_state = _SECOND_GEM_CLICKED;
			} else if((x1 == x2) && (y1 == y2)) {
				if (cursor[1]) {
					sge_destroy_object (cursor[1], NULL);
				    cursor[1] = NULL;
				}
				// If the player clicks the selected gem, deselect it
				if(cursor[0]) {
					sge_destroy_object(cursor[0], NULL);
					cursor[0] = NULL;
				}
				sge_object_blink_stop(g_gem_objects[x1][y1]);
				gi_state = _IDLE;
				gi_gem_clicked = 0;
			} else {
				if (cursor[1]) {
					sge_destroy_object (cursor[1], NULL);
				    cursor[1] = NULL;
				}
				// If the player clicks anywhere else, make that the first selection
				sge_object_blink_stop(g_gem_objects[x1][y1]);
				sge_object_blink_start(g_gem_objects[x2][y2]);
				x1 = x2;
				y1 = y2;
				if (cursor[0])
					sge_destroy_object (cursor[0], NULL);
				cursor[0] = sge_create_object (prefs.tile_size * x1,
						prefs.tile_size * y1,
						2, gi_cursor_pixbuf);
			}
		}else if(gi_gem_dragged)
		{
			//printf("gem dragged\n");
			if (cursor[1])
					sge_destroy_object (cursor[1], NULL);
			cursor[1] = sge_create_object (prefs.tile_size * gi_x_drag,
						prefs.tile_size * gi_y_drag,
						2, gi_cursor_pixbuf);
		}
		break;

	case _SECOND_GEM_CLICKED:
		if (!sge_object_is_moving (g_gem_objects[x1][y1]) && !sge_object_is_moving (g_gem_objects[x2][y2])) {
			gweled_swap_gems (x1, y1, x2, y2);
			if (!gweled_is_part_of_an_alignment (x1, y1) && !gweled_is_part_of_an_alignment (x2, y2)) {
				// re-swap gems
				sge_object_move_to (g_gem_objects[x1][y1],
						x2 * prefs.tile_size,
						y2 * prefs.tile_size);
				sge_object_move_to (g_gem_objects[x2][y2],
						x1 * prefs.tile_size,
						y1 * prefs.tile_size);
				// re-swap cursors
				sge_object_move_to (cursor[1],
						x2 * prefs.tile_size,
						y2 * prefs.tile_size);
				sge_object_move_to (cursor[0],
						x1 * prefs.tile_size,
						y1 * prefs.tile_size);

				gi_state = _ILLEGAL_MOVE;
			} else {
                gi_score_per_move = 0;
				gi_state = _MARK_ALIGNED_GEMS;
			}
			// fadeout cursors
			if (cursor[0])
				sge_object_fadeout (cursor[0]);
			if (cursor[1])
				sge_object_fadeout (cursor[1]);
			cursor[0] = NULL;
			cursor[1] = NULL;
		}
		break;

	case _ILLEGAL_MOVE:
		if (!sge_object_is_moving (g_gem_objects[x1][y1]) && !sge_object_is_moving (g_gem_objects[x2][y2])) {
			gweled_swap_gems (x1, y1, x2, y2);
			gi_state = _IDLE;
		}
		break;

	case _MARK_ALIGNED_GEMS:
		if (gweled_check_for_alignments () == TRUE) {
			gweled_take_down_deleted_gems ();
			gweled_remove_gems_and_update_score ();

            if(prefs.game_mode != ENDLESS_MODE) {
                if (gi_total_gems_removed <= gi_next_bonus_at)
				    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(g_progress_bar), (float) (gi_total_gems_removed - gi_previous_bonus_at) / (float) (gi_next_bonus_at - gi_previous_bonus_at));
			    else
				    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(g_progress_bar), 1.0);

                gi_state = _BOARD_REFILLING;
            }
			gweled_refill_board ();
			gweled_gems_fall_into_place (FALSE);
		} else {
			if (gweled_check_for_moves_left (NULL, NULL) == FALSE) {
				if ((gi_next_bonus_at == FIRST_BONUS_AT) && (prefs.game_mode != ENDLESS_MODE)) {
					gint i, j;
                    // TRANSLATORS: # is replaced with !!
					gweled_draw_game_message ("no moves left #", 1);
					memset (gi_nb_of_tiles, 0, 7 * sizeof (int));

					for (i = 0; i < BOARD_WIDTH; i++)
						for (j = 0; j < BOARD_HEIGHT; j++) {
							sge_destroy_object (g_gem_objects[i][j], NULL);
							gpc_game_board[i][j] = get_new_tile();
							gi_nb_of_tiles[gpc_game_board[i][j]]++;
						}
					g_do_not_score = TRUE;
					while(gweled_check_for_alignments ()) {
						gweled_remove_gems_and_update_score ();
						gweled_refill_board();
					};
					g_do_not_score = FALSE;
					for (i = 0; i < BOARD_WIDTH; i++)
						for (j = 0; j < BOARD_HEIGHT; j++) {
							g_gem_objects[i][j] = sge_create_object
								(i * prefs.tile_size,
							    	(j - BOARD_HEIGHT) * prefs.tile_size,
								1, gi_gems_pixbuf[gpc_game_board[i][j]]);
						}
					gweled_gems_fall_into_place (FALSE);
					gi_state = _MARK_ALIGNED_GEMS;
				} else {
					gweled_draw_message ("no moves left #");
					gi_game_running = 0;
					score.plain = gi_score;
					games_scores_set_category (highscores, "Normal");
					hiscore_rank = games_scores_add_score (highscores, score);
 					if (show_hiscores (hiscore_rank, TRUE) == GTK_RESPONSE_REJECT)
                        gtk_main_quit ();
                    else {
                        sge_destroy_all_objects ();
	                    gweled_draw_board ();
	                    gweled_start_new_game ();
                    }
					g_do_not_score = FALSE;
					gi_state = _IDLE;
				}
			} else {
				g_do_not_score = FALSE;
				gi_state = _IDLE;
			}
		}
		break;

	case _BOARD_REFILLING:
        if (!sge_objects_are_moving_on_layer (1)) {
			if (gi_total_gems_removed >= gi_next_bonus_at) {
				gi_previous_bonus_at = gi_next_bonus_at;
				gi_next_bonus_at *= 2;

				if (prefs.game_mode == TIMED_MODE)
					g_steps_for_timer = (gi_next_bonus_at - gi_previous_bonus_at) / TOTAL_STEPS_FOR_TIMER + 1;

                // draw bonus message and new level in game
                gi_bonus_multiply++;
                gi_level++;
                g_sprintf(msg_buffer, _("Level %d"), gi_level);
                gtk_progress_bar_set_text(GTK_PROGRESS_BAR (g_progress_bar), msg_buffer);
				g_sprintf (msg_buffer, "bonus x%d", gi_bonus_multiply >> 1);
				gweled_draw_game_message (msg_buffer, 2);

				gweled_delete_gems_for_bonus ();
				gweled_take_down_deleted_gems ();
				gweled_remove_gems_and_update_score ();

				if (prefs.game_mode == TIMED_MODE)
					gi_total_gems_removed = (gi_next_bonus_at + gi_previous_bonus_at) / 2;

				gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(g_progress_bar),
					(float) (gi_total_gems_removed - gi_previous_bonus_at) /
					(float) (gi_next_bonus_at - gi_previous_bonus_at));

				gweled_refill_board ();
				gweled_gems_fall_into_place (FALSE);
				g_do_not_score = TRUE;
			} else {
				gi_state = _MARK_ALIGNED_GEMS;
			}
		}
		break;
	default:
		break;
	}

	if(gi_state == _IDLE && gi_gem_clicked == FALSE && !hint_timeout)
	    hint_timeout = g_timeout_add_seconds (HINT_TIMEOUT, hint_callback, NULL);

	if((gi_state == _IDLE || gi_state == _FIRST_GEM_CLICKED) && gi_current_score == gi_score && ((prefs.game_mode == TIMED_MODE && gi_game_paused) || !(prefs.game_mode == TIMED_MODE)))
    {
        board_engine_id = 0;
        //g_debug("Board engine timer stopped");
        return FALSE;
    }
	return TRUE;
}

void respawn_board_engine_loop()
{
    if(!board_engine_id)
        board_engine_id = g_timeout_add (100, board_engine_loop, NULL);
}

void
gweled_start_new_game (void)
{
	gint i, j;

    gi_game_paused = 0;
	gi_score = 0;
	gi_current_score = 0;
    gi_score_per_move = 0;
	gi_bonus_multiply = 3;
	gi_level = 1;
	gi_previous_bonus_at = 0;
	gi_next_bonus_at = FIRST_BONUS_AT;
	gi_trigger_bonus = 0;
	g_steps_for_timer = FIRST_BONUS_AT / TOTAL_STEPS_FOR_TIMER;

	if(prefs.game_mode == TIMED_MODE)
		gi_total_gems_removed = FIRST_BONUS_AT / 2;
	else
		gi_total_gems_removed = 0;

	if(hint_timeout) {
        g_source_remove(hint_timeout);
        hint_timeout = 0;
    }

    if(prefs.game_mode != ENDLESS_MODE) {
        gchar *text = g_strdup_printf(_("Level %d"), 1);
	    gtk_progress_bar_set_text(GTK_PROGRESS_BAR (g_progress_bar), text );
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (g_progress_bar), 0.0);
	    g_free(text);
    }
    else
    {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR (g_progress_bar), "" );
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (g_progress_bar), 0.0);
    }


    gtk_label_set_markup ((GtkLabel *) g_score_label, "<span weight=\"bold\">000000</span>");

	memset (gi_nb_of_tiles, 0, 7 * sizeof (int));

	for (i = 0; i < BOARD_WIDTH; i++)
		for (j = 0; j < BOARD_HEIGHT; j++)
		{
			gpc_game_board[i][j] = get_new_tile ();
			gi_nb_of_tiles[gpc_game_board[i][j]]++;
			g_gem_objects[i][j] =
			    sge_create_object (i * prefs.tile_size,
							(j - BOARD_HEIGHT) * prefs.tile_size,
							1,
							gi_gems_pixbuf[gpc_game_board[i][j]]);
		}

	g_do_not_score = TRUE;
	while(gweled_check_for_alignments ()) {
		gweled_remove_gems_and_update_score ();
		gweled_refill_board();
	};
	g_do_not_score = FALSE;

    //test pattern for a known bug
/*
    gpc_game_board[0][7] = 0;
    gpc_game_board[1][7] = 0;
    gpc_game_board[2][7] = 1;
    gpc_game_board[3][7] = 0;
    gpc_game_board[4][7] = 1;
    gpc_game_board[5][7] = 1;
*/


	for (i = 0; i < BOARD_WIDTH; i++)
		for (j = 0; j < BOARD_HEIGHT; j++)
			g_gem_objects[i][j] = sge_create_object (i * prefs.tile_size, (j - BOARD_HEIGHT) * prefs.tile_size, 1,
													 gi_gems_pixbuf[gpc_game_board[i][j]]);

	gweled_gems_fall_into_place (TRUE);

	gi_game_running = -1;
	gi_state = _MARK_ALIGNED_GEMS;
}

GweledGameState gweled_get_current_game(void)
{
    GweledGameState game;
    int i, j;

    game.game_mode = prefs.game_mode;
    game.gi_score = gi_score;
    game.gi_total_gems_removed = gi_total_gems_removed;
    game.gi_bonus_multiply = gi_bonus_multiply;
    game.gi_previous_bonus_at = gi_previous_bonus_at;
    game.gi_next_bonus_at = gi_next_bonus_at;
    game.gi_level = gi_level;
    game.gi_trigger_bonus = gi_trigger_bonus;
    game.g_steps_for_timer = g_steps_for_timer;

    for (i = 0; i < BOARD_WIDTH; i++)
        for (j = 0; j < BOARD_HEIGHT; j++)
            game.gpc_game_board[i][j] = gpc_game_board[i][j];

    return game;
}

void gweled_set_previous_game(GweledGameState game)
{
    gchar *text_buffer;
    int i, j;

    prefs.game_mode = game.game_mode;
    gi_score = game.gi_score;
    gi_total_gems_removed = game.gi_total_gems_removed;
    gi_bonus_multiply = game.gi_bonus_multiply;
    gi_previous_bonus_at = game.gi_previous_bonus_at;
    gi_next_bonus_at = game.gi_next_bonus_at;
    gi_level = game.gi_level;
    gi_trigger_bonus = game.gi_trigger_bonus;
    g_steps_for_timer = game.g_steps_for_timer;
    gi_current_score = gi_score;

    // update game mode preferences
    init_pref_window();

    if(prefs.game_mode != ENDLESS_MODE) {
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (g_progress_bar),
                                (float)(gi_total_gems_removed -gi_previous_bonus_at)
                                / (float)(gi_next_bonus_at - gi_previous_bonus_at));
        text_buffer = g_strdup_printf(_("Level %d"), gi_level);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR (g_progress_bar), text_buffer);
        g_free(text_buffer);
    }

    text_buffer = g_strdup_printf("<span weight=\"bold\">%06d</span>", gi_current_score);
    gtk_label_set_markup (GTK_LABEL(g_score_label), text_buffer);
    g_free(text_buffer);

    sge_destroy_all_objects ();
    gweled_draw_board ();

    for (i = 0; i < BOARD_WIDTH; i++)
        for (j = 0; j < BOARD_HEIGHT; j++) {
            gpc_game_board[i][j] = game.gpc_game_board[i][j];
            g_gem_objects[i][j] = sge_create_object (i * prefs.tile_size, j * prefs.tile_size, 1,
													 gi_gems_pixbuf[gpc_game_board[i][j]]);
        }

    gi_game_running = -1;
    gi_state = _MARK_ALIGNED_GEMS;

    respawn_board_engine_loop();
    
    gtk_widget_set_sensitive(g_menu_pause, TRUE);
    
    welcome_screen_visibility (FALSE);

    gi_game_running = TRUE;
    gi_game_paused = FALSE;

}


void gweled_stop_game()
{
    board_set_pause(FALSE);
    respawn_board_engine_loop();
    gi_game_running = 0;
	sge_destroy_all_objects ();

    gtk_progress_bar_set_text(GTK_PROGRESS_BAR (g_progress_bar), "" );
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (g_progress_bar), 0.0);
    gtk_label_set_markup ((GtkLabel *) g_score_label, "<span weight=\"bold\">000000</span>");
}
