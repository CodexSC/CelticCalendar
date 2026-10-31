#include "astronomy.h"
#include "calendar.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

#define PI 3.14159265358979323846
#define DEG (PI / 180.0)

/* Default location: Coligny, France (46.38°N, 5.35°E) */
#define DEFAULT_LATITUDE 46.38
#define DEFAULT_LONGITUDE 5.35

/*
 * ============================================================
 * TRUE LUNAR PHASES
 * A mean 29.53-day cycle drifts up to ±14 hours from the real
 * Moon, which moves a new or full moon onto the wrong day about
 * half the time. These use the true phase instants instead
 * (Meeus, Astronomical Algorithms, ch. 49, accurate to minutes).
 * ============================================================
 */

/*
 * ΔT = TT - UT in seconds: the Earth's rotation runs irregularly behind
 * atomic time (Espenak & Meeus 2006 polynomials). Their 2005-2050 forecast
 * ran high, so that span follows the measured values (~64.7 s in 2005 to
 * ~69.2 s since 2020). Uncertain by hours in deep antiquity.
 */
static double delta_t_seconds(double y)
{
    double u, t;
    if (y < -500) {
        u = (y - 1820) / 100;
        return -20 + 32 * u * u;
    }
    if (y < 500) {
        u = y / 100;
        return 10583.6 - 1014.41 * u + 33.78311 * u * u - 5.952053 * u * u * u
             - 0.1798452 * pow(u, 4) + 0.022174192 * pow(u, 5) + 0.0090316521 * pow(u, 6);
    }
    if (y < 1600) {
        u = (y - 1000) / 100;
        return 1574.2 - 556.01 * u + 71.23472 * u * u + 0.319781 * u * u * u
             - 0.8503463 * pow(u, 4) - 0.005050998 * pow(u, 5) + 0.0083572073 * pow(u, 6);
    }
    if (y < 1700) {
        t = y - 1600;
        return 120 - 0.9808 * t - 0.01532 * t * t + t * t * t / 7129;
    }
    if (y < 1800) {
        t = y - 1700;
        return 8.83 + 0.1603 * t - 0.0059285 * t * t + 0.00013336 * t * t * t - pow(t, 4) / 1174000;
    }
    if (y < 1860) {
        t = y - 1800;
        return 13.72 - 0.332447 * t + 0.0068612 * t * t + 0.0041116 * t * t * t - 0.00037436 * pow(t, 4)
             + 0.0000121272 * pow(t, 5) - 0.0000001699 * pow(t, 6) + 0.000000000875 * pow(t, 7);
    }
    if (y < 1900) {
        t = y - 1860;
        return 7.62 + 0.5737 * t - 0.251754 * t * t + 0.01680668 * t * t * t
             - 0.0004473624 * pow(t, 4) + pow(t, 5) / 233174;
    }
    if (y < 1920) {
        t = y - 1900;
        return -2.79 + 1.494119 * t - 0.0598939 * t * t + 0.0061966 * t * t * t - 0.000197 * pow(t, 4);
    }
    if (y < 1941) {
        t = y - 1920;
        return 21.20 + 0.84493 * t - 0.076100 * t * t + 0.0020936 * t * t * t;
    }
    if (y < 1961) {
        t = y - 1950;
        return 29.07 + 0.407 * t - t * t / 233 + t * t * t / 2547;
    }
    if (y < 1986) {
        t = y - 1975;
        return 45.45 + 1.067 * t - t * t / 260 - t * t * t / 718;
    }
    if (y < 2005) {
        t = y - 2000;
        return 63.86 + 0.3345 * t - 0.060374 * t * t + 0.0017275 * t * t * t
             + 0.000651814 * pow(t, 4) + 0.00002373599 * pow(t, 5);
    }
    if (y < 2020) return 64.69 + 0.30 * (y - 2005);     /* measured */
    if (y < 2050) return 69.2;                           /* measured since 2020 */
    u = (y - 1820) / 100;
    if (y < 2150) return -20 + 32 * u * u - 0.5628 * (2150 - y);
    return -20 + 32 * u * u;
}

