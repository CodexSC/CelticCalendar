#define _GNU_SOURCE
#include <ncursesw/curses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <stdio.h>
#include "calendar.h"
#include "astronomy.h"
#include "festivals.h"
#include "glyphs.h"

/* Default location: Coligny, France */
#define LATITUDE 46.38

/* Color pair definitions */
#define COLOR_PAIR_TITLE 1
#define COLOR_PAIR_BORDER 2
#define COLOR_PAIR_AUSPICIOUS 3
#define COLOR_PAIR_INAUSPICIOUS 4
#define COLOR_PAIR_NEUTRAL 5
#define COLOR_PAIR_TODAY 6
#define COLOR_PAIR_FESTIVAL 7
#define COLOR_PAIR_MENU 8
#define COLOR_PAIR_SELECTED 9

/* Menu options */
#define MENU_TODAY 0
#define MENU_SEARCH_DATE 1
#define MENU_NEXT_MONTH 2
#define MENU_PREV_MONTH 3
#define MENU_QUIT 4
#define MENU_COUNT 5

static const char *menu_items[] = {
    "View Today's Calendar",
    "Search for Specific Date",
    "Next Celtic Month",
    "Previous Celtic Month",
    "Quit to Terminal"
};

/* Ensure we have a UTF-8 locale so emoji/line glyphs render correctly */
static int ensure_utf8_locale(void)
{
    setlocale(LC_ALL, "");
    const char *codeset = nl_langinfo(CODESET);
    if (!codeset || strcmp(codeset, "UTF-8") != 0) {
        setlocale(LC_ALL, "en_US.UTF-8");
        codeset = nl_langinfo(CODESET);
    }
    return codeset && strcmp(codeset, "UTF-8") == 0;
}

static void init_colors(void)
{
    if (!has_colors()) {
        return;
    }

    start_color();

    /* Simple, reliable color scheme */
    init_pair(COLOR_PAIR_TITLE, COLOR_YELLOW, COLOR_BLACK);      /* Title - gold */
    init_pair(COLOR_PAIR_BORDER, COLOR_GREEN, COLOR_BLACK);      /* Borders - green */
    init_pair(COLOR_PAIR_AUSPICIOUS, COLOR_BLUE, COLOR_BLACK);   /* MD - Blue */
    init_pair(COLOR_PAIR_INAUSPICIOUS, COLOR_RED, COLOR_BLACK);  /* D AMB - Red */
    init_pair(COLOR_PAIR_NEUTRAL, COLOR_WHITE, COLOR_BLACK);     /* D - White */
    init_pair(COLOR_PAIR_TODAY, COLOR_BLACK, COLOR_YELLOW);      /* Today highlight */
    init_pair(COLOR_PAIR_FESTIVAL, COLOR_GREEN, COLOR_BLACK);    /* Festivals - Green */
    init_pair(COLOR_PAIR_MENU, COLOR_WHITE, COLOR_BLACK);        /* Menu items */
    init_pair(COLOR_PAIR_SELECTED, COLOR_BLACK, COLOR_GREEN);    /* Selected menu */
}

/* Simple repeating knot band to hint at Celtic braidwork (ASCII-safe) */
static void draw_knot_band(WINDOW *win, int y, int width)
{
    static const char *knot = "<><=><>";
    int pat_len = (int)strlen(knot);
    for (int x = 1; x < width - 1; x += pat_len) {
        mvwaddnstr(win, y, x, knot, width - 2 - x);
    }
}

static void draw_celtic_border(WINDOW *win, int height, int width, const char *title)
{
    /* ASCII-only border to avoid ACS charset issues */
    /* Top/bottom */
    mvwhline(win, 0, 0, '-', width);
    mvwhline(win, height - 1, 0, '-', width);
    /* Sides */
    for (int y = 1; y < height - 1; y++) {
        mvwaddch(win, y, 0, '|');
        mvwaddch(win, y, width - 1, '|');
    }
    /* Corners */
    mvwaddch(win, 0, 0, '+');
    mvwaddch(win, 0, width - 1, '+');
    mvwaddch(win, height - 1, 0, '+');
    mvwaddch(win, height - 1, width - 1, '+');

    if (title) {
        int title_len = (int)strlen(title);
        int x = (width - title_len - 2) / 2;
        if (x < 1) x = 1;
        mvwprintw(win, 0, x, " %s ", title);
    }

    /* Interior knot band under the title for a subtle Celtic accent */
    draw_knot_band(win, 1, width);
}

