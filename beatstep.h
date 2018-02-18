/****************************************************************************
 * xtor - GTK based editor for MIDI synthesizers
 *
 * beatstep.h - Definitions for Arturia Beatstep
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

#ifndef _BEATSTEP_H_
#define _BEATSTEP_H_

#include "controller.h"

/* Incrementors, not counting volume dial */
#define BEATSTEP_INCREMENTORS 16
/* Buttons, not counting start and stop */
#define BEATSTEP_BUTTONS 16

/* Thanks to http://www.untergeek.de/2014/11/taming-arturias-beatstep-sysex-codes-for-programming-via-ipad/
 * for digging out this information including the sysex format. */

#define BEATSTEP_INCREMENTOR0			0x20
#define BEATSTEP_INCREMENTOR(INCREMENTOR) \
	(BEATSTEP_INCREMENTOR0 + (INCREMENTOR))

#define BEATSTEP_VOLUME_DIAL			0x30

/* Note that PLAY and STOP are seemingly in the wrong order compared to the
 * rest which are in a left-to-right-up-to-down fashion. */
#define BEATSTEP_BUTTON_PLAY			0x58
#define BEATSTEP_BUTTON_STOP			0x59
#define BEATSTEP_BUTTON_CNTRL_SEQ		0x5a
#define BEATSTEP_BUTTON_EXT_SYNC		0x5b
#define BEATSTEP_BUTTON_RECALL			0x5c
#define BEATSTEP_BUTTON_STORE			0x5d
#define BEATSTEP_BUTTON_SHIFT			0x5e
#define BEATSTEP_BUTTON_CHAN			0x5f

#define BEATSTEP_BUTTON0			0x70	
#define BEATSTEP_BUTTON(BUTTON) \
	(BEATSTEP_BUTTON0 + (BUTTON))

/* Modes for buttons. */
/* Position in sysex packet (SYSEX byte is byte #0):	8	9	10 */
/* For controller settings, controller no is byte 9) */
#define BEATSTEP_BUTTON_MODE_SET			0x01
#define BEATSTEP_BUTTON_MODE_OFF					0
/* CC_QUIET doesn't light pad leds while pressing. */
#define BEATSTEP_BUTTON_MODE_CC_QUIET 					1
#define BEATSTEP_BUTTON_MODE_MMC					7
#define BEATSTEP_BUTTON_MODE_CC 					8
#define BEATSTEP_BUTTON_MODE_NOTE					9
#define BEATSTEP_BUTTON_MODE_PROGRAM_CHANGE				11

#define BEATSTEP_BUTTON_MODE_MMC_COMMAND		0x03
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_STOP				1
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_PLAY				2
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_DEFERRED_PLAY			3
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_FF				4
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_REW				5
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_RECORD_STROBE			6
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_RECORD_EXIT			7
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_RECORD_READY			8
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_RECORD_PAUSE			9
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_RECORD_EJECT			10
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_RECORD_CHASE			11
#define BEATSTEP_BUTTON_MODE_MMC_COMMAND_INLIST_RESET			12

#define BEATSTEP_BUTTON_MODE_CC_CHANNEL			0x02 /* channel-1 */
#define BEATSTEP_BUTTON_MODE_CC_CONTROL_NO		0x03 /* control no */
#define BEATSTEP_BUTTON_MODE_CC_OFF			0x04 /* off value */
#define BEATSTEP_BUTTON_MODE_CC_ON			0x05 /* on value */
#define BEATSTEP_BUTTON_MODE_CC_BEHAVIOR		0x06
#define BEATSTEP_BUTTON_MODE_CC_BEHAVIOR_TOGGLE				0
#define BEATSTEP_BUTTON_MODE_CC_BEHAVIOR_GATE				1

