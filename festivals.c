#include "festivals.h"

/*
 * Celtic Calendar Festivals for Year 5289 (Nov 2025 - Oct 2026)
 *
 * The Coligny calendar marked festivals with IVOS runs spanning 3-9 days.
 * Major festivals typically lasted 3 nights (Trinox = "Three Nights").
 *
 * Cross-Quarter Days (Fire Festivals):
 *   Samhain (~225°), Imbolc (~315°), Beltane (~45°), Lughnasadh (~135°)
 *
 * Quarter Days (Solar Events):
 *   Winter Solstice (270°), Vernal Equinox (0°),
 *   Summer Solstice (90°), Autumn Equinox (180°)
 *
 * Multi-Day Festival Notation:
 *   TRINVX SAMONI = "Three Nights of Samhain"
 *   SINDIV IVOS = "This day a festival" (exceptional importance)
 */

/*
 * Solar-tied festivals (quarters and cross-quarters) are resolved dynamically
 * from the Sun's position; keep this table for any non-astronomical IVOS days.
 */
const Festival festivals[] = {
};

const int FESTIVAL_COUNT = sizeof(festivals)/sizeof(festivals[0]);

/*
 * Multi-Day Festival Definitions
 * Based on the Coligny calendar's IVOS patterns
 *
 * Format: {name, month, start_day, duration, type}
 * type: 0 = fire festival (cross-quarter), 1 = solar festival (quarter)
 */
const MultiFestival multi_festivals[] = {
    /* TRINVX SAMONI - Three Nights of Samhain (Celtic New Year) */
    {"Trinox Samoni", "TRINVX SAMONI", 0, 28, 5, 0},  /* Last 2 days of Cantlos + first 3 of Samonios */

    /* Yule - 3 nights around Winter Solstice */
    {"Trinox Giamoni", "TRINVX GIAMONI", 1, 20, 3, 1},

    /* Imbolc - Brigid's Day, 3 nights */
    {"Trinox Imbolc", "TRINVX IMBOLC", 3, 2, 3, 0},

    /* Ostara - Vernal Equinox, 3 nights */
    {"Trinox Ostara", "TRINVX OSTARA", 4, 19, 3, 1},

    /* Beltane - 5 days (end of Cantlos + start of Samonios in summer) */
    {"Beltane IVOS", "IVOS BELTAINE", 6, 1, 5, 0},

    /* Litha - Summer Solstice, 3 nights */
    {"Trinox Litha", "TRINVX LITHA", 7, 22, 3, 1},

    /* Lughnasadh - Games of Lugh, 3 nights */
    {"Trinox Lughnasadh", "TRINVX LUGHNASADH", 9, 3, 3, 0},

    /* Mabon - Autumn Equinox, 3 nights */
    {"Trinox Mabon", "TRINVX MABON", 10, 25, 3, 1}
};

const int MULTI_FESTIVAL_COUNT = sizeof(multi_festivals)/sizeof(multi_festivals[0]);

/*
 * Check if a given day falls within a multi-day festival
 * Returns: festival index (0-7) or -1 if not a festival day
 */
int get_multi_festival(int month, int day)
{
    for (int i = 0; i < MULTI_FESTIVAL_COUNT; i++) {
        if (month == multi_festivals[i].month) {
            int start = multi_festivals[i].start_day;
            int end = start + multi_festivals[i].duration - 1;
            if (day >= start && day <= end) {
                return i;
            }
        }
    }
    return -1;
}

/*
 * Get the day number within a multi-day festival (1, 2, 3...)
 */
int get_festival_day_number(int month, int day)
{
    for (int i = 0; i < MULTI_FESTIVAL_COUNT; i++) {
        if (month == multi_festivals[i].month) {
            int start = multi_festivals[i].start_day;
            int end = start + multi_festivals[i].duration - 1;
            if (day >= start && day <= end) {
                return day - start + 1;
            }
        }
    }
    return 0;
}