static void draw_main_menu(WINDOW *win, int selected)
{
    int height, width;
    getmaxyx(win, height, width);

    werase(win);
    draw_celtic_border(win, height, width, "CELTIC CALENDAR");

    /* Celtic decorative header */
    wattron(win, COLOR_PAIR(COLOR_PAIR_BORDER) | A_BOLD);
    draw_knot_band(win, 2, width);
    mvwprintw(win, 3, (width - 26) / 2, "<< Coligny Celtic Calendar >>");
    draw_knot_band(win, 4, width);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_BORDER) | A_BOLD);

    /* Menu items */
    int start_y = 7;
    for (int i = 0; i < MENU_COUNT; i++) {
        if (i == selected) {
            wattron(win, COLOR_PAIR(COLOR_PAIR_SELECTED) | A_BOLD);
            mvwprintw(win, start_y + i * 2, (width - strlen(menu_items[i]) - 4) / 2,
                     "> %s <", menu_items[i]);
            wattroff(win, COLOR_PAIR(COLOR_PAIR_SELECTED) | A_BOLD);
        } else {
            wattron(win, COLOR_PAIR(COLOR_PAIR_MENU));
            mvwprintw(win, start_y + i * 2, (width - strlen(menu_items[i])) / 2,
                     "%s", menu_items[i]);
            wattroff(win, COLOR_PAIR(COLOR_PAIR_MENU));
        }
    }

    /* Instructions */
    wattron(win, COLOR_PAIR(COLOR_PAIR_BORDER));
    mvwprintw(win, height - 4, (width - 50) / 2, "Up/Down Arrows  ENTER/SPACE Select  Q/ESC Quit");
    mvwprintw(win, height - 3, (width - 30) / 2, "Selected: %d (%s)", selected, menu_items[selected]);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_BORDER));
    wrefresh(win);
}

