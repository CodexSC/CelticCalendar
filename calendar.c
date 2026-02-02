#include "calendar.h"
#include "astronomy.h"
#include <time.h>
#include <stdio.h>

/*
 * Celtic Calendar Epoch and Cycle Constants
 *
 * The Coligny calendar uses a 5-year cycle (lustrum):
 * - Normal year: 354 days (6×30 + 6×29)
 * - Years 1 and 3 have a 30-day intercalary month = 384 days
 * - 5-year cycle: 354 + 384 + 354 + 384 + 354 = 1830 days
 *
 * Epoch: Aligned with Kali Yuga (Vedic Tradition)
 * Kali Yuga began February 17/18, 3102 BCE (JD 588465.5)
 * Celtic Year 1 = Samhain 3102 BCE (approximately Nov 3102 BCE)
 * We anchor Year 5127 to Nov 1, 2025 (JD 2460981)
 * This aligns Celtic Year 1 with Kali Yuga Year 1
 */

#define ANCHOR_YEAR 5127        /* Celtic Year 5127 starts at Samhain of 2025 */
#define ANCHOR_SAMHAIN_YEAR 2025/* Gregorian year whose Samhain anchors the Celtic year */

#define AGE_YEARS 31            /* Years per Age (Saturnian cycle) */
#define AGE_OFFSET (-16)        /* Offset to align age calculation */

/*
 * Cumulative days at start of each month (Giamos-first ordering)
 * Order: Giamonios, Simivisonnos, Equos, Elembivios, Aedrinios, Cantlos,
 *        Samonios, Dumannios, Riuros, Anagantios, Ogronnios, Cutios
 * This labels the Samhain start month as Giamonios.
 */
static const int month_start[12] = {0, 29, 59, 88, 117, 147, 176, 206, 235, 265, 294, 324};

/* Legacy cycle constants retained for documentation (unused) */
/* static const int year_days_in_cycle[5] = {384, 354, 384, 354, 354}; */
/* static const int cycle_year_start[5] = {0, 384, 738, 1122, 1476}; */

/* Target solar longitude for Samhain */
#define SAMHAIN_LONG 225.0

/* Find the JD of Samhain (Sun ≈ 225°) for a given Gregorian year */
static long jd_true_samhain_for_year(int greg_year)
{
    long start = jd_from_ymd(greg_year, 10, 15); /* search window Oct 15 */
    double best_diff = 1e9;
    long best_jd = start;
    for (int d = 0; d <= 60; d++) {
        long jd = start + d;
        double diff = sun_longitude(jd) - SAMHAIN_LONG;
        if (diff > 180.0) diff -= 360.0;
        if (diff < -180.0) diff += 360.0;
        double ad = (diff < 0) ? -diff : diff;
        if (ad < best_diff) {
            best_diff = ad;
            best_jd = jd;
        }
    }
    return best_jd;
}

/* Convert JD to Gregorian year (rough, good enough for selecting Samhain year) */
static int gregorian_year_from_jd(long jd)
{
    long z = jd + 1;
    long alpha = (long)((z - 1867216.25) / 36524.25);
    long a = z + 1 + alpha - alpha/4;
    long b = a + 1524;
    long c = (long)((b - 122.1) / 365.25);
    int greg_month = (int)(((b - (long)(365.25 * c)) / 30.6001) < 14 ? ((b - (long)(365.25 * c)) / 30.6001) - 1 : ((b - (long)(365.25 * c)) / 30.6001) - 13);
    int greg_year = (greg_month > 2) ? (int)(c - 4716) : (int)(c - 4715);
    return greg_year;
}

/* Get Samhain boundaries around a JD (solar Samhain ≈ 225°) */
static void samhain_bounds(long jd, long *prev_sam, long *next_sam, int *prev_sam_year)
{
    int gy = gregorian_year_from_jd(jd);
    long this_sam = jd_true_samhain_for_year(gy);
    if (jd < this_sam) {
        *prev_sam_year = gy - 1;
        *prev_sam = jd_true_samhain_for_year(gy - 1);
        *next_sam = this_sam;
    } else {
        *prev_sam_year = gy;
        *prev_sam = this_sam;
        *next_sam = jd_true_samhain_for_year(gy + 1);
    }
}

long jd_from_ymd(int Y, int M, int D)
{
    if (M <= 2) { Y--; M += 12; }
    int A = Y / 100;
    int B = 2 - A + A / 4;
    return (long)(365.25 * (Y + 4716)) +
           (long)(30.6001 * (M + 1)) +
           D + B - 1524;
}

long jd_today(void)
{
    time_t t = time(NULL);
    struct tm *g = gmtime(&t);
    return jd_from_ymd(g->tm_year + 1900, g->tm_mon + 1, g->tm_mday);
}

/*
 * Calculate Celtic year from Julian Day
 * Uses the 5-year cycle for precision
 */
int celtic_year_from_jd(long jd)
{
    long prev_sam, next_sam;
    int prev_year;
    samhain_bounds(jd, &prev_sam, &next_sam, &prev_year);
    int delta_years = prev_year - ANCHOR_SAMHAIN_YEAR;
    return ANCHOR_YEAR + delta_years;
}

/*
 * Calculate day of year (1-354 or 1-384 for intercalary years)
 */
int day_of_year(long jd)
{
    long prev_sam, next_sam;
    int prev_year;
    samhain_bounds(jd, &prev_sam, &next_sam, &prev_year);
    (void)prev_year;
    return (int)(jd - prev_sam) + 1;
}

