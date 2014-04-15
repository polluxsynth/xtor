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

static void
nocturn_cc_receiver(int chan, int controller_no, int value)
{
  if (controller_no == NOCTURN_CC_SPEED_DIAL)
    printf("Receive chan %d, CC %d:%d\n", chan, controller_no, value);
}

void
nocturn_init(struct controller *controller)
{
  /* Tell MIDI handler we want to receive CC. */
  midi_register_cc(CTRLR_PORT, nocturn_cc_receiver);

  controller->remote_midi_device = "Nocturn";
  controller->map_filename = "nocturn.glade";
}

/************************* End of file nocturn.c ****************************/
