#include "manhandle.h"

void state_initialize(char *files[], int file_count) {
    state_of_gui.page = 0;
    state_of_gui.page_count = file_count;
    messenger("");

    state_of_decisions.files = malloc(file_count*sizeof(struct state_of_file));
    for (int i = 0; i < file_count; i++) {
        state_of_decisions.files[i].complete = 0;
        state_of_decisions.files[i].file = strdup(files[i]);
    }
}

void state_free() {
    free(state_of_gui.message);
    if (state_of_gui.rip_message)
        free(state_of_gui.rip_message);

    for (int i = 0; i < state_of_gui.page_count; i++) {
        free(state_of_decisions.files[i].file);
    }
    free(state_of_decisions.files);
}

void curses_initialize(void) {
    initscr();
    cbreak();
    noecho();
    getmaxyx(stdscr, state_of_curses.rows, state_of_curses.cols);
    state_of_curses.main_win = newwin(state_of_curses.rows-1,
        state_of_curses.cols, 0, 0);
    state_of_curses.msg_win = newwin(1, state_of_curses.cols,
        state_of_curses.rows-1, 0);
    keypad(state_of_curses.msg_win, TRUE);
}

void curses_resize(void) {
    delwin(state_of_curses.main_win);
    delwin(state_of_curses.msg_win);
    getmaxyx(stdscr, state_of_curses.rows, state_of_curses.cols);
    state_of_curses.main_win = newwin(state_of_curses.rows-1,
        state_of_curses.cols, 0, 0);
    state_of_curses.msg_win = newwin(1, state_of_curses.cols,
        state_of_curses.rows-1, 0);
    keypad(state_of_curses.msg_win, TRUE);
}

void curses_free(void) {
    delwin(state_of_curses.main_win);
    delwin(state_of_curses.msg_win);
}

void print_menubar(void) {
    mvwprintw(state_of_curses.main_win, 0, 0, 
        "q:Quit   h/left:Prev   l/right:Next   ?:Help");
}

void print_file_meta(void) {
    mvwprintw(state_of_curses.main_win, 2, 0, "(%d/%d) '%s'", 
        state_of_gui.page+1, state_of_gui.page_count, 
        state_of_decisions.files[state_of_gui.page].file);
}

void print_menu(void) {
    if (strcmp(opts.paradigm, MULTI_CHOICE) == 0)
        mc_print_menu();
}

void display_file(void) {
    /* todo: capture cursor here */

    def_prog_mode();
    endwin();

    pid_t pid = fork();
    if (!pid) {
        int fd = open("/dev/null", O_RDONLY);
        dup2(fd, STDIN_FILENO);
        close(fd);
        setenv("MH_FILE", state_of_decisions.files[state_of_gui.page].file, 1);
        execl("/bin/bash", "bash", "-c", opts.file_display, NULL);
    }

    waitpid(pid, NULL, 0);

    reset_prog_mode();
    wrefresh(state_of_curses.main_win);
    wrefresh(state_of_curses.msg_win);
}

void pager_file(void) {
    def_prog_mode();
    endwin();

    setenv("MH_FILE", state_of_decisions.files[state_of_gui.page].file, 1);
    char *cmd;
    asprintf(&cmd, "less -fc <(%s)", opts.file_pager);
    system(cmd);
    free(cmd);

    reset_prog_mode();
    wrefresh(state_of_curses.main_win);
    wrefresh(state_of_curses.msg_win);
}

/* account for opts.paradigm */
void pager_help(void) {
    char *help = "GUI:\n"
                 "  key      function\n"
                 "  q        Quit\n"
                 "  h/left   Prev page\n"
                 "  l/right  Next page\n"
                 "  v        Run --file-pager\n"
                 "  d        Run --file-display\n"
                 "  p        Progress pager\n"
                 "  w        Write out decisions\n"
                 "  ?        Help pager\n";
    pager(help);
}

