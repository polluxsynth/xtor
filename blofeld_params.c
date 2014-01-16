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


struct blofeld_param blofeld_params[BLOFELD_PARAMS] = {
  { "reserved" }, /* 0 */
  { "Osc 1 Octave" },
  { "Osc 1 Semitone" },
  { "Osc 1 Detune" },
  { "Osc 1 Bend Range" },
  { "Osc 1 Keytrack" },
  { "Osc 1 FM Source" },
  { "Osc 1 FM Amount" },
  { "Osc 1 Wave" },
  { "Osc 1 Waveshape" },
  { "Osc 1 Shape Source" },
  { "Osc 1 Shape Amount" },
  { "reserved" },
  { "reserved" },
  { "Osc 1 Limit WT" },
  { "reserved" },
  { "Osc 1 Brilliance" }, /* 16 */
  { "Osc 2 Octave" },
  { "Osc 2 Semitone" },
  { "Osc 2 Detune" },
  { "Osc 2 Bend Range" },
  { "Osc 2 Keytrack" },
  { "Osc 2 FM Source" },
  { "Osc 2 FM Amount" },
  { "Osc 2 Wave" },
  { "Osc 2 Waveshape" },
  { "Osc 2 Shape Source" },
  { "Osc 2 Shape Amount" },
  { "reserved" },
  { "reserved" },
  { "Osc 2 Limit WT" },
  { "reserved" },
  { "Osc 2 Brilliance" }, /* 32 */
  { "Osc 3 Octave" },
  { "Osc 3 Semitone" },
  { "Osc 3 Detune" },
  { "Osc 3 Bend Range" },
  { "Osc 3 Keytrack" },
  { "Osc 3 FM Source" },
  { "Osc 3 FM Amount" },
  { "Osc 3 Wave" },
  { "Osc 3 Waveshape" },
  { "Osc 3 Shape Source" },
  { "Osc 3 Shape Amount" },
  { "reserved" },
  { "reserved" },
  { "Osc 3 Limit WT" },
  { "reserved" },
  { "Osc 3 Brilliance" }, /* 48 */
  { "Osc 2 Sync to Osc 3" },
  { "Osc Pitch Source" },
  { "Osc Pitch Amount" },
  { "reserved" },
  { "Glide" },
  { "reserved" },
  { "reserved" },
  { "Glide Mode" },
  { "Glide Rate" },
  { "Allocation/Unison" },
  { "Unison Detune" },
  { "reserved" },
  { "Mixer Osc 1 Level" },
  { "Mixer Osc 1 Balance" },
  { "Mixer Osc 2 Level" },
  { "Mixer Osc 2 Balance" },
  { "Mixer Osc 3 Level" },
  { "Mixer Osc 3 Balance" },
  { "Mixer Noise Level" },
  { "Mixer Noise Balance" },
  { "Mixer Noise Color" },
  { "reserved" },
  { "Mixer Ringmod Level" },
  { "Mixer Ringmod Balance" },
  { "reserved" },
  { "reserved" },
  { "reserved" },
  { "reserved" },
  { "Filter 1 Type" }, /* 77 */
  { "Filter 1 Cutoff" },
  { "reserved" },
  { "Filter 1 Resonance" },
  { "Filter 1 Drive" },
  { "Filter 1 Drive Curve" },
  { "reserved" },
  { "reserved" },
  { "reserved" },
  { "Filter 1 Keytrack" },
  { "Filter 1 Env Amount" },
  { "Filter 1 Env Velocity" },
  { "Filter 1 Mod Source" },
  { "Filter 1 Mod Amount" },
  { "Filter 1 FM Source" },
  { "Filter 1 FM Amount" },
  { "Filter 1 Pan" },
  { "Filter 1 Pan Source" },
  { "Filter 1 Pan Amount" },
  { "reserved" },
  { "Filter 1 Type" }, /* 97 */
  { "Filter 1 Cutoff" },
  { "reserved" },
  { "Filter 2 Resonance" },
  { "Filter 2 Drive" },
  { "Filter 2 Drive Curve" },
  { "reserved" },
  { "reserved" },
  { "reserved" },
  { "Filter 2 Keytrack" },
  { "Filter 2 Env Amount" },
  { "Filter 2 Env Velocity" },
  { "Filter 2 Mod Source" },
  { "Filter 2 Mod Amount" },
  { "Filter 2 FM Source" },
  { "Filter 2 FM Amount" },
  { "Filter 2 Pan" },
  { "Filter 2 Pan Source" },
  { "Filter 2 Pan Amount" },
  { "reserved" },
  { "Filter Routing" }, /* 117 */
  { "reserved" },
  { "reserved" },
  { "reserved" },
  { "Amplifier Volume" }, /* 121 */
  { "Amplifier Velocity" },
  { "Amplifier Mod Source" },
  { "Amplifier Mod Amount" },
  { "reserved" },
  { "reserved" },
  { "reserved" }
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

  for (i = 0; i < 128 /* TODO: BLOFELD_PARAMS */; i++) {
    if (!strcmp(blofeld_params[i].name, param_name)) {
      idx = i;
      break;
    }
  }
  return idx;
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
void blofeld_update_parameter(int parnum, int parlist, int value, int tell_who)
{
  printf("Blofeld param update: parno %d, value %d, tell_who %d\n", parnum, value, tell_who);
  if (parnum < BLOFELD_PARAMS)
    parameter_list[parnum] = value;
  if (tell_who & BLOFELD_TELL_SYNTH)
    send_parameter_update(parnum, 0, 0, value);
  if (tell_who & BLOFELD_TELL_UI)
    if (notify_ui)
      notify_ui(parnum, parlist, value, notify_ref);
}

#define blofeld_update_ui(parnum, parlist, value) \
        blofeld_update_parameter(parnum, parlist, value, BLOFELD_TELL_UI)

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
      case SNDP: blofeld_update_ui(MIDI_2BYTE(buf[HH], buf[PP]),
                                   buf[LL], buf[XX]);
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
