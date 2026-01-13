#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "calendar.h"
#include "astronomy.h"
#include "festivals.h"

#define CELL_WIDTH 9
#define INFO_WIDTH 71
#define GRID_SPAN_WIDTH (CELL_WIDTH * 7 + 6)

/* Moon phase glyphs: 0=new, 1=first quarter, 2=full, 3=last quarter */
static const char *moon_glyphs[4] = {"🌑", "🌓", "🌕", "🌗"};
static const char *zodiac_glyphs[12] = {"♈","♉","♊","♋","♌","♍","♎","♏","♐","♑","♒","♓"};
static const char *weekday_glyphs[7] = {"☉", "☽", "♂", "☿", "♃", "♀", "♄"};

static int display_width(const char *s);

/* Wide info box helpers to align with the 7-column grid */
static void info_border(const char *left, const char *right)
{
    fputs(left, stdout);
    for (int i = 0; i < INFO_WIDTH; i++) fputs("─", stdout);
    fputs(right, stdout);
    fputc('\n', stdout);
}

static void info_line(const char *text)
{
    int w = display_width(text);
    int pad = INFO_WIDTH - w;
    if (pad < 0) pad = 0;
    fputs("│", stdout);
    fputs(text, stdout);
    for (int i = 0; i < pad; i++) fputc(' ', stdout);
    fputs("│\n", stdout);
}

/* Span the calendar grid width (7 columns) with centered text */
static void grid_span_center(const char *text)
{
    int w = display_width(text);
    int pad = GRID_SPAN_WIDTH - w;
    if (pad < 0) pad = 0;
    int left = pad / 2;
    int right = pad - left;
    fputs("│", stdout);
    for (int i = 0; i < left; i++) fputc(' ', stdout);
    fputs(text, stdout);
    for (int i = 0; i < right; i++) fputc(' ', stdout);
    fputs("│\n", stdout);
}

static void print_border(const char *left, const char *mid, const char *right)
{
    fputs(left, stdout);
    for (int col = 0; col < 7; col++) {
        for (int i = 0; i < CELL_WIDTH; i++) fputs("─", stdout);
        fputs(col == 6 ? right : mid, stdout);
    }
    putchar('\n');
}

/* Center text within a CELL_WIDTH field using display width */
static void print_cell_center(const char *text)
{
    int w = display_width(text);
    int pad = CELL_WIDTH - w;
    if (pad < 0) pad = 0;
    int left = pad / 2;
    int right = pad - left;
    for (int i = 0; i < left; i++) putchar(' ');
    fputs(text, stdout);
    for (int i = 0; i < right; i++) putchar(' ');
}

static void print_week_header(void)
{
    const char *names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    printf("│");
    for (int i = 0; i < 7; i++) {
        print_cell_center(weekday_glyphs[i]);
        printf("│");
    }
    printf("\n│");
    for (int i = 0; i < 7; i++) {
        print_cell_center(names[i]);
        printf("│");
    }
    printf("\n");
}

/* Get the Coligny notation for a specific day */
static const char* get_coligny_notation(int month_index, int day, int is_atenoux_half)
{
    int mat = is_mat_month(month_index);

    /* Days 7-8-9 in first half = PRINNI LOUD/LAG (full moon triplet) */
    if (!is_atenoux_half && day >= 7 && day <= 9) {
        if (day == 7) return mat ? "PRINNI LOUD" : "PRINNI LAG";
        return mat ? "M D" : "D";
    }

    /* Days 22-23-24 in ATENOUX = N INIS R (dark moon nights) */
    if (is_atenoux_half && day >= 22 && day <= 24) {
        return "N INIS R";
    }

    /* Day 5 and 11 in first half = D AMB (inauspicious) */
    if (!is_atenoux_half && (day == 5 || day == 11)) {
        return "D AMB";
    }

    /* Odd days after 16 in ATENOUX = D AMB */
    if (is_atenoux_half && day > 16 && (day % 2 == 1)) {
        return "D AMB";
    }

    /* Default: MAT month = M D, ANM month = D */
    return mat ? "M D" : "D";
}

/* Get triple mark for a day (ƚıı, ıƚı, or ııƚ) */
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

