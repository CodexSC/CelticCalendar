#include <stdio.h>
#include "calendar.h"
#include "astronomy.h"
#include "festivals.h"

/* Moon phase glyphs: 0=new, 1=waxing (first quarter), 2=full, 3=waning (last quarter) */
static const char *moon_glyphs[4] = {"🌑", "🌓", "🌕", "🌗"};
static const char *zodiac_glyphs[12] = {"♈","♉","♊","♋","♌","♍","♎","♏","♐","♑","♒","♓"};
static const char *weekday_glyphs[7] = {"☉", "☽", "♂", "☿", "♃", "♀", "♄"};

/*
 * Authentic Coligny Calendar Notations:
 *
 * M D (MATIS DIVERTOMU) = auspicious day (MAT month)
 * D = neutral day (ANM month)
 * D AMB (DIVERTOMU AMBRIX RI) = inauspicious day
 * N = night notation (overwrites day notation)
 * N INIS R = night notation for dark moon days (7a-8a-9a)
 * PRINNI LOUD = "main part large" (full moon triplet in MAT months)
 * PRINNI LAG = "main part small" (full moon triplet in ANM months)
 * IVOS = festival marker
 * SINDIV IVOS = "this day a festival" (exceptional importance)
 * DIVERTOMU = virtual 30th day for 29-day months
 * Triple marks: ƚıı ıƚı ııƚ = daytime division markers
 */

/*
 * Get the Coligny notation for a specific day
 * Returns the full authentic notation string
 */
static const char* get_coligny_notation(int month_index, int day, int is_atenoux_half)
{
    int mat = is_mat_month(month_index);

    /* Days 7-8-9 in first half = PRINNI LOUD/LAG (full moon triplet) */
    if (!is_atenoux_half && day >= 7 && day <= 9) {
        if (day == 7) return mat ? "PRINNI LOUD" : "PRINNI LAG";
        return mat ? "M D" : "D";
    }

    /* Days 7a-8a-9a (22-24) in second half = N INIS R (dark moon nights) */
    if (is_atenoux_half && day >= 22 && day <= 24) {
        return "N INIS R";
    }

    /* D AMB pattern: days 5, 11 in first half; odd days (except 16) in second half */
    if (!is_atenoux_half && (day == 5 || day == 11)) {
        return "D AMB";
    }
    if (is_atenoux_half && day > 16 && (day % 2 == 1)) {
        return "D AMB";
    }

    /* Default: M D for MAT months, D for ANM months */
    return mat ? "M D" : "D";
}

/*
 * Get triple mark for a day (ƚıı, ıƚı, or ııƚ)
 * Triple marks divide daytime into three periods
 * Pattern: triplets over three days, followed by three days with none
 */
static const char* get_triple_mark(int day)
{
    int cycle = (day - 1) % 6;  /* 6-day cycle: 3 marked + 3 unmarked */
    if (cycle < 3) {
        switch (cycle) {
            case 0: return "ƚıı";
            case 1: return "ıƚı";
            case 2: return "ııƚ";
        }
    }
    return "   ";  /* No mark for days 4-6 of each cycle */
}

static char day_marker(int month_index, int day_of_month)
{
    int mat = is_mat_month(month_index);
    int amb = is_d_amb(day_of_month);

    if (amb) return '!';           /* D AMB - inauspicious */
    if (mat) return '*';           /* M D - auspicious (MAT month) */
    return ' ';                    /* D - neutral (ANM month) */
}

/* Get ordinal suffix for a day number */
static const char *ordinal_suffix(int day)
{
    if (day >= 11 && day <= 13) return "th";
    switch (day % 10) {
        case 1: return "st";
        case 2: return "nd";
        case 3: return "rd";
        default: return "th";
    }
}

