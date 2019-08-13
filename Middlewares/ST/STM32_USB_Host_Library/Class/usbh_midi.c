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


static USBH_StatusTypeDef USBH_MIDI_InterfaceInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_CSRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost);
static void MIDI_ProcessTransmission(USBH_HandleTypeDef *phost);
static void MIDI_ProcessReception(USBH_HandleTypeDef *phost);

USBH_ClassTypeDef MIDI_class = {
  "MIDI",
  USB_AUDIO_CLASS,
  USBH_MIDI_InterfaceInit,
  USBH_MIDI_InterfaceDeInit,
  USBH_MIDI_ClassRequest,
  USBH_MIDI_Process,
  USBH_MIDI_SOFProcess,
  NULL
};


static USBH_StatusTypeDef USBH_MIDI_InterfaceInit(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *MIDI_handle;
  USBH_StatusTypeDef status = USBH_FAIL;
  uint8_t itf_index = 0;
  
  itf_index = USBH_FindInterface(phost, USB_AUDIO_CLASS, USB_MIDISTREAMING_SUBCLASS, 0xFF);

  if (itf_index == 0xFF) {
    USBH_DbgLog("Cannot find the interface for %s class.", phost->pActiveClass->Name);
    status = USBH_FAIL;
  } else {
    USBH_SelectInterface(phost, itf_index);

    phost->pActiveClass->pData = USBH_malloc(sizeof(MIDI_HandleTypeDef));
    MIDI_handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;

    if (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[0].bEndpointAddress & 0x80) {
      MIDI_handle->data_itf.in_ep = (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[0].bEndpointAddress);
      MIDI_handle->data_itf.in_ep_size = phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[0].wMaxPacketSize;
    } else {
      MIDI_handle->data_itf.out_ep = (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[0].bEndpointAddress);
      MIDI_handle->data_itf.out_ep_size = phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[0].wMaxPacketSize;
    }

    if (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[1].bEndpointAddress & 0x80) {
      MIDI_handle->data_itf.in_ep = (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[1].bEndpointAddress);
      MIDI_handle->data_itf.in_ep_size = phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[1].wMaxPacketSize;
    } else {
      MIDI_handle->data_itf.out_ep = (phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[1].bEndpointAddress);
      MIDI_handle->data_itf.out_ep_size = phost->device.CfgDesc.Itf_Desc[itf_index].Ep_Desc[1].wMaxPacketSize;
    }

    MIDI_handle->state = MIDI_IDLE_STATE;

    MIDI_handle->data_itf.in_pipe = USBH_AllocPipe(phost, MIDI_handle->data_itf.in_ep);
    MIDI_handle->data_itf.out_pipe = USBH_AllocPipe(phost, MIDI_handle->data_itf.out_ep);

    USBH_OpenPipe(phost,
                  MIDI_handle->data_itf.in_pipe,
                  MIDI_handle->data_itf.in_ep,
                  phost->device.address,
                  phost->device.speed,
                  USB_EP_TYPE_BULK,
                  MIDI_handle->data_itf.in_ep_size);

    USBH_OpenPipe(phost,
                  MIDI_handle->data_itf.out_pipe,
                  MIDI_handle->data_itf.out_ep,
                  phost->device.address,
                  phost->device.speed,
                  USB_EP_TYPE_BULK,
                  MIDI_handle->data_itf.out_ep_size);

    USBH_LL_SetToggle(phost, MIDI_handle->data_itf.in_pipe, 0);
    USBH_LL_SetToggle(phost, MIDI_handle->data_itf.out_ep, 0);
    status = USBH_OK;
  }

  return status;
}

static USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *MIDI_handle = phost->pActiveClass->pData;

  if (MIDI_handle->data_itf.in_pipe) {
    USBH_ClosePipe(phost, MIDI_handle->data_itf.in_pipe);
    USBH_FreePipe(phost, MIDI_handle->data_itf.in_pipe);
    MIDI_handle->data_itf.in_pipe = 0;
  }

  if (MIDI_handle->data_itf.out_pipe) {
    USBH_ClosePipe(phost, MIDI_handle->data_itf.out_pipe);
    USBH_FreePipe(phost, MIDI_handle->data_itf.out_pipe);
    MIDI_handle->data_itf.out_pipe = 0;
  }

  if (phost->pActiveClass->pData) {
    USBH_free(phost->pActiveClass->pData);
    phost->pActiveClass->pData = 0;
  }

  return USBH_OK;
}

static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost) {
  /* TODO: handle standard requests. */
  phost->pUser(phost, HOST_USER_CLASS_ACTIVE);

  return USBH_OK;
}

static USBH_StatusTypeDef USBH_MIDI_CSRequest(USBH_HandleTypeDef *phost) {
  /* TODO: handle class-specific requests. */
  return USBH_OK;
}

static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *MIDI_handle = phost->pActiveClass->pData;
  USBH_StatusTypeDef status = USBH_BUSY;
  USBH_StatusTypeDef req_status = USBH_OK;

  switch (MIDI_handle->state) {
    case MIDI_IDLE_STATE:
      status = USBH_OK;
      break;

    case MIDI_TRANSFER_DATA:
      MIDI_ProcessTransmission(phost);
      MIDI_ProcessReception(phost);
      break;

    case MIDI_ERROR_STATE:
      req_status = USBH_ClrFeature(phost, 0x00);

      if (req_status == USBH_OK) {
        MIDI_handle->state = MIDI_IDLE_STATE;
      }
      break;

    default:
      break;
  }
  
  return status;
}

static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost) {
  return USBH_OK;
}

