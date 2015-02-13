/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * nocturn.h - Definitions for Novation Nocturn
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

#ifndef _NOCTURN_H_
#define _NOCTURN_H_

#include "controller.h"

/*
 * CC definitions for Nocturn:
 * Note that since this isn't really MIDI, some of the CC's overlap with
 * MIDI mode messages (i.e. CC 124..127).
 *
 * From Nocturn:
 * CC64..71: Incrementors 1..8: Value 1 => increase, value 127 => decrease
 * (If more than one increase/decrease per message interval, rare, 
 * then we get 2,3,4 or 126,125,124, etc.)
 * CC72: slider (7 bits)
 * CC73: slider ? (Arbitrarily 0 or 64 when moving slider)
 * CC74: Speed dial incrementor
 * CC81: Speed dial push (0 = up, 127 = down)
 * CC96..103: Incrementor push/touch (0 = up, 127 = down)
 *            (These tend to be touchy/glitchy)
 * CC112..127: Buttons 1..8 upper row, 1..8 lower row (0 = up, 127 = down)
 *
 * To Nocturn:
 * CC64..71 Incrementor LED ring value 0..127
 * CC72..79: LED ring mode: (values are in high nybble of value byte)
 *           0 = show ring from min to value
 *           16 = show ring from max to value
 *           32 = show ring from 0 (center) to value (up or down)
 *           48 = show ring from 0 (center) to value (both directions)
 *           64 = single diode at value
 *           80 = inverted, i.e. all but single diode at value
 * CC80     Speed dial incrementor LED ring value 0..127
 * CC81     Speed dial LED ring mode (see above)
 * CC112..127: Button LEDs: 0 = off, != 0 = on
 */

#define NOCTURN_CC_INCREMENTORS 8
#define NOCTURN_CC_BUTTONS 16

/* From Nocturn */
#define NOCTURN_CC_SLIDER_MSB			72
#define NOCTURN_CC_SLIDER_LSB			73
#define NOCTURN_CC_INCREMENTOR0			64
#define NOCTURN_CC_INCREMENTOR(INCREMENTOR) \
	(NOCTURN_CC_INCREMENTOR0 + (INCREMENTOR))
#define NOCTURN_CC_SPEED_DIAL			74
#define NOCTURN_CC_SPEED_DIAL_PUSH		81
#define NOCTURN_CC_INCREMENTOR0_PUSH		96
#define NOCTURN_CC_INCREMENTOR_PUSH(INCREMENTOR) \
	(NOCTURN_CC_INCREMENTOR0_PUSH + (INCREMENTOR))
#define NOCTURN_CC_BUTTON0			112
#define NOCTURN_CC_BUTTON(BUTTON) \
	(NOCTURN_CC_BUTTON0 + (BUTTON))

/* To Nocturn */
#define NOCTURN_CC_INCREMENTOR0_LED_VALUE	64
#define NOCTURN_CC_INCREMENTOR_LED_VALUE(INCREMENTOR) \
	(NOCTURN_CC_INCREMENTOR0_LED_VALUE + (INCREMENTOR))
#define NOCTURN_CC_INCREMENTOR0_LED_MODE	72
#define NOCTURN_CC_INCREMENTOR_LED_MODE(INCREMENTOR) \
	(NOCTURN_CC_INCREMENTOR0_LED_MODE + (INCREMENTOR))
#define NOCTURN_CC_SPEED_DIAL_LED_VALUE		80
#define NOCTURN_CC_SPEED_DIAL_LED_MODE		81
#define NOCTURN_CC_BUTTON0_LED			112
#define NOCTURN_CC_BUTTON_LED(BUTTON) \
	(NOCTURN_CC_BUTTON0_LED + (BUTTON))

/* LED modes */
#define NOCTURN_LED_MODE_MIN_TO_VALUE		0
#define NOCTURN_LED_MODE_MAX_TO_VALUE		16
#define NOCTURN_LED_MODE_CENTER_TO_VALUE	32
#define NOCTURN_LED_MODE_CENTER_TO_VALUE_BIDIR	48
#define NOCTURN_LED_MODE_SINGLE			64
#define NOCTURN_LED_MODE_SINGLE_INVERTED	80

/* Buttons */
#define NOCTURN_BUTTON_ON			127
#define NOCTURN_BUTTON_OFF			0

void nocturn_init(struct controller *controller);

#endif /* _NOCTURN_H_ */

/************************* End of file nocturn.h ****************************/