/* Decimal year of a Julian Day, for ΔT */
static double decimal_year(double jd)
{
    return 2000.0 + (jd - 2451545.0) / 365.25;
}

/* Mean new moon of 2000-01-06 (lunation k = 0) and mean synodic month */
#define NEW_MOON_EPOCH 2451550.09766
#define SYNODIC_MEAN   29.530588861

/*
 * Instant (Julian Day, UT) of a true lunar phase.
 * k counts lunations from the 2000-01-06 new moon;
 * quarter: 0=new, 1=first quarter, 2=full, 3=last quarter
 */
static double true_phase_jd(long lunation, int quarter)
{
    double k = lunation + quarter / 4.0;
    double T = k / 1236.85;
    double jde = NEW_MOON_EPOCH + SYNODIC_MEAN * k
               + 0.00015437 * T * T - 0.00000015 * T * T * T
               + 0.00000000073 * T * T * T * T;

    double E  = 1.0 - 0.002516 * T - 0.0000074 * T * T;
    double M  = (2.5534   + 29.10535670  * k - 0.0000014 * T * T) * DEG;  /* Sun's mean anomaly */
    double Mp = (201.5643 + 385.81693528 * k + 0.0107582 * T * T) * DEG;  /* Moon's mean anomaly */
    double F  = (160.7108 + 390.67050284 * k - 0.0016118 * T * T) * DEG;  /* Moon's argument of latitude */
    double O  = (124.7746 - 1.56375588   * k + 0.0020672 * T * T) * DEG;  /* Ascending node */
    double c;

    if (quarter == 0 || quarter == 2) {
        int full = (quarter == 2);
        c = (full ? -0.40614 : -0.40720) * sin(Mp)
          + (full ?  0.17302 :  0.17241) * E * sin(M)
          + (full ?  0.01614 :  0.01608) * sin(2 * Mp)
          + (full ?  0.01043 :  0.01039) * sin(2 * F)
          + (full ?  0.00734 :  0.00739) * E * sin(Mp - M)
          + (full ? -0.00515 : -0.00514) * E * sin(Mp + M)
          + (full ?  0.00209 :  0.00208) * E * E * sin(2 * M)
          - 0.00111 * sin(Mp - 2 * F)
          - 0.00057 * sin(Mp + 2 * F)
          + 0.00056 * E * sin(2 * Mp + M)
          - 0.00042 * sin(3 * Mp)
          + 0.00042 * E * sin(M + 2 * F)
          + 0.00038 * E * sin(M - 2 * F)
          - 0.00024 * E * sin(2 * Mp - M)
          - 0.00017 * sin(O);
    } else {
        c = -0.62801 * sin(Mp)
          + 0.17172 * E * sin(M)
          - 0.01183 * E * sin(Mp + M)
          + 0.00862 * sin(2 * Mp)
          + 0.00804 * sin(2 * F)
          + 0.00454 * E * sin(Mp - M)
          + 0.00204 * E * E * sin(2 * M)
          - 0.00180 * sin(Mp - 2 * F)
          - 0.00070 * sin(Mp + 2 * F)
          - 0.00040 * sin(3 * Mp)
          - 0.00034 * E * sin(2 * Mp - M)
          + 0.00032 * E * sin(M + 2 * F)
          + 0.00032 * E * sin(M - 2 * F)
          - 0.00028 * E * E * sin(Mp + 2 * M)
          + 0.00027 * E * sin(2 * Mp + M)
          - 0.00017 * sin(O);
        /* Quarter adjustment: added at first quarter, subtracted at last */
        double W = 0.00306 - 0.00038 * E * cos(M) + 0.00026 * cos(Mp)
                 - 0.00002 * cos(Mp - M) + 0.00002 * cos(Mp + M) + 0.00002 * cos(2 * F);
        c += (quarter == 1) ? W : -W;
    }

    jde += c;
    return jde - delta_t_seconds(decimal_year(jde)) / 86400.0;  /* TT -> UT */
}

/*
 * Celtic day on which an instant falls at Coligny.
 * The day begins at sunset, so a phase after sunset belongs to the next day.
 */