#define BEATSTEP_BUTTON_MODE_NOTE_CHANNEL		0x02 /* channel-1 */
#define BEATSTEP_BUTTON_MODE_NOTE_NO			0x03 /* note no */
#define BEATSTEP_BUTTON_MODE_NOTE_BEHAVIOR		0x06
#define BEATSTEP_BUTTON_MODE_NOTE_BEHAVIOR_TOGGLE			0
#define BEATSTEP_BUTTON_MODE_NOTE_BEHAVIOR_GATE				1

#define BEATSTEP_BUTTON_MODE_PROGRAM_CHANGE_CHANNEL	0x02 /* channel-1 */
#define BEATSTEP_BUTTON_MODE_PROGRAM_CHANGE_NO		0x03 /* program no */
#define BEATSTEP_BUTTON_MODE_PROGRAM_CHANGE_BANK_LSB	0x04 /* bank LSB */
#define BEATSTEP_BUTTON_MODE_PROGRAM_CHANGE_BANK_MSB	0x05 /* bank MSB */

/* Modes for knobs */
/* Position in sysex packet (SYSEX byte is byte #0):	byte 8	byte 10 */
#define BEATSTEP_KNOB_MODE_SET				0x01  
#define BEATSTEP_KNOB_MODE_OFF						0
#define BEATSTEP_KNOB_MODE_CC						1
#define BEATSTEP_KNOB_MODE_RPN						2

#define BEATSTEP_KNOB_MODE_CC_CHANNEL			0x02 /* channel-1 */
#define BEATSTEP_KNOB_MODE_CC_CONTROL_NO		0x03 /* control no */
#define BEATSTEP_KNOB_MODE_CC_MIN			0x04 /* min value */
#define BEATSTEP_KNOB_MODE_CC_MAX			0x05 /* max value */
#define BEATSTEP_KNOB_MODE_CC_BEHAVIOR			0x06
#define BEATSTEP_KNOB_MODE_CC_BEHAVIOR_ABSOLUTE				0	
/* Relative 1: value = 64 +/- delta */
#define BEATSTEP_KNOB_MODE_CC_BEHAVIOR_RELATIVE1			1
/* Relative 2: value = +/- delta (i.e. 1,2,3,etc vs 127,126,125,etc) */
#define BEATSTEP_KNOB_MODE_CC_BEHAVIOR_RELATIVE2			2
/* Relative 3: value = 16 +/- delta */
#define BEATSTEP_KNOB_MODE_CC_BEHAVIOR_RELATIVE3			3

#define BEATSTEP_KNOB_MODE_RPN_CHANNEL			0x02 /* channel-1 */
#define BEATSTEP_KNOB_MODE_RPN_GRANULARITY		0x03
/* CORASE mode => MSB sent, FINE mode => LSB sent */
#define BEATSTEP_KNOB_MODE_RPN_GRANULARITY_COARSE			0x06
#define BEATSTEP_KNOB_MODE_RPN_GRANULARITY_FINE				0x26
#define BEATSTEP_KNOB_MODE_RPN_BANK_LSB			0x04 /* bank LSB */
#define BEATSTEP_KNOB_MODE_RPN_BANK_MSB			0x05 /* bank MSB */
#define BEATSTEP_KNOB_MODE_RPN_TYPE			0x06
#define BEATSTEP_KNOB_MODE_RPN_TYPE_NRPN				0
#define BEATSTEP_KNOB_MODE_RPN_TYPE_RPN					1

#define BEATSTEP_GLOBAL_CTRLR_SETTING			0x41
#define BEATSTEP_GLOBAL_CTRLR_SETTING_VELOCITY_CURVE		3
#define BEATSTEP_GLOBAL_CTRLR_SETTING_VELOCITY_CURVE_LIN		0
#define BEATSTEP_GLOBAL_CTRLR_SETTING_VELOCITY_CURVE_LOG		1
#define BEATSTEP_GLOBAL_CTRLR_SETTING_VELOCITY_CURVE_EXP		2
#define BEATSTEP_GLOBAL_CTRLR_SETTING_VELOCITY_CURVE_FULL		3
#define BEATSTEP_GLOBAL_CTRLR_SETTING_ACCELERATION		4
#define BEATSTEP_GLOBAL_CTRLR_SETTING_ACCELERATION_SLOW			0
#define BEATSTEP_GLOBAL_CTRLR_SETTING_ACCELERATION_MEDIUM		1
#define BEATSTEP_GLOBAL_CTRLR_SETTING_ACCELERATION_FAST			2

