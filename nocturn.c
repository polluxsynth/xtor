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

static controller_notify_cb notify_ui = NULL;
static void *notify_ref;

static void
nocturn_register_notify_cb(controller_notify_cb cb, void *ref)
{
  notify_ui = cb;
  notify_ref = ref;
}

/* Scale factor for speed dial. Every click seems to produce two steps */
int speed_dial_scale = 2;
/* When turning quickly, incrementors output steps larger than 1, but
 * not that much larger, so apply an accelleration factor in this case. */
int incrementor_acceleration = 10;

static void
nocturn_cc_receiver(int chan, int controller_no, int value)
{
  static speed_dial_accumulator = 0;
  if (controller_no == NOCTURN_CC_SPEED_DIAL) {
    printf("Receive chan %d, CC %d:%d\n", chan, controller_no, value);
    if (value & 64) value = value - 128; /* sign extend */
    if (value > 1 || value < -1) value *= incrementor_acceleration;
    speed_dial_accumulator += value;
    if (speed_dial_accumulator > -speed_dial_scale &&
        speed_dial_accumulator < speed_dial_scale) return;
    value = speed_dial_accumulator / speed_dial_scale;
    if (notify_ui) notify_ui(0, value, notify_ref);
    speed_dial_accumulator -= value * speed_dial_scale;
  } else if (controller_no >= NOCTURN_CC_INCREMENTOR(0) &&
             controller_no <= NOCTURN_CC_INCREMENTOR(7))
  {
    printf("Receive chan %d, CC %d:%d\n", chan, controller_no, value);
    if (value & 64) value = value - 128; /* sign extend */
    if (notify_ui) notify_ui(controller_no - NOCTURN_CC_INCREMENTOR0 + 1,
                            value, notify_ref);
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