static void print_coligny_tablet(int month_index, int today_day, int mat)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║     COLIGNY TABLET NOTATION FOR TODAY            ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");

    const char *roman_numerals[] = {
        "", "I", "II", "III", "IIII", "V", "VI", "VII", "VIII", "VIIII",
        "X", "XI", "XII", "XIII", "XIIII", "XV"
    };

    int display_day = today_day;
    int is_second_half = (today_day > 15);
    if (is_second_half) display_day = today_day - 15;

    const char *notation = get_coligny_notation(month_index, today_day, is_second_half);
    const char *triple = get_triple_mark(today_day);

    printf("║  ◎ %-5s %s %-3s %-11s                     ║\n",
           roman_numerals[display_day],
           triple,
           mat ? "M" : " ",
           notation);

    if (today_day >= 7 && today_day <= 9 && !is_second_half) {
        printf("║  [PRINNI %s - Full Moon Triplet]               ║\n",
               mat ? "LOUD" : "LAG ");
    }
    if (is_second_half && today_day >= 22 && today_day <= 24) {
        printf("║  [N INIS R - Dark Moon Night]                   ║\n");
    }

    for (int f = 0; f < FESTIVAL_COUNT; f++) {
        if (festivals[f].month == month_index && festivals[f].day == today_day) {
            printf("║  [IVOS - Festival Day]                          ║\n");
            break;
        }
    }

    printf("╚══════════════════════════════════════════════════╝\n");

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
    printf("│ [] = Today marker inside the grid                │\n");
    printf("│ N INIS R = Dark moon night (days 22-24)          │\n");
    printf("│ PRINNI LOUD/LAG = Full moon marker               │\n");
    printf("│ ƚıı ıƚı ııƚ = Triple marks (daytime divisions)   │\n");
    printf("│ DIVERTOMU = Virtual 30th day (29-day months)     │\n");
    printf("└──────────────────────────────────────────────────┘\n");
}

/* One-time locale init so wcwidth() behaves with emojis */
static void ensure_locale(void)
{
    static int initialized = 0;
    if (!initialized) {
        setlocale(LC_ALL, "");
        initialized = 1;
    }
}

/* Treat emoji code points as width 2 on terminals that report 1 */
static int codepoint_width(wchar_t ch)
{
    int w = wcwidth(ch);
    if (w < 0) w = 1;
    if (w < 2) {
        if ((ch >= 0x1F300 && ch <= 0x1FAFF) || /* Misc emoji, moons */
            (ch >= 0x1F600 && ch <= 0x1F64F)) { /* Emoticons block */
            w = 2;
        }
    }
    return w;
}

/* Measure displayed width (accounts for double-width emoji) */
static int display_width(const char *s)
{
    ensure_locale();
    mbstate_t st = {0};
    int width = 0;
    const char *p = s;
    wchar_t wc;

    while (*p) {
        size_t len = mbrtowc(&wc, p, MB_CUR_MAX, &st);
        if (len == (size_t)-1 || len == (size_t)-2) {
            /* Invalid sequence; treat byte as width 1 and advance */
            ++p;
            ++width;
            memset(&st, 0, sizeof(st));
            continue;
        }
        int w = codepoint_width(wc);
        width += w;
        p += len;
    }
    return width;
}

/* Map to display glyphs (keep original emoji markers) */
static const char* status_glyph(int is_festival, char marker)
{
    if (is_festival && marker == '!') return "⚠";
    if (is_festival && marker == '*') return "☆";
    if (is_festival) return "⚐";
    if (marker == '!') return "!";
    if (marker == '*') return "*";
    return " ";
}

/* Render one fixed-width cell while keeping emojis and a bracketed today marker */
static void print_day_cell(int day, int mp, char marker, int is_festival, int is_today)
{
    const char *status = status_glyph(is_festival, marker);
    char cell[32];

    if (is_today) {
        snprintf(cell, sizeof(cell), "[%2d%s%s]", day, moon_glyphs[mp], status);
    } else {
        snprintf(cell, sizeof(cell), " %2d%s%s ", day, moon_glyphs[mp], status);
    }

    int w = display_width(cell);
    int pad = CELL_WIDTH - w;
    if (pad < 0) pad = 0;

    fputs(cell, stdout);
    for (int i = 0; i < pad; i++) putchar(' ');
}

/* Festival lookup: month/day match or astronomical cross-quarters */
static int is_festival_day(int month_index, int day, long jd)
{
    for (int f = 0; f < FESTIVAL_COUNT; f++) {
        if (festivals[f].month == month_index && festivals[f].day == day) {
            return 1;
        }
    }

    int yule = days_to_yule(jd); int ostara = days_to_ostara(jd);
    int litha = days_to_litha(jd); int mabon = days_to_mabon(jd);
    int samhain = days_to_true_samhain(jd); int imbolc = days_to_true_imbolc(jd);
    int beltane = days_to_true_beltane(jd); int lughnasadh = days_to_true_lughnasadh(jd);

    if (yule == 0 || ostara == 0 || litha == 0 || mabon == 0 ||
        samhain == 0 || imbolc == 0 || beltane == 0 || lughnasadh == 0) {
        return 1;
    }
    return 0;
}

