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

#ifndef __USBH_MIDI_H
#define __USBH_MIDI_H

#include "usbh_core.h"

#define USB_AUDIO_CLASS               0x01
#define USB_MIDISTREAMING_SUBCLASS    0x03
#define USBH_MIDI_CLASS               &MIDI_class

extern USBH_ClassTypeDef MIDI_class;

/* MIDI Code Index Numbers (CIN) */
typedef enum {
  MISCELLANEOUS = 0,
  CABLE_EVENTS,
  TWO_BYTE_MSG,
  THREE_BYTE_MSG,
  SYS_EX_START,
  SYS_EX_END_ONE_BYTE,
  SYS_EX_END_TWO_BYTES,
  SYS_EX_END_THREE_BYTES,
  NOTE_OFF,
  NOTE_ON,
  POLY_KEY_PRESS,
  CONTROL_CHANGE,
  PROGRAM_CHANGE,
  CHANNEL_PRESSURE,
  PITCH_BEND,
  SINGLE_BYTE_MSG
} MIDI_EventTypeDef;

/* Basic MIDI packet contents. A generic struct for
   MIDI messages, and another struct specifically for 
   NoteOn/NoteOff events */
typedef union {
  struct {
    uint8_t header;
    uint8_t byte0;
    uint8_t byte1;
    uint8_t byte2;
  };
  /* TODO: Fix issue with endianness */
  struct {
    uint8_t cin:4;
    uint8_t cn:4;
    uint8_t channel:4;
    uint8_t event:4;
    uint8_t key;
    uint8_t velocity;
  };
} MIDI_Packet;

/* States for MIDI host state machine */
typedef enum {
  MIDI_IDLE_STATE = 0,
  MIDI_TRANSFER_DATA,
  MIDI_ERROR_STATE
} MIDI_StateTypeDef;

/* States for MIDI data state machine */
typedef enum {
  MIDI_IDLE_DATA = 0,
  MIDI_SEND_DATA,
  MIDI_SEND_DATA_WAIT,
  MIDI_RECEIVE_DATA,
  MIDI_RECEIVE_DATA_WAIT
} MIDI_DataStateTypeDef;

/* States for standard and MIDI class requests */
typedef enum {
  MIDI_REQ_INIT = 0,
  MIDI_REQ_IDLE
} MIDI_ReqStateTypeDef;

/* Structure for storing endpoints and pipes */
typedef struct {
  uint8_t  in_pipe;
  uint8_t  out_pipe;
  uint8_t  in_ep;
  uint8_t  out_ep;
  uint16_t in_ep_size;
  uint16_t out_ep_size;
} MIDI_DataItfTypeDef;

/* Structure for MIDI process */
typedef struct {
  MIDI_DataItfTypeDef   data_itf;
  MIDI_StateTypeDef     state;
  MIDI_ReqStateTypeDef  req_state;
  uint8_t               *ptr_tx_data;
  uint8_t               *ptr_rx_data;
  uint16_t              tx_data_length;
  uint16_t              rx_data_length;
  MIDI_DataStateTypeDef tx_data_state;
  MIDI_DataStateTypeDef rx_data_state;
} MIDI_HandleTypeDef;

uint16_t USBH_MIDI_GetLastRxDataSize(USBH_HandleTypeDef *phost);

USBH_StatusTypeDef USBH_MIDI_Stop(USBH_HandleTypeDef *phost);

USBH_StatusTypeDef USBH_MIDI_Transmit(USBH_HandleTypeDef *phost,
                                uint8_t *pbuff,
                                uint32_t length);

USBH_StatusTypeDef USBH_MIDI_Receive(USBH_HandleTypeDef *phost,
                                uint8_t *pbuff,
                                uint32_t length);

void USBH_MIDI_TxCallback(USBH_HandleTypeDef *phost);

void USBH_MIDI_TxCallback(USBH_HandleTypeDef *phost);

#endif /* __USBH_MIDI_H */
