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

#ifndef __DELAY_LENGTHS_H
#define __DELAY_LENGTHS_H

#define MIDI_NOTE_OFFSET  108U

/* Delay lengths for Karplus-Strong algorithm. Values are sorted so that
   their index is related to the MIDI key number (with some offset). 

   e.g. Pressing A4 on the digital piano will produce a MIDI message with
   key number 69. The delay needed to produce A4 with the Karplus-Strong
   algorithm will be at index (MIDI_NOTE_OFFSET - 69) = 39. */
static const uint16_t note_delay_lengths[88] = {
  10,   11,   11,   12,   13,   14,   14,   15,
  16,   17,   18,   19,   21,   22,   23,   25,
  26,   28,   29,   31,   33,   35,   37,   39,
  42,   44,   47,   50,   53,   56,   59,   63,
  66,   70,   75,   79,   84,   89,   94,   100,
  106,  112,  119,  126,  133,  141,  150,  159,
  168,  178,  189,  200,  212,  225,  238,  252,
  267,  283,  300,  318,  337,  357,  378,  400,
  424,  450,  476,  505,  535,  566,  600,  636,
  674,  714,  756,  801,  849,  900,  953,  1010,
  1070, 1133, 1201, 1272, 1348, 1428, 1513, 1603
};

#endif /* __DELAY_LENGTHS_H */