#define BEATSTEP_GLOBAL_SETTING				0x50
#define BEATSTEP_GLOBAL_SETTING_SEQ_CHANNEL			0x01 /* 0..15 */
#define BEATSTEP_GLOBAL_SETTING_SEQ_XPOSE			0x02 /* 60<>0 */
#define BEATSTEP_GLOBAL_SETTING_SEQ_SCALE			0x03
#define BEATSTEP_GLOBAL_SETTING_SEQ_SCALE_CHROMATIC			0
#define BEATSTEP_GLOBAL_SETTING_SEQ_SCALE_MAJOR				1
#define BEATSTEP_GLOBAL_SETTING_SEQ_SCALE_MINOR				2
#define BEATSTEP_GLOBAL_SETTING_SEQ_SCALE_DORIAN			3
#define BEATSTEP_GLOBAL_SETTING_SEQ_SCALE_MIXOLYDIAN			4
#define BEATSTEP_GLOBAL_SETTING_SEQ_SCALE_HARM_MINOR			5
#define BEATSTEP_GLOBAL_SETTING_SEQ_SCALE_BLUES				6
#define BEATSTEP_GLOBAL_SETTING_SEQ_SCALE_USER				7
#define BEATSTEP_GLOBAL_SETTING_SEQ_MODE			0x04
#define BEATSTEP_GLOBAL_SETTING_SEQ_MODE_FORWARD			0
#define BEATSTEP_GLOBAL_SETTING_SEQ_MODE_REVERSE			1
#define BEATSTEP_GLOBAL_SETTING_SEQ_MODE_ALT				2
#define BEATSTEP_GLOBAL_SETTING_SEQ_MODE_RANDOM				3
#define BEATSTEP_GLOBAL_SETTING_SEQ_STEPSIZE			0x05
#define BEATSTEP_GLOBAL_SETTING_SEQ_STEPSIZE_CROTCHET			0
#define BEATSTEP_GLOBAL_SETTING_SEQ_STEPSIZE_QUAVER			1
#define BEATSTEP_GLOBAL_SETTING_SEQ_STEPSIZE_SEMIQUAVER			2
#define BEATSTEP_GLOBAL_SETTING_SEQ_STEPSIZE_DEMISEMIQUAVER		3
#define BEATSTEP_GLOBAL_SETTING_SEQ_PATTERN_LEN			0x06 /* 1..16 */
#define BEATSTEP_GLOBAL_SETTING_SEQ_SWING			0x07 /* 50..75*/
#define BEATSTEP_GLOBAL_SETTING_SEQ_GATE			0x08 /* 0..99*//
#define BEATSTEP_GLOBAL_SETTING_SEQ_LEGATO			0x09
#define BEATSTEP_GLOBAL_SETTING_SEQ_LEGATO_OFF				0
#define BEATSTEP_GLOBAL_SETTING_SEQ_LEGATO_ON				1
#define BEATSTEP_GLOBAL_SETTING_SEQ_LEGATO_RESET			2
/* The following would seem to conflict with SEQ_PATTERN_LEN ? */
#define BEATSTEP_GLOBAL_SETTING_MIDI_CHANNEL			0x06 /* 0..15 */
#define BEATSTEP_GLOBAL_SETTING_CVGATE_CHANNEL			0x0c /* 0..15 */

void beatstep_init(struct controller *controller);

#endif /* _BEATSTEP_H_ */

/************************* End of file beatstep.h ***************************/
