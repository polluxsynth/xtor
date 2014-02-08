/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * blofeld_params.c - Parameter management for Waldorf Blofeld.
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
#include <string.h>
#include "param.h"
#include "blofeld_params.h"
#include "midi.h"

#include "debug.h"

#define SYSEX_ID_WALDORF 0x3E
#define EQUIPMENT_ID_BLOFELD 0x13
#define SNDR 0x00 /* Sound Request */
#define SNDD 0x10 /* Sound Dump */
#define SNDP 0x20 /* Sound Parameter Change */
#define GLBR 0x04 /* Global Request */
#define GLBD 0x14 /* Global Dump */

/* Offsets in Waldorf dumps, see sysex manual */
#define EXC 0
#define IDW 1
#define IDE 2
#define DEV 3
#define IDM 4
/* SNDP offsets */
#define LL 5
#define HH 6
#define PP 7
#define XX 8
/* SNDD offsets */
#define BB 5
#define NN 6
#define SDATA 7

/* Parameter buffers: 00..19h are banks A..Z (not all banks exist) */
#define EDIT_BUF 0x7f

#define TELL_SYNTH 1
#define TELL_UI 2

struct limits {
  int min;
  int max;
};

/* Structure for parameter definitions */
/* Used for all parameters, including bitmapped ones */
struct blofeld_param {
  const char *name;
  struct limits *limits;
  struct blofeld_param *child;
  struct blofeld_bitmap_param *bm_param; /* NULL for ordinary parameters */
  /* More to come, such as CC number, where applicable */
};

/* Extra definitions for bitmapped parameters */
struct blofeld_bitmap_param {
  const char *parent_param_name; /* Name of parent; searched during init */
  struct blofeld_param *parent_param; /* Pointer to parent */
  int bitmask;
  int bitshift;
};

/* Limit structures */
struct limits norm = { 0, 127 };
struct limits oct = { 12, 112 };
struct limits bend = { -24, 24 };
struct limits bipolar = { -64, 63 };
struct limits semitone = { -12, 12 };
struct limits keytrack = { -200, 196 };
struct limits fmsource = { 0, 11 };
struct limits modsource = { 0, 30 };
struct limits moddest = { 0, 53 };
struct limits filterdrive = { 0, 12 };
struct limits fxdrive = { 0, 11 };
struct limits fx1type = { 0, 5 };
struct limits fx2type = { 0, 8 };
struct limits lfoshape = { 0, 5 };
struct limits lfophase = { 0, 355 };
struct limits envmode = { 0, 4 };
struct limits wave = { 0, 72 };
struct limits wave3 = { 0, 4 };
struct limits onoff = { 0, 1 };
struct limits threebit = { 0, 7 };
struct limits sixbit = { 0, 63 };
struct limits modop = { 0, 7 };
struct limits arpmode = { 0, 3 };
struct limits arppat = { 0, 15 };
struct limits arpclock = { 0, 42 };
struct limits arplength = { 0, 43 };
struct limits arpoct = { 0, 9 };
struct limits arpdir = { 0, 3 };
struct limits arpsortord = { 0, 5 };
struct limits arpvel = { 0, 6 };
struct limits arpplen = { 0, 15 };
struct limits arptempo = { 40, 300 };
struct limits ascii = { 32, 127 };
struct limits category = { 0, 12 };

/* Bitmapped parameters additional structures */
struct blofeld_bitmap_param unison = { "Allocation Mode", NULL, 0x70, 4 };
struct blofeld_bitmap_param allocation = { "Allocation Mode", NULL, 0x01, 0 };

struct blofeld_bitmap_param fenvmode = { "Filter Envelope Trig+Mode", NULL, 0x07, 0 };
struct blofeld_bitmap_param fenvtrig = { "Filter Envelope Trig+Mode", NULL, 0x20, 5 };
struct blofeld_bitmap_param aenvmode = { "Amplifier Envelope Trig+Mode", NULL, 0x07, 0 };
struct blofeld_bitmap_param aenvtrig = { "Amplifier Envelope Trig+Mode", NULL, 0x20, 5 };
struct blofeld_bitmap_param env3mode = { "Envelope 3 Trig+Mode", NULL, 0x07, 0 };
struct blofeld_bitmap_param env4trig = { "Envelope 4 Trig+Mode", NULL, 0x20, 5 };
struct blofeld_bitmap_param env4mode = { "Envelope 4 Trig+Mode", NULL, 0x07, 0 };
struct blofeld_bitmap_param env3trig = { "Envelope 3 Trig+Mode", NULL, 0x20, 5 };

/* LFO speed and clock are the same parameter viewed in different ways
 * depending on the Clocked parameter. Speed is normal 0..127, but clock
 * is every second value, i.e. 0->0..1, 1->2..3, etc , so suitable for bitmap
 * parameter.
 */
