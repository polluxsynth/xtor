/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * beatstep.c - Interface for Arturia Beatstep controller
 *
 * Copyright (C) 2015  Ricard Wanderlof <ricard2015@butoba.net>
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
#include "beatstep.h"

#include "debug.h"

/* UI definitions */
#define INCREMENTOR_ROW 0
#define BUTTON_ROW 1

#define TOP_INCREMENTOR_ROW 0
#define BOTTOM_INCREMENTOR_ROW 8

#define TOP_BUTTON_ROW 0
#define BOTTOM_BUTTON_ROW 8

/* For parameter control, we consider the top button in the two rows to be
 * increment and the bottom button to be decrement. */
#define BEATSTEP_BUTTON_ROWS 2
#define BEATSTEP_BUTTON_GROUP_SIZE (BEATSTEP_BUTTONS / BEATSTEP_BUTTON_ROWS)
#define INCREMENT_BUTTON(BUTTON) BEATSTEP_BUTTON(BUTTON)
#define DECREMENT_BUTTON(BUTTON) \
        BEATSTEP_BUTTON((BUTTON) + BEATSTEP_BUTTON_GROUP_SIZE)

static controller_notify_cb notify_ui = NULL;
static void *notify_ref;
#define NOTIFY_UI if (notify_ui) notify_ui

static controller_jump_button_cb jump_button_ui = NULL;
static void *jump_button_ref;
#define JUMP_BUTTON_UI if (jump_button_ui) jump_button_ui

static void
beatstep_send_sysex(int request, int function, int control, int value)
{
  unsigned char sndr[] = { SYSEX,
                           0x00, 0x20, 0x6B, /* Arturia sysex ID */
                           0x7f, 0x42, /* Dev.ID, Beatstep ? */
                           request ? 0x01 : 0x02, 0x00,
                           function, control, request ? EOX : value, EOX };

  midi_send_sysex(CTRLR_PORT, sndr, sizeof(sndr) - request);
  /* Don't really like this at all, but apparently some form of delay is
   * needed to avoid message overruns. Since the underlying
   * snd_seq_event_output_direct() call copies the event and data to a local
   * structure before its write() call to the sequencer device, there shouldn't
   * be any overruns on the top level. Question is if the delay is needed
   * for the Beatstep or if has something to do with the MIDI (and/or USB)
   * stack in Linux. In the latter case, one would think that there would be
   * some form of functionality to check that it was ok to send more data,
   * but I haven't found any such functin.
   * Empirically, 100us seems to be enough, but we put in a larger delay to
   * be on the safe side. */
  usleep(1000);
}

#define beatstep_send_setting(function, control, value) \
        beatstep_send_sysex(0, function, control, value)
#define beatstep_get_setting(function, control) \
        beatstep_send_sysex(1, function, control, 0)

static void
beatstep_set_relative(int beatstep_control, int control_no)
{
  beatstep_send_setting(BEATSTEP_KNOB_MODE_SET, beatstep_control,
                        BEATSTEP_KNOB_MODE_CC);
  beatstep_send_setting(BEATSTEP_KNOB_MODE_CC_CHANNEL, beatstep_control,
                        0);
  beatstep_send_setting(BEATSTEP_KNOB_MODE_CC_CONTROL_NO, beatstep_control,
                        control_no);
  beatstep_send_setting(BEATSTEP_KNOB_MODE_CC_BEHAVIOR, beatstep_control,
                        BEATSTEP_KNOB_MODE_CC_BEHAVIOR_RELATIVE1);
}

static void
beatstep_set_ccbutton(int beatstep_control, int control_no)
{
  beatstep_send_setting(BEATSTEP_BUTTON_MODE_SET, beatstep_control,
                        BEATSTEP_BUTTON_MODE_CC);
  beatstep_send_setting(BEATSTEP_BUTTON_MODE_CC_CHANNEL, beatstep_control,
                        0);
  beatstep_send_setting(BEATSTEP_BUTTON_MODE_CC_CONTROL_NO, beatstep_control,
                        control_no);
  beatstep_send_setting(BEATSTEP_BUTTON_MODE_CC_OFF, beatstep_control,
                        0);
  /* We set 1 for on, not 127. No special reason. Just because we can. */
  beatstep_send_setting(BEATSTEP_BUTTON_MODE_CC_ON, beatstep_control,
                        1);
  beatstep_send_setting(BEATSTEP_BUTTON_MODE_CC_BEHAVIOR, beatstep_control,
                        BEATSTEP_BUTTON_MODE_CC_BEHAVIOR_GATE);
}

