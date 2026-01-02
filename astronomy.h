#ifndef ASTRONOMY_H
#define ASTRONOMY_H

int moon_phase(long jd);
int sun_sign(long jd);
int moon_sign(long jd);
double sun_longitude(long jd);  /* Ecliptic longitude in degrees */

/* Lunar-synced Celtic month functions */
long find_full_moon_before(long jd);
int lunar_day_of_month(long jd);
int lunar_month_length(long jd);
long find_samonios_start(int greg_year);      /* Full moon near Samhain */
int lunar_celtic_month_index(long jd);        /* Month by lunation count */

/* Sunset calculations (Celtic day begins at sunset) */
double calculate_sunset(long jd, double latitude);
int is_after_sunset(long jd, double current_hour, double latitude);
long celtic_jd_from_time(long jd, double current_hour, double latitude);
void get_sunset_time_str(long jd, double latitude, char *buffer, int buf_size);

/* Metonic Cycle (19-year lunisolar synchronization) */
int metonic_year(long jd);           /* Year within cycle (1-19) */
int metonic_lunation(long jd);       /* Lunation within cycle (1-235) */
int metonic_cycle_number(long jd);   /* Total cycles since epoch */
double metonic_drift_hours(long jd); /* Accumulated drift in hours */

/* Pleiades heliacal rising (Samhain marker) */
int days_to_pleiades_rising(long jd);
int is_pleiades_rising(long jd);

/* True cross-quarter days (astronomical midpoints) */
int days_to_solar_longitude(long jd, double target_longitude);
int days_to_true_samhain(long jd);
int days_to_true_imbolc(long jd);
int days_to_true_beltane(long jd);
int days_to_true_lughnasadh(long jd);
int nearest_cross_quarter(long jd, int *days_to_event);

/* Solilunar festivals (Sun position + Moon phase alignment) */
long find_solilunar_samhain(int greg_year);
int is_solilunar_festival(long jd);
int days_to_solilunar_samhain(long jd);

/* Solstices and Equinoxes (quarter days) */
int days_to_yule(long jd);           /* Winter Solstice - 270° */
int days_to_ostara(long jd);         /* Vernal Equinox - 0° */
int days_to_litha(long jd);          /* Summer Solstice - 90° */
int days_to_mabon(long jd);          /* Autumn Equinox - 180° */
int nearest_eightfold_event(long jd, int *days_to_event);

#endif
