/*
 * Copyright (C) 2019 Ray Santana
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __INSTRUMENT_PLAYER_H
#define __INSTRUMENT_PLAYER_H

#include "usbh_core.h"
#include "usbh_midi.h"
#include "stm32f411e_discovery_audio.h"
#include "instrument_model.h"

#define AUDIO_VOLUME      70U
#define SAMPLE_FREQUENCY  44100U
#define RX_BUFFER_SIZE    64U

extern void Error_Handler(void);

void instrument_player_init(void);
void instrument_player_play(void);

#endif /* __INSTRUMENT_PLAYER_H */