#include "manhandle.h"

/*
 * This readline integration is due to
 *
 * https://github.com/ulfalizer/readline-and-ncurses
 *
 * Copyright (c) 2015-2019, Ulf Magnusson <ulfalizer@gmail.com>
 */

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

void print_msg_win_readline(void) {
    wclear(curses.msg_win);
    mvwprintw(curses.msg_win, 0, 0, "%s%s", rl_prompt, rl_line_buffer);
    wmove(curses.msg_win, 0, rl_point+strlen(rl_prompt));
    wrefresh(curses.msg_win);
}

/* general purpose curses readline */
char *msg_win_readline(char *prompt, char *default_value) {
    struct timespec nano_escdelay = {.tv_nsec = get_escdelay() * 1000000};

    rl_getc_function = readline_getc;
    rl_input_available_hook = readline_input_available;

    rl_catch_signals = 0;
    rl_catch_sigwinch = 0;
    rl_change_environment = 0;

    rl_callback_handler_install(prompt, readline_handler);

    keypad(curses.msg_win, FALSE);
    if (default_value)
        rl_insert_text(default_value);
    mhreadline.reading = 1;

    int ch, ch2;
    while (mhreadline.reading) {
        /* print state */
        if (mhreadline.resized) {
            curses_resize();
            mhreadline.resized = 0;
        }
        print_main_win();
        print_msg_win_readline();

        /* get input */
        ch = wgetch(curses.msg_win);

        /* change state */
        switch (ch) {
        case ESC:
            /*
             * We are interpreting our own escape sequences. Wait ESCDELAY and
             * check for the presence of another character.
             */
            nanosleep(&nano_escdelay, NULL);

            nodelay(curses.msg_win, TRUE);
            ch2 = wgetch(curses.msg_win);
            nodelay(curses.msg_win, FALSE);

            if (ch2 < 0) {
                quit_readline();
            } else {
                readline_read_char(ch);
                readline_read_char(ch2);
            }
            break;
        case KEY_RESIZE:
            mhreadline.resized = 1;
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