static void print_grid_half(int month_index, long jd_start, int start_day, int end_day, int today_day)
{
    long jd_of_start = jd_start + start_day - 1;
    int weekday_of_start = (int)((jd_of_start + 1) % 7);

    printf("│");
    for (int i = 0; i < weekday_of_start; i++) {
        printf("%*s│", CELL_WIDTH, "");
    }

    for (int day = start_day; day <= end_day; day++) {
        long jd = jd_start + day - 1;
        int day_weekday = (int)((jd + 1) % 7);

        if (day_weekday == 0 && day > start_day) {
            putchar('\n');
            print_border("├","┼","┤");
            printf("│");
        }

        int mp = moon_phase(jd);
        char marker = day_marker(month_index, day);
        int festival = is_festival_day(month_index, day, jd);

        print_day_cell(day, mp, marker, festival, day == today_day);
        printf("│");
    }
}

void print_celtic_month(int month_index, long jd_start, long jd_today)
{
    int today_day = (int)(jd_today - jd_start) + 1;
    int weekday = (int)((jd_today + 1) % 7);
    int today_moon = moon_phase(jd_today);
    int today_moon_zodiac = moon_sign(jd_today);
    int today_sun_zodiac = sun_sign(jd_today);
    int month_days = get_month_days(month_index);
    int mat = is_mat_month(month_index);
    char line[128];

    info_border("┌", "┐");
    snprintf(line, sizeof(line), "  %-12s (%s)",
             get_celtic_month_name(month_index),
             get_month_abbrev(month_index));
    info_line(line);
    snprintf(line, sizeof(line), "  %-36s - %2d days",
             mat ? "Matis (lucky/complete month)" : "Anmatu (unlucky/incomplete month)",
             month_days);
    info_line(line);
    info_border("└", "┘");

    info_border("┌", "┐");
    snprintf(line, sizeof(line), "  Today: Day %2d - %s - %s%s - Sun %s",
             today_day,
             weekday_glyphs[weekday],
             moon_glyphs[today_moon],
             zodiac_glyphs[today_moon_zodiac],
             zodiac_glyphs[today_sun_zodiac]);
    info_line(line);
    if (is_atenoux(today_day)) info_line("  ═══ ATENOUX (Second Coicise) ═══");
    else                       info_line("  ═══ First Coicise ═══");
    info_border("└", "┘");

    info_border("┌", "┐");
    int has_festival = 0;
    for (int f = 0; f < FESTIVAL_COUNT; f++) {
        if (festivals[f].month == month_index) {
            snprintf(line, sizeof(line), "  IVOS: %-33s Day %2d", festivals[f].name, festivals[f].day);
            info_line(line);
            has_festival = 1;
        }
    }
    if (!has_festival) info_line("  (No major festivals this month)");
    info_border("└", "┘");

    info_border("┌", "┐");
    info_line("  * = M D - Auspicious (MAT month)");
    info_line("  ! = D AMB - Inauspicious day");
    info_line("  ☆ = IVOS + Auspicious");
    info_line("  ⚐ = IVOS + Neutral");
    info_line("  ⚠ = IVOS + Inauspicious");
    info_line("  [] = Today (grid marker)");
    info_border("└", "┘");
    printf("\n");

    print_border("┌","┬","┐");
    grid_span_center("FIRST COICISE (Days I - XV)");
    grid_span_center("🌕 Full Moon → 🌑 New Moon");
    print_border("├","┼","┤");
    print_week_header();
    print_border("├","┼","┤");

    print_grid_half(month_index, jd_start, 1, 15, today_day);

    int last_weekday_first = (int)((jd_start + 15) % 7);
    for (int i = last_weekday_first; i < 6; i++) printf("%*s│", CELL_WIDTH, "");
    printf("\n");
    print_border("└","┴","┘");
    printf("\n");

    printf("        ════════ ATENOUX (🌑) ════════\n");
    printf("           \"Returning Night\"\n\n");

    print_border("┌","┬","┐");
    if (month_days == 30)
        grid_span_center("SECOND COICISE (Days XVI - XXX)");
    else
        grid_span_center("SECOND COICISE (Days XVI - XXIX)");
    grid_span_center("🌑 New Moon → 🌕 Full Moon");
    print_border("├","┼","┤");
    print_week_header();
    print_border("├","┼","┤");

    print_grid_half(month_index, jd_start, 16, month_days, today_day);

    int last_day_weekday = (int)((jd_start + month_days) % 7);
    for (int i = last_day_weekday; i < 6; i++) printf("%*s│", CELL_WIDTH, "");
    printf("\n");
    print_border("└","┴","┘");

    if (month_days == 29) {
        printf("\n        ◎ XXX  DIVERTOMU  (virtual 30th day)\n");
    }

    print_coligny_tablet(month_index, today_day, mat);
}