static long celtic_day_of_instant(double jd_ut)
{
    /* Local solar time, counted in days from the civil midnight */
    double local = jd_ut + 0.5 + DEFAULT_LONGITUDE / 360.0;
    long day = (long)floor(local);              /* civil date (JD at noon) */
    double hour = (local - day) * 24.0;
    return celtic_jd_from_time(day, hour, DEFAULT_LATITUDE);
}

/* Celtic day of a given phase of a given lunation */
static long phase_day(long lunation, int quarter)
{
    return celtic_day_of_instant(true_phase_jd(lunation, quarter));
}

/* Latest lunation whose given phase falls on or before jd */
static long lunation_on_or_before(long jd, int quarter)
{
    long k = (long)floor((jd - NEW_MOON_EPOCH) / SYNODIC_MEAN - quarter / 4.0) + 1;
    while (phase_day(k, quarter) > jd) k--;
    return k;
}

/* Moon phases (8-step): 0=new, 1=waxing crescent, 2=first quarter, 3=waxing gibbous,
 * 4=full, 5=waning gibbous, 6=last quarter, 7=waning crescent. Each primary phase
 * is shown on exactly one day: the Celtic day on which it really occurs. */
int moon_phase(long jd)
{
    long k = lunation_on_or_before(jd, 0);   /* lunation of the last new moon */
    if (jd == phase_day(k, 0)) return 0;

    for (int q = 1; q <= 3; q++) {
        long day = phase_day(k, q);
        if (jd < day)  return 2 * q - 1;     /* between primaries */
        if (jd == day) return 2 * q;         /* quarter or full */
    }
    return 7;                                /* after last quarter, before next new */
}

/*
 * Sun's apparent ecliptic longitude at an instant (Julian Day, fractional)
 * Meeus, Astronomical Algorithms, ch. 25: accurate to ~0.01° (~15 minutes)
 */
static double sun_apparent_longitude(double jd)
{
    jd += delta_t_seconds(decimal_year(jd)) / 86400.0;        /* UT -> TT */
    double T  = (jd - 2451545.0) / 36525.0;                   /* centuries since J2000 */
    double L0 = 280.46646 + 36000.76983 * T + 0.0003032 * T * T; /* mean longitude */
    double M  = (357.52911 + 35999.05029 * T - 0.0001537 * T * T) * DEG; /* mean anomaly */
    double C  = (1.914602 - 0.004817 * T - 0.000014 * T * T) * sin(M)    /* equation of centre */
              + (0.019993 - 0.000101 * T) * sin(2 * M)
              + 0.000289 * sin(3 * M);
    double omega = (125.04 - 1934.136 * T) * DEG;             /* Moon's node, for nutation */

    double lambda = L0 + C - 0.00569 - 0.00478 * sin(omega);  /* aberration + nutation */
    lambda = fmod(lambda, 360.0);
    if (lambda < 0) lambda += 360.0;
    return lambda;
}

/* Instant (JD, UT) at which the Sun reaches a longitude, the crossing nearest jd */
static double sun_reaches_jd(long jd, double target_longitude)
{
    double t = jd;
    for (int i = 0; i < 6; i++) {
        double diff = fmod(target_longitude - sun_apparent_longitude(t) + 540.0, 360.0) - 180.0;
        t += diff / 0.98564736;   /* Sun's mean motion, degrees per day */
    }
    return t;
}

/*
 * Sun's ecliptic longitude (tropical zodiac)
 * Reference: Vernal Equinox (Sun at 0° Aries) occurs around March 20
 */
int sun_sign(long jd)
{
    /* Convert to zodiac sign (0=Aries, 1=Taurus, ... 11=Pisces) */
    return (int)(sun_apparent_longitude(jd) / 30.0);
}

/*
 * Moon's ecliptic longitude (tropical zodiac)
 * Mean longitude plus the six largest periodic terms, accurate to ~0.3°
 * (the mean longitude alone can be ~6° off, enough to change sign)
 */
