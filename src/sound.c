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

#include <mikmod.h>
#include <pthread.h>
#include <glib.h>

#include "sound.h"
#include "board_engine.h"

static pthread_t thread;

static MODULE *module;
static SAMPLE *swap_sfx = NULL;
static SAMPLE *click_sfx = NULL;

static gboolean is_playing;
static gboolean sound_available;

void sound_thread(void *ptr)
{
	while (1) {
	    g_usleep(10000);
		MikMod_Update();
    }
}

void sound_init()
{
    /* register all the drivers */
    MikMod_RegisterDriver(&drv_esd);
    MikMod_RegisterDriver(&drv_alsa);
    MikMod_RegisterDriver(&drv_oss);
    MikMod_RegisterDriver(&drv_nos);

    /* register only the s3m module loader */
    MikMod_RegisterLoader(&load_s3m);

    /* initialize the library */
    if (MikMod_Init("")) {
        g_printerr("Could not initialize sound, reason: %s\n", MikMod_strerror(MikMod_errno));
        MikMod_Exit();
        sound_available = FALSE;
    } else {
        sound_available = TRUE;
        MikMod_EnableOutput();
        pthread_create(&thread, NULL, (void *)&sound_thread, NULL);
        g_print("Audio driver choosen: %s\n", md_driver->Name);
    }

    is_playing = FALSE;
}


void sound_music_play()
{
	if (!is_playing && sound_available) {
	    // load module
	    module = Player_Load(DATADIR "/sounds/gweled/autonom.s3m", 64, 0);
    	if (module) {
    	    Player_Start(module);
			Player_SetVolume(64);
			is_playing = TRUE;

    	} else
    	    g_printerr("Could not load module, reason: %s\n", MikMod_strerror(MikMod_errno));
	}
}

void sound_music_stop()
{
	if (is_playing && sound_available) {
    	if (module)
		{
	    	Player_Stop();
	    	Player_Free(module);
	    	is_playing = FALSE;
		}
	}
}

// load sound fx
void sound_load_samples()
{
    if (sound_available == FALSE)
        return;

    if (!swap_sfx)
        swap_sfx = Sample_Load(DATADIR "/sounds/gweled/swap.wav");
    if (!swap_sfx)
        g_warning("Could not load swap.wav, reason: %s", MikMod_strerror(MikMod_errno));

    if (!click_sfx)
        click_sfx = Sample_Load(DATADIR "/sounds/gweled/click.wav");
    if (!click_sfx)
        g_warning("Could not load click.wav, reason: %s", MikMod_strerror(MikMod_errno));

    MikMod_SetNumVoices(-1, 2);
}

void sound_unload_samples()
{
    if (swap_sfx) {
		Sample_Free(swap_sfx);
		swap_sfx = NULL;
	}
	if (click_sfx) {
		Sample_Free(click_sfx);
		click_sfx = NULL;
	}
}

// play sound fx
void sound_play_sample(gweled_sound_samples sample)
{
    if (sound_available == FALSE)
        return;

    switch (sample) {
        case CLICK_EVENT:
            if(click_sfx)
                Sample_Play(click_sfx, 0, 0);
            break;
        case SWAP_EVENT:
            if(swap_sfx)
                Sample_Play(swap_sfx, 0, 0);
            break;
    }
}

void sound_destroy()
{
    if (sound_available) {
        sound_music_stop();

        pthread_cancel(thread);
        pthread_join(thread, NULL);

        sound_unload_samples();

        MikMod_DisableOutput();
	    MikMod_Exit();

	    sound_available = FALSE;
	}
}

gboolean sound_get_enabled()
{
    return sound_available;
}
