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


#include <pthread.h>
#include <glib.h>

#include "sound.h"
#include "board_engine.h"
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

static pthread_t thread;

static Mix_Music *module;
static Mix_Chunk *swap_sfx = NULL;
static Mix_Chunk *click_sfx = NULL;

static gboolean is_playing;
static gboolean sound_available;

static int audio_rate = 44100;
static uint16_t audio_format = MIX_DEFAULT_FORMAT; /* 16-bit stereo */
static int audio_channels = MIX_DEFAULT_CHANNELS;
static int audio_buffers = 1024;


void sound_init()
{
    SDL_Init(SDL_INIT_AUDIO);
    Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers);
    sound_available = TRUE;

    /* register all the drivers */
    //MikMod_RegisterDriver(&drv_esd);
   // MikMod_RegisterDriver(&drv_alsa);
    //MikMod_RegisterDriver(&drv_oss);
   // MikMod_RegisterDriver(&drv_nos);

    /* register only the s3m module loader */
   // MikMod_RegisterLoader(&load_s3m);

    /* initialize the library */
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        sound_available = FALSE;
    } else {
        if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) >= 0)
            sound_available = TRUE;
        else
            sound_available = FALSE;
    }
    if (!sound_available)
        g_printerr("Could not initialize sound, reason: %s\n", Mix_GetError());

    is_playing = FALSE;
}


void sound_music_play()
{
	if (!is_playing && sound_available) {
	    module = Mix_LoadMUS(DATADIR "/sounds/gweled/autonom.s3m");
    	if (module) {

    	    Mix_PlayMusic(module,-1);
			is_playing = TRUE;

    	} else
    	    g_printerr("Could not load module, reason: %s\n", Mix_GetError());
	}
}

void sound_music_stop()
{
	if (is_playing && sound_available) {
    	if (module)
		{
		    Mix_HaltMusic();
		    Mix_FreeMusic(module);
	    	//Player_Stop();
	    	//Player_Free(module);
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
        swap_sfx = Mix_LoadWAV(DATADIR "/sounds/gweled/swap.wav");
    if (!swap_sfx)
        g_warning("Could not load swap.wav, reason: %s", Mix_GetError());

    if (!click_sfx)
        click_sfx = Mix_LoadWAV(DATADIR "/sounds/gweled/click.wav");
    if (!click_sfx)
        g_warning("Could not load click.wav, reason: %s", Mix_GetError());

}

void sound_unload_samples()
{
    if (swap_sfx) {
		Mix_FreeChunk(swap_sfx);
		swap_sfx = NULL;
	}
	if (click_sfx) {
		Mix_FreeChunk(click_sfx);
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
                Mix_PlayChannel(-1,click_sfx, 0);
            break;
        case SWAP_EVENT:
            if(swap_sfx)
                Mix_PlayChannel(-1,swap_sfx, 0);
            break;
    }
}

void sound_destroy()
{
    if (sound_available) {
        sound_music_stop();

        sound_unload_samples();

        Mix_CloseAudio();

	    sound_available = FALSE;
	}
}

gboolean sound_get_enabled()
{
    return sound_available;
}