static void
beatstep_controls_init(void)
{
  int knob, button;

  /* We need to configure the Beatstep for our purposes, so we set up all
   * knobs and buttons to generate CC. We could set up any CC values, but for
   * simplicity we use the same CC value as the Beatstep control number
   * (i.e. 0x20 for the first knob, 0x30 for the volume dial, etc) */
  for (knob = 0; knob < BEATSTEP_INCREMENTORS; knob++)
    beatstep_set_relative(BEATSTEP_INCREMENTOR(knob),
                          BEATSTEP_INCREMENTOR(knob));

  beatstep_set_relative(BEATSTEP_VOLUME_DIAL,
                        BEATSTEP_VOLUME_DIAL);

  for (button = 0; button < BEATSTEP_BUTTONS; button++)
    beatstep_set_ccbutton(BEATSTEP_BUTTON(button),
                          BEATSTEP_BUTTON(button));
  /* We will use the SHIFT button for page jump shift. */
  beatstep_set_ccbutton(BEATSTEP_BUTTON_SHIFT, BEATSTEP_BUTTON_SHIFT);
  /* Set knob acceleration to medium */
  beatstep_send_setting(BEATSTEP_GLOBAL_CTRLR_SETTING,
                        BEATSTEP_GLOBAL_CTRLR_SETTING_ACCELERATION,
                        BEATSTEP_GLOBAL_CTRLR_SETTING_ACCELERATION_FAST);
}

static void
beatstep_register_notify_cb(controller_notify_cb cb, void *ref)
{
  notify_ui = cb;
  notify_ref = ref;
}

static void
beatstep_register_jump_button_cb(controller_jump_button_cb cb, void *ref)
{
  jump_button_ui = cb;
  jump_button_ref = ref;
}

static void
beatstep_cc_receiver(int chan, int controller_no, int value)
{
  static int shifted = 0; /* set to shift_state when shift = start ? pressed  */
  int knob = -1, alt_knob = -1, jump_button = -1;
  dprintf("Got CC(ch %d) %d:%d\n", chan, controller_no, value);

  /* Volume dial */
  if (controller_no == BEATSTEP_VOLUME_DIAL)
    knob = 0;
  /* Rest of the knobs */
  else if (controller_no >= BEATSTEP_INCREMENTOR(0) &&
             controller_no < BEATSTEP_INCREMENTOR(BEATSTEP_INCREMENTORS))
      knob = controller_no - BEATSTEP_INCREMENTOR(0) + 1;
  /* buttons */
  else if (controller_no >= BEATSTEP_BUTTON(0) &&
           controller_no < BEATSTEP_BUTTON(BEATSTEP_BUTTONS) && value)
     jump_button = controller_no - BEATSTEP_BUTTON(0) + 1;
  else if (controller_no == BEATSTEP_BUTTON_SHIFT && value) /* SHIFT pressed */
    shifted ^= 1; /* pressing shift twice cancels it */

  /* Handle events percolated from above */
  if (knob >= 0) { /* volume dial or other knob turned */
    int row = knob >= BOTTOM_INCREMENTOR_ROW + 1 ? BUTTON_ROW : INCREMENTOR_ROW;
    if (row == BUTTON_ROW) {
      /* Alternative interpretation: all 16 knobs map to up to 16 variable
       * parameters, rather than 8 variable and 8 stepped. */
      alt_knob = knob;
      knob -= BOTTOM_INCREMENTOR_ROW;
    }
    NOTIFY_UI(knob, alt_knob, row, value-64, notify_ref);
  } else if (jump_button >= 0) {
    /* Jump button matrix conceptually 4 rows (8x4), with bottom 2 rows
     * intended for page jumps. For the Beatstep, pressing STOP (= shift)
     * allows us to reach the two bottom rows of the four. */
    int bottom_row = jump_button >= BOTTOM_BUTTON_ROW + 1;
    if (bottom_row) jump_button -= BOTTOM_BUTTON_ROW;
    JUMP_BUTTON_UI(1 + (bottom_row ? 1 : 0) + (shifted ? 2 : 0),
                   jump_button, jump_button_ref);
    shifted = 0;
  }
}

static void beatstep_midi_init(struct controller *controller)
{
  /* Tell MIDI handler we want to receive CC. */
  midi_connect(CTRLR_PORT, controller->remote_midi_device);
  midi_register_cc(CTRLR_PORT, beatstep_cc_receiver);
  beatstep_controls_init();
}

void
beatstep_init(struct controller *controller)
{
  controller->controller_register_notify_cb =
    beatstep_register_notify_cb;
  controller->controller_register_jump_button_cb = 
    beatstep_register_jump_button_cb;
  controller->controller_midi_init = beatstep_midi_init;

  controller->remote_midi_device = "Arturia BeatStep";
  controller->map_filename = "beatstep.glade"; /* not used */
}

/************************* End of file beatstep.c ****************************/
