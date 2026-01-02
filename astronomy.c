#include "astronomy.h"
#include "calendar.h"
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846

/* Moon phases: 0=new, 1=first quarter, 2=full, 3=last quarter */
int moon_phase(long jd)
{
    /* Reference: New Moon on Jan 6, 2000 at JD 2451550.1 */
    double phase = fmod((jd - 2451550.1) / 29.53058867, 1.0);
    if (phase < 0) phase += 1.0;
    if (phase < 0.125) return 0;      /* New moon */
    else if (phase < 0.375) return 1; /* First quarter (waxing) */
    else if (phase < 0.625) return 2; /* Full moon */
    else if (phase < 0.875) return 3; /* Last quarter (waning) */
    else return 0;                    /* New moon */
}

/*
 * Sun's ecliptic longitude (tropical zodiac)
 * Uses simplified formula accurate to ~1°
 * Reference: Vernal Equinox (Sun at 0° Aries) occurs around March 20
 */
int sun_sign(long jd)
{
    /* Days since J2000.0 epoch (Jan 1, 2000 12:00 TT) */
    double d = jd - 2451545.0;

    /* Mean longitude of the Sun (degrees) */
    double L = fmod(280.460 + 0.9856474 * d, 360.0);
    if (L < 0) L += 360.0;

    /* Mean anomaly of the Sun (degrees) */
    double g = fmod(357.528 + 0.9856003 * d, 360.0);
    if (g < 0) g += 360.0;

    /* Ecliptic longitude (with equation of center correction) */
    double lambda = L + 1.915 * sin(g * PI / 180.0) + 0.020 * sin(2 * g * PI / 180.0);
    lambda = fmod(lambda, 360.0);
    if (lambda < 0) lambda += 360.0;

    /* Convert to zodiac sign (0=Aries, 1=Taurus, ... 11=Pisces) */
    return (int)(lambda / 30.0);
}

/*
 * Moon's ecliptic longitude (tropical zodiac)
 * Simplified formula - Moon moves ~13.2° per day
 * Reference: Known Moon position at J2000.0
 */
int moon_sign(long jd)
{
    /* Days since J2000.0 */
    double d = jd - 2451545.0;

    /* Moon's mean longitude (degrees) */
    /* At J2000.0, Moon was at ~218° (Scorpio) */
    double L = fmod(218.32 + 13.176396 * d, 360.0);
    if (L < 0) L += 360.0;

    /* Convert to zodiac sign */
    return (int)(L / 30.0);
}

/*
 * Calculate approximate ecliptic longitude of the Sun
 * Returns degrees (0-360)
 */
double sun_longitude(long jd)
{
    double d = jd - 2451545.0;
    double L = fmod(280.460 + 0.9856474 * d, 360.0);
    if (L < 0) L += 360.0;
    double g = fmod(357.528 + 0.9856003 * d, 360.0);
    if (g < 0) g += 360.0;
    double lambda = L + 1.915 * sin(g * PI / 180.0) + 0.020 * sin(2 * g * PI / 180.0);
    lambda = fmod(lambda, 360.0);
    if (lambda < 0) lambda += 360.0;
    return lambda;
}

/*
 * Find the JD of the most recent full moon before or on given JD
 * Celtic months begin at the full moon
 */
long find_full_moon_before(long jd)
{
    /* Reference: New Moon on Jan 6, 2000 at JD 2451550.1 */
    double synodic = 29.53058867;
    double new_moon_ref = 2451550.1;

    /* Phase: 0=new, 0.5=full */
    double phase = fmod((jd - new_moon_ref) / synodic, 1.0);
    if (phase < 0) phase += 1.0;

    /* Days since last full moon (full moon is at phase 0.5) */
    double days_since_full;
    if (phase >= 0.5) {
        days_since_full = (phase - 0.5) * synodic;
    } else {
        days_since_full = (phase + 0.5) * synodic;
    }

    return jd - (long)days_since_full;
}

/*
 * Get the day within the current lunar month (1-30)
 * Month starts at full moon, ATENOUX falls at new moon (~day 15)
 */
