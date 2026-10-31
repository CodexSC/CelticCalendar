/*
 * Essential checks: the calendar must stay aligned with the sky.
 *
 * Build & run from the project folder:
 *   gcc -Wall -I. -o test_calendar tests/test_calendar.c astronomy.c calendar.c -lm
 *   ./test_calendar
 */
#include <stdio.h>
#include "astronomy.h"
#include "calendar.h"

static int failures = 0;

static void check(int ok, const char *what)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) failures++;
}

int main(void)
{
    /* 1. Moon phases fall on the real (published) dates, 2026 */
    check(moon_phase(jd_from_ymd(2026, 10, 10)) == 0, "new moon 2026-10-10 (15:50 UTC)");
    check(moon_phase(jd_from_ymd(2026, 10, 26)) == 4, "full moon 2026-10-26 (04:11 UTC)");
    check(moon_phase(jd_from_ymd(2026, 11, 9))  == 0, "new moon 2026-11-09 (07:02 UTC)");
    check(moon_phase(jd_from_ymd(2026, 3, 19))  == 0, "new moon 2026-03-19 (01:24 UTC)");
    check(moon_phase(jd_from_ymd(2026, 2, 2))   == 4, "full moon 2026-02-01 22:10 UTC, after sunset -> Feb 2");

    /* 2. Every month 1990-2060: one primary phase per day, day 1 = sixth day of the moon (Pliny) */
    long a = jd_from_ymd(1990, 1, 1), b = jd_from_ymd(2060, 1, 1);
    int bad_phase = 0, bad_pliny = 0, bad_length = 0, months = 0;
    int prev = moon_phase(a - 1);
    for (long jd = a; jd < b; jd++) {
        int p = moon_phase(jd);
        int step = (p - prev + 8) % 8;
        if (!(step == 1 || (step == 0 && p % 2 == 1))) bad_phase++;
        prev = p;

        if (lunar_day_of_month(jd) == 1) {
            months++;
            long n = jd;
            while (moon_phase(n) != 0) n--;
            if (jd - n != 5) bad_pliny++;
            int len = lunar_month_length(jd);
            if ((len != 29 && len != 30) || find_lunar_month_start(jd + len) != jd + len) bad_length++;
        }
    }
    check(bad_phase == 0,  "moon phases advance in order, one primary phase per day");
    check(bad_pliny == 0,  "every month begins on the sixth day of the moon");
    check(bad_length == 0, "every month is 29 or 30 days");

    /* 3. Samhain (Sun at 225°) always falls in Samonios; years are 12 or 13 lunations */
    int bad_samhain = 0, bad_year = 0;
    for (int y = 1990; y < 2060; y++) {
        long sun = jd_from_ymd(y, 10, 20);
        while (days_to_true_samhain(sun) > 0) sun++;
        long start = find_samonios_start(y);
        if (sun < start || sun >= start + lunar_month_length(start)) bad_samhain++;
        if (lunar_celtic_month_index(start) != 6) bad_samhain++;

        int len = (int)(find_samonios_start(y + 1) - start);
        if (!((len >= 353 && len <= 355) || (len >= 383 && len <= 385))) bad_year++;
    }
    check(bad_samhain == 0, "Samhain falls in Samonios every year");
    check(bad_year == 0,    "every year is 354 or 384 days (12 or 13 lunations)");

    /* 3b. Beltane (Sun at 45°) falls in Giamonios; months run in order, with the
     *     extra month before Giamonios (Rantaranos) or before Samonios (Quimonios) */
    int bad_beltane = 0, bad_order = 0, mid = 0, end = 0;
    for (int y = 1990; y < 2060; y++) {
        long start = find_samonios_start(y), next = find_samonios_start(y + 1);
        long bel = start + 160;
        while (days_to_true_beltane(bel) > 0) bel++;
        if (lunar_celtic_month_index(bel) != 0) bad_beltane++;

        int expect = 6, inserted = 0;
        for (long m = start; m < next; m = find_lunar_month_start(m + 32)) {
            int idx = lunar_celtic_month_index(m);
            if (idx == 13 && expect == 0 && !inserted) { inserted = 1; mid++; continue; }
            if (idx == 12 && expect == 6 && !inserted) { inserted = 1; end++; continue; }
            if (idx != expect) bad_order++;
            expect = (expect + 1) % 12;
        }
        if (expect != 6) bad_order++;
    }
    check(bad_beltane == 0, "Beltane falls in Giamonios every year");
    check(bad_order == 0,   "months run in order; extra month only before Giamonios or Samonios");
    printf("      (extra month mid-year %d times, at year end %d times)\n", mid, end);

    /* 4. The eight divisions of the ecliptic land on their Celtic day (published 2026 times) */
    long mar = jd_from_ymd(2026, 3, 20), jun = jd_from_ymd(2026, 6, 21);
    long sep = jd_from_ymd(2026, 9, 23), dec = jd_from_ymd(2026, 12, 22);
    check(days_to_ostara(mar) == 0, "March equinox 2026-03-20 (14:46 UTC)");
    check(days_to_litha(jun) == 0,  "June solstice 2026-06-21 (08:24 UTC)");
    check(days_to_mabon(sep) == 0,  "September equinox 2026-09-23 (00:05 UTC)");
    check(days_to_yule(dec) == 0,   "December solstice 2026-12-21 20:50 UTC, after sunset -> Dec 22");

    printf("\n%d months checked, %s\n", months, failures ? "FAILURES" : "all ok");
    return failures != 0;
}
