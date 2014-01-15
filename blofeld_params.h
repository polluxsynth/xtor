#define BLOFELD_PARAMS 383

#define BLOFELD_TELL_SYNTH 1
#define BLOFELD_TELL_UI 2

struct blofeld_param {
  const char *name;
  /* More to come, such as CC number, where applicable */
};

struct blofeld_param blofeld_params[BLOFELD_PARAMS];

/* Initialize internal structures */
void blofeld_init(void);

/* Register callback for parameter updates */
typedef void (*blofeld_notify_cb)(void *ref, int parnum, int parlist, int val);

void blofeld_register_notify_cb(void *ref, blofeld_notify_cb cb);

/* Find parameter index from parameter name */
int blofeld_find_index(const char *param_name);

/* Set new parameter value in parameter list, and tell interested party */
#define blofeld_update_param(parnum, parlist, value) \
        blofeld_update_parameter(parnum, parlist, value, BLOFELD_TELL_SYNTH)

void blofeld_update_parameter(int parnum, int parlist, int value, int tell_who);

/* Fetch parameter value from parameter list */
int blofeld_fetch_parameter(int parnum, int parlist);