void print_celtic_month(int month_index, long jd_start, long jd_today)
{
    int today_day = (int)(jd_today - jd_start) + 1;
    int weekday = (int)((jd_today + 1) % 7);  /* 0=Sun, 1=Mon, etc. */
    int today_moon = moon_phase(jd_today);
    int today_moon_zodiac = moon_sign(jd_today);
    int today_sun_zodiac = sun_sign(jd_today);
    int month_days = get_month_days(month_index);
    int mat = is_mat_month(month_index);

    /* Month header with MAT/ANM status */
    printf("+--------------------------------------------------+\n");
    printf("|  %-12s (%s)                                  |\n",
           get_celtic_month_name(month_index),
           get_month_abbrev(month_index));
    printf("|  %s - %d days                          |\n",
           mat ? "Matis (lucky/complete month)    " : "Anmatu (unlucky/incomplete month)",
           month_days);
    printf("+--------------------------------------------------+\n");

    /* Current day astronomical info */
    printf("|  Today: Day %2d - %s - %s%s - Sun %s            |\n",
           today_day,
           weekday_glyphs[weekday],
           moon_glyphs[today_moon],
           zodiac_glyphs[today_moon_zodiac],
           zodiac_glyphs[today_sun_zodiac]);

    /* Show if in ATENOUX (second half-month) */
    if (is_atenoux(today_day)) {
        printf("|  === ATENOUX (Second Coicise) ===                |\n");
    } else {
        printf("|  === First Coicise ===                           |\n");
    }

    printf("+--------------------------------------------------+\n");

    /* Print festivals for this month */
    int has_festival = 0;
    for (int f = 0; f < FESTIVAL_COUNT; f++) {
        if (festivals[f].month == month_index) {
            printf("|  IVOS: %-32s Day %2d |\n",
                   festivals[f].name, festivals[f].day);
            has_festival = 1;
        }
    }
    if (!has_festival) {
        printf("|  (No major festivals this month)                 |\n");
    }
    printf("+--------------------------------------------------+\n");

    /* Legend */
    printf("|  * = Matis Diuertomu (auspicious day)            |\n");
    printf("|  ! = Diuertomu Anmatu (inauspicious day)         |\n");
    printf("+--------------------------------------------------+\n\n");

    /* === FIRST COICÍSE (Days 1-15) === */
    printf("+------+------+------+------+------+\n");
    printf("|   FIRST COICISE (Days I - XV)    |\n");
    printf("+------+------+------+------+------+\n");

    /* First coicíse: days 1-15 in 3 rows of 5 */
    for (int day = 1; day <= 15; day++) {
        long jd = jd_start + day - 1;
        int mp = moon_phase(jd);
        char marker = day_marker(month_index, day);

        /* Check if festival day */
        int is_festival = 0;
        for (int f = 0; f < FESTIVAL_COUNT; f++) {
            if (festivals[f].month == month_index && festivals[f].day == day) {
                is_festival = 1;
                break;
            }
        }

        /* Print table cell - 6 chars: space, 2-digit day, moon, marker, space */
        /* Festival + Quality combined markers:
         * ☆ = Festival + Auspicious (IVOS M D)
         * ⚐ = Festival + Neutral (IVOS D)
         * ⚠ = Festival + Inauspicious (IVOS D AMB)
         */
        if ((day - 1) % 5 == 0) printf("|");

        if (day == today_day)
            if (is_festival && marker == '!')
                printf("[%2d%s⚠]", day, moon_glyphs[mp]);
            else if (is_festival && marker == '*')
                printf("[%2d%s☆]", day, moon_glyphs[mp]);
            else if (is_festival)
                printf("[%2d%s⚐]", day, moon_glyphs[mp]);
            else if (marker == '!')
                printf("[%2d%s!]", day, moon_glyphs[mp]);
            else if (marker == '*')
                printf("[%2d%s*]", day, moon_glyphs[mp]);
            else
                printf("[%2d%s]", day, moon_glyphs[mp]);
        else if (is_festival && marker == '!')
            printf(" %2d%s⚠", day, moon_glyphs[mp]);  /* Festival + Inauspicious */
        else if (is_festival && marker == '*')
            printf(" %2d%s☆", day, moon_glyphs[mp]);  /* Festival + Auspicious */
        else if (is_festival)
            printf(" %2d%s⚐", day, moon_glyphs[mp]);  /* Festival + Neutral */
        else if (marker == '!')
            printf(" %2d%s!", day, moon_glyphs[mp]);
        else if (marker == '*')
            printf(" %2d%s*", day, moon_glyphs[mp]);
        else
            printf(" %2d%s ", day, moon_glyphs[mp]);

        if (day % 5 == 0) {
            printf("|\n");
            if (day < 15)
                printf("+------+------+------+------+------+\n");
        }
    }
    printf("+------+------+------+------+------+\n\n");

    /* === ATENOUX MARKER === */
    printf("      ======== ATENOUX ========\n\n");

    /* === SECOND COICÍSE (Days 16-29/30) === */
    printf("+------+------+------+------+------+\n");
    if (month_days == 30)
        printf("| SECOND COICISE (Days XVI - XXX)  |\n");
    else
        printf("| SECOND COICISE (Days XVI - XXIX) |\n");
    printf("+------+------+------+------+------+\n");

    /* Second coicíse: days 16-29/30 */
    for (int day = 16; day <= month_days; day++) {
        long jd = jd_start + day - 1;
        int mp = moon_phase(jd);
        char marker = day_marker(month_index, day);

        /* Check if festival day */
        int is_festival = 0;
        for (int f = 0; f < FESTIVAL_COUNT; f++) {
            if (festivals[f].month == month_index && festivals[f].day == day) {
                is_festival = 1;
                break;
            }
        }

        /* Print table cell - Festival + Quality combined markers */
        if ((day - 16) % 5 == 0) printf("|");

        if (day == today_day)
            if (is_festival && marker == '!')
                printf("[%2d%s⚠]", day, moon_glyphs[mp]);
            else if (is_festival && marker == '*')
                printf("[%2d%s☆]", day, moon_glyphs[mp]);
            else if (is_festival)
                printf("[%2d%s⚐]", day, moon_glyphs[mp]);
            else if (marker == '!')
                printf("[%2d%s!]", day, moon_glyphs[mp]);
            else if (marker == '*')
                printf("[%2d%s*]", day, moon_glyphs[mp]);
            else
                printf("[%2d%s]", day, moon_glyphs[mp]);
        else if (is_festival && marker == '!')
            printf(" %2d%s⚠", day, moon_glyphs[mp]);  /* Festival + Inauspicious */
        else if (is_festival && marker == '*')
            printf(" %2d%s☆", day, moon_glyphs[mp]);  /* Festival + Auspicious */
        else if (is_festival)
            printf(" %2d%s⚐", day, moon_glyphs[mp]);  /* Festival + Neutral */
        else if (marker == '!')
            printf(" %2d%s!", day, moon_glyphs[mp]);
        else if (marker == '*')
            printf(" %2d%s*", day, moon_glyphs[mp]);
        else
            printf(" %2d%s ", day, moon_glyphs[mp]);

        if ((day - 15) % 5 == 0) {
            printf("|\n");
            if (day < month_days && (month_days - day) >= 5)
                printf("+------+------+------+------+------+\n");
        }
    }

    /* Handle partial last row for 29-day months */
    int remaining = (month_days - 15) % 5;
    if (remaining != 0) {
        for (int i = remaining; i < 5; i++) {
            printf("      ");
        }
        printf("|\n");
    }
    printf("+------+------+------+------+------+\n");
}

