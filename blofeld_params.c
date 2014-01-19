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
#define LL 5
#define HH 6
#define PP 7
#define XX 8

#define TELL_SYNTH 1
#define TELL_UI 2

struct limits {
  int min;
  int max;
};

struct blofeld_param {
  const char *name;
  struct limits *limits;
  /* More to come, such as CC number, where applicable */
};

struct blofeld_param blofeld_params[BLOFELD_PARAMS];

struct limits norm = { 0, 127 };
struct limits oct = { 12, 112 };
struct limits bend = { -24, 24 };
struct limits bipolar = { -64, 63 };
#define detune bipolar
#define balance bipolar
#define pan bipolar
struct limits semitone = { -12, 12 };
struct limits keytrack = { -200, 196 };
struct limits fmsource = { 0, 11 };
struct limits modsource = { 0, 30 };
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

struct blofeld_param blofeld_params[BLOFELD_PARAMS] = {
  { "reserved", &norm }, /* 0 */
  { "Osc 1 Octave", &oct },
  { "Osc 1 Semitone", &semitone },
  { "Osc 1 Detune", &detune },
  { "Osc 1 Bend Range", &bend },
  { "Osc 1 Keytrack", &keytrack },
  { "Osc 1 FM Source", &fmsource },
  { "Osc 1 FM Amount", &norm },
  { "Osc 1 Wave", &wave },
  { "Osc 1 Waveshape", &norm },
  { "Osc 1 Shape Source", &modsource },
  { "Osc 1 Shape Amount", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Osc 1 Limit WT", &onoff },
  { "reserved", &norm },
  { "Osc 1 Brilliance", &norm }, /* 16 */
  { "Osc 2 Octave", &oct },
  { "Osc 2 Semitone", &semitone },
  { "Osc 2 Detune", &detune },
  { "Osc 2 Bend Range", &bend },
  { "Osc 2 Keytrack", &keytrack },
  { "Osc 2 FM Source", &fmsource },
  { "Osc 2 FM Amount", &norm },
  { "Osc 2 Wave", &wave },
  { "Osc 2 Waveshape", &norm },
  { "Osc 2 Shape Source", &modsource },
  { "Osc 2 Shape Amount", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Osc 2 Limit WT", &onoff },
  { "reserved", &norm },
  { "Osc 2 Brilliance", &norm }, /* 32 */
  { "Osc 3 Octave", &oct },
  { "Osc 3 Semitone", &semitone },
  { "Osc 3 Detune", &detune },
  { "Osc 3 Bend Range", &bend },
  { "Osc 3 Keytrack", &keytrack },
  { "Osc 3 FM Source", &fmsource },
  { "Osc 3 FM Amount", &norm },
  { "Osc 3 Wave", &wave3 },
  { "Osc 3 Waveshape", &norm },
  { "Osc 3 Shape Source", &modsource },
  { "Osc 3 Shape Amount", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Osc 3 Limit WT", &onoff },
  { "reserved", &norm },
  { "Osc 3 Brilliance", &norm }, /* 48 */
  { "Osc 2 Sync to Osc 3", &norm },
  { "Osc Pitch Source", &norm },
  { "Osc Pitch Amount", &norm },
  { "reserved", &norm },
  { "Glide", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Glide Mode", &norm },
  { "Glide Rate", &norm },
  { "Allocation/Unison", &norm },
  { "Unison Detune", &norm },
  { "reserved", &norm },
  { "Osc 1 Level", &norm },
  { "Osc 1 Balance", &balance },
  { "Osc 2 Level", &norm },
  { "Osc 2 Balance", &balance },
  { "Osc 3 Level", &norm },
  { "Osc 3 Balance", &balance },
  { "Noise Level", &norm },
  { "Noise Balance", &balance },
  { "Noise Color", &norm },
  { "reserved", &norm },
  { "Ringmod Level", &norm },
  { "Ringmod Balance", &balance },
  { "reserved", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Filter 1 Type", &norm }, /* 77 */
  { "Filter 1 Cutoff", &norm },
  { "reserved", &norm },
  { "Filter 1 Resonance", &norm },
  { "Filter 1 Drive", &norm },
  { "Filter 1 Drive Curve", &filterdrive },
  { "reserved", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Filter 1 Keytrack", &keytrack },
  { "Filter 1 Env Amount", &norm },
  { "Filter 1 Env Velocity", &norm },
  { "Filter 1 Mod Source", &norm },
  { "Filter 1 Mod Amount", &norm },
  { "Filter 1 FM Source", &fmsource },
  { "Filter 1 FM Amount", &norm },
  { "Filter 1 Pan", &pan },
  { "Filter 1 Pan Source", &norm },
  { "Filter 1 Pan Amount", &norm },
  { "reserved", &norm },
  { "Filter 2 Type", &norm }, /* 97 */
  { "Filter 2 Cutoff", &norm },
  { "reserved", &norm },
  { "Filter 2 Resonance", &norm },
  { "Filter 2 Drive", &norm },
  { "Filter 2 Drive Curve", &filterdrive },
  { "reserved", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Filter 2 Keytrack", &keytrack },
  { "Filter 2 Env Amount", &norm },
  { "Filter 2 Env Velocity", &norm },
  { "Filter 2 Mod Source", &norm },
  { "Filter 2 Mod Amount", &norm },
  { "Filter 2 FM Source", &fmsource },
  { "Filter 2 FM Amount", &norm },
  { "Filter 2 Pan", &pan },
  { "Filter 2 Pan Source", &norm },
  { "Filter 2 Pan Amount", &norm },
  { "reserved", &norm },
  { "Filter Routing", &norm }, /* 117 */
  { "reserved", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Amplifier Volume", &norm }, /* 121 */
  { "Amplifier Velocity", &norm },
  { "Amplifier Mod Source", &norm },
  { "Amplifier Mod Amount", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Effect 1 Type", &fx1type }, /* 128 */
  { "Effect 1 Mix", &norm },
  { "Effect 1 Parameter 1", &norm },
  { "Effect 1 Parameter 2", &norm },
  { "Effect 1 Parameter 3", &norm },
  { "Effect 1 Parameter 4", &norm },
  { "Effect 1 Parameter 5", &norm },
  { "Effect 1 Parameter 6", &norm },
  { "Effect 1 Parameter 7", &norm },
  { "Effect 1 Parameter 8", &norm },
  { "Effect 1 Parameter 9", &onoff },
  { "Effect 1 Parameter 10", &fxdrive },
  { "Effect 1 Parameter 11", &norm },
  { "Effect 1 Parameter 12", &norm },
  { "Effect 1 Parameter 13", &norm },
  { "Effect 1 Parameter 14", &norm },
  { "Effect 2 Type", &fx2type }, /* 146 */
  { "Effect 2 Mix", &norm },
  { "Effect 2 Parameter 1", &norm },
  { "Effect 2 Parameter 2", &norm },
  { "Effect 2 Parameter 3", &norm },
  { "Effect 2 Parameter 4", &norm },
  { "Effect 2 Parameter 5", &norm },
  { "Effect 2 Parameter 6", &norm },
  { "Effect 2 Parameter 7", &norm },
  { "Effect 2 Parameter 8", &norm },
  { "Effect 2 Parameter 9", &onoff },
  { "Effect 2 Parameter 10", &fxdrive },
  { "Effect 2 Parameter 11", &norm },
  { "Effect 2 Parameter 12", &norm },
  { "Effect 2 Parameter 13", &norm },
  { "Effect 2 Parameter 14", &norm },
  { "LFO 1 Shape", &lfoshape }, /* 160 */
  { "LFO 1 Speed", &norm },
  { "reserved", &norm },
  { "LFO 1 Sync", &onoff },
  { "LFO 1 Clocked", &onoff },
  { "LFO 1 Phase", &lfophase },
  { "LFO 1 Delay", &norm },
  { "LFO 1 Fade", &bipolar },
  { "reserved", &norm },
  { "reserved", &norm },
  { "LFO 1 Keytrack", &keytrack },
  { "reserved", &norm },
  { "LFO 2 Shape", &lfoshape }, /* 172 */
  { "LFO 2 Speed", &norm },
  { "reserved", &norm },
  { "LFO 2 Sync", &onoff },
  { "LFO 2 Clocked", &onoff },
  { "LFO 2 Phase", &lfophase },
  { "LFO 2 Delay", &norm },
  { "LFO 2 Fade", &bipolar },
  { "reserved", &norm },
  { "reserved", &norm },
  { "LFO 2 Keytrack", &keytrack },
  { "reserved", &norm },
  { "LFO 3 Shape", &lfoshape }, /* 184 */
  { "LFO 3 Speed", &norm },
  { "reserved", &norm },
  { "LFO 3 Sync", &onoff },
  { "LFO 3 Clocked", &onoff },
  { "LFO 3 Phase", &lfophase },
  { "LFO 3 Delay", &norm },
  { "LFO 3 Fade", &bipolar },
  { "reserved", &norm },
  { "reserved", &norm },
  { "LFO 3 Keytrack", &keytrack },
  { "reserved", &norm },
  { "Filter Envelope Mode", &envmode }, /* 196 */
  { "reserved", &norm },
  { "reserved", &norm },
  { "Filter Envelope Attack", &norm },
  { "Filter Envelope Attack Level", &norm },
  { "Filter Envelope Decay", &norm },
  { "Filter Envelope Sustain", &norm },
  { "Filter Envelope Decay 2", &norm },
  { "Filter Envelope Sustain 2", &norm },
  { "Filter Envelope Release", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Amplifier Envelope Mode", &envmode }, /* 208 */
  { "reserved", &norm },
  { "reserved", &norm },
  { "Amplifier Envelope Attack", &norm },
  { "Amplifier Envelope Attack Level", &norm },
  { "Amplifier Envelope Decay", &norm },
  { "Amplifier Envelope Sustain", &norm },
  { "Amplifier Envelope Decay 2", &norm },
  { "Amplifier Envelope Sustain 2", &norm },
  { "Amplifier Envelope Release", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Envelope 3 Mode", &envmode }, /* 220 */
  { "reserved", &norm },
  { "reserved", &norm },
  { "Envelope 3 Attack", &norm },
  { "Envelope 3 Attack Level", &norm },
  { "Envelope 3 Decay", &norm },
  { "Envelope 3 Sustain", &norm },
  { "Envelope 3 Decay 2", &norm },
  { "Envelope 3 Sustain 2", &norm },
  { "Envelope 3 Release", &norm },
  { "reserved", &norm },
  { "reserved", &norm },
  { "Envelope 4 Mode", &envmode }, /* 232 */
  { "reserved", &norm },
  { "reserved", &norm },
  { "Envelope 4 Attack", &norm },
  { "Envelope 4 Attack Level", &norm },
  { "Envelope 4 Decay", &norm },
  { "Envelope 4 Sustain", &norm },
  { "Envelope 4 Decay 2", &norm },
  { "Envelope 4 Sustain 2", &norm },
  { "Envelope 4 Release", &norm },
  { "reserved", &norm },
  { "reserved", &norm }
  /* More to come ... */
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

  for (i = 0; i < 244 /* TODO: BLOFELD_PARAMS */; i++) {
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
  if (param_num < BLOFELD_PARAMS && props) {
    props->ui_min = blofeld_params[param_num].limits->min;
    props->ui_max = blofeld_params[param_num].limits->max;
    /* set sane values for step size */
    int range = props->ui_max + 1 - props->ui_min;
    props->ui_step = (range / 128 > 1) ? 2 : 1;
    return 0;
  }
  return -1;
}

static void send_parameter_update(int parnum, int buffer, int devno, int value)
{
  unsigned char sndp[] = { SYSEX,
                           SYSEX_ID_WALDORF,
                           EQUIPMENT_ID_BLOFELD,
                           0x00, /* device number */
                           SNDP,
			   0,    /* buffer number */
			   0, 0, /* parameter number, big endian */
			   0,    /* value */
                           EOX };
  if (parnum < BLOFELD_PARAMS) {
    sndp[3] = devno;
    sndp[5] = buffer;
    sndp[6] = parnum >> 7;
    sndp[7] = parnum & 127;
    sndp[8] = value;

    midi_send_sysex(sndp, sizeof(sndp));
  }
}

/* called from UI when parameter updated */
void blofeld_update_param(int parnum, int parlist, int value)
{
  if (parnum < BLOFELD_PARAMS) {
    int min = blofeld_params[parnum].limits->min;
    int max = blofeld_params[parnum].limits->max;
    int range = max + 1 - min;
    if (range > 128) /* really only keytrack */
      value = value * 128 / range;
    if (min < 0)
      value += 64;
    else if (min == 12) /* octave */
      value = 12 * value + 16;
    parameter_list[parnum] = value;
    printf("Blofeld update param: parno %d, value %d\n", parnum, value);
    send_parameter_update(parnum, 0, 0, value);
  }
}

/* called from MIDI when parameter updated */
void update_ui(int parnum, int parlist, int value)
{
  printf("Blofeld update ui: parno %d, value %d\n", parnum, value);
  if (parnum < BLOFELD_PARAMS) {
    parameter_list[parnum] = value;
    int min = blofeld_params[parnum].limits->min;
    int max = blofeld_params[parnum].limits->max;
    int range = max + 1 - min;
    if (min < 0)
      value -= 64;
    else if (min == 12) /* octave */
      value = (value - 16 ) / 12;
    if (range > 128) /* really only keytrack */
      value = value * range / 128;
    notify_ui(parnum, parlist, value, notify_ref);
  }
}

int blofeld_fetch_parameter(int parnum, int parlist)
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
      case SNDD:
      case SNDR:
      case GLBR:
      case GLBD:
      default: break; /* ignore these */
    }
  }
}


void blofeld_init(void)
{
  midi_register_sysex(SYSEX_ID_WALDORF, blofeld_sysex, BLOFELD_PARAMS + 10);
}

void blofeld_register_notify_cb(blofeld_notify_cb cb, void *ref)
{
  notify_ui = cb;
  notify_ref = ref;
}
