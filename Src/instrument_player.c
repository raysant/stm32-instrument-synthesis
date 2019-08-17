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

#include "instrument_player.h"
#include "delay_lengths.h"


uint8_t midi_rx_buffer[RX_BUFFER_SIZE];
static InstrumentModel instrument;


/* Initialize instrument model and audio peripheral */
void instrument_player_init(void) {
  uint32_t buffer_size;

  if (instrument_model_init(&instrument, 0) != INSTRUMENT_OK) {
    error_handler();
  }
  if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, AUDIO_VOLUME, SAMPLE_FREQUENCY) != AUDIO_OK) {
    error_handler();
  }

  /* Start playing from instrument's audio buffer */
  buffer_size = instrument.buffer_len * sizeof(int16_t);
  if (BSP_AUDIO_OUT_Play((uint16_t*)instrument.audio_p, buffer_size) != AUDIO_OK) {
    error_handler();
  }
}

/* Play the instrument :) */
void instrument_player_play(void) {
  if (instrument_model_process(&instrument) != INSTRUMENT_OK) {
    error_handler();
  }
}

/* DMA has finished reading first half of audio buffer */
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
  section_ready = BUFFER_SECTION_FIRST_HALF;
}

/* DMA has finished reading second half of audio buffer */
void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
  /* Must divide buffer size by 2 because of the way the ChangeBuffer 
     function works */
  uint16_t buffer_size = instrument.buffer_len * sizeof(int16_t) / 2;

  BSP_AUDIO_OUT_ChangeBuffer((uint16_t*)instrument.audio_p, buffer_size);
  section_ready = BUFFER_SECTION_SECOND_HALF;
}

/* Process MIDI packets and change the note according to key pressed
   on digital piano */
void usbh_midi_rx_callback(USBH_HandleTypeDef *phost) {
  MIDI_Packet *packet_p = (MIDI_Packet*)&midi_rx_buffer[0];
  uint16_t num_of_packets;
  uint16_t note_delay;

  num_of_packets = usbh_midi_last_rx_size(phost) / 4;
  while (num_of_packets--) {
    /* Check if MIDI message is a Note-On event */
    if (GET_CIN(packet_p->header) == NOTE_ON) {
      note_delay = note_delay_lengths[MIDI_NOTE_OFFSET - packet_p->byte2];

      if (instrument_model_change(&instrument, note_delay) != INSTRUMENT_OK) {
        error_handler();
      }
    }

    ++packet_p;
  }
  /* Refill MIDI RX buffer */
  usbh_midi_receive(phost, &midi_rx_buffer[0], RX_BUFFER_SIZE);
}