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


extern USBH_HandleTypeDef hUSB_host;
extern volatile BufferSection section_ready;

uint8_t MIDI_rx_buffer[RX_BUFFER_SIZE];
static InstrumentModel instrument;


/* Initialize instrument model and audio peripheral */
void instrument_player_init(void) {

  uint32_t byte_size;

  if (instrument_model_init(&instrument, 0) != INSTRUMENT_OK) {
    Error_Handler();
  }

  if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, AUDIO_VOLUME, SAMPLE_FREQUENCY) != AUDIO_OK) {
    Error_Handler();
  }

  /* Start playing from instrument's audio buffer */
  byte_size = instrument.buffer_len * sizeof(int16_t);
  if (BSP_AUDIO_OUT_Play((uint16_t*)instrument.audio_ptr, byte_size) != AUDIO_OK) {
    Error_Handler();
  }
}

/* Play the instrument :) */
void instrument_player_play(void) {
  if (instrument_model_process(&instrument) != INSTRUMENT_OK) {
    Error_Handler();
  }
}

 /* Process MIDI packets and change the note according to key pressed
  * on digital piano */
void USBH_MIDI_RxCallback(USBH_HandleTypeDef *phost) {
  MIDI_Packet packet;
  uint8_t *midi_ptr = &MIDI_rx_buffer[0];
  uint16_t num_of_packets;
  uint16_t note_delay;

  num_of_packets = USBH_MIDI_GetLastRxDataSize(phost) / 4;
  while (num_of_packets--) {
    /* Organize bytes in MIDI packet */
    packet.header = *(midi_ptr++);
    packet.byte0 = *(midi_ptr++);
    packet.byte1 = *(midi_ptr++);
    packet.byte2 = *(midi_ptr++);

    /* Check if MIDI packet is a Note On event */
    if ((packet.byte0 & 0xF0) == 0x90) {
      /* Get piano note pressed and corresponding delay length for the
         instrument model */
      note_delay = note_delay_lengths[MIDI_NOTE_OFFSET - packet.byte1];

      if (instrument_model_change(&instrument, note_delay) != INSTRUMENT_OK) {
        Error_Handler();
      }
    }
  }

  /* Refill MIDI RX buffer */
  USBH_MIDI_Receive(phost, &MIDI_rx_buffer[0], RX_BUFFER_SIZE);
}

/* DMA has finished reading first half of audio buffer */
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
  section_ready = BUFFER_SECTION_FIRST_HALF;
}

  /* DMA has finished reading second half of audio buffer */
void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
  uint16_t buffer_bytes = AUDIO_CHANNELS * AUDIO_BUFFER_SIZE;

  section_ready = BUFFER_SECTION_SECOND_HALF;
  BSP_AUDIO_OUT_ChangeBuffer((uint16_t*)instrument.audio_ptr, buffer_bytes);
}
