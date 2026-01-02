#ifndef FESTIVALS_H
#define FESTIVALS_H

typedef struct {
    const char *name;
    int month;  /* 0=Samonios .. 11=Cantlos */
    int day;    /* 1..30 */
} Festival;

/*
 * Multi-Day Festival (Trinox = Three Nights)
 * Based on the Coligny calendar's IVOS patterns
 */
typedef struct {
    const char *name;           /* Modern name */
    const char *coligny_name;   /* Authentic Coligny notation */
    int month;                  /* Month index (0-11) */
    int start_day;              /* First day of festival */
    int duration;               /* Number of days/nights */
    int type;                   /* 0 = fire festival, 1 = solar */
} MultiFestival;

extern const Festival festivals[];
extern const int FESTIVAL_COUNT;

extern const MultiFestival multi_festivals[];
extern const int MULTI_FESTIVAL_COUNT;

/* Functions */
int get_multi_festival(int month, int day);
int get_festival_day_number(int month, int day);

#endif