/* Render the rich text month view (same as CLI) into the ncurses window */
static void render_month_view(WINDOW *win, long jd_actual)
{
    int height, width;
    getmaxyx(win, height, width);

    /* Compute Celtic context for the month being viewed and anchor "Today" to the real current date */
    time_t t = time(NULL);
    struct tm *local = localtime(&t);
    double current_hour = 12.0;
    if (local) current_hour = local->tm_hour + local->tm_min / 60.0;

    long today_jd = jd_today();
    int after_sunset = is_after_sunset(today_jd, current_hour, LATITUDE);
    long celtic_today_jd = celtic_jd_from_time(today_jd, current_hour, LATITUDE);

    long celtic_view_jd = celtic_jd_from_time(jd_actual, current_hour, LATITUDE);
    int month_idx = lunar_celtic_month_index(celtic_view_jd);
    long jd_month_start = find_full_moon_before(celtic_view_jd);
    int month_days = lunar_month_length(celtic_view_jd);

    /* Capture CLI rendering into a buffer */
    char *buf = NULL;
    size_t len = 0;
    FILE *mem = open_memstream(&buf, &len);
    if (!mem) {
        /* Show a helpful message instead of a blank screen when buffer creation fails */
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 1, 2, "Unable to render calendar (open_memstream failed).");
        mvwprintw(win, 2, 2, "Try recompiling with -lncursesw -lm or check memory limits.");
        wrefresh(win);
        wgetch(win);
        return;
    }

    FILE *saved = stdout;
    stdout = mem;
    print_celtic_month_lunar(month_idx, jd_month_start, celtic_today_jd, today_jd, month_days, after_sunset);
    fflush(mem);
    stdout = saved;
    fclose(mem);

    if (!buf || len == 0) {
        /* Protect against empty output so the user sees an actionable message */
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 1, 2, "Calendar output was empty.");
        mvwprintw(win, 2, 2, "Run the CLI (./celtic_calendar) to verify data, then retry.");
        wrefresh(win);
        wgetch(win);
        free(buf);
        return;
    }

    /* Display buffer line by line inside the window */
    /* Split buffer into lines to load into a pad for scrolling */
    int line_count = 0;
    int max_line_len = 0;
    for (char *p = buf; *p; p++) {
        if (*p == '\n') {
            line_count++;
        }
    }
    line_count++; /* last line */

    char *cursor = buf;
    for (int i = 0; i < line_count; i++) {
        char *nl = strchr(cursor, '\n');
        int len_line = nl ? (int)(nl - cursor) : (int)strlen(cursor);
        if (len_line > max_line_len) max_line_len = len_line;
        if (!nl) break;
        cursor = nl + 1;
    }

    int pad_h = line_count + 2;
    int pad_w = (max_line_len + 4 > width - 2) ? max_line_len + 4 : width - 2;
    WINDOW *pad = newpad(pad_h, pad_w);
    if (!pad) { free(buf); return; }

    cursor = buf;
    for (int row = 0; row < line_count; row++) {
        char *nl = strchr(cursor, '\n');
        int len_line = nl ? (int)(nl - cursor) : (int)strlen(cursor);
        char saved = '\0';
        if (nl) { saved = cursor[len_line]; cursor[len_line] = '\0'; }
        mvwprintw(pad, row, 0, "%s", cursor);
        if (nl) {
            cursor[len_line] = saved;
            cursor = nl + 1;
        }
    }

    /* Constrain drawing to the inside of the border */
    int view_h = height - 3;  /* leave one interior row for the help text */
    if (view_h < 1) view_h = 1;
    int view_w = width - 2;   /* interior cols */
    int top = 0;
    int max_top = pad_h - view_h;
    if (max_top < 0) max_top = 0;

    int win_y, win_x;
    getbegyx(win, win_y, win_x);

    int ch;
    do {
        werase(win);
        box(win, 0, 0);

        wattron(win, COLOR_PAIR(COLOR_PAIR_MENU));
        mvwprintw(win, height - 2, 1, "↑↓ PgUp/PgDn scroll • Home/End • q/ESC back");
        wattroff(win, COLOR_PAIR(COLOR_PAIR_MENU));

        /* Draw border/help first, then overlay the pad so content stays visible */
        wrefresh(win);

        /* Render pad inside the window border; offsets keep content visible */
        prefresh(pad, top, 0,
             win_y + 1, win_x + 1,
             win_y + view_h, win_x + view_w);

        ch = wgetch(win);
        switch (ch) {
            case KEY_UP:    if (top > 0) top--; break;
            case KEY_DOWN:  if (top < max_top) top++; break;
            case KEY_PPAGE: top -= view_h; if (top < 0) top = 0; break;
            case KEY_NPAGE: top += view_h; if (top > max_top) top = max_top; break;
            case KEY_HOME:  top = 0; break;
            case KEY_END:   top = max_top; break;
            default: break;
        }
    } while (ch != 'q' && ch != 'Q' && ch != 27);

    delwin(pad);
    free(buf);
}

static void display_calendar_view(WINDOW *win, long jd_date)
{
    render_month_view(win, jd_date);
}

static long get_date_from_user(WINDOW *win)
{
    int height, width;
    getmaxyx(win, height, width);

    werase(win);
    draw_celtic_border(win, height, width, "SEARCH FOR DATE");

    wattron(win, COLOR_PAIR(COLOR_PAIR_MENU) | A_BOLD);
    mvwprintw(win, 3, (width - 40) / 2, "Enter Gregorian Date");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_MENU) | A_BOLD);

    int year, month, day;

    echo();
    curs_set(1);

    wattron(win, COLOR_PAIR(COLOR_PAIR_NEUTRAL));
    mvwprintw(win, 6, (width - 20) / 2, "Year (e.g. 2026): ");
    wscanw(win, "%d", &year);

    mvwprintw(win, 8, (width - 20) / 2, "Month (1-12): ");
    wscanw(win, "%d", &month);

    mvwprintw(win, 10, (width - 20) / 2, "Day (1-31): ");
    wscanw(win, "%d", &day);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_NEUTRAL));

    noecho();
    curs_set(0);

    return jd_from_ymd(year, month, day);
}

