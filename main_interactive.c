#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Declare the clean UI function */
void run_interactive_ui(void);

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    /* Launch interactive UI by default */
    run_interactive_ui();
    return 0;
}