void print_celtic_month_lunar(int month_index, long jd_start, long jd_celtic, long jd_actual, int month_days, int after_sunset)
{
    int today_day_raw = (int)(jd_celtic - jd_start) + 1;
    int in_month = (today_day_raw >= 1 && today_day_raw <= month_days);
    int today_day = in_month ? today_day_raw : 0; /* use 0 to keep the panel present even when outside */
    int celtic_weekday = (int)((jd_celtic + 1) % 7);
    int today_moon = moon_phase(jd_actual);
    int today_moon_zodiac = moon_sign(jd_actual);
    int today_sun_zodiac = sun_sign(jd_actual);
    int mat = (month_days == 30);
    char line[160];

    (void)after_sunset; /* unused in condensed today panel */

    info_border("┌", "┐");
    snprintf(line, sizeof(line), "  %-12s (%s)",
             get_celtic_month_name(month_index),
             get_month_abbrev(month_index));
    info_line(line);
    snprintf(line, sizeof(line), "  %-36s - %2d days",
             mat ? "Matis (lucky/complete month)" : "Anmatu (unlucky/incomplete month)",
             month_days);
    info_line(line);
    info_border("└", "┘");

    if (in_month) {
        info_border("┌", "┐");
        snprintf(line, sizeof(line), "  Today: Day %2d (%s) - %s%s - Sun %s",
             today_day,
             weekday_glyphs[celtic_weekday],
             moon_glyphs[today_moon],
             zodiac_glyphs[today_moon_zodiac],
             zodiac_glyphs[today_sun_zodiac]);
        info_line(line);
        if (today_day > 15) info_line("  ═══ ATENOUX (Second Coicise) ═══");
        else               info_line("  ═══ First Coicise ═══");
        info_border("└", "┘");
    }

    info_border("┌", "┐");
    int has_festival = 0;
    for (int f = 0; f < FESTIVAL_COUNT; f++) {
        if (festivals[f].month == month_index) {
            snprintf(line, sizeof(line), "  IVOS: %-33s Day %2d", festivals[f].name, festivals[f].day);
            info_line(line);
            has_festival = 1;
        }
    }
    if (!has_festival) info_line("  (No major festivals this month)");
    info_border("└", "┘");
    printf("\n");

    print_border("┌","┬","┐");
    grid_span_center("FIRST COICISE (Days I - XV)");
    grid_span_center("🌕 Full Moon → 🌑 New Moon");
    print_border("├","┼","┤");
    print_week_header();
    print_border("├","┼","┤");

    print_grid_half(month_index, jd_start, 1, 15, today_day);
    int last_weekday_first = (int)((jd_start + 15) % 7);
    for (int i = last_weekday_first; i < 6; i++) printf("%*s│", CELL_WIDTH, "");
    printf("\n");
    print_border("└","┴","┘");
    printf("\n");

    printf("        ════════ ATENOUX (🌑) ════════\n");
    printf("           \"Returning Night\"\n\n");

    print_border("┌","┬","┐");
    if (month_days == 30)
        grid_span_center("SECOND COICISE (Days XVI - XXX)");
    else
        grid_span_center("SECOND COICISE (Days XVI - XXIX)");
    grid_span_center("🌑 New Moon → 🌕 Full Moon");
    print_border("├","┼","┤");
    print_week_header();
    print_border("├","┼","┤");

    print_grid_half(month_index, jd_start, 16, month_days, today_day);
    int last_day_weekday = (int)((jd_start + month_days) % 7);
    for (int i = last_day_weekday; i < 6; i++) printf("%*s│", CELL_WIDTH, "");
    printf("\n");
    print_border("└","┴","┘");

    if (month_days == 29) printf("\n        ◎ XXX  DIVERTOMU  (virtual 30th day)\n");

    if (in_month) {
        print_coligny_tablet(month_index, today_day, mat);
    }
}
