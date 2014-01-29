#define BLOFELD_PARAMS 383

struct param_properties {
  int ui_min;  /* user interface minimum */
  int ui_max;  /* user interface maximum */
  int ui_step; /* used interface step size */
};

/* Initialize internal structures; return total number of UI params  */
void blofeld_init(int *params);

/* Register callback for parameter updates */
typedef void (*blofeld_notify_cb)(int parnum, int parlist, void *valptr, void *);

void blofeld_register_notify_cb(blofeld_notify_cb cb, void *ref);

/* Find parameter index from parameter name */
int blofeld_find_index(const char *param_name);

/* Get min and max bounds, and final range for parameters */
/* Returns 0 if properties struct filled in 
 * (i.e. could find parameter and property !NULL), else -1 */
int blofeld_get_param_properties(int param_num, struct param_properties *prop);

/* Fetch parameter dump from Blofeld */
void blofeld_get_dump(int parlist);

/* Set new parameter value in parameter list, and notify synth */
void blofeld_update_param(int parnum, int buf_no, const void *valptr);

/* Fetch parameter value from parameter list */
void *blofeld_fetch_parameter(int parnum, int buf_no);