int lunar_day_of_month(long jd)
{
    long month_start = find_full_moon_before(jd);
    return (int)(jd - month_start) + 1;
}

/*
 * Get the length of the current lunar month (29 or 30 days)
 * Based on actual time to next full moon
 */
int lunar_month_length(long jd)
{
    long this_full = find_full_moon_before(jd);
    long next_full = find_full_moon_before(jd + 30);  /* Find next full moon */
    if (next_full <= this_full) {
        next_full = find_full_moon_before(jd + 35);
    }
    int length = (int)(next_full - this_full);
    return (length >= 30) ? 30 : 29;
}

/*
 * Find the full moon nearest to Samhain (sun at 225°) for a given Gregorian year
 * This marks the start of Samonios and the Celtic year
 */
long find_samonios_start(int greg_year)
{
    /* Find when sun is at 225° (Samhain) - typically Nov 7 */
    long jd_nov1 = jd_from_ymd(greg_year, 11, 1);
    long jd_samhain = jd_nov1;

    for (int d = 0; d < 15; d++) {
        double sun = sun_longitude(jd_nov1 + d);
        if (sun >= 224.5 && sun <= 225.5) {
            jd_samhain = jd_nov1 + d;
            break;
        }
    }

    /* Find the full moon nearest to Samhain (within ~7 days before) */
    /* The full moon before or around Samhain starts Samonios */
    long jd_full = find_full_moon_before(jd_samhain + 3);

    return jd_full;
}

/*
 * Get the lunar Celtic month index (0-11 or 12 for intercalary)
 * Based on counting lunations from Samonios start (full moon near Samhain)
 */
int lunar_celtic_month_index(long jd)
{
    /* Determine which Gregorian year's Samhain we're relative to */
    /* Celtic year starts around Nov, so:
     * - Jan-Oct: use previous year's Samhain
     * - Nov-Dec: check if before/after this year's Samhain
     */

    /* Get approximate Gregorian date from JD */
    long z = jd + 1;
    long alpha = (long)((z - 1867216.25) / 36524.25);
    long a = z + 1 + alpha - alpha/4;
    long b = a + 1524;
    long c = (long)((b - 122.1) / 365.25);
    long dd = (long)(365.25 * c);
    long e = (long)((b - dd) / 30.6001);
    int greg_month = (e < 14) ? e - 1 : e - 13;
    int greg_year = (greg_month > 2) ? c - 4716 : c - 4715;

    /* Find Samonios start for this Celtic year */
    int samhain_year = (greg_month >= 11) ? greg_year : greg_year - 1;
    long jd_samonios = find_samonios_start(samhain_year);

    /* If we're before this year's Samonios, use previous year */
    if (jd < jd_samonios) {
        samhain_year--;
        jd_samonios = find_samonios_start(samhain_year);
    }

    /* Count lunations since Samonios start */
    long jd_current_month = find_full_moon_before(jd);
    int month_count = 0;
    long jd_check = jd_samonios;

    while (jd_check < jd_current_month) {
        jd_check = find_full_moon_before(jd_check + 32);  /* Jump to next full moon */
        month_count++;
        if (month_count > 13) break;  /* Safety limit */
    }

    /* Return month index (0=Samonios, 11=Cantlos, 12=intercalary) */
    return (month_count > 11) ? -1 : month_count;  /* -1 for intercalary */
}

/*
 * ============================================================
 * SUNSET CALCULATIONS
 * The Celtic day begins at sunset, not midnight.
 * "For two divisions were formerly on the year... and the night
 *  in each case precedes the day." - Cormac's Glossary
 * ============================================================
 */

/* Default latitude: Coligny, France (46.38°N) */
#define DEFAULT_LATITUDE 46.38
#define DEFAULT_LONGITUDE 5.35

/*
 * Calculate sunset time for a given Julian Day and latitude
 * Returns hours after midnight (local solar time)
 * Uses simplified sunrise equation
 */
