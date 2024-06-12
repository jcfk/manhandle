#include "manhandle.h"

struct mhreadline mhreadline;

void readline_handler(char *line) {
    mhreadline.reading = 0;
    mhreadline.line = line;
}

int readline_getc(FILE *dummy) {
    mhreadline.input_available = 0;
    return mhreadline.ch;
}

int readline_input_available(void) {
    return mhreadline.input_available;
}

void readline_read_char(char ch) {
    mhreadline.input_available = 1;
    mhreadline.ch = ch;
    rl_callback_read_char();
}

void quit_readline(void) {
    mhreadline.reading = 0;
    mhreadline.line = NULL;
}

/* general purpose curses readline */
/* (inspiration of https://github.com/ulfalizer/readline-and-ncurses) */
char *msg_win_readline(char *prompt, char *default_value) {
    struct timespec nano_escdelay = {.tv_nsec = get_escdelay() * 1000000};

    /* todo probably going to want to redo this with keypad on */
    keypad(curses.msg_win, FALSE);
    rl_getc_function = readline_getc;
    rl_input_available_hook = readline_input_available;
    rl_callback_handler_install(prompt, readline_handler);

    if (default_value)
        rl_insert_text(default_value);
    mhreadline.reading = 1;

    int ch, ch2;
    while (mhreadline.reading) {
        /* print state */
        wclear(curses.msg_win);
        if (prompt) {
            mvwprintw(curses.msg_win, 0, 0, "%s%s", prompt, rl_line_buffer);
        } else {
            mvwprintw(curses.msg_win, 0, 0, "%s", rl_line_buffer);
        }
        wmove(curses.msg_win, 0, rl_point+strlen(prompt));
        wrefresh(curses.msg_win);

        /* get input */
        ch = wgetch(curses.msg_win);

        /* change state */
        switch (ch) {
        case ESC:
            nodelay(curses.msg_win, TRUE);
            nanosleep(&nano_escdelay, NULL);
            ch2 = wgetch(curses.msg_win);
            if (ch2 < 0) { /* user means ESC */
                quit_readline();
            } else {
                readline_read_char(ch);
                readline_read_char(ch2);
            }
            nodelay(curses.msg_win, FALSE);
            break;
        default:
            readline_read_char(ch);
            break;
        }
    }

    rl_callback_handler_remove();
    keypad(curses.msg_win, TRUE);

    return mhreadline.line;
}
