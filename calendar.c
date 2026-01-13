#include "calendar.h"
#include <time.h>

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

#define ANCHOR_JD 2460981L      /* Nov 1, 2025 = JD 2460981 */
#define ANCHOR_YEAR 5127        /* Celtic Year 5127 starts Nov 1, 2025 (Kali Yuga aligned) */
#define ANCHOR_CYCLE_POS 0      /* Year 5127 is at position 0 in the 5-year cycle */

#define CYCLE_YEARS 5           /* 5-year Coligny cycle */
#define CYCLE_DAYS 1830         /* Days in 5-year cycle */
#define NORMAL_YEAR 354         /* Days in normal year (no intercalary) */
#define INTERCALARY_YEAR 384    /* Days in year with intercalary month */

#define AGE_YEARS 31            /* Years per Age (Saturnian cycle) */
#define AGE_OFFSET (-16)        /* Offset to align age calculation */

/* Cumulative days at start of each month */
static const int month_start[12] = {0, 30, 59, 89, 118, 148, 178, 207, 237, 266, 295, 325};

/* Days in each year of the 5-year cycle (1-indexed in cycle) */
/* Years 1 and 3 have intercalary months */
static const int year_days_in_cycle[5] = {384, 354, 384, 354, 354};

/* Cumulative days at start of each year in cycle */
static const int cycle_year_start[5] = {0, 384, 738, 1122, 1476};

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
    long delta = jd - ANCHOR_JD;

    /* Calculate full 5-year cycles from anchor */
    int full_cycles = (int)(delta / CYCLE_DAYS);
    int remaining_days = (int)(delta % CYCLE_DAYS);

    if (remaining_days < 0) {
        full_cycles--;
        remaining_days += CYCLE_DAYS;
    }

    /* Find year within cycle */
    int year_in_cycle = 0;
    for (int i = 0; i < 5; i++) {
        if (remaining_days < cycle_year_start[i] + year_days_in_cycle[i]) {
            year_in_cycle = i;
            break;
        }
    }

    return ANCHOR_YEAR + full_cycles * CYCLE_YEARS + year_in_cycle;
}

/*
 * Calculate day of year (1-354 or 1-384 for intercalary years)
 */
int day_of_year(long jd)
{
    long delta = jd - ANCHOR_JD;

    int full_cycles = (int)(delta / CYCLE_DAYS);
    int remaining_days = (int)(delta % CYCLE_DAYS);

    if (remaining_days < 0) {
        full_cycles--;
        remaining_days += CYCLE_DAYS;
    }

    /* Find day within current year */
    for (int i = 0; i < 5; i++) {
        if (remaining_days < cycle_year_start[i] + year_days_in_cycle[i]) {
            return remaining_days - cycle_year_start[i] + 1;
        }
    }

    return remaining_days - cycle_year_start[4] + 1;
}

/*
 * Helper: get year_in_cycle (0-4) from Celtic year number
 * Uses ANCHOR_CYCLE_POS to correctly align cycle position
 */
static int year_in_cycle_from_year(int year)
{
    int delta = year - ANCHOR_YEAR;
    int pos = (ANCHOR_CYCLE_POS + (delta % CYCLE_YEARS) + CYCLE_YEARS) % CYCLE_YEARS;
    return pos;
}

/*
 * Helper: get year_in_cycle (0-4) directly from JD
 */
static int year_in_cycle_from_jd(long jd)
{
    long delta = jd - ANCHOR_JD;
    int remaining_days = (int)(delta % CYCLE_DAYS);

    if (remaining_days < 0) {
        remaining_days += CYCLE_DAYS;
    }

    for (int i = 0; i < 5; i++) {
        if (remaining_days < cycle_year_start[i] + year_days_in_cycle[i]) {
            return i;
        }
    }
    return 4;  /* Last year of cycle */
}

/*
 * Get the length of the current Celtic year
 */