void pager_progress(void) {
    char *progress;
    if (strcmp(opts.paradigm, MULTI_CHOICE) == 0) {
        progress = mc_progress();
    }
    pager(progress);
    free(progress);
}

/* general pager */
void pager(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    def_prog_mode();
    endwin();

    char tempfile[] = "/tmp/manhandle.XXXXXX";
    int tempfd = mkstemp(tempfile);
    vdprintf(tempfd, fmt, ap);
    close(tempfd);

    char *cmd;
    asprintf(&cmd, "less -cS '%s'", tempfile);
    system(cmd);
    free(cmd);

    unlink(tempfile);

    reset_prog_mode();
    wrefresh(state_of_curses.main_win);
    wrefresh(state_of_curses.msg_win);
}

/* general messenger */
void messenger(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    free(state_of_gui.message);
    vasprintf(&state_of_gui.message, fmt, ap);
}

int ask(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char* message;
    vasprintf(&message, fmt, ap);

    wclear(state_of_curses.msg_win);
    mvwprintw(state_of_curses.msg_win, 0, 0, "%s", message);
    wrefresh(state_of_curses.msg_win);
    free(message);

    char key;
    for (;;) {
        key = wgetch(state_of_curses.msg_win);
        switch (key) {
            case 'y':
                wclear(state_of_curses.msg_win);
                wrefresh(state_of_curses.msg_win);
                return 1;
            case 'n':
                wclear(state_of_curses.msg_win);
                wrefresh(state_of_curses.msg_win);
                return 0;
            default:
                break;
        }
    }
}

void nav_prev(void) {
    if (state_of_gui.page > 0)
        state_of_gui.page -= 1;
}

void nav_next(void) {
    if (state_of_gui.page < state_of_gui.page_count-1)
        state_of_gui.page += 1;
}

void ask_write_out(void) {
    if (opts.execute_immediately) {
        messenger("Cannot write out with --execute-immediately.");
        return;
    }

    if (!ask("Confirm write out? (y/n)"))
        return;

    for (int i = 0; i < state_of_gui.page_count; i+=1) {
        if (state_of_decisions.files[i].complete) {
            if (execute_decision(i))
                return;
        }
    }
    asprintf(&state_of_gui.rip_message, "All decisions successfully executed.\n");
    state_of_gui.shall_exit = 1;
}

void ask_exit(void) {
    if (!ask("Confirm quit? (y/n)"))
        return;
    state_of_gui.shall_exit = 1;
}

void handle_key(char key) {
    switch (key) {
        case 'q':
            ask_exit();
            break;
        case 'h':
            nav_prev();
            break;
        case 'l':
            nav_next();
            break;
        case 'v':
            pager_file();
            break;
        case 'd':
            display_file();
            break;
        case 'p':
            pager_progress();
            break;
        case 'w':
            ask_write_out();
            break;
        case '?':
            pager_help();
            break;
        case (char)KEY_RESIZE:
            state_of_gui.resized = 1;
            break;
        default: 
            /* becoz each paradigm has effectively its own gui */
            if (strcmp(opts.paradigm, MULTI_CHOICE) == 0) {
                mc_handle_key(key);
            } else if (strcmp(opts.paradigm, SHORT_ANSWER) == 0) {
                sa_handle_key(key);
            }
            break;
    }
}

void gui_loop(void) {
    char key;
    for (;;) {
        /* print state */
        if (state_of_gui.resized) {
            curses_resize();
            state_of_gui.resized = 0;
        }
        wclear(state_of_curses.main_win);
        print_menubar();
        print_file_meta();
        print_menu();
        wrefresh(state_of_curses.main_win);

        /* print message */
        wclear(state_of_curses.msg_win);
        mvwprintw(state_of_curses.msg_win, 0, 0, "%s", state_of_gui.message);
        wrefresh(state_of_curses.msg_win);

        /* get input */
        key = wgetch(state_of_curses.msg_win);

        /* change state */
        messenger("");
        handle_key(key);

        /* exit */
        if (state_of_gui.shall_exit)
            break;
    }
}

