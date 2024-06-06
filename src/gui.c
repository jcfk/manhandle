#include "manhandle.h"

void state_initialize(char *files[], int file_count) {
    /* initialize state_of_gui */
    state_of_gui.page = 0;
    state_of_gui.page_count = file_count;
    messenger("");

    /* initialize state_of_decisions */
    state_of_decisions.files = safe_malloc((size_t)file_count*sizeof(struct state_of_file));
    for (int i = 0; i < file_count; i++) {
        state_of_decisions.files[i].complete = 0;
        state_of_decisions.files[i].file = files[i];

        if (STREQ(opts.paradigm, MULTI_CHOICE)) {
            mc_state_initialize(i);
        } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
            sa_state_initialize(i);
        }
    }
}

void state_free() {
    /* state_of_gui */
    free(state_of_gui.message);
    if (state_of_gui.rip_message)
        free(state_of_gui.rip_message);

    /* state_of_decisions */
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
    mvwprintw(state_of_curses.main_win, GUI_MENUBAR_Y, GUI_MENUBAR_X, 
        "q:Quit   h/left:Prev   l/right:Next   ?:Help");
}

void print_file_meta(void) {
    mvwprintw(state_of_curses.main_win, GUI_FILEMETA_Y, GUI_FILEMETA_X,
              "(%d/%d) '%s'", state_of_gui.page+1, state_of_gui.page_count, 
              state_of_decisions.files[state_of_gui.page].file);
}

void print_menu(void) {
    if (STREQ(opts.paradigm, MULTI_CHOICE)) {
        mc_print_menu();
    } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
        sa_print_menu();
    }
}

void display_file(void) {
    if (!opts.file_display) {
        messenger("No display command given (--file-display).");
        return;
    }

    /* todo: capture cursor here */

    def_prog_mode();
    endwin();

    pid_t pid = safe_fork();
    if (pid == 0) {
        int fd = open("/dev/null", O_RDONLY);
        if (fd < 0)
            syscall_err("open");

        if (dup2(fd, STDIN_FILENO) < 0) syscall_err("dup2");
        if (close(fd) < 0) syscall_err("close");

        safe_setenv("MH_FILE", state_of_decisions.files[state_of_gui.page].file, 1);

        execl("/bin/bash", "bash", "-c", opts.file_display, NULL);
        syscall_err("execl");
    }

    if (waitpid(pid, NULL, 0) < 0) syscall_err("waitpid");

    reset_prog_mode();
    wrefresh(state_of_curses.main_win);
    wrefresh(state_of_curses.msg_win);
}

void pager_file(void) {
    def_prog_mode();
    endwin();

    safe_setenv("MH_FILE", state_of_decisions.files[state_of_gui.page].file, 1);
    char *cmd;
    safe_asprintf(&cmd, "less -fc <(%s)", opts.file_pager);
    safe_system(cmd);
    free(cmd);
    safe_unsetenv("MH_FILE");

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

/* ugly */
void pager_progress(void) {
    char *progress;
    if (STREQ(opts.paradigm, MULTI_CHOICE)) {
        progress = mc_progress();
    } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
        progress = sa_progress();
    } else {
        progress = strdup("");
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

    char *tempfile = make_tmp_name();
    int tempfd = mkstemp(tempfile);
    if (tempfd < 0) {
        syscall_err("mkstemp");
    } else {
        vdprintf(tempfd, fmt, ap); /* check err? */
        if (close(tempfd) < 0) syscall_err("close");

        char *cmd;
        safe_asprintf(&cmd, "less -cS '%s'", tempfile);
        safe_system(cmd);
        free(cmd);
        if (unlink(tempfile) < 0) syscall_err("unlink");
    }

    free(tempfile);

    reset_prog_mode();
    wrefresh(state_of_curses.main_win);
    wrefresh(state_of_curses.msg_win);
}

/* general-purpose text editor */
void editor(char **strp) {
    /* choose a fallback editor, like nano */
    char* editor;
    if (opts.editor) {
        editor = opts.editor;
    } else {
        editor = getenv("EDITOR");
        if (!editor) {
            messenger("Set EDITOR in the environment.");
            return;
        }
    }

    def_prog_mode();
    endwin();

    char *tempfile = make_tmp_name();
    int tempfd = mkstemp(tempfile);
    if (tempfd < 0) {
        syscall_err("mkstemp");
    } else {
        if (*strp)
            dprintf(tempfd, "%s", *strp);
        if (close(tempfd) < 0) syscall_err("close");

        char *cmd;
        safe_asprintf(&cmd, "%s %s", editor, tempfile);
        int status = safe_system(cmd);
        free(cmd);

        if (status) {
            messenger("Editor exited with code %d. No changes saved.", status);
        } else {
            char *new_str;
            read_whole_file(tempfile, &new_str);
            if (strlen(new_str) > 0) {
                if (*strp)
                    free(*strp);
                *strp = new_str;
            } else {
                *strp = NULL;
            }
        }

        if (unlink(tempfile) < 0) syscall_err("unlink");
    }

    free(tempfile);

    reset_prog_mode();
    wrefresh(state_of_curses.main_win);
    wrefresh(state_of_curses.msg_win);
}

/* general purpose messenger */
void messenger(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    free(state_of_gui.message);
    safe_vasprintf(&state_of_gui.message, fmt, ap);
}

int ask(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char* message;
    safe_vasprintf(&message, fmt, ap);

    wclear(state_of_curses.msg_win);
    mvwprintw(state_of_curses.msg_win, 0, 0, "%s", message);
    wrefresh(state_of_curses.msg_win);
    free(message);

    char key;
    for (;;) {
        key = (char)wgetch(state_of_curses.msg_win);
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
    if (state_of_gui.page > 0) {
        state_of_gui.page -= 1;
    } else if (state_of_gui.page == 0) {
        messenger("You've reached the beginning.");
    }
}

void nav_next(void) {
    if (state_of_gui.page < state_of_gui.page_count - 1) {
        state_of_gui.page += 1;
    } else if (state_of_gui.page == state_of_gui.page_count - 1) {
        messenger("You've reached the end.");
    }
}

void ask_write_out(void) {
    if (opts.execute_immediately) {
        messenger("Cannot write out due to the presence of --execute-immediately.");
        return;
    }

    if (!ask("Confirm execute all decisions and exit? (y/n)"))
        return;

    for (int i = 0; i < state_of_gui.page_count; i+=1) {
        if (state_of_decisions.files[i].complete) {
            if (execute_decision(i)) {
                state_of_gui.exit_code = 1;
                state_of_gui.shall_exit = 1;
                return;
            }
        }
    }

    state_of_gui.rip_message = strdup("Successfully executed decisions.\n");
    state_of_gui.shall_exit = 1;
}

void ask_exit(void) {
    if (!ask("Confirm quit? (y/n)"))
        return;
    state_of_gui.shall_exit = 1;
}

/* get escape */
void handle_key(char key) {
    switch (key) {
        case 'q':
            ask_exit();
            break;
        case (char)KEY_LEFT:
        case 'h':
            nav_prev();
            break;
        case (char)KEY_RIGHT:
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
            if (STREQ(opts.paradigm, MULTI_CHOICE)) {
                mc_handle_key(key);
            } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
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
        key = (char)wgetch(state_of_curses.msg_win);

        /* change state */
        messenger("");
        handle_key(key);

        /* exit */
        if (state_of_gui.shall_exit)
            break;
    }
}