int current_year_length(long jd)
{
    int yic = year_in_cycle_from_jd(jd);
    return year_days_in_cycle[yic];
}

/*
 * Calculate month index (0-11, or 0-12 for intercalary years)
 * For simplicity, we ignore intercalary month and use 0-11
 */
int celtic_month_index(long jd)
{
    int doy = day_of_year(jd);

    /* Handle intercalary month at start of years 1 and 3 in cycle */
    int year_in_cycle = year_in_cycle_from_jd(jd);

    /* Years 0 and 2 in cycle (1st and 3rd years) have intercalary month */
    if (year_in_cycle == 0 || year_in_cycle == 2) {
        if (doy <= 30) {
            return -1;  /* Intercalary month (Quimonios) */
        }
        doy -= 30;  /* Adjust for intercalary month */
    }

    /* Find month from adjusted day of year */
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
    int year_in_cycle = year_in_cycle_from_jd(jd);

    /* Handle intercalary month */
    if (year_in_cycle == 0 || year_in_cycle == 2) {
        if (doy <= 30) {
            return doy;  /* Day in intercalary month */
        }
        doy -= 30;
    }

    int month = celtic_month_index(jd);
    if (month < 0) month = 0;
    return doy - month_start[month];
}

long jd_start_of_celtic_month(int year, int month)
{
    /* Calculate JD for start of given Celtic year */
    int delta_years = year - ANCHOR_YEAR;
    int full_cycles = delta_years / CYCLE_YEARS;
    int rem = delta_years % CYCLE_YEARS;
    if (rem < 0) {
        full_cycles--;
        rem += CYCLE_YEARS;
    }

    long jd = ANCHOR_JD + (long)full_cycles * CYCLE_DAYS;

    /* Add days for years within the current cycle */
    for (int i = 0; i < rem; i++) {
        int pos = (ANCHOR_CYCLE_POS + i) % CYCLE_YEARS;
        jd += year_days_in_cycle[pos];
    }

    /* Get position of target year in cycle using consistent helper */
    int year_in_cycle = year_in_cycle_from_year(year);

    /* Add intercalary month days if this year has one */
    if (year_in_cycle == 0 || year_in_cycle == 2) {
        jd += 30;  /* Skip intercalary month at start of year */
    }

    /* Add days for months 0 to month-1 */
    if (month > 0 && month < 12) {
        jd += month_start[month];
    }

    return jd;
}

/*
 * Get JD for start of a Celtic year (day 1, including intercalary if present)
 */
long jd_start_of_celtic_year(int year)
{
    int delta_years = year - ANCHOR_YEAR;
    int full_cycles = delta_years / CYCLE_YEARS;
    int rem = delta_years % CYCLE_YEARS;
    if (rem < 0) {
        full_cycles--;
        rem += CYCLE_YEARS;
    }

    long jd = ANCHOR_JD + (long)full_cycles * CYCLE_DAYS;

    /* Add days for years within the current cycle */
    for (int i = 0; i < rem; i++) {
        int pos = (ANCHOR_CYCLE_POS + i) % CYCLE_YEARS;
        jd += year_days_in_cycle[pos];
    }

    return jd;
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
    /* MAT months in new order: GIA, SIM, EQU, ELE, AED, CAN, SAM, DUM, RIV, ANA, OGR, CUT */
    /* Original MAT: SAM, RIV, OGR, CUT, SIM, AED (old idx: 0,2,4,5,7,10) */
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
    /* Month lengths in new order: GIA, SIM, EQU, ELE, AED, CAN, SAM, DUM, RIV, ANA, OGR, CUT */
    /* Original: 30(SAM),29(DUM),30(RIV),29(ANA),30(OGR),30(CUT),29(GIA),30(SIM),29(EQU),29(ELE),30(AED),29(CAN) */
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
 * Second half (16-30): Every odd day EXCEPT day 16 (1a in Celtic counting)
 * (Day 1 considered neither odd nor even in Celtic tradition)
 */
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