/*
 * Helper: get year_in_cycle (0-4) from Celtic year number
 * Uses ANCHOR_CYCLE_POS to correctly align cycle position
 */
/* Legacy placeholder; no longer used with Samhain-based year starts */
/*
 * Helper: get year_in_cycle (0-4) directly from JD
 */
/* Legacy placeholder; no longer used with Samhain-based year starts */
/* Cycle helpers removed; Samhain-based year start replaces cycle math */

/*
 * Get the length of the current Celtic year
 */
int current_year_length(long jd)
{
    long prev_sam, next_sam;
    int prev_year;
    samhain_bounds(jd, &prev_sam, &next_sam, &prev_year);
    (void)prev_year;
    return (int)(next_sam - prev_sam);
}

/*
 * Calculate month index (0-11, or 0-12 for intercalary years)
 * For simplicity, we ignore intercalary month and use 0-11
 */
int celtic_month_index(long jd)
{
    int doy = day_of_year(jd);
    for (int m = 11; m >= 0; m--) {
        if (doy > month_start[m]) {
            return m;
        }
    }
    return 0;
}

int day_of_month(long jd)
{
    int doy = day_of_year(jd);
    int month = celtic_month_index(jd);
    return doy - month_start[month];
}

long jd_start_of_celtic_month(int year, int month)
{
    int samhain_year = ANCHOR_SAMHAIN_YEAR + (year - ANCHOR_YEAR);
    long jd_year_start = jd_true_samhain_for_year(samhain_year);
    if (month > 0 && month < 12) {
        jd_year_start += month_start[month];
    }
    return jd_year_start;
}

/*
 * Get JD for start of a Celtic year (day 1, including intercalary if present)
 */
long jd_start_of_celtic_year(int year)
{
    int samhain_year = ANCHOR_SAMHAIN_YEAR + (year - ANCHOR_YEAR);
    return jd_true_samhain_for_year(samhain_year);
}

double elapsed_fraction(long jd)
{
    int day = day_of_year(jd);
    int year_len = current_year_length(jd);
    return (double)(day - 1) / (double)year_len;
}

int days_remaining(long jd)
{
    return current_year_length(jd) - day_of_year(jd);
}

void age_and_year_in_age(long jd, int *age, int *year_in_age)
{
    int year = celtic_year_from_jd(jd);
    int adjusted = year + AGE_OFFSET;
    *age = adjusted / AGE_YEARS;
    *year_in_age = (adjusted - 1) % AGE_YEARS + 1;
}


const char *get_celtic_month_name(int month_index)
{
    static const char *months[] = {
        "Giamonios", "Simivisonnos", "Equos", "Elembivios",
        "Aedrinios", "Cantlos", "Samonios", "Dumannios",
        "Riuros", "Anagantios", "Ogronnios", "Cutios"
    };
    if (month_index == -1) return "Quimonios";  /* Intercalary month */
    if(month_index < 0 || month_index > 11) return "Unknown";
    return months[month_index];
}


const char *get_month_abbrev(int month_index)
{
    static const char *abbrevs[] = {
        "GIA", "SIM", "EQU", "ELE", "AED", "CAN",
        "SAM", "DUM", "RIV", "ANA", "OGR", "CUT"
    };
    if (month_index == -1) return "QUI";  /* Intercalary month */
    if(month_index < 0 || month_index > 11) return "???";
    return abbrevs[month_index];
}

/*
 * MAT (good/auspicious) months: SAM, RIV, OGR, CUT, SIM, AED
 * ANM (not good) months: DUM, ANA, GIA, EQU, ELE, CAN
 * Summer season (SAM-CUT) has 4 MAT, Winter (GIA-CAN) has 2 MAT
 */
int is_mat_month(int month_index)
{
    /* MAT months in Giamos-first order: SIM, AED, SAM, RIV, OGR, CUT */
    static const int mat[] = {0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1};
    if(month_index < 0 || month_index > 11) return 0;
    return mat[month_index];
}

/*
 * Authentic Coligny month lengths:
 * 30 days: SAM, RIV, OGR, CUT, SIM, AED (MAT months)
 * 29 days: DUM, ANA, GIA, ELE, CAN (ANM months)
 * Variable: EQU (29 or 30, we use 29 by default)
 */
int get_month_days(int month_index)
{
    /* Authentic lengths rotated so Giamonios opens the year */
    static const int days[] = {29, 30, 29, 29, 30, 29, 30, 29, 30, 29, 30, 30};
    if(month_index < 0 || month_index > 11) return 30;
    return days[month_index];
}

/*
 * ATENOUX divides month at day 15/16. Days 1-15 are first coicíse,
 * days 16-29/30 are second coicíse (after ATENOUX "renewal")
 */
int is_atenoux(int day_of_month)
{
    return (day_of_month > 15) ? 1 : 0;
}

/*
 * D AMB (D AMBRIX RI) - inauspicious days pattern from Coligny:
 * First half (1-15): Days 5 and 11 only
 * Second half (16-30): Every odd day EXCEPT day 16 (=day 1a) */
int is_d_amb(int day_of_month)
{
    if (day_of_month <= 15) {
        /* First coicíse: only days 5 and 11 are D AMB */
        return (day_of_month == 5 || day_of_month == 11) ? 1 : 0;
    } else {
        /* Second coicíse (after ATENOUX): odd days except 16 (=day 1a) */
        if (day_of_month == 16) return 0;  /* Day 16 (1a) not inauspicious */
        return (day_of_month % 2 == 1) ? 1 : 0;  /* Odd days */
    }
}

