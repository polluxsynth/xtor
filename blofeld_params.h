#define BLOFELD_PARAMS 383

struct blofeld_param {
  const char *name;
  /* More to come, such as CC number, where applicable */
};

struct blofeld_param blofeld_params[BLOFELD_PARAMS];

/* Find parameter index from parameter name */
int blofeld_find_index(const char *param_name);