struct blofeld_bitmap_param lfo1speed = { "LFO 1 Clock+Speed", NULL, 0x7f, 0 };
struct blofeld_bitmap_param lfo1clock = { "LFO 1 Clock+Speed", NULL, 0x7e, 1 };
struct blofeld_bitmap_param lfo2speed = { "LFO 2 Clock+Speed", NULL, 0x7f, 0 };
struct blofeld_bitmap_param lfo2clock = { "LFO 2 Clock+Speed", NULL, 0x7e, 1 };
struct blofeld_bitmap_param lfo3speed = { "LFO 3 Clock+Speed", NULL, 0x7f, 0 };
struct blofeld_bitmap_param lfo3clock = { "LFO 3 Clock+Speed", NULL, 0x7e, 1 };

/* FX 2 has two parameters with different data types */
/* Damping is continuous 0..127, polarity is 0..1 */
struct blofeld_bitmap_param fx2damping = { "Effect 2 Parameter 154.", NULL, 0x7f, 0 };
struct blofeld_bitmap_param fx2polarity = { "Effect 2 Parameter 154.", NULL, 0x7f, 0 };
/* Spread is continuous -64..+63, curve is 0..11 */
struct blofeld_bitmap_param fx2spread = { "Effect 2 Parameter 155.", NULL, 0x7f, 0 };
struct blofeld_bitmap_param fx2curve = { "Effect 2 Parameter 155.", NULL, 0x7f, 0 };

#define ARP_STEP_BITMAPS(N) \
struct blofeld_bitmap_param arpstep ## N = { "Arpeggiator Pattern StGlAcc " #N, NULL, 0x70, 4 }; \
struct blofeld_bitmap_param arpglide ## N = { "Arpeggiator Pattern StGlAcc " #N, NULL, 0x08, 3 }; \
struct blofeld_bitmap_param arpacc ## N = { "Arpeggiator Pattern StGlAcc " #N, NULL, 0x07, 0 }; \
struct blofeld_bitmap_param arplen ## N = { "Arpeggiator Pattern TimLen " #N, NULL, 0x70, 4 }; \
struct blofeld_bitmap_param arptim ## N = { "Arpeggiator Pattern TimLen " #N, NULL, 0x07, 0 }

ARP_STEP_BITMAPS(1);
ARP_STEP_BITMAPS(2);
ARP_STEP_BITMAPS(3);
ARP_STEP_BITMAPS(4);
ARP_STEP_BITMAPS(5);
ARP_STEP_BITMAPS(6);
ARP_STEP_BITMAPS(7);
ARP_STEP_BITMAPS(8);
ARP_STEP_BITMAPS(9);
ARP_STEP_BITMAPS(10);
ARP_STEP_BITMAPS(11);
ARP_STEP_BITMAPS(12);
ARP_STEP_BITMAPS(13);
ARP_STEP_BITMAPS(14);
ARP_STEP_BITMAPS(15);
ARP_STEP_BITMAPS(16);

/* string parameter: bitmask == 0, bitshift == string length */
struct blofeld_bitmap_param patchname = { "Name Char 1", NULL, 0, 16 };

#define BLOFELD_PARAMS_ALL (sizeof(blofeld_params) / \
                            sizeof(blofeld_params[0]))

/* Some definitions for tedious parameters that occur multiple times */

