#include "manhandle.h"

struct mhrl_gui {
    int shall_exit;
    int exit_without_line;
    int c;
    int input_available;
    char *prompt;
    char *line;
    char *default_line;
};

struct mhrl_gui mhrl_gui = {
    .shall_exit = 0,
    .exit_without_line = 0,
    .c = 0,
    .input_available = 0,
    .prompt = NULL,
    .line = NULL,
    .default_line = NULL
};

int mhrl_startup_hook(void) {
    rl_insert_text(mhrl_gui.default_line);
    return 0;
}

int mhrl_getc_function(FILE *dummy) {
    mhrl_gui.input_available = 0;
    return mhrl_gui.c;
}

int mhrl_input_available_hook(void) {
    return mhrl_gui.input_available;
}

int mhrl_handle_esc(int count, int key) {
    mhrl_gui.shall_exit = 1;
    mhrl_gui.exit_without_line = 1;
    return 0;
}

void mhrl_line_handler(char *line) {
    rl_callback_handler_remove();
    mhrl_gui.shall_exit = 1;
    mhrl_gui.line = line;
}

void print_readline(void) {
    wclear(curses.msg_win);

    // Splitting the line up at the cursor seems to automatically take care of
    // cursor positioning, so all the horrible multi-byte/multi-column cases
    // don't need to be done by hand.
    char *line_head = safe_strndup(rl_line_buffer, rl_point);
    char *line_tail = rl_line_buffer + strlen(line_head);
    char *full_head = safe_malloc(strlen(mhrl_gui.prompt) + strlen(line_head) + 1);
    memcpy(full_head, mhrl_gui.prompt, strlen(mhrl_gui.prompt));
    memcpy(full_head + strlen(mhrl_gui.prompt), line_head, strlen(line_head) + 1);

    int y, x;
    mvwaddstr(curses.msg_win, 0, 0, full_head);
    getyx(curses.msg_win, y, x);
    wprintw(curses.msg_win, line_tail);
    wmove(curses.msg_win, y, x);
    wrefresh(curses.msg_win);

    free(line_head);
    free(full_head);
}

char *msg_win_readline(char *prompt, char *default_line) {
    log_debug("readline: begin readline...");

    // Initialize gui state
    mhrl_gui.shall_exit = 0;
    mhrl_gui.exit_without_line = 0;
    mhrl_gui.c = 0;
    mhrl_gui.input_available = 0;
    mhrl_gui.prompt = prompt;
    mhrl_gui.line = NULL;
    mhrl_gui.default_line = default_line;

    // readline does not handle signals
    rl_catch_signals = 0;
    rl_catch_sigwinch = 0;

    rl_redisplay_function = print_readline;
    rl_startup_hook = mhrl_startup_hook;
    rl_getc_function = mhrl_getc_function;
    rl_input_available_hook = mhrl_input_available_hook;
    rl_callback_handler_install(NULL, mhrl_line_handler);

    // Initialize terminal for readline
    keypad(curses.msg_win, FALSE);
    rl_bind_keyseq("\e\e", mhrl_handle_esc);

    int c;
    for (;;) {
        if (mhrl_gui.shall_exit)
            break;

        print_readline();
        c = wgetch(curses.msg_win);

        log_debug("readline: recieved char %d (%c)", c, c);

        switch (c) {
            case ERR:
                syscall_err("wgetch", 1);
                break;
            // case ESC:
            //     mhrl_gui.shall_exit = 1;
            //     break;
            case KEY_RESIZE:  // this is given even if keypad() is false
                curses_resize();
                break;
            default:
                mhrl_gui.c = c;
                mhrl_gui.input_available = 1;
                rl_callback_read_char();
                break;
        }
    }

    // Reset terminal settings
    keypad(curses.msg_win, TRUE);

    log_debug("readline: end readline...");
    if (mhrl_gui.exit_without_line) return (char *)NULL;
    return mhrl_gui.line;
}
