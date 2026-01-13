#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <string.h>
#include "calendar.h"
#include "astronomy.h"
#include "glyphs.h"

/* Default location: Coligny, France (where the calendar was found) */
#define LATITUDE 46.38
/* Match the width of month grids (71 chars including borders) */
#define BOX_WIDTH 71

/* Locale-aware width helpers so emoji align in boxes */
static void ensure_locale(void)
{
    static int initialized = 0;
    if (!initialized) {
        setlocale(LC_ALL, "");
        initialized = 1;
    }
}

static int codepoint_width(wchar_t ch)
{
    int w = wcwidth(ch);
    if (w < 0) w = 1;
    if (w < 2) {
        if ((ch >= 0x1F300 && ch <= 0x1FAFF) || (ch >= 0x1F600 && ch <= 0x1F64F)) {
            w = 2;
        }
    }
    return w;
}

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
            ++p;
            ++width;
            memset(&st, 0, sizeof(st));
            continue;
        }
        width += codepoint_width(wc);
        p += len;
    }
    return width;
}

static void box_border(const char *left, const char *right)
{
    fputs(left, stdout);
    for (int i = 0; i < BOX_WIDTH; i++) fputs("─", stdout);
    fputs(right, stdout);
    fputc('\n', stdout);
}

static void box_line(const char *text)
{
    int w = display_width(text);
    int pad = BOX_WIDTH - w;
    if (pad < 0) pad = 0;
    fputs("│", stdout);
    fputs(text, stdout);
    for (int i = 0; i < pad; i++) fputc(' ', stdout);
    fputs("│\n", stdout);
}

