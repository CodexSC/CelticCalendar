#ifndef GLYPHS_H
#define GLYPHS_H

#include "calendar.h"
#include "astronomy.h"

/* Print a Celtic month with festivals, moon phases, and zodiac signs */
void print_celtic_month(int month_index, long jd_start, long jd_today);

/* Print a lunar-synced Celtic month (starts at full moon) */
void print_celtic_month_lunar(int month_index, long jd_start, long jd_celtic, long jd_actual, int month_days, int after_sunset);

#endif