uint16_t USBH_MIDI_GetLastRxDataSize(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *MIDI_handle = phost->pActiveClass->pData;

  if (phost->gState == HOST_CLASS) {
    return USBH_LL_GetLastXferSize(phost, MIDI_handle->data_itf.in_pipe);
  } else {
    return 0;
  }
}

USBH_StatusTypeDef USBH_MIDI_Stop(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *MIDI_handle = phost->pActiveClass->pData;

  if (phost->gState == HOST_CLASS) {
    MIDI_handle->state = MIDI_IDLE_STATE;

    USBH_ClosePipe(phost, MIDI_handle->data_itf.in_pipe);
    USBH_ClosePipe(phost, MIDI_handle->data_itf.out_pipe);
  }

  return USBH_OK;
}

USBH_StatusTypeDef USBH_MIDI_Transmit(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint32_t length) {
  MIDI_HandleTypeDef *MIDI_handle = phost->pActiveClass->pData;
  USBH_StatusTypeDef status = USBH_BUSY;

  if ((MIDI_handle->state == MIDI_IDLE_STATE) || (MIDI_handle->state == MIDI_TRANSFER_DATA)) {
    MIDI_handle->ptr_tx_data = pbuff;
    MIDI_handle->tx_data_length = length;
    MIDI_handle->state = MIDI_TRANSFER_DATA;
    MIDI_handle->tx_data_state = MIDI_SEND_DATA;
    status = USBH_OK;
  }

  return status;
}

USBH_StatusTypeDef USBH_MIDI_Receive(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint32_t length) {
  MIDI_HandleTypeDef *MIDI_handle = phost->pActiveClass->pData;
  USBH_StatusTypeDef status = USBH_BUSY;

  if ((MIDI_handle->state == MIDI_IDLE_STATE) || (MIDI_handle->state == MIDI_TRANSFER_DATA)) {
    MIDI_handle->ptr_rx_data = pbuff;
    MIDI_handle->rx_data_length = length;
    MIDI_handle->state = MIDI_TRANSFER_DATA;
    MIDI_handle->rx_data_state = MIDI_RECEIVE_DATA;
    status = USBH_OK;
  }

  return status;
}

static void MIDI_ProcessTransmission(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *MIDI_handle = phost->pActiveClass->pData;
  USBH_URBStateTypeDef URB_status = USBH_URB_IDLE;

  switch (MIDI_handle->tx_data_state) {
    case MIDI_SEND_DATA:
      if (MIDI_handle->tx_data_length > MIDI_handle->data_itf.out_ep_size) {
        USBH_BulkSendData(phost,
                          MIDI_handle->ptr_tx_data,
                          MIDI_handle->data_itf.out_ep_size,
                          MIDI_handle->data_itf.out_pipe,
                          1);
      } else {
        USBH_BulkSendData(phost,
                          MIDI_handle->ptr_tx_data,
                          MIDI_handle->tx_data_length,
                          MIDI_handle->data_itf.out_pipe,
                          1);
      }

      MIDI_handle->tx_data_state = MIDI_SEND_DATA_WAIT;
      break;

    case MIDI_SEND_DATA_WAIT:
      URB_status = USBH_LL_GetURBState(phost, MIDI_handle->data_itf.out_pipe);

      if (URB_status == USBH_URB_DONE) {
        if (MIDI_handle->tx_data_length > MIDI_handle->data_itf.out_ep_size) {
          MIDI_handle->tx_data_length -= MIDI_handle->data_itf.out_ep_size;
          MIDI_handle->ptr_tx_data += MIDI_handle->data_itf.out_ep_size;
        } else {
          MIDI_handle->tx_data_length = 0;
        }

        if (MIDI_handle->tx_data_length > 0) {
          MIDI_handle->tx_data_state = MIDI_SEND_DATA;
        } else {
          MIDI_handle->tx_data_state = MIDI_IDLE_DATA;
          USBH_MIDI_TxCallback(phost);
        }
      } else if (URB_status == USBH_URB_NOTREADY) {
        MIDI_handle->tx_data_state = MIDI_SEND_DATA;
      }
      break;

    default:
      break;
  }
}

static void MIDI_ProcessReception(USBH_HandleTypeDef *phost) {
  MIDI_HandleTypeDef *MIDI_handle = phost->pActiveClass->pData;
  USBH_URBStateTypeDef URB_status = USBH_URB_IDLE;
  uint16_t length;

  switch (MIDI_handle->rx_data_state) {
    case MIDI_RECEIVE_DATA:
      USBH_BulkReceiveData(phost,
                          MIDI_handle->ptr_rx_data,
                          MIDI_handle->data_itf.in_ep_size,
                          MIDI_handle->data_itf.in_pipe);

      MIDI_handle->rx_data_state = MIDI_RECEIVE_DATA_WAIT;
      break;

    case MIDI_RECEIVE_DATA_WAIT:
      URB_status = USBH_LL_GetURBState(phost, MIDI_handle->data_itf.in_pipe);

      if (URB_status == USBH_URB_DONE) {
        length = USBH_LL_GetLastXferSize(phost, MIDI_handle->data_itf.in_pipe);

        if (((MIDI_handle->rx_data_length - length) > 0) && (length > MIDI_handle->data_itf.in_ep_size)) {
          MIDI_handle->rx_data_length -= length;
          MIDI_handle->ptr_rx_data += length;
          MIDI_handle->rx_data_state = MIDI_RECEIVE_DATA;
        } else {
          MIDI_handle->rx_data_state = MIDI_IDLE_DATA;
          USBH_MIDI_RxCallback(phost);
        }
      }
      break;

    default:
      break;
  }
}

__weak void USBH_MIDI_TxCallback(USBH_HandleTypeDef *phost) {

}

__weak void USBH_MIDI_RxCallback(USBH_HandleTypeDef *phost) {

}
