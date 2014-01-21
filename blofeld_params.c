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

/* Structure for parameter definitions */
/* Used for all parameters, including bitmapped ones */
struct blofeld_param {
  const char *name;
  union {
    struct limits *limits;
    struct blofeld_param *child;
  } l_ch; /* Initialized for ordinary parameters, NULL for combined ones */
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
struct limits threebit = { 0, 7 };

/* Bitmapped parameters additional structures */
struct blofeld_bitmap_param unison = { "Allocation Mode", NULL, 0x70, 4 };
struct blofeld_bitmap_param allocation = { "Allocation Mode", NULL, 0x01, 0 };

#define BLOFELD_PARAMS_ALL (sizeof(blofeld_params) / \
                            sizeof(struct blofeld_param))

/* The Parameter Definiton List */
struct blofeld_param blofeld_params[] = {
  { "reserved", &norm, NULL }, /* 0 */
  { "Osc 1 Octave", &oct, NULL },
  { "Osc 1 Semitone", &semitone, NULL },
  { "Osc 1 Detune", &detune, NULL },
  { "Osc 1 Bend Range", &bend, NULL },
  { "Osc 1 Keytrack", &keytrack, NULL },
  { "Osc 1 FM Source", &fmsource, NULL },
  { "Osc 1 FM Amount", &norm, NULL },
  { "Osc 1 Wave", &wave, NULL },
  { "Osc 1 Waveshape", &norm, NULL },
  { "Osc 1 Shape Source", &modsource, NULL },
  { "Osc 1 Shape Amount", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Osc 1 Limit WT", &onoff, NULL },
  { "reserved", &norm, NULL },
  { "Osc 1 Brilliance", &norm, NULL }, /* 16 */
  { "Osc 2 Octave", &oct, NULL },
  { "Osc 2 Semitone", &semitone, NULL },
  { "Osc 2 Detune", &detune, NULL },
  { "Osc 2 Bend Range", &bend, NULL },
  { "Osc 2 Keytrack", &keytrack, NULL },
  { "Osc 2 FM Source", &fmsource, NULL },
  { "Osc 2 FM Amount", &norm, NULL },
  { "Osc 2 Wave", &wave, NULL },
  { "Osc 2 Waveshape", &norm, NULL },
  { "Osc 2 Shape Source", &modsource, NULL },
  { "Osc 2 Shape Amount", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Osc 2 Limit WT", &onoff, NULL },
  { "reserved", &norm, NULL },
  { "Osc 2 Brilliance", &norm, NULL }, /* 32 */
  { "Osc 3 Octave", &oct, NULL },
  { "Osc 3 Semitone", &semitone, NULL },
  { "Osc 3 Detune", &detune, NULL },
  { "Osc 3 Bend Range", &bend, NULL },
  { "Osc 3 Keytrack", &keytrack, NULL },
  { "Osc 3 FM Source", &fmsource, NULL },
  { "Osc 3 FM Amount", &norm, NULL },
  { "Osc 3 Wave", &wave3, NULL },
  { "Osc 3 Waveshape", &norm, NULL },
  { "Osc 3 Shape Source", &modsource, NULL },
  { "Osc 3 Shape Amount", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Osc 3 Limit WT", &onoff, NULL },
  { "reserved", &norm, NULL },
  { "Osc 3 Brilliance", &norm, NULL }, /* 48 */
  { "Osc 2 Sync to Osc 3", &norm, NULL },
  { "Osc Pitch Source", &norm, NULL },
  { "Osc Pitch Amount", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Glide", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Glide Mode", &norm, NULL },
  { "Glide Rate", &norm, NULL },
  { "Allocation Mode", NULL, NULL },
  { "Unison Detune", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Osc 1 Level", &norm, NULL },
  { "Osc 1 Balance", &balance, NULL },
  { "Osc 2 Level", &norm, NULL },
  { "Osc 2 Balance", &balance, NULL },
  { "Osc 3 Level", &norm, NULL },
  { "Osc 3 Balance", &balance, NULL },
  { "Noise Level", &norm, NULL },
  { "Noise Balance", &balance, NULL },
  { "Noise Color", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Ringmod Level", &norm, NULL },
  { "Ringmod Balance", &balance, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Filter 1 Type", &norm, NULL }, /* 77 */
  { "Filter 1 Cutoff", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Filter 1 Resonance", &norm, NULL },
  { "Filter 1 Drive", &norm, NULL },
  { "Filter 1 Drive Curve", &filterdrive, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Filter 1 Keytrack", &keytrack, NULL },
  { "Filter 1 Env Amount", &norm, NULL },
  { "Filter 1 Env Velocity", &norm, NULL },
  { "Filter 1 Mod Source", &norm, NULL },
  { "Filter 1 Mod Amount", &norm, NULL },
  { "Filter 1 FM Source", &fmsource, NULL },
  { "Filter 1 FM Amount", &norm, NULL },
  { "Filter 1 Pan", &pan, NULL },
  { "Filter 1 Pan Source", &norm, NULL },
  { "Filter 1 Pan Amount", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Filter 2 Type", &norm, NULL }, /* 97 */
  { "Filter 2 Cutoff", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Filter 2 Resonance", &norm, NULL },
  { "Filter 2 Drive", &norm, NULL },
  { "Filter 2 Drive Curve", &filterdrive, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Filter 2 Keytrack", &keytrack, NULL },
  { "Filter 2 Env Amount", &norm, NULL },
  { "Filter 2 Env Velocity", &norm, NULL },
  { "Filter 2 Mod Source", &norm, NULL },
  { "Filter 2 Mod Amount", &norm, NULL },
  { "Filter 2 FM Source", &fmsource, NULL },
  { "Filter 2 FM Amount", &norm, NULL },
  { "Filter 2 Pan", &pan, NULL },
  { "Filter 2 Pan Source", &norm, NULL },
  { "Filter 2 Pan Amount", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Filter Routing", &onoff, NULL }, /* 117 */
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Amplifier Volume", &norm, NULL }, /* 121 */
  { "Amplifier Velocity", &norm, NULL },
  { "Amplifier Mod Source", &norm, NULL },
  { "Amplifier Mod Amount", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Effect 1 Type", &fx1type, NULL }, /* 128 */
  { "Effect 1 Mix", &norm, NULL },
  { "Effect 1 Parameter 1", &norm, NULL },
  { "Effect 1 Parameter 2", &norm, NULL },
  { "Effect 1 Parameter 3", &norm, NULL },
  { "Effect 1 Parameter 4", &norm, NULL },
  { "Effect 1 Parameter 5", &norm, NULL },
  { "Effect 1 Parameter 6", &norm, NULL },
  { "Effect 1 Parameter 7", &norm, NULL },
  { "Effect 1 Parameter 8", &norm, NULL },
  { "Effect 1 Parameter 9", &onoff, NULL },
  { "Effect 1 Parameter 10", &fxdrive, NULL },
  { "Effect 1 Parameter 11", &norm, NULL },
  { "Effect 1 Parameter 12", &norm, NULL },
  { "Effect 1 Parameter 13", &norm, NULL },
  { "Effect 1 Parameter 14", &norm, NULL },
  { "Effect 2 Type", &fx2type, NULL }, /* 146 */
  { "Effect 2 Mix", &norm, NULL },
  { "Effect 2 Parameter 1", &norm, NULL },
  { "Effect 2 Parameter 2", &norm, NULL },
  { "Effect 2 Parameter 3", &norm, NULL },
  { "Effect 2 Parameter 4", &norm, NULL },
  { "Effect 2 Parameter 5", &norm, NULL },
  { "Effect 2 Parameter 6", &norm, NULL },
  { "Effect 2 Parameter 7", &norm, NULL },
  { "Effect 2 Parameter 8", &norm, NULL },
  { "Effect 2 Parameter 9", &onoff, NULL },
  { "Effect 2 Parameter 10", &fxdrive, NULL },
  { "Effect 2 Parameter 11", &norm, NULL },
  { "Effect 2 Parameter 12", &norm, NULL },
  { "Effect 2 Parameter 13", &norm, NULL },
  { "Effect 2 Parameter 14", &norm, NULL },
  { "LFO 1 Shape", &lfoshape, NULL }, /* 160 */
  { "LFO 1 Speed", &norm, NULL },
  { "reserved", &norm, NULL },
  { "LFO 1 Sync", &onoff, NULL },
  { "LFO 1 Clocked", &onoff, NULL },
  { "LFO 1 Phase", &lfophase, NULL },
  { "LFO 1 Delay", &norm, NULL },
  { "LFO 1 Fade", &bipolar, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "LFO 1 Keytrack", &keytrack, NULL },
  { "reserved", &norm, NULL },
  { "LFO 2 Shape", &lfoshape, NULL }, /* 172 */
  { "LFO 2 Speed", &norm, NULL },
  { "reserved", &norm, NULL },
  { "LFO 2 Sync", &onoff, NULL },
  { "LFO 2 Clocked", &onoff, NULL },
  { "LFO 2 Phase", &lfophase, NULL },
  { "LFO 2 Delay", &norm, NULL },
  { "LFO 2 Fade", &bipolar, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "LFO 2 Keytrack", &keytrack, NULL },
  { "reserved", &norm, NULL },
  { "LFO 3 Shape", &lfoshape, NULL }, /* 184 */
  { "LFO 3 Speed", &norm, NULL },
  { "reserved", &norm, NULL },
  { "LFO 3 Sync", &onoff, NULL },
  { "LFO 3 Clocked", &onoff, NULL },
  { "LFO 3 Phase", &lfophase, NULL },
  { "LFO 3 Delay", &norm, NULL },
  { "LFO 3 Fade", &bipolar, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "LFO 3 Keytrack", &keytrack, NULL },
  { "reserved", &norm, NULL },
  { "Filter Envelope Mode", &envmode, NULL }, /* 196 */
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Filter Envelope Attack", &norm, NULL },
  { "Filter Envelope Attack Level", &norm, NULL },
  { "Filter Envelope Decay", &norm, NULL },
  { "Filter Envelope Sustain", &norm, NULL },
  { "Filter Envelope Decay 2", &norm, NULL },
  { "Filter Envelope Sustain 2", &norm, NULL },
  { "Filter Envelope Release", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Amplifier Envelope Mode", &envmode, NULL }, /* 208 */
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Amplifier Envelope Attack", &norm, NULL },
  { "Amplifier Envelope Attack Level", &norm, NULL },
  { "Amplifier Envelope Decay", &norm, NULL },
  { "Amplifier Envelope Sustain", &norm, NULL },
  { "Amplifier Envelope Decay 2", &norm, NULL },
  { "Amplifier Envelope Sustain 2", &norm, NULL },
  { "Amplifier Envelope Release", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Envelope 3 Mode", &envmode, NULL }, /* 220 */
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Envelope 3 Attack", &norm, NULL },
  { "Envelope 3 Attack Level", &norm, NULL },
  { "Envelope 3 Decay", &norm, NULL },
  { "Envelope 3 Sustain", &norm, NULL },
  { "Envelope 3 Decay 2", &norm, NULL },
  { "Envelope 3 Sustain 2", &norm, NULL },
  { "Envelope 3 Release", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Envelope 4 Mode", &envmode, NULL }, /* 232 */
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  { "Envelope 4 Attack", &norm, NULL },
  { "Envelope 4 Attack Level", &norm, NULL },
  { "Envelope 4 Decay", &norm, NULL },
  { "Envelope 4 Sustain", &norm, NULL },
  { "Envelope 4 Decay 2", &norm, NULL },
  { "Envelope 4 Sustain 2", &norm, NULL },
  { "Envelope 4 Release", &norm, NULL },
  { "reserved", &norm, NULL },
  { "reserved", &norm, NULL },
  /* More to come ... */
  /* Bitmap parameters */
  { "Unison", &threebit, &unison },
  { "Allocation", &onoff, &allocation },
  { "", NULL, NULL }
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
    props->ui_min = blofeld_params[param_num].l_ch.limits->min;
    props->ui_max = blofeld_params[param_num].l_ch.limits->max;
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
  if (parnum < BLOFELD_PARAMS_ALL) {
    int min = blofeld_params[parnum].l_ch.limits->min;
    int max = blofeld_params[parnum].l_ch.limits->max;
    int range = max + 1 - min;
    if (range > 128) /* really only keytrack */
      value = value * 128 / range;
    if (min < 0)
      value += 64;
    else if (min == 12) /* octave */
      value = 12 * value + 16;
    /* TODO: If bitmap param, update parent, then update value and send it */
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
    /* TODO: Determine if param has children, process them all separately */
    int min = blofeld_params[parnum].l_ch.limits->min;
    int max = blofeld_params[parnum].l_ch.limits->max;
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
    if (!bm_param->parent_param->l_ch.child) {
      bm_param->parent_param->l_ch.child = param;
      printf("Param %s has first child %s\n", param->bm_param->parent_param->name, param->bm_param->parent_param->l_ch.child->name);
    }
  }


  midi_register_sysex(SYSEX_ID_WALDORF, blofeld_sysex, BLOFELD_PARAMS + 10);
}

void blofeld_register_notify_cb(blofeld_notify_cb cb, void *ref)
{
  notify_ui = cb;
  notify_ref = ref;
}