int moon_sign(long jd)
{
    /* Days since J2000.0 */
    double d = jd - 2451545.0;

    double L  = 218.316 + 13.176396 * d;            /* Moon's mean longitude */
    double Mp = (134.963 + 13.064993 * d) * DEG;    /* Moon's mean anomaly */
    double M  = (357.529 + 0.98560028 * d) * DEG;   /* Sun's mean anomaly */
    double D  = (297.850 + 12.190749 * d) * DEG;    /* Mean elongation from the Sun */
    double F  = (93.272 + 13.229350 * d) * DEG;     /* Argument of latitude */

    double lon = L + 6.289 * sin(Mp)                /* equation of the centre */
                   + 1.274 * sin(2 * D - Mp)        /* evection */
                   + 0.658 * sin(2 * D)             /* variation */
                   + 0.214 * sin(2 * Mp)
                   - 0.186 * sin(M)                 /* annual equation */
                   - 0.114 * sin(2 * F);
    lon = fmod(lon, 360.0);
    if (lon < 0) lon += 360.0;

    /* Convert to zodiac sign */
    return (int)(lon / 30.0);
}

/*
 * Calculate approximate ecliptic longitude of the Sun
 * Returns degrees (0-360)
 */
double sun_longitude(long jd)
{
    return sun_apparent_longitude(jd);
}

/*
 * Find the JD of the most recent full moon before or on given JD
 */
long find_full_moon_before(long jd)
{
    return phase_day(lunation_on_or_before(jd, 2), 2);
}

/*
 * Months and years begin on the sixth day of the moon, counting the day of
 * the new moon as the first (Pliny, Natural History 16.95: the sixth day of
 * the moon "makes for them the beginnings of months and years", when the
 * Moon has strength yet is not half full). Day 1 thus falls at a moon age
 * of about 5 days, the full moon on day 10-11 and the next new moon ~day 25.
 */
#define PLINY_MOON_DAY 6

/* Day 1 of the month of this lunation: the sixth day after its new moon */
static long month_start_of_lunation(long lunation)
{
    return phase_day(lunation, 0) + (PLINY_MOON_DAY - 1);
}

/* Lunation whose month contains jd */
static long month_lunation(long jd)
{
    return lunation_on_or_before(jd - (PLINY_MOON_DAY - 1), 0);
}

/*
 * Find the JD of day 1 of the lunar month containing jd
 */
long find_lunar_month_start(long jd)
{
    return month_start_of_lunation(month_lunation(jd));
}

/*
 * Get the day within the current lunar month (1-30)
 * Day 1 = sixth day of the moon; full moon on day 10-11
 */
int lunar_day_of_month(long jd)
{
    long month_start = find_lunar_month_start(jd);
    return (int)(jd - month_start) + 1;
}

/*
 * Get the length of the current lunar month (29 or 30 days)
 * Based on actual time to next full moon
 */
int lunar_month_length(long jd)
{
    long k = month_lunation(jd);
    int length = (int)(month_start_of_lunation(k + 1) - month_start_of_lunation(k));
    return (length >= 30) ? 30 : 29;
}

/*
 * Samonios for a given Gregorian year: the month in which the Sun reaches
 * 225° (Samhain), so Samhain always falls within Samonios. The Celtic year
 * begins on its day 1 (the sixth day of the moon) and lasts until the next
 * Samonios, so a year holds 12 or 13 lunations (354 or 384 days), as on
 * the tablet.
 */
static long samonios_lunation(int greg_year)
{
    /* Day the Sun reaches 225°; the Gregorian date only opens the search */
    long jd_samhain = jd_from_ymd(greg_year, 10, 20);
    while (days_to_true_samhain(jd_samhain) > 0) jd_samhain++;

    /* The lunar month containing that day */
    return month_lunation(jd_samhain);
}

/* Day 1 of Samonios, which is New Year's day */
long find_samonios_start(int greg_year)
{
    return month_start_of_lunation(samonios_lunation(greg_year));
}

/*
 * Get the lunar Celtic month index (0-11, or 12/13 for the intercalary months)
 * Based on counting lunations from Samonios start (full moon near Samhain)
 */