/*
 * Print a lunar-synced Celtic month
 * Month starts at full moon, ATENOUX falls at new moon (~day 15)
 * This keeps moon phases aligned with the calendar structure
 * jd_celtic = Celtic day (incremented after sunset)
 * jd_actual = Actual calendar day (for weekday display)
 * after_sunset = 1 if we've crossed into Celtic "next day"
 */
void print_celtic_month_lunar(int month_index, long jd_start, long jd_celtic, long jd_actual, int month_days, int after_sunset)
{
    int today_day = (int)(jd_celtic - jd_start) + 1;
    int gregorian_weekday = (int)((jd_actual + 1) % 7);  /* Gregorian weekday */
    int celtic_weekday = (int)((jd_celtic + 1) % 7);     /* Celtic weekday (after sunset) */
    int today_moon = moon_phase(jd_actual);
    int today_moon_zodiac = moon_sign(jd_actual);
    int today_sun_zodiac = sun_sign(jd_actual);
    int mat = (month_days == 30);  /* 30-day months are MAT */

    /* Month header */
    printf("+--------------------------------------------------+\n");
    printf("|  %-12s (%s)                                  |\n",
           get_celtic_month_name(month_index),
           get_month_abbrev(month_index));
    printf("|  %s - %d days                          |\n",
           mat ? "Matis (lucky/complete month)    " : "Anmatu (unlucky/incomplete month)",
           month_days);
    printf("+--------------------------------------------------+\n");

    /* Current day info - show Celtic weekday with transition if after sunset */
    if (after_sunset && gregorian_weekday != celtic_weekday) {
        /* After sunset: show Celtic weekday with (Greg→Celtic) notation */
        printf("|  Today: Day %2d - %s (%s→%s) - %s%s - Sun %s  |\n",
               today_day,
               weekday_glyphs[celtic_weekday],
               weekday_glyphs[gregorian_weekday],
               weekday_glyphs[celtic_weekday],
               moon_glyphs[today_moon],
               zodiac_glyphs[today_moon_zodiac],
               zodiac_glyphs[today_sun_zodiac]);
    } else {
        printf("|  Today: Day %2d - %s - %s%s - Sun %s            |\n",
               today_day,
               weekday_glyphs[celtic_weekday],
               moon_glyphs[today_moon],
               zodiac_glyphs[today_moon_zodiac],
               zodiac_glyphs[today_sun_zodiac]);
    }

    /* Show coicise position */
    if (today_day > 15) {
        printf("|  === ATENOUX (Second Coicise) ===                |\n");
    } else {
        printf("|  === First Coicise ===                           |\n");
    }
    printf("+--------------------------------------------------+\n");

    /* Single-Day Festivals (old system - kept for reference) */
    int has_festival = 0;
    for (int f = 0; f < FESTIVAL_COUNT; f++) {
        if (festivals[f].month == month_index) {
            printf("|  IVOS: %-32s Day %2d |\n",
                   festivals[f].name, festivals[f].day);
            has_festival = 1;
        }
    }

    /* Multi-Day Festivals (Trinox) - old system */
    for (int f = 0; f < MULTI_FESTIVAL_COUNT; f++) {
        if (multi_festivals[f].month == month_index) {
            printf("|  %s                             |\n",
                   multi_festivals[f].coligny_name);
            printf("|    \"%s\" - Days %d-%d (%d nights)       |\n",
                   multi_festivals[f].name,
                   multi_festivals[f].start_day,
                   multi_festivals[f].start_day + multi_festivals[f].duration - 1,
                   multi_festivals[f].duration);
            has_festival = 1;
        }
    }

    /* ═══════════════════════════════════════════════════════════════
     * ASTRONOMICAL FESTIVALS (Dynamic - based on solar longitude)
     * Check if any of the 8 Wheel of Year festivals fall within
     * the current lunar month (jd_start to jd_start + month_days)
     * ═══════════════════════════════════════════════════════════════ */
    struct {
        const char *name;
        const char *coligny;
        int (*days_func)(long);
        double longitude;
    } astro_festivals[8] = {
        {"Yule (Winter Solstice)", "TRINVX GIAMONI", days_to_yule, 270.0},
        {"Imbolc", "TRINVX IMBOLC", days_to_true_imbolc, 315.0},
        {"Ostara (Vernal Equinox)", "TRINVX OSTARA", days_to_ostara, 0.0},
        {"Beltane", "IVOS BELTAINE", days_to_true_beltane, 45.0},
        {"Litha (Summer Solstice)", "TRINVX LITHA", days_to_litha, 90.0},
        {"Lughnasadh", "TRINVX LUGHNASADH", days_to_true_lughnasadh, 135.0},
        {"Mabon (Autumn Equinox)", "TRINVX MABON", days_to_mabon, 180.0},
        {"Samhain", "TRINVX SAMONI", days_to_true_samhain, 225.0}
    };

    for (int f = 0; f < 8; f++) {
        int days_until = astro_festivals[f].days_func(jd_actual);
        /* Check if festival falls within this month (0 to month_days - today_day) */
        int days_remaining_in_month = month_days - today_day;
        if (days_until >= 0 && days_until <= days_remaining_in_month) {
            int festival_day = today_day + days_until;
            printf("|  ★ %s                    |\n", astro_festivals[f].coligny);
            printf("|    %s on Day %d (in %d days)    |\n",
                   astro_festivals[f].name, festival_day, days_until);
            has_festival = 1;
        }
    }

    if (!has_festival) {
        printf("|  (No major festivals this month)                 |\n");
    }
    printf("+--------------------------------------------------+\n");

    /* Check if today is during a multi-day festival */
    int multi_fest = get_multi_festival(month_index, today_day);
    if (multi_fest >= 0) {
        int fest_day = get_festival_day_number(month_index, today_day);
        printf("╔══════════════════════════════════════════════════╗\n");
        printf("║  🔥 FESTIVAL IN PROGRESS                         ║\n");
        printf("║  %s                              ║\n",
               multi_festivals[multi_fest].coligny_name);
        printf("║  Night %d of %d                                  ║\n",
               fest_day, multi_festivals[multi_fest].duration);
        printf("╚══════════════════════════════════════════════════╝\n");
    }

    /* Legend */
    printf("|  * = Matis Diuertomu (auspicious day)            |\n");
    printf("|  ! = Diuertomu Anmatu (inauspicious day)         |\n");
    printf("|  ☆ = Festival + Auspicious (IVOS M D)            |\n");
    printf("|  ⚐ = Festival + Neutral (IVOS D)                 |\n");
    printf("|  ⚠ = Festival + Inauspicious (IVOS D AMB)        |\n");
    printf("+--------------------------------------------------+\n\n");

    /* === FIRST COICÍSE (Days 1-15) - Full Moon to New Moon === */
    printf("+------+------+------+------+------+\n");
    printf("|   FIRST COICISE (Days I - XV)    |\n");
    printf("|   🌕 Full Moon → 🌑 New Moon     |\n");
    printf("+------+------+------+------+------+\n");

    for (int day = 1; day <= 15; day++) {
        long jd = jd_start + day - 1;
        int mp = moon_phase(jd);
        char marker = day_marker(month_index, day);

        /* Check for old-style fixed festivals */
        int is_festival = 0;
        for (int f = 0; f < FESTIVAL_COUNT; f++) {
            if (festivals[f].month == month_index && festivals[f].day == day) {
                is_festival = 1;
                break;
            }
        }

        /* Check for astronomical festivals (solstices, equinoxes, cross-quarters) */
        if (!is_festival) {
            int yule = days_to_yule(jd);
            int ostara = days_to_ostara(jd);
            int litha = days_to_litha(jd);
            int mabon = days_to_mabon(jd);
            int samhain = days_to_true_samhain(jd);
            int imbolc = days_to_true_imbolc(jd);
            int beltane = days_to_true_beltane(jd);
            int lughnasadh = days_to_true_lughnasadh(jd);
            if (yule == 0 || ostara == 0 || litha == 0 || mabon == 0 ||
                samhain == 0 || imbolc == 0 || beltane == 0 || lughnasadh == 0) {
                is_festival = 1;
            }
        }

        if ((day - 1) % 5 == 0) printf("|");

        if (day == today_day)
            if (is_festival && marker == '!')
                printf("[%2d%s⚠]", day, moon_glyphs[mp]);
            else if (is_festival && marker == '*')
                printf("[%2d%s☆]", day, moon_glyphs[mp]);
            else if (is_festival)
                printf("[%2d%s⚐]", day, moon_glyphs[mp]);
            else if (marker == '!')
                printf("[%2d%s!]", day, moon_glyphs[mp]);
            else if (marker == '*')
                printf("[%2d%s*]", day, moon_glyphs[mp]);
            else
                printf("[%2d%s]", day, moon_glyphs[mp]);
        else if (is_festival && marker == '!')
            printf(" %2d%s⚠", day, moon_glyphs[mp]);  /* Festival + Inauspicious */
        else if (is_festival && marker == '*')
            printf(" %2d%s☆", day, moon_glyphs[mp]);  /* Festival + Auspicious */
        else if (is_festival)
            printf(" %2d%s⚐", day, moon_glyphs[mp]);  /* Festival + Neutral */
        else if (marker == '!')
            printf(" %2d%s!", day, moon_glyphs[mp]);
        else if (marker == '*')
            printf(" %2d%s*", day, moon_glyphs[mp]);
        else
            printf(" %2d%s ", day, moon_glyphs[mp]);

        if (day % 5 == 0) {
            printf("|\n");
            if (day < 15)
                printf("+------+------+------+------+------+\n");
        }
    }
    printf("+------+------+------+------+------+\n\n");

    /* === ATENOUX - New Moon === */
    printf("    ======== ATENOUX (🌑) ========\n");
    printf("       \"Returning Night\"\n\n");

    /* === SECOND COICÍSE (Days 16-29/30) - New Moon to Full Moon === */
    printf("+------+------+------+------+------+\n");
    if (month_days == 30)
        printf("| SECOND COICISE (Days XVI - XXX)  |\n");
    else
        printf("| SECOND COICISE (Days XVI - XXIX) |\n");
    printf("|   🌑 New Moon → 🌕 Full Moon     |\n");
    printf("+------+------+------+------+------+\n");

    for (int day = 16; day <= month_days; day++) {
        long jd = jd_start + day - 1;
        int mp = moon_phase(jd);
        char marker = day_marker(month_index, day);

        /* Check for old-style fixed festivals */
        int is_festival = 0;
        for (int f = 0; f < FESTIVAL_COUNT; f++) {
            if (festivals[f].month == month_index && festivals[f].day == day) {
                is_festival = 1;
                break;
            }
        }

        /* Check for astronomical festivals (solstices, equinoxes, cross-quarters) */
        if (!is_festival) {
            int yule = days_to_yule(jd);
            int ostara = days_to_ostara(jd);
            int litha = days_to_litha(jd);
            int mabon = days_to_mabon(jd);
            int samhain = days_to_true_samhain(jd);
            int imbolc = days_to_true_imbolc(jd);
            int beltane = days_to_true_beltane(jd);
            int lughnasadh = days_to_true_lughnasadh(jd);
            if (yule == 0 || ostara == 0 || litha == 0 || mabon == 0 ||
                samhain == 0 || imbolc == 0 || beltane == 0 || lughnasadh == 0) {
                is_festival = 1;
            }
        }

        if ((day - 16) % 5 == 0) printf("|");

        if (day == today_day)
            if (is_festival && marker == '!')
                printf("[%2d%s⚠]", day, moon_glyphs[mp]);
            else if (is_festival && marker == '*')
                printf("[%2d%s☆]", day, moon_glyphs[mp]);
            else if (is_festival)
                printf("[%2d%s⚐]", day, moon_glyphs[mp]);
            else if (marker == '!')
                printf("[%2d%s!]", day, moon_glyphs[mp]);
            else if (marker == '*')
                printf("[%2d%s*]", day, moon_glyphs[mp]);
            else
                printf("[%2d%s]", day, moon_glyphs[mp]);
        else if (is_festival && marker == '!')
            printf(" %2d%s⚠", day, moon_glyphs[mp]);  /* Festival + Inauspicious */
        else if (is_festival && marker == '*')
            printf(" %2d%s☆", day, moon_glyphs[mp]);  /* Festival + Auspicious */
        else if (is_festival)
            printf(" %2d%s⚐", day, moon_glyphs[mp]);  /* Festival + Neutral */
        else if (marker == '!')
            printf(" %2d%s!", day, moon_glyphs[mp]);
        else if (marker == '*')
            printf(" %2d%s*", day, moon_glyphs[mp]);
        else
            printf(" %2d%s ", day, moon_glyphs[mp]);

        if ((day - 15) % 5 == 0) {
            printf("|\n");
            if (day < month_days && (month_days - day) >= 5)
                printf("+------+------+------+------+------+\n");
        }
    }

    /* Handle partial last row */
    int rem = (month_days - 15) % 5;
    if (rem != 0) {
        for (int i = rem; i < 5; i++) {
            printf("      ");
        }
        printf("|\n");
    }
    printf("+------+------+------+------+------+\n");

    /* Show DIVERTOMU marker for 29-day months */
    if (month_days == 29) {
        printf("\n  ◎ XXX  DIVERTOMU  (virtual 30th day)\n");
    }

    /* === COLIGNY NOTATION DETAIL FOR TODAY === */
    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║     COLIGNY TABLET NOTATION FOR TODAY            ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");

    /* Roman numeral for day */
    const char *roman_numerals[] = {
        "", "I", "II", "III", "IIII", "V", "VI", "VII", "VIII", "VIIII",
        "X", "XI", "XII", "XIII", "XIIII", "XV"
    };

    int display_day = today_day;
    int is_second_half = (today_day > 15);
    if (is_second_half) display_day = today_day - 15;

    const char *notation = get_coligny_notation(month_index, today_day, is_second_half);
    const char *triple = get_triple_mark(today_day);

    printf("║  ◎ %-5s %s %-3s %-11s                   ║\n",
           roman_numerals[display_day],
           triple,
           mat ? "M" : " ",
           notation);

    /* Show special notations */
    if (today_day >= 7 && today_day <= 9 && !is_second_half) {
        printf("║  [PRINNI %s - Full Moon Triplet]               ║\n",
               mat ? "LOUD" : "LAG ");
    }
    if (is_second_half && today_day >= 22 && today_day <= 24) {
        printf("║  [N INIS R - Dark Moon Night]                   ║\n");
    }

    /* Check for IVOS (festival) */
    for (int f = 0; f < FESTIVAL_COUNT; f++) {
        if (festivals[f].month == month_index && festivals[f].day == today_day) {
            printf("║  [IVOS - Festival Day]                          ║\n");
            break;
        }
    }

    printf("╚══════════════════════════════════════════════════╝\n");

    /* === LEGEND === */
    printf("\n┌──────────────────────────────────────────────────┐\n");
    printf("│ COLIGNY NOTATION KEY:                            │\n");
    printf("├──────────────────────────────────────────────────┤\n");
    printf("│ ◎ = Peg hole (marks current day)                 │\n");
    printf("│ * = M D - Matis Divertomu (auspicious day)       │\n");
    printf("│   = D - Divertomu (neutral day)                  │\n");
    printf("│ ! = D AMB - Divertomu Ambrix Ri (inauspicious)   │\n");
    printf("│ ☆ = IVOS M D - Festival + Auspicious             │\n");
    printf("│ ⚐ = IVOS D - Festival + Neutral                  │\n");
    printf("│ ⚠ = IVOS D AMB - Festival + Inauspicious         │\n");
    printf("│ N INIS R = Dark moon night (days 22-24)          │\n");
    printf("│ PRINNI LOUD/LAG = Full moon marker               │\n");
    printf("│ ƚıı ıƚı ııƚ = Triple marks (daytime divisions)   │\n");
    printf("│ DIVERTOMU = Virtual 30th day (29-day months)     │\n");
    printf("└──────────────────────────────────────────────────┘\n");
}