double calculate_sunset(long jd, double latitude)
{
    /* Days since J2000.0 */
    double d = jd - 2451545.0;

    /* Sun's mean anomaly */
    double g = fmod(357.529 + 0.98560028 * d, 360.0);
    if (g < 0) g += 360.0;
    double g_rad = g * PI / 180.0;

    /* Sun's mean longitude */
    double L = fmod(280.459 + 0.98564736 * d, 360.0);
    if (L < 0) L += 360.0;

    /* Ecliptic longitude */
    double lambda = L + 1.915 * sin(g_rad) + 0.020 * sin(2 * g_rad);
    lambda = fmod(lambda, 360.0);
    if (lambda < 0) lambda += 360.0;
    double lambda_rad = lambda * PI / 180.0;

    /* Obliquity of ecliptic */
    double epsilon = 23.439 - 0.0000004 * d;
    double epsilon_rad = epsilon * PI / 180.0;

    /* Solar declination */
    double delta = asin(sin(epsilon_rad) * sin(lambda_rad));

    /* Hour angle at sunset (-0.833° for atmospheric refraction) */
    double lat_rad = latitude * PI / 180.0;
    double cos_H = (sin(-0.833 * PI / 180.0) - sin(lat_rad) * sin(delta))
                   / (cos(lat_rad) * cos(delta));

    /* Clamp for polar regions */
    if (cos_H > 1.0) return 12.0;   /* No sunset - return noon */
    if (cos_H < -1.0) return 24.0;  /* No sunrise - return midnight */

    /* Hour angle in hours */
    double H = acos(cos_H) * 180.0 / PI / 15.0;

    /* Sunset time = solar noon + hour angle */
    /* Solar noon is approximately 12:00 local solar time */
    double sunset_hour = 12.0 + H;

    return sunset_hour;
}

/*
 * Check if current time is after sunset (i.e., Celtic "next day")
 * Returns 1 if after sunset, 0 if before
 */
int is_after_sunset(long jd, double current_hour, double latitude)
{
    double sunset_hour = calculate_sunset(jd, latitude);
    return (current_hour >= sunset_hour) ? 1 : 0;
}

/*
 * Get the Celtic day (accounting for sunset start)
 * If after sunset, we're in the next Celtic day
 */
long celtic_jd_from_time(long jd, double current_hour, double latitude)
{
    if (is_after_sunset(jd, current_hour, latitude)) {
        return jd + 1;  /* After sunset = next Celtic day */
    }
    return jd;
}

/*
 * Get sunset time as hours:minutes string
 */
void get_sunset_time_str(long jd, double latitude, char *buffer, int buf_size)
{
    double hours = calculate_sunset(jd, latitude);
    int h = (int)hours;
    int m = (int)((hours - h) * 60.0);
    snprintf(buffer, buf_size, "%02d:%02d", h, m);
}

/*
 * ============================================================
 * METONIC CYCLE (19-Year Lunisolar Synchronization)
 *
 * The Metonic cycle is the period after which the phases of the
 * Moon recur on the same day of the solar year:
 *   - 19 tropical years ≈ 6939.602 days
 *   - 235 synodic months ≈ 6939.688 days
 *   - Difference: only ~2 hours over 19 years!
 *
 * Reference: First Metonic cycle of our era began at the
 * new moon closest to the vernal equinox of year 1 CE.
 * ============================================================
 */

#define METONIC_YEARS 19
#define METONIC_MONTHS 235
#define METONIC_DAYS 6939.688
#define SYNODIC_MONTH 29.53058867

/* Reference: New Moon near vernal equinox, ~March 23, 1 CE = JD 1721424 */
#define METONIC_EPOCH_JD 1721424L

/*
 * Get the current position in the Metonic cycle
 * Returns: year within cycle (1-19)
 */
int metonic_year(long jd)
{
    double cycles = (jd - METONIC_EPOCH_JD) / METONIC_DAYS;
    double position = fmod(cycles, 1.0);
    if (position < 0) position += 1.0;
    return (int)(position * METONIC_YEARS) + 1;
}

/*
 * Get the current lunation number within the Metonic cycle
 * Returns: lunation (1-235)
 */
