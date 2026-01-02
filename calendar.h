#ifndef CALENDAR_H
#define CALENDAR_H

long jd_from_ymd(int Y, int M, int D);
long jd_today(void);

int celtic_year_from_jd(long jd);
int day_of_year(long jd);
int day_of_month(long jd);
int celtic_month_index(long jd);
long jd_start_of_celtic_month(int year, int month);
long jd_start_of_celtic_year(int year);

double elapsed_fraction(long jd);
int days_remaining(long jd);
int current_year_length(long jd);
void age_and_year_in_age(long jd, int *age, int *year_in_age);

const char *get_celtic_month_name(int month_index);

/* Coligny calendar features */
int is_mat_month(int month_index);      /* 1 if MAT (auspicious), 0 if ANM */
int get_month_days(int month_index);    /* Authentic 29/30 day pattern */
int is_atenoux(int day_of_month);       /* 1 if in second half-month */
int is_d_amb(int day_of_month);         /* 1 if D AMB (inauspicious day) */
const char *get_month_abbrev(int month_index);  /* 3-letter abbreviation */

#endif
