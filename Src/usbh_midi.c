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

#include "usbh_midi.h"


static USBH_StatusTypeDef usbh_midi_interface_init(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef usbh_midi_interface_deinit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef usbh_midi_class_request(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef usbh_midi_cs_request(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef usbh_midi_process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef usbh_midi_sof_process(USBH_HandleTypeDef *phost);
static void midi_process_tx(USBH_HandleTypeDef *phost);
static void midi_process_rx(USBH_HandleTypeDef *phost);

USBH_ClassTypeDef midi_class = {
  "MIDI",
  USB_AUDIO_CLASS,
  usbh_midi_interface_init,
  usbh_midi_interface_deinit,
  usbh_midi_class_request,
  usbh_midi_process,
  usbh_midi_sof_process,
  NULL
};


/* Get in/out endpoints of device and open in/out pipes */
static USBH_StatusTypeDef usbh_midi_interface_init(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *midi_handle;
  uint8_t itf_index = 0;
  
  /* MIDIStreaming uses no interface protocol */
  itf_index = USBH_FindInterface(phost, USB_AUDIO_CLASS, USB_MIDISTREAMING_SUBCLASS, 0xFF);

  if (itf_index == 0xFF) {
    USBH_DbgLog("Cannot find the interface for %s class.", phost->pActiveClass->Name);
    return USBH_FAIL;
  } else {
    USBH_SelectInterface(phost, itf_index);

    phost->pActiveClass->pData = USBH_malloc(sizeof(MIDI_HandleTypeDef));
    midi_handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;

    /* Determine if address is for in or out endpoint */
    if (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[0].bEndpointAddress & 0x80) {
      midi_handle->data_itf.in_ep = (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[0].bEndpointAddress);
      midi_handle->data_itf.in_ep_size = phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[0].wMaxPacketSize;
    } else {
      midi_handle->data_itf.out_ep = (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[0].bEndpointAddress);
      midi_handle->data_itf.out_ep_size = phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[0].wMaxPacketSize;
    }

    if (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[1].bEndpointAddress & 0x80) {
      midi_handle->data_itf.in_ep = (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[1].bEndpointAddress);
      midi_handle->data_itf.in_ep_size = phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[1].wMaxPacketSize;
    } else {
      midi_handle->data_itf.out_ep = (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[1].bEndpointAddress);
      midi_handle->data_itf.out_ep_size = phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[1].wMaxPacketSize;
    }

    midi_handle->state = MIDI_IDLE_STATE;

    /* Allocate and open a channel for the endpoints */
    midi_handle->data_itf.in_pipe = USBH_AllocPipe(phost, midi_handle->data_itf.in_ep);
    midi_handle->data_itf.out_pipe = USBH_AllocPipe(phost, midi_handle->data_itf.out_ep);
    USBH_OpenPipe(phost,
                  midi_handle->data_itf.in_pipe,
                  midi_handle->data_itf.in_ep,
                  phost->device.address,
                  phost->device.speed,
                  USB_EP_TYPE_BULK,
                  midi_handle->data_itf.in_ep_size);
    USBH_OpenPipe(phost,
                  midi_handle->data_itf.out_pipe,
                  midi_handle->data_itf.out_ep,
                  phost->device.address,
                  phost->device.speed,
                  USB_EP_TYPE_BULK,
                  midi_handle->data_itf.out_ep_size);

    USBH_LL_SetToggle(phost, midi_handle->data_itf.in_pipe, 0);
    USBH_LL_SetToggle(phost, midi_handle->data_itf.out_ep, 0);
  }

  return USBH_OK;
}


/* Free up space used by channels and midi class handle */
static USBH_StatusTypeDef usbh_midi_interface_deinit(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *midi_handle = phost->pActiveClass->pData;

  if (midi_handle->data_itf.in_pipe) {
    USBH_ClosePipe(phost, midi_handle->data_itf.in_pipe);
    USBH_FreePipe(phost, midi_handle->data_itf.in_pipe);
    midi_handle->data_itf.in_pipe = 0;
  }

  if (midi_handle->data_itf.out_pipe) {
    USBH_ClosePipe(phost, midi_handle->data_itf.out_pipe);
    USBH_FreePipe(phost, midi_handle->data_itf.out_pipe);
    midi_handle->data_itf.out_pipe = 0;
  }

  if (phost->pActiveClass->pData) {
    USBH_free(phost->pActiveClass->pData);
    phost->pActiveClass->pData = 0;
  }

  return USBH_OK;
}

/* Handle standard audio class request or call usbh_midi_cs_request */
static USBH_StatusTypeDef usbh_midi_class_request(USBH_HandleTypeDef *phost) {
  /* TODO: handle standard requests. -_- */
  phost->pUser(phost, HOST_USER_CLASS_ACTIVE);
  return USBH_OK;
}

/* Handle USB-MIDI related requests */
static USBH_StatusTypeDef usbh_midi_cs_request(USBH_HandleTypeDef *phost) {
  /* TODO: handle class-specific requests. -_- */
  /* Must have both set and get functions for supported request.
     Stall control pipe if request not supported */
  return USBH_OK;
}

/* Manage state machine for MIDI data transfers */
static USBH_StatusTypeDef usbh_midi_process(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *midi_handle = phost->pActiveClass->pData;
  USBH_StatusTypeDef status = USBH_BUSY;
  USBH_StatusTypeDef req_status = USBH_OK;

  switch (midi_handle->state) {
    case MIDI_IDLE_STATE:
      status = USBH_OK;
      break;

    case MIDI_TRANSFER_DATA:
      midi_process_tx(phost);
      midi_process_rx(phost);
      break;

    case MIDI_ERROR_STATE:
      req_status = USBH_ClrFeature(phost, 0x00);

      if (req_status == USBH_OK) {
        /* Change state to waiting */
        midi_handle->state = MIDI_IDLE_STATE;
      }
      break;

    default:
      break;
  }

  return status;
}

/* Manage start-of-frame (SOF) callback */
static USBH_StatusTypeDef usbh_midi_sof_process(USBH_HandleTypeDef *phost) {
  return USBH_OK;
}

/* Get the size of the last MIDI reception */
uint16_t usbh_midi_last_rx_size(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *midi_handle = phost->pActiveClass->pData;

  if (phost->gState == HOST_CLASS) {
    return USBH_LL_GetLastXferSize(phost, midi_handle->data_itf.in_pipe);
  } else {
    return 0;
  }
}

/* Stop current MIDI communication */
USBH_StatusTypeDef usbh_midi_stop(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *midi_handle = phost->pActiveClass->pData;

  if (phost->gState == HOST_CLASS) {
    midi_handle->state = MIDI_IDLE_STATE;

    USBH_ClosePipe(phost, midi_handle->data_itf.in_pipe);
    USBH_ClosePipe(phost, midi_handle->data_itf.out_pipe);
  }

  return USBH_OK;
}

/* Notify state machine that data needs to be transmitted */
USBH_StatusTypeDef usbh_midi_transmit(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint32_t length) {
  MIDI_HandleTypeDef *midi_handle = phost->pActiveClass->pData;
  USBH_StatusTypeDef status = USBH_BUSY;

  if ((midi_handle->state == MIDI_IDLE_STATE) || (midi_handle->state == MIDI_TRANSFER_DATA)) {
    midi_handle->tx_data_p = pbuff;
    midi_handle->tx_data_length = length;
    midi_handle->state = MIDI_TRANSFER_DATA;
    midi_handle->tx_data_state = MIDI_SEND_DATA;
    status = USBH_OK;
  }

  return status;
}

/* Notify state machine that data needs to be received */
USBH_StatusTypeDef usbh_midi_receive(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint32_t length) {
  MIDI_HandleTypeDef *midi_handle = phost->pActiveClass->pData;
  USBH_StatusTypeDef status = USBH_BUSY;

  if ((midi_handle->state == MIDI_IDLE_STATE) || (midi_handle->state == MIDI_TRANSFER_DATA)) {
    midi_handle->rx_data_p = pbuff;
    midi_handle->rx_data_length = length;
    midi_handle->state = MIDI_TRANSFER_DATA;
    midi_handle->rx_data_state = MIDI_RECEIVE_DATA;
    status = USBH_OK;
  }

  return status;
}

/* Function for sending MIDI data to device */
static void midi_process_tx(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *midi_handle = phost->pActiveClass->pData;
  USBH_URBStateTypeDef urb_status = USBH_URB_IDLE;

  switch (midi_handle->tx_data_state) {
    case MIDI_SEND_DATA:
      if (midi_handle->tx_data_length > midi_handle->data_itf.out_ep_size) {
        USBH_BulkSendData(phost,
                          midi_handle->tx_data_p,
                          midi_handle->data_itf.out_ep_size,
                          midi_handle->data_itf.out_pipe,
                          1);
      } else {
        USBH_BulkSendData(phost,
                          midi_handle->tx_data_p,
                          midi_handle->tx_data_length,
                          midi_handle->data_itf.out_pipe,
                          1);
      }
      midi_handle->tx_data_state = MIDI_SEND_DATA_WAIT;
      break;

    case MIDI_SEND_DATA_WAIT:
      urb_status = USBH_LL_GetURBState(phost, midi_handle->data_itf.out_pipe);

      /* Wait for data transmission to complete */
      if (urb_status == USBH_URB_DONE) {
        if (midi_handle->tx_data_length > midi_handle->data_itf.out_ep_size) {
          /* If tx data size is larger than the max packet size, send next chunk */
          midi_handle->tx_data_length -= midi_handle->data_itf.out_ep_size;
          midi_handle->tx_data_p += midi_handle->data_itf.out_ep_size;
        } else {
          midi_handle->tx_data_length = 0;
        }

        if (midi_handle->tx_data_length > 0) {
          midi_handle->tx_data_state = MIDI_SEND_DATA;
        } else {
          midi_handle->tx_data_state = MIDI_IDLE_DATA;
          usbh_midi_tx_callback(phost);
        }
      } else if (urb_status == USBH_URB_NOTREADY) {
        midi_handle->tx_data_state = MIDI_SEND_DATA;
      }
      break;

    default:
      break;
  }
}

/* Function for receiving MIDI data from device */
static void midi_process_rx(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *midi_handle = phost->pActiveClass->pData;
  USBH_URBStateTypeDef urb_status = USBH_URB_IDLE;
  uint16_t length;

  switch (midi_handle->rx_data_state) {
    case MIDI_RECEIVE_DATA:
      USBH_BulkReceiveData(phost,
                          midi_handle->rx_data_p,
                          midi_handle->data_itf.in_ep_size,
                          midi_handle->data_itf.in_pipe);

      midi_handle->rx_data_state = MIDI_RECEIVE_DATA_WAIT;
      break;

    case MIDI_RECEIVE_DATA_WAIT:
      urb_status = USBH_LL_GetURBState(phost, midi_handle->data_itf.in_pipe);

      /* Wait for data reception to complete */
      if (urb_status == USBH_URB_DONE) {
        length = USBH_LL_GetLastXferSize(phost, midi_handle->data_itf.in_pipe);

        if (((midi_handle->rx_data_length - length) > 0) && (length > midi_handle->data_itf.in_ep_size)) {
          /* If rx data size is larger than the max packet size, get next chunk */
          midi_handle->rx_data_length -= length;
          midi_handle->rx_data_p += length;
          midi_handle->rx_data_state = MIDI_RECEIVE_DATA;
        } else {
          midi_handle->rx_data_state = MIDI_IDLE_DATA;
          usbh_midi_rx_callback(phost);
        }
      }
      break;

    default:
      break;
  }
}

__weak void usbh_midi_tx_callback(USBH_HandleTypeDef *phost) {
/* Function implemented by user application */
}

__weak void usbh_midi_rx_callback(USBH_HandleTypeDef *phost) {
/* Function implemented by user application */
}
