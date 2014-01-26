#include <stdio.h>
#include <string.h>
#include "blofeld_params.h"
#include "midi.h"

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
struct limits modop = { 0, 7 };
struct limits arpmode = { 0, 3 };
struct limits arppat = { 0, 15 };
struct limits arpclock = { 0, 42 };
struct limits arplength = { 0, 43 };
struct limits arpoct = { 1, 10 };
struct limits arpdir = { 0, 3 };
struct limits arpsortord = { 0, 5 };
struct limits arpvel = { 0, 6 };
struct limits arpplen = { 1, 16 };
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

#define ARP_STEP_BITMAPS(N) \
struct blofeld_bitmap_param arpstep ## N = { "Arp Pattern StGlAcc " #N, NULL, 0x70, 4 }; \
struct blofeld_bitmap_param arpglide ## N = { "Arp Pattern StGlAcc " #N, NULL, 0x08, 3 }; \
struct blofeld_bitmap_param arpacc ## N = { "Arp Pattern StGlAcc " #N, NULL, 0x07, 0 }; \
struct blofeld_bitmap_param arplen ## N = { "Arp Pattern TimLen " #N, NULL, 0x70, 4 }; \
struct blofeld_bitmap_param arptim ## N = { "Arp Pattern TimLen " #N, NULL, 0x07, 0 }

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
  { "Arp Step " #N "Type", &threebit, NULL, &arpstep ## N }, \
  { "Arp Step " #N "Glide", &onoff, NULL, &arpglide ## N }, \
  { "Arp Step " #N "Accent", &threebit, NULL, &arpacc ## N }, \
  { "Arp Step " #N "Timing", &threebit, NULL, &arptim ## N }, \
  { "Arp Step " #N "Length", &threebit, NULL, &arplen ## N }

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
  { "Effect 1 Parameter 1", &norm, NULL, NULL },
  { "Effect 1 Parameter 2", &norm, NULL, NULL },
  { "Effect 1 Parameter 3", &norm, NULL, NULL },
  { "Effect 1 Parameter 4", &norm, NULL, NULL },
  { "Effect 1 Parameter 5", &norm, NULL, NULL },
  { "Effect 1 Parameter 6", &norm, NULL, NULL },
  { "Effect 1 Parameter 7", &norm, NULL, NULL },
  { "Effect 1 Parameter 8", &norm, NULL, NULL },
  { "Effect 1 Parameter 9", &onoff, NULL, NULL },
  { "Effect 1 Parameter 10", &fxdrive, NULL, NULL },
  { "Effect 1 Parameter 11", &norm, NULL, NULL },
  { "Effect 1 Parameter 12", &norm, NULL, NULL },
  { "Effect 1 Parameter 13", &norm, NULL, NULL },
  { "Effect 1 Parameter 14", &norm, NULL, NULL },
  { "Effect 2 Type", &fx2type, NULL, NULL }, /* 146 */
  { "Effect 2 Mix", &norm, NULL, NULL },
  { "Effect 2 Parameter 1", &norm, NULL, NULL },
  { "Effect 2 Parameter 2", &norm, NULL, NULL },
  { "Effect 2 Parameter 3", &norm, NULL, NULL },
  { "Effect 2 Parameter 4", &norm, NULL, NULL },
  { "Effect 2 Parameter 5", &norm, NULL, NULL },
  { "Effect 2 Parameter 6", &norm, NULL, NULL },
  { "Effect 2 Parameter 7", &norm, NULL, NULL },
  { "Effect 2 Parameter 8", &norm, NULL, NULL },
  { "Effect 2 Parameter 9", &onoff, NULL, NULL },
  { "Effect 2 Parameter 10", &fxdrive, NULL, NULL },
  { "Effect 2 Parameter 11", &norm, NULL, NULL },
  { "Effect 2 Parameter 12", &norm, NULL, NULL },
  { "Effect 2 Parameter 13", &norm, NULL, NULL },
  { "Effect 2 Parameter 14", &norm, NULL, NULL },
  { "LFO 1 Shape", &lfoshape, NULL, NULL }, /* 160 */
  { "LFO 1 Speed", &norm, NULL, NULL },
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
  { "LFO 2 Speed", &norm, NULL, NULL },
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
  { "LFO 3 Speed", &norm, NULL, NULL },
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
  { "", NULL, NULL, NULL }
};

int parameter_list[BLOFELD_PARAMS];

/* Callback and parameter for parameter updates */
blofeld_notify_cb notify_ui;
void *notify_ref;;

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

  if (parnum < BLOFELD_PARAMS)
    midi_send_sysex(sndp, sizeof(sndp));
}

/* called from UI when parameter updated */
void blofeld_update_param(int parnum, int buf_no, int value)
{
  if (parnum >= BLOFELD_PARAMS_ALL) /* sanity check */
    return;

  struct blofeld_param *param = &blofeld_params[parnum];

  int min = param->limits->min;
  int max = param->limits->max;
  int range = max + 1 - min;
  if (range > 128) /* really only keytrack */
    value = value * 128 / range;
  if (min < 0) /* bipolar parameter */
    value += 64; /* center around mid range (64) */
  else if (min == 12) /* octave */
    value = 12 * value + 16; /* coding for octave parameters */

  /* If bitmap param, fetch parent, then update value and send it */
  if (param->bm_param) {
    parnum = param->bm_param->parent_param - blofeld_params;
    int mask = param->bm_param->bitmask;
    int shift = param->bm_param->bitshift;
    /* mask out non-changed bits, then or with new value */
    value = (parameter_list[parnum] & ~mask) | (value << shift);
  }

  /* Update parameter list, then send to Blofeld */
  parameter_list[parnum] = value;
  printf("Blofeld update param: parno %d, buf %d, value %d\n", parnum, buf_no, value);
  send_parameter_update(parnum, buf_no, 0, value);
}

static void update_ui_param(struct blofeld_param *param, int buf_no, int value)
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
  notify_ui(parnum, buf_no, value, notify_ref);
}

/* called from MIDI when parameter updated */
void update_ui(int parnum, int buf_no, int value)
{
  if (parnum >= BLOFELD_PARAMS
                               || parnum >= 244 /* TODO: remove */
                              ) /* sanity check */
    return;

  struct blofeld_param *param = &blofeld_params[parnum];

  printf("Blofeld update ui: parno %d, buf %d, value %d\n", parnum, buf_no, value);

  parameter_list[parnum] = value;

  if (!param->child) /* no children => ordinary parameter */
    update_ui_param(param, buf_no, value);
  else {
    /* Get first child */
    struct blofeld_param *child = param->child;
    /* Children of same parent are always grouped together. Parent points
     * to first child, so we just keep examining children until we find one
     * with a different parent. 
     */
    do {
      int mask = child->bm_param->bitmask;
      int shift = child->bm_param->bitshift;
      update_ui_param(child, buf_no, (value & mask) >> shift);
      child++;
    } while (child->bm_param && child->bm_param->parent_param == param);
  }
}

static void update_ui_all(unsigned char *param_buf, int buf_no)
{
  int parnum;
  static int force = 1; /* force complete update first time called */

  for (parnum = 0; parnum < BLOFELD_PARAMS; parnum++) {
    /* Only send UI updates for parameters that differ */
    if (param_buf[parnum] != parameter_list[parnum] || force) {
      int value = parameter_list[parnum] = param_buf[parnum];
      update_ui(parnum, buf_no, value);
    }
  }
  force = 0;
}

int blofeld_fetch_parameter(int parnum, int buf_no)
{
  if (parnum < BLOFELD_PARAMS)
    return parameter_list[parnum];
  return -1;
}

void blofeld_sysex(void *buffer, int len)
{
  unsigned char *buf = buffer;

  printf("Blofeld received sysex, len %d\n", len);
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

void blofeld_init(int *params)
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

    int parent_parno = blofeld_find_index(bm_param->parent_param_name);
    if (parent_parno < 0) {
      printf("Invalid bitmap param %s\n", bm_param->parent_param_name);
      continue;
    }
    /* Put link to parent in bitmap parameter */
    bm_param->parent_param = &blofeld_params[parent_parno];
    printf("Param %s has parent %s\n", param->name, param->bm_param->parent_param->name);
    /* Put link to first child in parent. The limit/child member of
     * combined parameters must always be initialized to NULL, fairly
     * logical since such parameters have no limits on their own, relying
     * on the child limits for the individual fields.
     * The other children are assumed to lie after the first one in the
     * parameter definition list, with the same bm_param->parent.
      */
    if (!bm_param->parent_param->child) {
      bm_param->parent_param->child = param;
      printf("Param %s has first child %s\n", param->bm_param->parent_param->name, param->bm_param->parent_param->child->name);
    }
  }

  midi_register_sysex(SYSEX_ID_WALDORF, blofeld_sysex, BLOFELD_PARAMS + 10);

  /* Return # parameters we have, including derived (e.g. bitmapped) types */
  if (params)
    *params = sizeof(blofeld_params)/sizeof(blofeld_params[0]);
}

void blofeld_register_notify_cb(blofeld_notify_cb cb, void *ref)
{
  notify_ui = cb;
  notify_ref = ref;
}
