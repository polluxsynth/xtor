#include <string.h>
#include "blofeld_params.h"

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
  { "Osc 1 Shape amount" },
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
  { "Osc 2 Shape amount" },
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
  { "Osc 3 Shape amount" },
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
