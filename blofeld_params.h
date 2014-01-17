#define BLOFELD_PARAMS 383

/* Initialize internal structures */
void blofeld_init(void);

/* Register callback for parameter updates */
typedef void (*blofeld_notify_cb)(int parnum, int parlist, int val, void *);

void blofeld_register_notify_cb(blofeld_notify_cb cb, void *ref);

/* Find parameter index from parameter name */
int blofeld_find_index(const char *param_name);

/* Get min and max bounds for parameters, respectively */
int blofeld_get_min(int param_num);
int blofeld_get_max(int param_num);

/* Set new parameter value in parameter list, and notify synth */
void blofeld_update_param(int parnum, int parlist, int value);

/* Fetch parameter value from parameter list */
int blofeld_fetch_parameter(int parnum, int parlist);