int metonic_lunation(long jd)
{
    double cycles = (jd - METONIC_EPOCH_JD) / METONIC_DAYS;
    double position = fmod(cycles, 1.0);
    if (position < 0) position += 1.0;
    return (int)(position * METONIC_MONTHS) + 1;
}

/*
 * Get the total number of Metonic cycles since epoch
 */
int metonic_cycle_number(long jd)
{
    return (int)((jd - METONIC_EPOCH_JD) / METONIC_DAYS) + 1;
}

/*
 * Calculate drift from ideal Metonic alignment (in hours)
 * Positive = ahead of moon, Negative = behind
 */
double metonic_drift_hours(long jd)
{
    int cycle = metonic_cycle_number(jd);
    /* Each cycle drifts by ~0.086 days = ~2.07 hours */
    return cycle * 2.07;
}

/*
 * ============================================================
 * PLEIADES HELIACAL RISING
 *
 * The heliacal rising of the Pleiades (when they first become
 * visible before dawn after being hidden by the sun) was a
 * crucial astronomical marker for many ancient cultures.
 *
 * For the Celts, Samhain may have been timed to coincide with
 * the heliacal rising of the Pleiades in late October/early
 * November.
 *
 * The Pleiades have an ecliptic longitude of ~60° (in Taurus).
 * Heliacal rising occurs when the Sun is ~15-18° below them.
 * ============================================================
 */

#define PLEIADES_LONGITUDE 60.0  /* Ecliptic longitude in degrees */
#define HELIACAL_OFFSET 17.0     /* Sun degrees below for visibility */

/*
 * Check if today is near the heliacal rising of the Pleiades
 * Returns: days until/since heliacal rising (negative = past)
 */
int days_to_pleiades_rising(long jd)
{
    double sun_long = sun_longitude(jd);

    /* Heliacal rising when Sun is ~17° behind Pleiades */
    double rising_sun_long = PLEIADES_LONGITUDE - HELIACAL_OFFSET;
    if (rising_sun_long < 0) rising_sun_long += 360.0;

    /* Calculate angular distance */
    double diff = rising_sun_long - sun_long;
    if (diff > 180.0) diff -= 360.0;
    if (diff < -180.0) diff += 360.0;

    /* Convert to days (sun moves ~1°/day) */
    return (int)(diff);
}

/*
 * Check if Pleiades are currently in heliacal rising period
 * Returns: 1 if within ±3 days of heliacal rising
 */
int is_pleiades_rising(long jd)
{
    int days = days_to_pleiades_rising(jd);
    return (days >= -3 && days <= 3) ? 1 : 0;
}

/*
 * ============================================================
 * TRUE CROSS-QUARTER DAYS
 *
 * The astronomical cross-quarter days are the exact midpoints
 * between solstices and equinoxes:
 *   - Samhain: midpoint between Autumn Equinox & Winter Solstice (~225°)
 *   - Imbolc: midpoint between Winter Solstice & Vernal Equinox (~315°)
 *   - Beltane: midpoint between Vernal Equinox & Summer Solstice (~45°)
 *   - Lughnasadh: midpoint between Summer Solstice & Autumn Equinox (~135°)
 *
 * These differ from the traditional calendar dates by several days.
 * ============================================================
 */

/* Solar longitudes for astronomical events */
#define WINTER_SOLSTICE  270.0
#define VERNAL_EQUINOX   0.0
#define SUMMER_SOLSTICE  90.0
#define AUTUMN_EQUINOX   180.0

/* True cross-quarter points (midpoints between quarter days) */
#define SAMHAIN_LONGITUDE    225.0  /* (180 + 270) / 2 */
#define IMBOLC_LONGITUDE     315.0  /* (270 + 360) / 2 */
#define BELTANE_LONGITUDE    45.0   /* (0 + 90) / 2 */
#define LUGHNASADH_LONGITUDE 135.0  /* (90 + 180) / 2 */

/*
 * Calculate days until a specific solar longitude
 * Returns: days until sun reaches target longitude (can be negative if past)
 */
