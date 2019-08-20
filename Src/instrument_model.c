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

#include "instrument_model.h"


volatile BufferSection section_ready = BUFFER_SECTION_NONE;
static int16_t audio_buffer[AUDIO_CHANNELS * AUDIO_BUFFER_SIZE];
static int16_t mem_buffer[AUDIO_BUFFER_SIZE / 2];


/* Check if instrument model handle is valid then initialize values */
InstrumentStatus instrument_model_init(InstrumentModel *model, uint16_t delay) {
  if (model == NULL) {
    return INSTRUMENT_ERROR;
  }

  /* Initialize values for instrument */
  model->audio_p = &audio_buffer[0];
  model->buffer_len = sizeof(audio_buffer) / sizeof(int16_t);
  model->max_delay = delay;
  model->section_done = BUFFER_SECTION_NONE;

  /* Initialize values for model's memory (circular) buffer */
  model->memory.mem_p = &mem_buffer[0];
  model->memory.mem_len = sizeof(mem_buffer) / sizeof(int16_t);
  model->memory.rw_index = 0;

  return INSTRUMENT_OK;
}

/* Generate an excitation signal for the instrument and store it
   in the instrument's memory */
__STATIC_INLINE void instrument_model_excite(InstrumentModel *model, uint32_t delay) {
  uint32_t index_limit = model->memory.mem_len - 1;
  float rand_num = 0.0f;

  while (delay--) {
    rand_num = 2.0f * MAX_AMPLITUDE * ((float)rand() / (float)(RAND_MAX)) - 1.0f;

    /* Start from the last read/write position and watch for 
       when the buffer wraps around */
    model->memory.mem_p[model->memory.rw_index] = rand_num;
    model->memory.rw_index = (model->memory.rw_index + 1) & index_limit;
  }
}

/* Use the LPF for the Karplus-Strong algorithm and store the result into
   the current position in the audio buffer */
__STATIC_INLINE void instrument_model_filter(InstrumentModel *model, int16_t *pbuffer) {
  int16_t *mem_p = model->memory.mem_p;
  uint32_t index_limit = model->memory.mem_len - 1;
  uint32_t delay_index;
  float past_val1;
  float past_val2;
  int16_t result;

  /* y[n] = 0.5 * ( y[n-D] + y[n-D-1] ) */
  delay_index = model->memory.rw_index - model->max_delay;
  past_val1 = (float)mem_p[delay_index & index_limit];
  past_val2 = (float)mem_p[(delay_index-1) & index_limit];
  result = (int16_t)(0.5f * (past_val1 + past_val2));

  *pbuffer = result;
#if (AUDIO_CHANNELS == 2)
  *(pbuffer + 1) = result;
#endif

  /* Store result in the memory and update the read/write index */
  mem_p[model->memory.rw_index] = result;
  model->memory.rw_index = (model->memory.rw_index + 1) & index_limit;
}

/* Check if the current buffer section needs to be processed */
InstrumentStatus instrument_model_process(InstrumentModel *model) {
  BufferSection section_ready_cpy = section_ready;
  int16_t *buffer_p = NULL;
  uint32_t loop_count = 0;

  if (model == NULL) {
    return INSTRUMENT_ERROR;
  }

  if (model->section_done != section_ready_cpy) {
    loop_count = AUDIO_BUFFER_SIZE / 2;

    if (section_ready_cpy == BUFFER_SECTION_FIRST_HALF) {
      /* Start at the beginning of the first buffer section */
      buffer_p = model->audio_p;
    } else {
      /* Start at the beginning of the second buffer section */
      buffer_p = model->audio_p + (AUDIO_CHANNELS * AUDIO_BUFFER_SIZE / 2);
    }

    while (loop_count--) {
      /* Apply the filter to the current buffer section */
      instrument_model_filter(model, buffer_p);
      buffer_p += AUDIO_CHANNELS;
    }
    model->section_done = section_ready_cpy;
  }

  return INSTRUMENT_OK;
}

/* Update the instrument model's properties */
InstrumentStatus instrument_model_change(InstrumentModel *model, uint16_t delay) {
  if (model == NULL) {
    return INSTRUMENT_ERROR;
  }

  model->max_delay = delay;
  model->section_done = BUFFER_SECTION_NONE;

  /* Store the excitation signal into the memory buffer */
  instrument_model_excite(model, delay);

  return INSTRUMENT_OK;
}