int lunar_celtic_month_index(long jd)
{
    /* Determine which Gregorian year's Samhain we're relative to.
     * Samonios can start as early as October, so compare against this
     * year's Samonios start rather than the Gregorian month. */

    /* Find Samonios start for this Celtic year */
    int samhain_year = civil_year_from_jd(jd);
    long jd_samonios = find_samonios_start(samhain_year);

    /* If we're before this year's Samonios, use previous year */
    if (jd < jd_samonios) {
        samhain_year--;
        jd_samonios = find_samonios_start(samhain_year);
    }

    /* Count lunations since Samonios start */
    long jd_current_month = find_lunar_month_start(jd);
    int month_count = 0;
    long jd_check = jd_samonios;

    while (jd_check < jd_current_month) {
        jd_check = find_lunar_month_start(jd_check + 32);  /* Jump to next month start */
        month_count++;
        if (month_count > 13) break;  /* Safety limit */
    }

    /* Giamonios is the month in which the Sun reaches 45° (Beltane), as
     * Samonios is the month of 225°. When it comes seven months after
     * Samonios, the extra month (Rantaranos) precedes it; otherwise a
     * 13-month year ends with Quimonios, before the next Samonios. */
    long jd_beltane = jd_samonios + 160;
    while (days_to_true_beltane(jd_beltane) > 0) jd_beltane++;
    long jd_giamonios = find_lunar_month_start(jd_beltane);
    int giamonios_count = 0;
    for (jd_check = jd_samonios; jd_check < jd_giamonios; giamonios_count++)
        jd_check = find_lunar_month_start(jd_check + 32);
    if (giamonios_count == 7) {
        if (month_count == 6) return 13;     /* intercalary (Rantaranos) */
        if (month_count > 6) month_count--;
    }

    /* Patch: shift so 0 = Giamonios, not Samonios */
    int shifted = (month_count + 6) % 12; // 0=GIA, 6=SAM
    return (month_count > 11) ? 12 : shifted;  /* 12 = intercalary (Quimonios) */
}

/*
 * ============================================================
 * SUNSET CALCULATIONS
 * The Celtic day begins at sunset, not midnight.
 * "For two divisions were formerly on the year... and the night
 *  in each case precedes the day." - Cormac's Glossary
 * ============================================================
 */

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

    /* Equation of time: the true Sun runs up to ~16 minutes ahead of or
     * behind the mean Sun, so solar noon is not exactly 12:00 */
    double alpha = atan2(cos(epsilon_rad) * sin(lambda_rad), cos(lambda_rad)) * 180.0 / PI;
    double eot_deg = fmod(L - alpha + 540.0, 360.0) - 180.0;   /* -180..180 */
    double solar_noon = 12.0 - eot_deg / 15.0;

    /* Sunset time = solar noon + hour angle (local mean solar time) */
    double sunset_hour = solar_noon + H;

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
 * Current date and hour in local mean solar time at Coligny, the clock in
 * which sunset is computed (UTC + longitude; independent of time zone/DST)
 */
void solar_time_now(long *jd, double *hour)
{
    time_t t = time(NULL);
    struct tm utc = *gmtime(&t);
    *jd = jd_from_ymd(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday);
    *hour = utc.tm_hour + utc.tm_min / 60.0 + DEFAULT_LONGITUDE / 15.0;
    if (*hour >= 24.0) {
        *hour -= 24.0;
        (*jd)++;
    }
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
 * Returns: days until sun reaches target longitude (can be negative if past).
 * The exact crossing is placed on its Celtic day (from sunset), like the
 * lunar phases.
 */
int days_to_solar_longitude(long jd, double target_longitude)
{
    return (int)(celtic_day_of_instant(sun_reaches_jd(jd, target_longitude)) - jd);
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
    /* The full moon of Samonios, the month of the Sun at 225° */
    return phase_day(samonios_lunation(greg_year), 2);
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
            if (phase == 4) return 1;  /* Full Moon */
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
    int greg_year = civil_year_from_jd(jd);

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