int days_to_solar_longitude(long jd, double target_longitude)
{
    double sun_long = sun_longitude(jd);

    double diff = target_longitude - sun_long;
    if (diff > 180.0) diff -= 360.0;
    if (diff < -180.0) diff += 360.0;

    /* Sun moves ~0.9856° per day */
    return (int)(diff / 0.9856);
}

/*
 * Get days to next astronomical Samhain (true cross-quarter)
 */
int days_to_true_samhain(long jd)
{
    return days_to_solar_longitude(jd, SAMHAIN_LONGITUDE);
}

/*
 * Get days to next astronomical Imbolc
 */
int days_to_true_imbolc(long jd)
{
    return days_to_solar_longitude(jd, IMBOLC_LONGITUDE);
}

/*
 * Get days to next astronomical Beltane
 */
int days_to_true_beltane(long jd)
{
    return days_to_solar_longitude(jd, BELTANE_LONGITUDE);
}

/*
 * Get days to next astronomical Lughnasadh
 */
int days_to_true_lughnasadh(long jd)
{
    return days_to_solar_longitude(jd, LUGHNASADH_LONGITUDE);
}

/*
 * Get the nearest cross-quarter event and days to it
 * Returns: 0=Samhain, 1=Imbolc, 2=Beltane, 3=Lughnasadh
 */
int nearest_cross_quarter(long jd, int *days_to_event)
{
    int sam = days_to_true_samhain(jd);
    int imb = days_to_true_imbolc(jd);
    int bel = days_to_true_beltane(jd);
    int lug = days_to_true_lughnasadh(jd);

    /* Make all positive (days until) */
    if (sam < 0) sam += 365;
    if (imb < 0) imb += 365;
    if (bel < 0) bel += 365;
    if (lug < 0) lug += 365;

    /* Find minimum */
    int min_days = sam;
    int event = 0;

    if (imb < min_days) { min_days = imb; event = 1; }
    if (bel < min_days) { min_days = bel; event = 2; }
    if (lug < min_days) { min_days = lug; event = 3; }

    *days_to_event = min_days;
    return event;
}

/*
 * ═══════════════════════════════════════════════════════════════════════════
 * SOLILUNAR FESTIVAL CALCULATIONS
 * The Celtic festivals were likely solilunar - combining solar position with
 * lunar phase. The festival occurs when:
 *   - Sun reaches the cross-quarter longitude (e.g., 225° for Samhain)
 *   - AND Moon is at Full or New (or nearest such phase)
 * ═══════════════════════════════════════════════════════════════════════════
 */

/*
 * Find the JD of solilunar Samhain for a given Gregorian year
 * Returns the Full Moon nearest to when Sun is at 225°
 */
long find_solilunar_samhain(int greg_year)
{
    /* Find when sun reaches 225° */
    long jd_nov1 = jd_from_ymd(greg_year, 11, 1);
    long jd_solar = jd_nov1;

    for (int d = 0; d < 20; d++) {
        double sun = sun_longitude(jd_nov1 + d);
        if (sun >= 224.5 && sun <= 225.5) {
            jd_solar = jd_nov1 + d;
            break;
        }
    }

    /* Check if Full Moon or New Moon falls on solar Samhain (perfect alignment) */
    int phase = moon_phase(jd_solar);
    if (phase == 2 || phase == 0) {
        return jd_solar;  /* Perfect solilunar alignment! */
    }

    /* Find Full Moon nearest to solar Samhain */
    long jd_full = find_full_moon_before(jd_solar + 10);

    /* Distance from solar Samhain to full moon */
    int days_to_full = (int)(jd_full - jd_solar);

    /* If full moon is more than 7 days away, check previous full moon */
    if (days_to_full > 7) {
        long jd_prev_full = find_full_moon_before(jd_solar - 1);
        int days_to_prev = (int)(jd_solar - jd_prev_full);
        if (days_to_prev < days_to_full) {
            jd_full = jd_prev_full;
        }
    }

    return jd_full;
}

/*
 * Check if a given JD is a solilunar festival (Sun at cross-quarter + significant moon)
 * Returns: 0=not festival, 1=Full Moon alignment, 2=New Moon alignment
 */
