#ifndef __DUMP_SER_ABS_FILE__
#define __DUMP_SER_ABS_FILE__

void abs_filename_append_to( const char *label);
void abs_filename_remove_from( const char *label);
const char *abs_filename_value();
long abs_filename_strlen();
void abs_filename_flush();

#endif
