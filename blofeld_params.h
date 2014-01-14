#define BLOFELD_PARAMS 383

struct blofeld_param {
  const char *name;
  /* More to come, such as CC number, where applicable */
};

struct blofeld_param blofeld_params[BLOFELD_PARAMS];

/* Find parameter index from parameter name */
int blofeld_find_index(const char *param_name);

/* Set new parameter value in parameter list, and send to Blofeld */
void blofeld_update_parameter(int parnum, int parlist, int value);

/* Fetch parameter value from parameter list */
int blofeld_fetch_parameter(int parnum, int parlist);