#define MODIFIER(N) \
  { "Modifier " #N " Source A", &modsource, NULL, NULL }, \
  { "Modifier " #N " Source B", &modsource, NULL, NULL }, \
  { "Modifier " #N " Operation", &modop, NULL, NULL }, \
  { "Modifier " #N " Constant", &bipolar, NULL, NULL }

#define MODULATION(N) \
  { "Modulation " #N " Source", &modsource, NULL, NULL }, \
  { "Modulation " #N " Destination", &moddest, NULL, NULL }, \
  { "Modulation " #N " Amount", &bipolar, NULL, NULL } \

#define ARPSTEP(N) \
  { "Arp Step Type " #N ".", &threebit, NULL, &arpstep ## N }, \
  { "Arp Step Glide " #N ".", &onoff, NULL, &arpglide ## N }, \
  { "Arp Step Accent " #N ".", &threebit, NULL, &arpacc ## N }, \
  { "Arp Step Timing " #N ".", &threebit, NULL, &arptim ## N }, \
  { "Arp Step Length " #N ".", &threebit, NULL, &arplen ## N }

static char patch_name[] = "Patch Name";

/* The Parameter Definition List */
/* Note: Owing to the design of the UI, in order to have the same parameter
 * appear in more than one place, parameters who are references by the UI
 * cannot have names that end with a digit,
 * This is because glade suffixes widget names with numbers when copying,
 * which we filter out in the main ui driver in order to have widgets
 * with the same base name (i.e. without the number suffix) that control the
 * same parameter. 
 */
struct blofeld_param blofeld_params[] = {
  /* name, limits, child, bm_param */
  { "reserved", NULL, NULL, NULL }, /* 0 */
  { "Osc 1 Octave", &oct, NULL, NULL },
  { "Osc 1 Semitone", &semitone, NULL, NULL },
  { "Osc 1 Detune", &bipolar, NULL, NULL },
  { "Osc 1 Bend Range", &bend, NULL, NULL },
  { "Osc 1 Keytrack", &keytrack, NULL, NULL },
  { "Osc 1 FM Source", &fmsource, NULL, NULL },
  { "Osc 1 FM Amount", &norm, NULL, NULL },
  { "Osc 1 Wave", &wave, NULL, NULL },
  { "Osc 1 Waveshape", &norm, NULL, NULL },
  { "Osc 1 Shape Source", &modsource, NULL, NULL },
  { "Osc 1 Shape Amount", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Osc 1 Limit WT", &onoff, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Osc 1 Brilliance", &norm, NULL, NULL }, /* 16 */
  { "Osc 2 Octave", &oct, NULL, NULL },
  { "Osc 2 Semitone", &semitone, NULL, NULL },
  { "Osc 2 Detune", &bipolar, NULL, NULL },
  { "Osc 2 Bend Range", &bend, NULL, NULL },
  { "Osc 2 Keytrack", &keytrack, NULL, NULL },
  { "Osc 2 FM Source", &fmsource, NULL, NULL },
  { "Osc 2 FM Amount", &norm, NULL, NULL },
  { "Osc 2 Wave", &wave, NULL, NULL },
  { "Osc 2 Waveshape", &norm, NULL, NULL },
  { "Osc 2 Shape Source", &modsource, NULL, NULL },
  { "Osc 2 Shape Amount", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Osc 2 Limit WT", &onoff, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Osc 2 Brilliance", &norm, NULL, NULL }, /* 32 */
  { "Osc 3 Octave", &oct, NULL, NULL },
  { "Osc 3 Semitone", &semitone, NULL, NULL },
  { "Osc 3 Detune", &bipolar, NULL, NULL },
  { "Osc 3 Bend Range", &bend, NULL, NULL },
  { "Osc 3 Keytrack", &keytrack, NULL, NULL },
  { "Osc 3 FM Source", &fmsource, NULL, NULL },
  { "Osc 3 FM Amount", &norm, NULL, NULL },
  { "Osc 3 Wave", &wave3, NULL, NULL },
  { "Osc 3 Waveshape", &norm, NULL, NULL },
  { "Osc 3 Shape Source", &modsource, NULL, NULL },
  { "Osc 3 Shape Amount", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Osc 3 Limit WT", &onoff, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Osc 3 Brilliance", &norm, NULL, NULL }, /* 48 */
  { "Osc 2 Sync to Osc 3", &norm, NULL, NULL },
  { "Osc Pitch Source", &norm, NULL, NULL },
  { "Osc Pitch Amount", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Glide", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Glide Mode", &norm, NULL, NULL },
  { "Glide Rate", &norm, NULL, NULL },
  { "Allocation Mode", NULL, NULL, NULL },
  { "Unison Detune", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Osc 1 Level", &norm, NULL, NULL },
  { "Osc 1 Balance", &bipolar, NULL, NULL },
  { "Osc 2 Level", &norm, NULL, NULL },
  { "Osc 2 Balance", &bipolar, NULL, NULL },
  { "Osc 3 Level", &norm, NULL, NULL },
  { "Osc 3 Balance", &bipolar, NULL, NULL },
  { "Noise Level", &norm, NULL, NULL },
  { "Noise Balance", &bipolar, NULL, NULL },
  { "Noise Color", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Ringmod Level", &norm, NULL, NULL },
  { "Ringmod Balance", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Filter 1 Type", &norm, NULL, NULL }, /* 77 */
  { "Filter 1 Cutoff", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Filter 1 Resonance", &norm, NULL, NULL },
  { "Filter 1 Drive", &norm, NULL, NULL },
  { "Filter 1 Drive Curve", &filterdrive, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Filter 1 Keytrack", &keytrack, NULL, NULL },
  { "Filter 1 Env Amount", &bipolar, NULL, NULL },
  { "Filter 1 Env Velocity", &bipolar, NULL, NULL },
  { "Filter 1 Mod Source", &norm, NULL, NULL },
  { "Filter 1 Mod Amount", &bipolar, NULL, NULL },
  { "Filter 1 FM Source", &fmsource, NULL, NULL },
  { "Filter 1 FM Amount", &norm, NULL, NULL },
  { "Filter 1 Pan", &bipolar, NULL, NULL },
  { "Filter 1 Pan Source", &norm, NULL, NULL },
  { "Filter 1 Pan Amount", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Filter 2 Type", &norm, NULL, NULL }, /* 97 */
  { "Filter 2 Cutoff", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Filter 2 Resonance", &norm, NULL, NULL },
  { "Filter 2 Drive", &norm, NULL, NULL },
  { "Filter 2 Drive Curve", &filterdrive, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Filter 2 Keytrack", &keytrack, NULL, NULL },
  { "Filter 2 Env Amount", &bipolar, NULL, NULL },
  { "Filter 2 Env Velocity", &bipolar, NULL, NULL },
  { "Filter 2 Mod Source", &norm, NULL, NULL },
  { "Filter 2 Mod Amount", &bipolar, NULL, NULL },
  { "Filter 2 FM Source", &fmsource, NULL, NULL },
  { "Filter 2 FM Amount", &norm, NULL, NULL },
  { "Filter 2 Pan", &bipolar, NULL, NULL },
  { "Filter 2 Pan Source", &norm, NULL, NULL },
  { "Filter 2 Pan Amount", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Filter Routing", &onoff, NULL, NULL }, /* 117 */
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Amplifier Volume", &norm, NULL, NULL }, /* 121 */
  { "Amplifier Velocity", &bipolar, NULL, NULL },
  { "Amplifier Mod Source", &norm, NULL, NULL },
  { "Amplifier Mod Amount", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Effect 1 Type", &fx1type, NULL, NULL }, /* 128 */
  { "Effect 1 Mix", &norm, NULL, NULL },
  { "Effect 1 Parameter 130.", &norm, NULL, NULL },
  { "Effect 1 Parameter 131.", &norm, NULL, NULL },
  { "Effect 1 Parameter 132.", &norm, NULL, NULL },
  { "Effect 1 Parameter 133.", &norm, NULL, NULL },
  { "Effect 1 Parameter 134.", &norm, NULL, NULL },
  { "Effect 1 Parameter 135.", &norm, NULL, NULL },
  { "Effect 1 Parameter 136.", &norm, NULL, NULL },
  { "Effect 1 Parameter 137.", &norm, NULL, NULL },
  { "Effect 1 Parameter 138.", &onoff, NULL, NULL },
  { "Effect 1 Parameter 139.", &fxdrive, NULL, NULL },
  { "Effect 1 Parameter 140.", &norm, NULL, NULL },
  { "Effect 1 Parameter 141.", &norm, NULL, NULL },
  { "Effect 1 Parameter 142.", &norm, NULL, NULL },
  { "Effect 1 Parameter 143.", &norm, NULL, NULL },
  { "Effect 2 Type", &fx2type, NULL, NULL }, /* 144 */
  { "Effect 2 Mix", &norm, NULL, NULL },
  { "Effect 2 Parameter 146.", &norm, NULL, NULL },
  { "Effect 2 Parameter 147.", &norm, NULL, NULL },
  { "Effect 2 Parameter 148.", &norm, NULL, NULL },
  { "Effect 2 Parameter 149.", &norm, NULL, NULL },
  { "Effect 2 Parameter 150.", &norm, NULL, NULL },
  { "Effect 2 Parameter 151.", &norm, NULL, NULL },
  { "Effect 2 Parameter 152.", &norm, NULL, NULL },
  { "Effect 2 Parameter 153.", &norm, NULL, NULL },
  { "Effect 2 Parameter 154.", &onoff, NULL, NULL },
  { "Effect 2 Parameter 155.", &fxdrive, NULL, NULL },
  { "Effect 2 Parameter 156.", &norm, NULL, NULL },
  { "Effect 2 Parameter 157.", &norm, NULL, NULL },
  { "Effect 2 Parameter 158.", &norm, NULL, NULL },
  { "Effect 2 Parameter 159.", &norm, NULL, NULL },
  { "LFO 1 Shape", &lfoshape, NULL, NULL }, /* 160 */
  { "LFO 1 Clock+Speed", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "LFO 1 Sync", &onoff, NULL, NULL },
  { "LFO 1 Clocked", &onoff, NULL, NULL },
  { "LFO 1 Phase", &lfophase, NULL, NULL },
  { "LFO 1 Delay", &norm, NULL, NULL },
  { "LFO 1 Fade", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "LFO 1 Keytrack", &keytrack, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "LFO 2 Shape", &lfoshape, NULL, NULL }, /* 172 */
  { "LFO 2 Clock+Speed", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "LFO 2 Sync", &onoff, NULL, NULL },
  { "LFO 2 Clocked", &onoff, NULL, NULL },
  { "LFO 2 Phase", &lfophase, NULL, NULL },
  { "LFO 2 Delay", &norm, NULL, NULL },
  { "LFO 2 Fade", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "LFO 2 Keytrack", &keytrack, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "LFO 3 Shape", &lfoshape, NULL, NULL }, /* 184 */
  { "LFO 3 Clock+Speed", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "LFO 3 Sync", &onoff, NULL, NULL },
  { "LFO 3 Clocked", &onoff, NULL, NULL },
  { "LFO 3 Phase", &lfophase, NULL, NULL },
  { "LFO 3 Delay", &norm, NULL, NULL },
  { "LFO 3 Fade", &bipolar, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "LFO 3 Keytrack", &keytrack, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Filter Envelope Trig+Mode", &envmode, NULL, NULL }, /* 196 */
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Filter Envelope Attack", &norm, NULL, NULL },
  { "Filter Envelope Attack Level", &norm, NULL, NULL },
  { "Filter Envelope Decay", &norm, NULL, NULL },
  { "Filter Envelope Sustain", &norm, NULL, NULL },
  { "Filter Envelope Decay Two", &norm, NULL, NULL },
  { "Filter Envelope Sustain Two", &norm, NULL, NULL },
  { "Filter Envelope Release", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Amplifier Envelope Trig+Mode", &envmode, NULL, NULL }, /* 208 */
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Amplifier Envelope Attack", &norm, NULL, NULL },
  { "Amplifier Envelope Attack Level", &norm, NULL, NULL },
  { "Amplifier Envelope Decay", &norm, NULL, NULL },
  { "Amplifier Envelope Sustain", &norm, NULL, NULL },
  { "Amplifier Envelope Decay Two", &norm, NULL, NULL },
  { "Amplifier Envelope Sustain Two", &norm, NULL, NULL },
  { "Amplifier Envelope Release", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Envelope 3 Trig+Mode", &envmode, NULL, NULL }, /* 220 */
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Envelope 3 Attack", &norm, NULL, NULL },
  { "Envelope 3 Attack Level", &norm, NULL, NULL },
  { "Envelope 3 Decay", &norm, NULL, NULL },
  { "Envelope 3 Sustain", &norm, NULL, NULL },
  { "Envelope 3 Decay Two", &norm, NULL, NULL },
  { "Envelope 3 Sustain Two", &norm, NULL, NULL },
  { "Envelope 3 Release", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Envelope 4 Trig+Mode", &envmode, NULL, NULL }, /* 232 */
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Envelope 4 Attack", &norm, NULL, NULL },
  { "Envelope 4 Attack Level", &norm, NULL, NULL },
  { "Envelope 4 Decay", &norm, NULL, NULL },
  { "Envelope 4 Sustain", &norm, NULL, NULL },
  { "Envelope 4 Decay Two", &norm, NULL, NULL },
  { "Envelope 4 Sustain Two", &norm, NULL, NULL },
  { "Envelope 4 Release", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  MODIFIER(1), /* 245 */
  MODIFIER(2),
  MODIFIER(3),
  MODIFIER(4),
  MODULATION(1), /* 261 */
  MODULATION(2),
  MODULATION(3),
  MODULATION(4),
  MODULATION(5),
  MODULATION(6),
  MODULATION(7),
  MODULATION(8),
  MODULATION(9),
  MODULATION(10),
  MODULATION(11),
  MODULATION(12),
  MODULATION(13),
  MODULATION(14),
  MODULATION(15),
  MODULATION(16),
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Arpeggiator Mode", &arpmode, NULL, NULL }, /* 311 */
  { "Arpeggiator Pattern", &arppat, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Arpeggiator Clock", &arpclock, NULL, NULL },
  { "Arpeggiator Length", &arplength, NULL, NULL },
  { "Arpeggiator Octave", &arpoct, NULL, NULL },
  { "Arpeggiator Direction", &arpdir, NULL, NULL },
  { "Arpeggiator Sort Order", &arpsortord, NULL, NULL },
  { "Arpeggiator Arp Velocity", &arpvel, NULL, NULL },
  { "Arpeggiator Timing Factor", &norm, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Arpeggiator Ptn Reset", &onoff, NULL, NULL },
  { "Arpeggiator Ptn Length", &arpplen, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Arpeggiator Tempo", &arptempo, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 1", NULL, NULL, NULL }, /* 327 */
  { "Arpeggiator Pattern StGlAcc 2", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 3", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 4", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 5", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 6", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 7", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 8", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 9", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 10", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 11", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 12", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 13", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 14", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 15", NULL, NULL, NULL },
  { "Arpeggiator Pattern StGlAcc 16", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 1", NULL, NULL, NULL }, /* 343 */
  { "Arpeggiator Pattern TimLen 2", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 3", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 4", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 5", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 6", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 7", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 8", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 9", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 10", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 11", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 12", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 13", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 14", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 15", NULL, NULL, NULL },
  { "Arpeggiator Pattern TimLen 16", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL }, /* 359 */
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "Name Char 1", &ascii, NULL, NULL },
  { "Name Char 2", &ascii, NULL, NULL },
  { "Name Char 3", &ascii, NULL, NULL },
  { "Name Char 4", &ascii, NULL, NULL },
  { "Name Char 5", &ascii, NULL, NULL },
  { "Name Char 6", &ascii, NULL, NULL },
  { "Name Char 7", &ascii, NULL, NULL },
  { "Name Char 8", &ascii, NULL, NULL },
  { "Name Char 9", &ascii, NULL, NULL },
  { "Name Char 10", &ascii, NULL, NULL },
  { "Name Char 11", &ascii, NULL, NULL },
  { "Name Char 12", &ascii, NULL, NULL },
  { "Name Char 13", &ascii, NULL, NULL },
  { "Name Char 14", &ascii, NULL, NULL },
  { "Name Char 15", &ascii, NULL, NULL },
  { "Name Char 16", &ascii, NULL, NULL },
  { "Category", &category, NULL, NULL }, /* 379 */
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL },
  { "reserved", NULL, NULL, NULL }, /* 382 */
  /* Bitmap parameters */
  { "Unison", &threebit, NULL, &unison },
  { "Allocation", &onoff, NULL, &allocation },
  { "Filter Envelope Mode", &envmode, NULL, &fenvmode },
  { "Filter Envelope Trig", &onoff, NULL, &fenvtrig },
  { "Amplifier Envelope Mode", &envmode, NULL, &aenvmode },
  { "Amplifier Envelope Trig", &onoff, NULL, &aenvtrig },
  { "Envelope 3 Mode", &envmode, NULL, &env3mode },
  { "Envelope 3 Trig", &onoff, NULL, &env3trig },
  { "Envelope 4 Mode", &envmode, NULL, &env4mode },
  { "Envelope 4 Trig", &onoff, NULL, &env4trig },
  { "LFO 1 Speed", &norm, NULL, &lfo1speed },
  { "LFO 1 Clock", &sixbit, NULL, &lfo1clock },
  { "LFO 2 Speed", &norm, NULL, &lfo2speed },
  { "LFO 2 Clock", &sixbit, NULL, &lfo2clock },
  { "LFO 3 Speed", &norm, NULL, &lfo3speed },
  { "LFO 3 Clock", &sixbit, NULL, &lfo3clock },
  ARPSTEP(1),
  ARPSTEP(2),
  ARPSTEP(3),
  ARPSTEP(4),
  ARPSTEP(5),
  ARPSTEP(6),
  ARPSTEP(7),
  ARPSTEP(8),
  ARPSTEP(9),
  ARPSTEP(10),
  ARPSTEP(11),
  ARPSTEP(12),
  ARPSTEP(13),
  ARPSTEP(14),
  ARPSTEP(15),
  ARPSTEP(16),
  { "Effect 2 Spread", &bipolar, NULL, &fx2spread },
  { "Effect 2 Curve", &filterdrive, NULL, &fx2curve },
  { "Effect 2 Damping", &norm, NULL, &fx2damping },
  { "Effect 2 Polarity", &onoff, NULL, &fx2polarity },
  { patch_name, NULL, NULL, &patchname },
  { "", NULL, NULL, NULL }
};

int parameter_list[BLOFELD_PARAMS];

int paste_buffer[BLOFELD_PARAMS];

/* Callback and parameter for parameter updates */
notify_cb notify_ui;
void *notify_ref;

int blofeld_find_index(const char *param_name)
{
  int idx = -1; /* not found */
  int i;

  if (!param_name) return idx;

  for (i = 0; i < BLOFELD_PARAMS_ALL; i++) {
    if (!strcmp(blofeld_params[i].name, param_name)) {
      idx = i;
      break;
    }
  }

  return idx;
}

int blofeld_get_param_properties(int param_num,
                                 struct param_properties *props)
{
  if (param_num < BLOFELD_PARAMS_ALL && props) {
    props->ui_min = blofeld_params[param_num].limits->min;
    props->ui_max = blofeld_params[param_num].limits->max;
    /* set sane values for step size */
    int range = props->ui_max + 1 - props->ui_min;
    props->ui_step = (range / 128 > 1) ? 2 : 1;
    return 0;
  }
  return -1;
}

void blofeld_get_dump(int buffer)
{
  unsigned char sndr[] = { SYSEX,
                           SYSEX_ID_WALDORF,
                           EQUIPMENT_ID_BLOFELD,
                           0, /* device number */
                           SNDR,
                           EDIT_BUF,
                           buffer,
                           EOX };

  midi_send_sysex(sndr, sizeof(sndr));
}

static void send_parameter_update(int parnum, int buf_no, int devno, int value)
{
  unsigned char sndp[] = { SYSEX,
                           SYSEX_ID_WALDORF,
                           EQUIPMENT_ID_BLOFELD,
                           devno, /* device number */
                           SNDP,
                           buf_no,
                           parnum >> 7, parnum & 127, /* big endian */
                           value,
                           EOX };

  dprintf("Blofeld update param: parnum %d, buf %d, value %d\n",
          parnum, buf_no, value);
  if (parnum < BLOFELD_PARAMS)
    midi_send_sysex(sndp, sizeof(sndp));
}


/* Prototype for forward declaration */
static void update_ui_int_param_children(struct blofeld_param *param,
                                         struct blofeld_param *excepted_child,
                                         int buf_no, int value, int mask);


/* Update numeric (integer) parameter and send to Blofeld */
static void update_int_param(struct blofeld_param *param,
                             int parnum, int buf_no, int value)
{
  int min = param->limits->min;
  int max = param->limits->max;
  int range = max + 1 - min;
  if (range > 128) /* really only keytrack */
    value = value * 128 / range;
  if (min < 0) /* bipolar parameter */
    value += 64; /* center around mid range (64) */
  else if (min == 12) /* octave */
    value = 12 * value + 16; /* coding for octave parameters */

  /* If bitmap param, fetch parent, then update value */
  if (param->bm_param) {
    struct blofeld_param *parent = param->bm_param->parent_param;
    if (parent == NULL) {
      eprintf("Warning: bitmap parameter %s has no parent!\n", param->name);
      return;
    }
    parnum = parent - blofeld_params;
    int mask = param->bm_param->bitmask;
    int shift = param->bm_param->bitshift;
    /* mask out non-changed bits, then or with new value */
    value = (parameter_list[parnum] & ~mask) | (value << shift);

    /* Update UI for all children that have a bitmask that overlaps, 
     * (skipping the one we've just received the update for)  */
    /* This happens for for instance LFO Speed vs Clock */
    update_ui_int_param_children(parent, param, buf_no, value, mask);
  }

  /* Update parameter list, then send to Blofeld */
  parameter_list[parnum] = value;
  send_parameter_update(parnum, buf_no, 0, value);
}

/* Update string parameter and send to Blofeld */
static void update_str_param(struct blofeld_param *param, int parnum,
                             int buf_no, const unsigned char *string)
{
  /* String parameters must be bitmap parameters, and bitmask must be == 0 */
  if (!param->bm_param || param->bm_param->bitmask)
    return;

  struct blofeld_param *parent = param->bm_param->parent_param;
  if (parent == NULL) {
    eprintf("Warning: bitmap/string parameter %s has no parent!\n", param->name);
    return;
  }
  parnum = parent - blofeld_params; /* param no of first char of parent */
  /* Now update each char parameter in the param list, then
   * send it on to Blofeld. */
  int len = param->bm_param->bitshift; /* we use bitshift field as (max) len */
  while (len--) {
    /* If we run out of the end of the string, the rest of the chars are ' ' */
    /* So once we hit \0, stay there, otherwise move on. */
    unsigned char ch = (*string) ? (*string++) : ' ';
    /* To minimize amount of data to be sent, only update the parameters that
     * are changed. Otherwise, we send the whole string each time a single
     * character is updated. We could do this for ordinary parameters too,
     * but the gain would be much less. */
    if (parameter_list[parnum] != ch) {
      parameter_list[parnum] = ch;
      send_parameter_update(parnum, buf_no, 0, ch);
    }
    parnum++;
  }
}

/* called from UI when parameter updated */
void blofeld_update_param(int parnum, int buf_no, const void *valptr)
{
  if (parnum >= BLOFELD_PARAMS_ALL || !valptr) /* sanity check */
    return;

  struct blofeld_param *param = &blofeld_params[parnum];

  if (param->limits) /* string parameters have limits set to NULL */
    update_int_param(param, parnum, buf_no, *(const int *)valptr);
  else
    update_str_param(param, parnum, buf_no, valptr);

}

static void update_ui_int_param(struct blofeld_param *param, int buf_no, int value)
{
  int min = param->limits->min;
  int max = param->limits->max;
  int parnum = param - blofeld_params;
  int range = max + 1 - min;
  if (min < 0)
    value -= 64;
  else if (min == 12) /* octave */
    value = (value - 16 ) / 12;
  if (range > 128) /* really only keytrack */
    value = value * range / 128;
  notify_ui(parnum, buf_no, &value, notify_ref);
}

static void update_ui_str_param(struct blofeld_param *param, int buf_no)
{
  /* here we assume the caller has validated that we are a bm/str parameter */
  int len = param->bm_param->bitshift;
  unsigned char string[len + 1];
  int parnum = param - blofeld_params;
  int parent_parnum = param->bm_param->parent_param - blofeld_params;
  int i;

  for (i = 0; i < len; i++)
    string[i] = (unsigned char) parameter_list[parent_parnum + i];
  string[len] = '\0';
  notify_ui(parnum, buf_no, string, notify_ref);
}

/* update the ui for all children of the supplied param but only if the 
 * bitmask overlaps with the supplied mask. */
static void update_ui_int_param_children(struct blofeld_param *param,
                                         struct blofeld_param *excepted_child,
                                         int buf_no, int value, int mask)
{
  struct blofeld_param *child = param->child;
  if (!child) return; /* We shouldn't be called in this case, but .. */
  dprintf("Updating ui for children of %s, mask %d\n", param->name, mask);
  /* Children of same parent are always grouped together. Parent points
   * to first child, so we just keep examining children until we find one
   * with a different parent. 
   */
  do {
    int bitmask = child->bm_param->bitmask;
    int bitshift = child->bm_param->bitshift;
    if ((bitmask & mask) && child != excepted_child) {
      dprintf("Updating child %s: bitmask %d mask %d\n",
              param->name, bitmask, mask);
      update_ui_int_param(child, buf_no, (value & bitmask) >> bitshift);
    }
    child++;
  } while (child->bm_param && child->bm_param->parent_param == param);
}


/* called from MIDI (or paste buf) when parameter updated */
void update_ui(int parnum, int buf_no, int value)
{
  if (parnum >= BLOFELD_PARAMS) /* sanity check */
    return;

  struct blofeld_param *param = &blofeld_params[parnum];

  dprintf("Blofeld update ui: parno %d, buf %d, value %d\n",
          parnum, buf_no, value);

  parameter_list[parnum] = value;

  if (!param->child) { /* no children => ordinary parameter ... */
    if (param->limits) /* ... unless it has no limits, then it's 'reserved' */
      update_ui_int_param(param, buf_no, value);
    return;
  }

  struct blofeld_param *child = param->child;
  if (!child->bm_param->bitmask) { /* string parameter */
    update_ui_str_param(child, buf_no);
    return;
  }

  /* bitmapped parameter */
  /* update all the children */
  update_ui_int_param_children(param, NULL, buf_no, value, 0x7f);
}

static void update_ui_all(unsigned char *param_buf, int buf_no)
{
  int parnum;
  static int force = 1; /* force complete update first time called */

  for (parnum = 0; parnum < BLOFELD_PARAMS; parnum++) {
    /* Only send UI updates for parameters that differ */
    if (param_buf[parnum] != parameter_list[parnum] || force) {
      update_ui(parnum, buf_no, param_buf[parnum]);
    }
  }
  force = 0;
}

void *blofeld_fetch_parameter(int parnum, int buf_no)
{
  if (parnum < BLOFELD_PARAMS)
    return &parameter_list[parnum];
  return NULL;
}

void blofeld_sysex(void *buffer, int len)
{
  unsigned char *buf = buffer;

  dprintf("Blofeld received sysex, len %d\n", len);
  if (len > IDE && buf[IDE] == EQUIPMENT_ID_BLOFELD) {
    switch (buf[IDM]) {
      case SNDP: update_ui(MIDI_2BYTE(buf[HH], buf[PP]), buf[LL], buf[XX]);
                 break;
      case SNDD: if (buf[BB] == EDIT_BUF)
                 /* TODO: Verify checksum */
                   update_ui_all(&buf[SDATA], buf[NN]);
                 break;
      case SNDR:
      case GLBR:
      case GLBD:
      default: break; /* ignore these */
    }
  }
}

void blofeld_register_notify_cb(notify_cb cb, void *ref)
{
  notify_ui = cb;
  notify_ref = ref;
}

/* Copy selected parameters to selected paste buffer */
void *blofeld_copy_to_paste(int par_from, int par_to, int buf_no, int paste_buf)
{
  void *src = &parameter_list[par_from];
  void *dest = &paste_buffer[par_from];
  int len = (par_to + 1 - par_from) * sizeof(parameter_list[0]);

  memcpy(dest, src, len);
}

/* Copy selected parameters from selected paste buffer */
void *blofeld_copy_from_paste(int par_from, int par_to, int buf_no, int paste_buf)
{
  int parnum;

  /* update parameter_list ui with pasted parameters */
  for (parnum = par_from; parnum <= par_to; parnum++) {
    /* Only send UI updates for parameters that differ */
    if (paste_buffer[parnum] != parameter_list[parnum]) {
      update_ui(parnum, buf_no, paste_buffer[parnum]);
    }
  }
}

/* Called when ui wants to know what the patch name parameter is called */
const char *blofeld_get_patch_name_id(void)
{
  return patch_name;
}

void blofeld_init(struct param_handler *param_handler)
{
  int idx;

  /* Scan all params, searching for bm_params, signified by having the
   * bm_param member !NULL (it's pointing to the struct blofeld_bitmap_param
   * for the parameter).
   * For each bm_param, we then fill in the parent, and fill in the current
   * parameter as the child for the parent, if it is the first child 
   * (the rest of the parameters with the same parent are assumed to lie)
   * sequentially after the first one).
   */
  for (idx = 0; 
       idx < sizeof(blofeld_params) / sizeof(struct blofeld_param);
       idx++) {
    struct blofeld_param *param = &blofeld_params[idx];
    struct blofeld_bitmap_param *bm_param = param->bm_param;

    if (!bm_param) continue; /* not a bitmapped param, go to next */

    /* bitmapped/string param */

    int parent_parno = blofeld_find_index(bm_param->parent_param_name);
    if (parent_parno < 0) {
      dprintf("Invalid bitmap param %s\n", bm_param->parent_param_name);
      continue;
    }
    struct blofeld_param *parent_param = &blofeld_params[parent_parno];

    /* Put link to parent in bitmap parameter */
    bm_param->parent_param = parent_param;

    dprintf("Param %s has parent %s\n", param->name, parent_param->name);

    /* Put link to first child in parent. The 'limits' member of
     * combined parameters must always be initialized to NULL, fairly
     * logical since such parameters have no limits on their own, relying
     * on the child limits for the individual fields.
     * The other children are assumed to lie after the first one in the
     * parameter definition list, with the same bm_param->parent.
     */
    /* For string parameters, we have many parents, one for each character,
     * so update them all. The bitmask field is == 0 for string parameters,
     * with the string length being in the bitshift field. */
    int params = (bm_param->bitmask) ? 1 : (bm_param->bitshift);
    while (params--) {
      if (!parent_param->child) {
        parent_param->child = param;
        dprintf("Param %s has first child %s\n",
                parent_param->name, param->bm_param->parent_param->child->name);
      }
      parent_param++; /* next parent */
    }
  }

  midi_register_sysex(SYSEX_ID_WALDORF, blofeld_sysex, BLOFELD_PARAMS + 10);

  /* Fill in param_handler struct */

  /* # parameters we have, including derived (e.g. bitmapped) types */
  param_handler->params = sizeof(blofeld_params)/sizeof(blofeld_params[0]);

  param_handler->remote_midi_device = "Waldorf Blofeld";
  param_handler->name = "Blofeld";
  param_handler->ui_filename = "blofeld.glade";

  /* Fill in function pointers */
  param_handler->param_register_notify_cb = blofeld_register_notify_cb;
  param_handler->param_find_index = blofeld_find_index;
  param_handler->param_get_properties = blofeld_get_param_properties;
  param_handler->param_update_parameter = blofeld_update_param;
  param_handler->param_fetch_parameter = blofeld_fetch_parameter;
  param_handler->param_get_patch_name_id = blofeld_get_patch_name_id;
}

