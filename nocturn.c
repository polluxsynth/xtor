/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * nocturn.c - Interface for Novation Nocturn controller
 *             Assumes the controller is connected via an application
 *             providing a MIDI port called 'Nocturn ...'.
 *
 * Copyright (C) 2014  Ricard Wanderlof <ricard2013@butoba.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ****************************************************************************/

#include <stdio.h>
#include "midi.h"
#include "controller.h"
#include "nocturn.h"

#include "debug.h"

#undef BUTTONS_TO_INCREMENTORS

#define KNOB_ROW 0
#ifdef BUTTONS_TO_INCREMENTORS
#define INCREMENTOR_ROW 0
#else
#define INCREMENTOR_ROW 1
#endif

/* For parameter control, we consider the top button in the two rows to be
 * increment and the bottom button to be decrement. */
#define NOCTURN_BUTTON_ROWS 2
#define NOCTURN_BUTTON_GROUP_SIZE (NOCTURN_CC_BUTTONS / NOCTURN_BUTTON_ROWS)
#define INCREMENT_CC_BUTTON(BUTTON) NOCTURN_CC_BUTTON(BUTTON)
#define DECREMENT_CC_BUTTON(BUTTON) \
        NOCTURN_CC_BUTTON((BUTTON) + NOCTURN_BUTTON_GROUP_SIZE)

static controller_notify_cb notify_ui = NULL;
static void *notify_ref;

static void
nocturn_register_notify_cb(controller_notify_cb cb, void *ref)
{
  notify_ui = cb;
  notify_ref = ref;
}

/* Scale factor for knobs. 
 * For speed dial, every click seems to produce two steps; use same
 * scaling for all */
static int knob_scale = 2;
/* When turning quickly, incrementors output steps larger than 1, but
 * not that much larger, so apply an acceleration factor in this case. */
static int incrementor_acceleration = 10;

/* Perform conversion 7-bit 2's complement (i.e. MIDI 2's complement)
 * to true 2's complement, scaling (knob_scale steps correspond to one
 * UI step) and handle knob acceleration. */
static int
accelerate(int knob, int value)
{
  static knob_accumulator[NOCTURN_CC_INCREMENTORS + 1] = { 0 };

  printf("Accellerate knob %d:%d\n", knob, value);
  if (value & 64) value = value - 128; /* sign extend */
  if (value > 1 || value < -1) value *= incrementor_acceleration;
  knob_accumulator[knob] += value;
  if (knob_accumulator[knob] > -knob_scale &&
      knob_accumulator[knob] < knob_scale) return;
  value = knob_accumulator[knob] / knob_scale;
  knob_accumulator[knob] -= value * knob_scale;

  return value;
}
 
static void
nocturn_cc_receiver(int chan, int controller_no, int value)
{
  int knob = -1;

  /* 'Increment' buttons = top row */
  if (controller_no >= INCREMENT_CC_BUTTON(0) &&
      controller_no < INCREMENT_CC_BUTTON(NOCTURN_BUTTON_GROUP_SIZE) &&
      value == 127) {
    if (notify_ui) notify_ui(controller_no - INCREMENT_CC_BUTTON(0) + 1,
                             INCREMENTOR_ROW, 1, notify_ref);
  /* 'Decrement' buttons = top row */
  } else if (controller_no >= DECREMENT_CC_BUTTON(0) &&
             controller_no < DECREMENT_CC_BUTTON(NOCTURN_BUTTON_GROUP_SIZE) &&
             value == 127) {
    if (notify_ui) notify_ui(controller_no - DECREMENT_CC_BUTTON(0) + 1,
                            INCREMENTOR_ROW, -1, notify_ref);
  /* Speed dial */
  } else if (controller_no == NOCTURN_CC_SPEED_DIAL)
    knob = 0;
  /* Rest of the knobs */
  else if (controller_no >= NOCTURN_CC_INCREMENTOR(0) &&
           controller_no <= NOCTURN_CC_INCREMENTOR(7))
    knob = controller_no - NOCTURN_CC_INCREMENTOR(0) + 1;

  if (knob >= 0) {
    if (notify_ui)
      notify_ui(knob, KNOB_ROW, accelerate(knob, value), notify_ref);
  }
}

void
nocturn_init(struct controller *controller)
{
  /* Tell MIDI handler we want to receive CC. */
  midi_register_cc(CTRLR_PORT, nocturn_cc_receiver);

  controller->controller_register_notify_cb = nocturn_register_notify_cb;

  controller->remote_midi_device = "Nocturn";
  controller->map_filename = "nocturn.glade";
}

/************************* End of file nocturn.c ****************************/
