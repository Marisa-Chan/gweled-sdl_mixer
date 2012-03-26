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

#ifndef _BOARD_ENGINE_H_
#define _BOARD_ENGINE_H_

#define BOARD_WIDTH   8
#define BOARD_HEIGHT  8

typedef enum e_gweled_game_mode
{
    NORMAL_MODE,
    TIMED_MODE,
    ENDLESS_MODE
} gweled_game_mode;

typedef struct s_gweled_prefs
{
	gweled_game_mode game_mode;
	gint tile_size;
	gboolean music_on;
	gboolean sounds_on;
}GweledPrefs;

typedef struct s_gweled_gamestate
{
    gint gpc_game_board[BOARD_WIDTH][BOARD_HEIGHT];
    gweled_game_mode game_mode;
    gint gi_score;
    gint gi_total_gems_removed;
    gint gi_bonus_multiply;
    gint gi_previous_bonus_at;
    gint gi_next_bonus_at;
    gint gi_level;
    gint gi_trigger_bonus;
    guint g_steps_for_timer;

} GweledGameState;

// FUNCTIONS
void gweled_start_new_game(void);
void gweled_swap_gems(gint x1, gint y1, gint x2, gint y2);
void gweled_refill_board(void);

void board_set_pause(gboolean value);

gboolean board_get_pause(void);

void respawn_board_engine_loop();

GweledGameState
gweled_get_current_game(void);

void
gweled_set_previous_game(GweledGameState game);

void
gweled_stop_game();

#endif