void run_interactive_ui(void)
{
    /* Prefer UTF-8 so emoji/line art render instead of mojibake */
    int has_utf8 = ensure_utf8_locale();

    /* Initialize ncurses */
    if (initscr() == NULL) {
        fprintf(stderr, "Error initializing ncurses\n");
        return;
    }

    if (!has_utf8) {
        /* Surface a warning but keep running; user can fix env and restart */
        attron(A_BOLD);
        mvprintw(0, 0, "Warning: non-UTF-8 locale; set LANG/LC_ALL=en_US.UTF-8 for full glyphs.");
        attroff(A_BOLD);
        refresh();
    }

    /* Check minimum terminal size */
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    if (max_y < 20 || max_x < 60) {
        endwin();
        fprintf(stderr, "Terminal too small. Need at least 20x60.\n");
        return;
    }

    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(50);  /* 50ms timeout for input */

    /* Comprehensive mouse disabling */
    printf("\033[?1000l");  /* Disable X10 mouse reporting */
    printf("\033[?1001l");  /* Disable highlight tracking */
    printf("\033[?1002l");  /* Disable button event tracking */
    printf("\033[?1003l");  /* Disable any motion tracking */
    printf("\033[?1004l");  /* Disable focus in/out events */
    printf("\033[?1005l");  /* Disable UTF-8 mouse mode */
    printf("\033[?1006l");  /* Disable SGR mouse mode */
    printf("\033[?1015l");  /* Disable urxvt mouse mode */
    printf("\033[?2004l");  /* Disable bracketed paste mode */
    fflush(stdout);

    /* Initialize colors */
    init_colors();

    /* Create main window */
    WINDOW *main_win = newwin(max_y - 2, max_x - 4, 1, 2);
    if (main_win == NULL) {
        endwin();
        fprintf(stderr, "Error creating window\n");
        return;
    }

    /* Enable window input */
    keypad(main_win, TRUE);

    int selected = 0;
    int running = 1;
    long current_jd = jd_today();

    /* Clear screen and refresh */
    clear();
    refresh();

    while (running) {
        draw_main_menu(main_win, selected);

        int ch = getch();

        /* Handle timeout (ERR) - just continue loop */
        if (ch == ERR) {
            continue;
        }

        /* Simple escape sequence filtering */
        if (ch == 27) {  /* ESC - either quit or mouse sequence */
            timeout(10);  /* Quick timeout to check for sequence */
            int next_ch = getch();
            timeout(50);  /* Restore normal timeout */

            if (next_ch == ERR) {
                /* Single ESC - quit */
                running = 0;
                break;
            } else {
                /* Part of escape sequence - ignore and continue */
                continue;
            }
        }

        switch (ch) {
            case KEY_UP:
                selected = (selected - 1 + MENU_COUNT) % MENU_COUNT;
                break;

            case KEY_DOWN:
                selected = (selected + 1) % MENU_COUNT;
                break;

            case 10:  /* LF - Line Feed */
            case 13:  /* CR - Carriage Return */
            case ' ':  /* Space bar as alternative */
            case KEY_ENTER:
                timeout(-1);  /* Make operations blocking */
                switch (selected) {
                    case MENU_TODAY:
                        current_jd = jd_today();
                        display_calendar_view(main_win, current_jd);
                        break;

                    case MENU_SEARCH_DATE:
                        current_jd = get_date_from_user(main_win);
                        display_calendar_view(main_win, current_jd);
                        break;

                    case MENU_NEXT_MONTH:
                        {
                            /* Align navigation with the lunar month rendering used in this UI. */
                            long celtic_jd = celtic_jd_from_time(current_jd, 12.0, LATITUDE);
                            long month_start = find_full_moon_before(celtic_jd);
                            int month_len = lunar_month_length(celtic_jd);
                            current_jd = month_start + month_len + 1; /* jump to start of next lunar month */
                        }
                        display_calendar_view(main_win, current_jd);
                        break;

                    case MENU_PREV_MONTH:
                        {
                            long celtic_jd = celtic_jd_from_time(current_jd, 12.0, LATITUDE);
                            long month_start = find_full_moon_before(celtic_jd);
                            /* Step to just before this month's full moon, then rendering will pick the prior month. */
                            current_jd = month_start - 1;
                        }
                        display_calendar_view(main_win, current_jd);
                        break;

                    case MENU_QUIT:
                        running = 0;
                        break;
                }
                timeout(50);  /* Return to normal timeout after any selection */
                break;

            case 'q':
            case 'Q':
                running = 0;
                break;
        }
    }

    /* Cleanup */
    delwin(main_win);

    /* Restore terminal settings and disable any mouse tracking */
    printf("\033[?1000l");  /* Disable mouse tracking */
    printf("\033[?1002l");  /* Disable button event tracking */
    printf("\033[?1003l");  /* Disable any motion tracking */
    printf("\033[0m");      /* Reset all attributes */
    fflush(stdout);

    endwin();
}
