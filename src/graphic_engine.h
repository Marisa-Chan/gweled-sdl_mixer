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

#ifndef _GRAPHIC_ENGINE_H
#define _GRAPHIC_ENGINE_H

#include <glib.h>

#define FONT_WIDTH    24
#define FONT_HEIGHT   20


void
gweled_load_font (void);

void
gweled_load_pixmaps (void);

void
gweled_init_glyphs (void);

void
gweled_draw_message (gchar *in_message);

void
gweled_draw_game_message (gchar *in_message, int timing);

void
gweled_draw_message_at (gchar *in_message, gint msg_x, gint msg_y);

void
gweled_gems_fall_into_place (gboolean with_delay);

T_SGEObject*
gweled_draw_character (int x, int y, int layer, char character);

void
gweled_draw_board (void);

void
gweled_set_objects_size (gint size);

#endif