int main(int argc, char *argv[])
{
    long jd;
    double current_hour;
    struct tm local_time;
    struct tm *local;

    if (argc == 4) {
        /* User specified date: year month day */
        int year = atoi(argv[1]);
        int month = atoi(argv[2]);
        int day = atoi(argv[3]);
        jd = jd_from_ymd(year, month, day);
        /* Assume noon for specified dates */
        current_hour = 12.0;
        local_time.tm_hour = 12;
        local_time.tm_min = 0;
        local = &local_time;
    } else {
        /* Use current date/time */
        jd = jd_today();
        time_t t = time(NULL);
        local = localtime(&t);
        current_hour = local->tm_hour + local->tm_min / 60.0;
    }

    /* Calculate sunset time */
    char sunset_str[16];
    get_sunset_time_str(jd, LATITUDE, sunset_str, sizeof(sunset_str));

    /* Check if we're in the Celtic "next day" (after sunset) */
    int after_sunset = is_after_sunset(jd, current_hour, LATITUDE);
    long celtic_jd = celtic_jd_from_time(jd, current_hour, LATITUDE);

    int year = celtic_year_from_jd(celtic_jd);
    int doy = day_of_year(celtic_jd);
    int remaining = days_remaining(celtic_jd);
    double frac = elapsed_fraction(celtic_jd);
    int age, year_in_age;
    age_and_year_in_age(celtic_jd, &age, &year_in_age);

    /* Use TRUE lunar-synced month calculation */
    /* Month starts at full moon, ATENOUX at new moon */
    int month_idx = lunar_celtic_month_index(celtic_jd);
    long jd_month_start = find_full_moon_before(celtic_jd);
    int celtic_month_days = lunar_month_length(celtic_jd);

    printf("Celtic Calendar — Daily View (Lunar-Synced)\n");
    printf("═══════════════════════════════════════════════════\n");
    printf("Celtic Year: %d\n", year);
    printf("Elapsed fraction of current year: %.2f\n", frac);
    printf("Age: %d | Year in Age: %d\n", age, year_in_age);
    printf("Day of Year: %d / %d | Days Remaining: %d\n", doy, current_year_length(celtic_jd), remaining);
    printf("═══════════════════════════════════════════════════\n");

    /* Celtic day timing information */
    char line[128];
    box_border("┌", "┐");
    box_line(" CELTIC DAY RECKONING (Sunset to Sunset) ");
    box_border("├", "┤");
    snprintf(line, sizeof(line), " Current Time: %02d:%02d", local->tm_hour, local->tm_min);
    box_line(line);
    snprintf(line, sizeof(line), " Sunset Today: %s (Coligny, 46.38°N)", sunset_str);
    box_line(line);
    if (after_sunset) {
        box_line(" ☽ After Sunset — Celtic day has begun");
    } else {
        box_line(" ☉ Before Sunset — Still previous Celtic day");
    }
    box_line(" \"The night in each case precedes the day.\"");
    box_border("└", "┘");
    printf("\n");

    /* ═══════════════════════════════════════════════════════════════
     * METONIC CYCLE (19-Year Lunisolar Synchronization)
     * ═══════════════════════════════════════════════════════════════ */
    int met_year = metonic_year(celtic_jd);
    int met_lunation = metonic_lunation(celtic_jd);
    int met_cycle = metonic_cycle_number(celtic_jd);
    double met_drift = metonic_drift_hours(celtic_jd);

        box_border("┌", "┐");
        box_line(" METONIC CYCLE (19-Year Lunisolar Sync) ");
        box_border("├", "┤");
        snprintf(line, sizeof(line), " Cycle #%d | Year %d of 19 | Lunation %d of 235", met_cycle, met_year, met_lunation);
        box_line(line);
        snprintf(line, sizeof(line), " Accumulated drift: %.1f hours (%.1f days)", met_drift, met_drift / 24.0);
        box_line(line);
        box_line(" 235 moons = 19 years (error: ~2 hrs/cycle)");
        box_border("└", "┘");
        printf("\n");

    /* ═══════════════════════════════════════════════════════════════
     * PLEIADES HELIACAL RISING (Samhain Marker)
     * ═══════════════════════════════════════════════════════════════ */
    int pleiades_days = days_to_pleiades_rising(celtic_jd);
    int pleiades_now = is_pleiades_rising(celtic_jd);

    box_border("┌", "┐");
    box_line(" PLEIADES (Seven Sisters) - Samhain Marker ");
    box_border("├", "┤");
    if (pleiades_now) {
        box_line(" ✧ HELIACAL RISING NOW ✧");
        box_line(" The Pleiades rise before dawn - Samhain time!");
    } else if (pleiades_days > 0) {
        snprintf(line, sizeof(line), " Days until heliacal rising: %d", pleiades_days);
        box_line(line);
        box_line(" (Pleiades hidden by Sun's glare)");
    } else {
        snprintf(line, sizeof(line), " Days since heliacal rising: %d", -pleiades_days);
        box_line(line);
        box_line(" (Pleiades visible in pre-dawn sky)");
    }
    box_border("└", "┘");
    printf("\n");

    /* ═══════════════════════════════════════════════════════════════
     * THE EIGHT-FOLD YEAR (Solstices, Equinoxes & Cross-Quarters)
     * ═══════════════════════════════════════════════════════════════ */
    int yule_days = days_to_yule(celtic_jd);
    int imb_days = days_to_true_imbolc(celtic_jd);
    int ostara_days = days_to_ostara(celtic_jd);
    int bel_days = days_to_true_beltane(celtic_jd);
    int litha_days = days_to_litha(celtic_jd);
    int lug_days = days_to_true_lughnasadh(celtic_jd);
    int mabon_days = days_to_mabon(celtic_jd);
    int sam_days = days_to_true_samhain(celtic_jd);

    double sun_long = sun_longitude(celtic_jd);

    box_border("┌", "┐");
    box_line(" THE EIGHT-FOLD YEAR (Wheel of the Year) ");
    box_border("├", "┤");
    snprintf(line, sizeof(line), " Current Sun Longitude: %5.1f°", sun_long);
    box_line(line);
    box_line(" ");
    box_line(" ══ SOLSTICES & EQUINOXES (Quarter Days) ══");
    snprintf(line, sizeof(line), " Yule    (270°) Winter Solstice:  %+4d days", yule_days);
    box_line(line);
    snprintf(line, sizeof(line), " Ostara  (  0°) Vernal Equinox:   %+4d days", ostara_days);
    box_line(line);
    snprintf(line, sizeof(line), " Litha   ( 90°) Summer Solstice:  %+4d days", litha_days);
    box_line(line);
    snprintf(line, sizeof(line), " Mabon   (180°) Autumn Equinox:   %+4d days", mabon_days);
    box_line(line);
    box_line(" ");
    box_line(" ══ CROSS-QUARTERS (Fire Festivals) ══");
    snprintf(line, sizeof(line), " Samhain (225°) Winter's Gate:    %+4d days", sam_days);
    box_line(line);
    snprintf(line, sizeof(line), " Imbolc  (315°) Spring Stirring:  %+4d days", imb_days);
    box_line(line);
    snprintf(line, sizeof(line), " Beltane ( 45°) Summer's Gate:    %+4d days", bel_days);
    box_line(line);
    snprintf(line, sizeof(line), " Lughnasadh(135°) Harvest Home:   %+4d days", lug_days);
    box_line(line);
    box_line(" ");

    /* Highlight nearest event from the eight-fold year */
    int nearest_days;
    int nearest = nearest_eightfold_event(celtic_jd, &nearest_days);
    const char *eightfold_names[] = {
        "Yule", "Imbolc", "Ostara", "Beltane",
        "Litha", "Lughnasadh", "Mabon", "Samhain"
    };
    snprintf(line, sizeof(line), " → Next: %-10s in %3d days", eightfold_names[nearest], nearest_days);
    box_line(line);
    box_border("└", "┘");
    printf("\n");

    print_celtic_month_lunar(month_idx, jd_month_start, celtic_jd, jd, celtic_month_days, after_sunset);
    return 0;
}
