#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "calendar.h"
#include "astronomy.h"
#include "glyphs.h"

/* Default location: Coligny, France (where the calendar was found) */
#define LATITUDE 46.38

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
    printf("┌─────────────────────────────────────────────────┐\n");
    printf("│ CELTIC DAY RECKONING (Sunset to Sunset)         │\n");
    printf("├─────────────────────────────────────────────────┤\n");
    printf("│ Current Time: %02d:%02d                              │\n",
           local->tm_hour, local->tm_min);
    printf("│ Sunset Today: %s (Coligny, 46.38°N)            │\n", sunset_str);
    if (after_sunset) {
        printf("│ ☽ After Sunset — Celtic day has begun          │\n");
    } else {
        printf("│ ☉ Before Sunset — Still previous Celtic day    │\n");
    }
    printf("│ \"The night in each case precedes the day.\"      │\n");
    printf("└─────────────────────────────────────────────────┘\n");
    printf("\n");

    /* ═══════════════════════════════════════════════════════════════
     * METONIC CYCLE (19-Year Lunisolar Synchronization)
     * ═══════════════════════════════════════════════════════════════ */
    int met_year = metonic_year(celtic_jd);
    int met_lunation = metonic_lunation(celtic_jd);
    int met_cycle = metonic_cycle_number(celtic_jd);
    double met_drift = metonic_drift_hours(celtic_jd);

    printf("┌─────────────────────────────────────────────────┐\n");
    printf("│ METONIC CYCLE (19-Year Lunisolar Sync)          │\n");
    printf("├─────────────────────────────────────────────────┤\n");
    printf("│ Cycle #%d | Year %d of 19 | Lunation %d of 235  │\n",
           met_cycle, met_year, met_lunation);
    printf("│ Accumulated drift: %.1f hours (%.1f days)        │\n",
           met_drift, met_drift / 24.0);
    printf("│ 235 moons = 19 years (error: ~2 hrs/cycle)      │\n");
    printf("└─────────────────────────────────────────────────┘\n");
    printf("\n");

    /* ═══════════════════════════════════════════════════════════════
     * PLEIADES HELIACAL RISING (Samhain Marker)
     * ═══════════════════════════════════════════════════════════════ */
    int pleiades_days = days_to_pleiades_rising(celtic_jd);
    int pleiades_now = is_pleiades_rising(celtic_jd);

    printf("┌─────────────────────────────────────────────────┐\n");
    printf("│ PLEIADES (Seven Sisters) - Samhain Marker       │\n");
    printf("├─────────────────────────────────────────────────┤\n");
    if (pleiades_now) {
        printf("│ ✧ HELIACAL RISING NOW ✧                        │\n");
        printf("│ The Pleiades rise before dawn - Samhain time!  │\n");
    } else if (pleiades_days > 0) {
        printf("│ Days until heliacal rising: %d                  │\n", pleiades_days);
        printf("│ (Pleiades hidden by Sun's glare)               │\n");
    } else {
        printf("│ Days since heliacal rising: %d                  │\n", -pleiades_days);
        printf("│ (Pleiades visible in pre-dawn sky)             │\n");
    }
    printf("└─────────────────────────────────────────────────┘\n");
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

    printf("┌─────────────────────────────────────────────────┐\n");
    printf("│ THE EIGHT-FOLD YEAR (Wheel of the Year)         │\n");
    printf("├─────────────────────────────────────────────────┤\n");
    printf("│ Current Sun Longitude: %5.1f°                   │\n", sun_long);
    printf("│                                                 │\n");
    printf("│ ══ SOLSTICES & EQUINOXES (Quarter Days) ══      │\n");
    printf("│ Yule    (270°) Winter Solstice:  %+4d days      │\n", yule_days);
    printf("│ Ostara  (  0°) Vernal Equinox:   %+4d days      │\n", ostara_days);
    printf("│ Litha   ( 90°) Summer Solstice:  %+4d days      │\n", litha_days);
    printf("│ Mabon   (180°) Autumn Equinox:   %+4d days      │\n", mabon_days);
    printf("│                                                 │\n");
    printf("│ ══ CROSS-QUARTERS (Fire Festivals) ══           │\n");
    printf("│ Samhain (225°) Winter's Gate:    %+4d days      │\n", sam_days);
    printf("│ Imbolc  (315°) Spring Stirring:  %+4d days      │\n", imb_days);
    printf("│ Beltane ( 45°) Summer's Gate:    %+4d days      │\n", bel_days);
    printf("│ Lughnasadh(135°) Harvest Home:   %+4d days      │\n", lug_days);
    printf("│                                                 │\n");

    /* Highlight nearest event from the eight-fold year */
    int nearest_days;
    int nearest = nearest_eightfold_event(celtic_jd, &nearest_days);
    const char *eightfold_names[] = {
        "Yule", "Imbolc", "Ostara", "Beltane",
        "Litha", "Lughnasadh", "Mabon", "Samhain"
    };
    printf("│ → Next: %-10s in %3d days                 │\n",
           eightfold_names[nearest], nearest_days);
    printf("└─────────────────────────────────────────────────┘\n");
    printf("\n");

    print_celtic_month_lunar(month_idx, jd_month_start, celtic_jd, jd, celtic_month_days, after_sunset);
    return 0;
}
