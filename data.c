#include "data.h"

/* Giamos-first ordering to align with calendar.c (includes intercalary Quimonios, Rantaranos) */
const char *celtic_month_names[NUM_MONTHS] = {
    "Giamonios", "Simivisonnos", "Equos", "Elembivios",
    "Aedrinios", "Cantlos", "Samonios", "Dumannios",
    "Riuros", "Anagantios", "Ogronnios", "Cutios", "Quimonios",
    "Rantaranos"
};

/* 8-step lunar cycle: 0=new → 4=full → 7=waning crescent (use explicit moon emoji) */
const char *moon_symbols[8] = { "🌑","🌒","🌓","🌔","🌕","🌖","🌗","🌘" };

const char *zodiac_symbols[12] = { "♈","♉","♊","♋","♌","♍","♎","♏","♐","♑","♒","♓" };