int is_solilunar_festival(long jd)
{
    double sun = sun_longitude(jd);
    int phase = moon_phase(jd);

    /* Check if sun is near any cross-quarter */
    double cross_quarters[] = {225.0, 315.0, 45.0, 135.0};  /* Sam, Imb, Bel, Lug */

    for (int i = 0; i < 4; i++) {
        double diff = sun - cross_quarters[i];
        if (diff > 180) diff -= 360;
        if (diff < -180) diff += 360;

        if (diff >= -2.0 && diff <= 2.0) {
            /* Sun is at cross-quarter! Check moon */
            if (phase == 2) return 1;  /* Full Moon */
            if (phase == 0) return 2;  /* New Moon */
        }
    }

    return 0;
}

/*
 * Get days to next solilunar Samhain (Full Moon nearest Sun at 225°)
 */
int days_to_solilunar_samhain(long jd)
{
    /* Get current Gregorian year */
    long z = jd + 1;
    long alpha = (long)((z - 1867216.25) / 36524.25);
    long a = z + 1 + alpha - alpha/4;
    long b = a + 1524;
    long c = (long)((b - 122.1) / 365.25);
    long dd = (long)(365.25 * c);
    long e = (long)((b - dd) / 30.6001);
    int greg_month = (e < 14) ? e - 1 : e - 13;
    int greg_year = (greg_month > 2) ? c - 4716 : c - 4715;

    /* Find solilunar Samhain for this year */
    long jd_samhain = find_solilunar_samhain(greg_year);

    /* If already passed, find next year's */
    if (jd_samhain < jd) {
        jd_samhain = find_solilunar_samhain(greg_year + 1);
    }

    return (int)(jd_samhain - jd);
}

/*
 * ═══════════════════════════════════════════════════════════════════════════
 * SOLSTICES AND EQUINOXES (Quarter Days)
 * ═══════════════════════════════════════════════════════════════════════════
 */

/*
 * Days to Winter Solstice (Yule) - Sun at 270°
 */
int days_to_yule(long jd)
{
    return days_to_solar_longitude(jd, WINTER_SOLSTICE);
}

/*
 * Days to Vernal Equinox (Ostara) - Sun at 0°
 */
int days_to_ostara(long jd)
{
    return days_to_solar_longitude(jd, VERNAL_EQUINOX);
}

/*
 * Days to Summer Solstice (Litha) - Sun at 90°
 */
int days_to_litha(long jd)
{
    return days_to_solar_longitude(jd, SUMMER_SOLSTICE);
}

/*
 * Days to Autumn Equinox (Mabon) - Sun at 180°
 */
int days_to_mabon(long jd)
{
    return days_to_solar_longitude(jd, AUTUMN_EQUINOX);
}

/*
 * Get the nearest event from the eight-fold year
 * Returns: 0-7 for the eight festivals
 * 0=Yule, 1=Imbolc, 2=Ostara, 3=Beltane, 4=Litha, 5=Lughnasadh, 6=Mabon, 7=Samhain
 */
int nearest_eightfold_event(long jd, int *days_to_event)
{
    int events[8];
    events[0] = days_to_yule(jd);        /* 270° */
    events[1] = days_to_true_imbolc(jd); /* 315° */
    events[2] = days_to_ostara(jd);      /* 0° */
    events[3] = days_to_true_beltane(jd);/* 45° */
    events[4] = days_to_litha(jd);       /* 90° */
    events[5] = days_to_true_lughnasadh(jd); /* 135° */
    events[6] = days_to_mabon(jd);       /* 180° */
    events[7] = days_to_true_samhain(jd);/* 225° */

    /* Make all positive (days until next occurrence) */
    for (int i = 0; i < 8; i++) {
        if (events[i] < 0) events[i] += 365;
    }

    /* Find minimum */
    int min_days = events[0];
    int nearest = 0;

    for (int i = 1; i < 8; i++) {
        if (events[i] < min_days) {
            min_days = events[i];
            nearest = i;
        }
    }

    *days_to_event = min_days;
    return nearest;
}
