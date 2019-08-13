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

#ifndef __INSTRUMENT_MODEL_H
#define __INSTRUMENT_MODEL_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "stm32f411e_discovery.h"

#define AUDIO_CHANNELS     2U
#define AUDIO_BUFFER_SIZE  4096U
#define MAX_AMPLITUDE      32760.0f

/* Enum to tracking which buffer section
   has been processed */
typedef enum {
  BUFFER_SECTION_NONE = 0,
  BUFFER_SECTION_FIRST_HALF,
  BUFFER_SECTION_SECOND_HALF
} BufferSection;

typedef enum {
  INSTRUMENT_OK,
  INSTRUMENT_ERROR
} InstrumentStatus;

/* Structure to hold past values for the
   instrument model */
typedef struct {
  int16_t  *mem_ptr;
  uint16_t mem_len;
  int16_t  rw_index;
} ModelMemory;

/* Structure for storing the instrument
   model's properties */
typedef struct {
  int16_t       *audio_ptr;
  uint16_t      buffer_len;
  uint16_t      max_delay;
  ModelMemory   past_vals;
  BufferSection section_done;
} InstrumentModel;

InstrumentStatus instrument_model_init(InstrumentModel *model, uint16_t delay);
InstrumentStatus instrument_model_process(InstrumentModel *model);
InstrumentStatus instrument_model_change(InstrumentModel *model, uint16_t delay);

#endif /* __INSTRUMENT_MODEL_H */
