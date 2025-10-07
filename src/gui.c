#include "manhandle.h"

void state_initialize(char *files[], int file_count) {
    /* initialize gui */
    gui.page = 0;
    gui.page_count = file_count;
    messenger("");

    /* initialize questions */
    questions.qs = safe_malloc((size_t)file_count*sizeof(struct question));
    for (int i = 0; i < file_count; i++) {
        questions.qs[i].answered = 0;
        questions.qs[i].file = safe_strdup(files[i]);

        if (STREQ(opts.paradigm, MULTI_CHOICE)) {
            mc_state_initialize(i);
        } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
            sa_state_initialize(i);
        } else if (STREQ(opts.paradigm, FUZZY_FINDER)) {
            ff_state_initialize(i);
        }
    }
}

void state_free() {
    /* gui */
    free(gui.message);
    if (gui.rip_message)
        free(gui.rip_message);

    /* questions */
    for (int i = 0; i < gui.page_count; i++)
        free(questions.qs[i].file);
    free(questions.qs);
}

void curses_initialize(void) {
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    getmaxyx(stdscr, curses.rows, curses.cols);
    set_escdelay(REASONABLE_ESCDELAY);

    curses.main_win = newwin(curses.rows-1, curses.cols, 0, 0);
    curses.msg_win = newwin(1, curses.cols, curses.rows-1, 0);

    keypad(curses.msg_win, TRUE);
    nodelay(curses.msg_win, FALSE);
}

void curses_resize(void) {
    getmaxyx(stdscr, curses.rows, curses.cols);

    wresize(curses.main_win, curses.rows-1, curses.cols);
    wresize(curses.msg_win, 1, curses.cols);
    mvwin(curses.msg_win, curses.rows-1, 0);
}

void curses_free(void) {
    delwin(curses.main_win);
    delwin(curses.msg_win);
}

void print_menubar(void) {
    mvwprintw(curses.main_win, GUI_MENUBAR_Y, GUI_MENUBAR_X, 
              "q:Quit   h/left:Prev   l/right:Next   ?:Help");
}

void print_file_meta(void) {
    mvwprintw(curses.main_win, GUI_FILEMETA_Y, GUI_FILEMETA_X,
              "(%d/%d) '%s'", gui.page+1, gui.page_count,
              questions.qs[gui.page].file);
}

void print_menu(void) {
    if (STREQ(opts.paradigm, MULTI_CHOICE)) {
        mc_print_menu();
    } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
        sa_print_menu();
    } else if (STREQ(opts.paradigm, FUZZY_FINDER)) {
        ff_print_menu();
    }
}

void print_main_win(void) {
    wclear(curses.main_win);
    print_menubar();
    print_file_meta();
    print_menu();
    wrefresh(curses.main_win);
}

void print_msg_win(void) {
    wclear(curses.msg_win);
    mvwprintw(curses.msg_win, 0, 0, "%s", gui.message);
    wrefresh(curses.msg_win);
}

void display_file(void) {
    if (!opts.file_display) {
        messenger("No display command given (--file-display).");
        return;
    }

    def_prog_mode();
    endwin();

    pid_t pid = safe_fork();
    if (pid == 0) {
        int fd = open("/dev/null", O_RDONLY);
        if (fd < 0) syscall_err("open", 1);

        SAFE_NEG(dup2, fd, STDIN_FILENO);
        SAFE_NEG(close, fd);

        SAFE_NEG(setenv, "MH_FILE", questions.qs[gui.page].file, 1);
        SAFE_NEG(execl, "/bin/sh", "sh", "-c", opts.file_display, NULL);
    }

    SAFE_NEG(waitpid, pid, NULL, 0);

    reset_prog_mode();
    wrefresh(curses.main_win);
    wrefresh(curses.msg_win);
}

void stat_pager(void) {
    def_prog_mode();
    endwin();

    SAFE_NEG(setenv, "MH_FILE", questions.qs[gui.page].file, 1);
    char *cmd;
    SAFE_NEG_NE(asprintf, &cmd, "%s | less -c", opts.file_pager);
    safe_system(cmd);

    free(cmd);
    SAFE_NEG(unsetenv, "MH_FILE");

    reset_prog_mode();
    wrefresh(curses.main_win);
    wrefresh(curses.msg_win);
}

/* todo account for opts.paradigm */
void help_pager(void) {
    string_pager(GUI_HELP);
}

void progress_pager(void) {
    char *progress = NULL;
    if (STREQ(opts.paradigm, MULTI_CHOICE)) {
        progress = mc_progress();
    } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
        progress = sa_progress();
    } else if (STREQ(opts.paradigm, FUZZY_FINDER)) {
        progress = ff_progress();
    }
    if (progress) {
        string_pager(progress);
        free(progress);
    }
}

void log_pager(void) {
    if (log_fpath) {
        file_pager(log_fpath);
    } else {
        messenger("Logging could not be enabled; no logfile exists.");
    }
}

void string_pager(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char *tempfile = make_tmp_name();
    int tempfd = safe_mkstemp(tempfile);

    SAFE_NEG_NE(vdprintf, tempfd, fmt, ap);
    SAFE_NEG(close, tempfd);
    file_pager(tempfile);
    SAFE_NEG(unlink, tempfile);

    free(tempfile);
}

void file_pager(char *fpath) {
    def_prog_mode();
    endwin();

    char *cmd;
    SAFE_NEG_NE(asprintf, &cmd, "less -cS '%s'", fpath);
    safe_system(cmd);

    free(cmd);

    reset_prog_mode();
    wrefresh(curses.main_win);
    wrefresh(curses.msg_win);
}

/* general-purpose text editor */
void editor(char **strp) {
    if (!opts.editor) {
        messenger("No editor provided. Set EDITOR in the environment or use --editor.");
        return;
    }

    def_prog_mode();
    endwin();

    char *tempfile = make_tmp_name();
    int tempfd = safe_mkstemp(tempfile);

    if (*strp)
        SAFE_NEG_NE(dprintf, tempfd, "%s", *strp);

    SAFE_NEG(close, tempfd);

    char *cmd;
    SAFE_NEG_NE(asprintf, &cmd, "%s %s", opts.editor, tempfile);
    int status = safe_system(cmd);
    free(cmd);

    if (status) {
        /* todo this message for example could be overwritten by a message in
            a calling function */
        messenger("Editor exited with code %d. No changes saved.", status);
    } else {
        char *new_str;
        read_whole_file(tempfile, &new_str);
        if (*strp) free(*strp);
        *strp = new_str;
    }

    SAFE_NEG(unlink, tempfile);

    free(tempfile);

    reset_prog_mode();
    wrefresh(curses.main_win);
    wrefresh(curses.msg_win);
}

/* general purpose messenger */
void messenger(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    free(gui.message);
    SAFE_NEG_NE(vasprintf, &gui.message, fmt, ap);
}

int ask(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char* message;
    SAFE_NEG_NE(vasprintf, &message, fmt, ap);
    wclear(curses.msg_win);
    mvwprintw(curses.msg_win, 0, 0, "%s", message);
    wrefresh(curses.msg_win);
    free(message);

    int key;
    for (;;) {
        key = wgetch(curses.msg_win);
        switch (key) {
            case ERR:
                syscall_err("wgetch", 1);
                break;
            case 'y':
                wclear(curses.msg_win);
                wrefresh(curses.msg_win);
                return 1;
            case ESC:
            case 'n':
                wclear(curses.msg_win);
                wrefresh(curses.msg_win);
                return 0;
            default:
                break;
        }
    }
}

void nav_prev(void) {
    if (gui.page > 0) {
        gui.page -= 1;
    } else if (gui.page == 0) {
        messenger("You've reached the beginning.");
    }
}

void nav_next(void) {
    if (gui.page < gui.page_count - 1) {
        gui.page += 1;
    } else if (gui.page == gui.page_count - 1) {
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

    for (int i = 0; i < gui.page_count; i+=1) {
        if (questions.qs[i].answered) {
            if (execute_decision(i)) {
                gui.exit_code = 1;
                gui.shall_exit = 1;
                return;
            }
        }
    }

    gui.rip_message = safe_strdup("Successfully executed decisions.\n");
    gui.shall_exit = 1;
}

void ask_exit(void) {
    if (!ask("Confirm quit? (y/n)"))
        return;
    gui.shall_exit = 1;
}

void rename_current_file(void) {
    if (opts.execute_immediately && questions.qs[gui.page].answered) {
        messenger("Cannot rename file whose decision has been executed.");
        return;
    }

    char *new_filename = msg_win_readline("Rename: ", questions.qs[gui.page].file);
    if (!new_filename)
        return;

    char *cmd;
    SAFE_NEG_NE(asprintf, &cmd, "mv \"$MH_FILE\" \"%s\"", new_filename);
    SAFE_NEG(setenv, "MH_FILE", questions.qs[gui.page].file, 1);
    int status = safe_system(cmd);
    SAFE_NEG(unsetenv, "MH_FILE");
    free(cmd);
    if (status) {
        messenger("Unable to rename file (mv exited with status %d).", status);
        return;
    }

    free(questions.qs[gui.page].file);
    questions.qs[gui.page].file = new_filename;
}

void unanswer(void) {
    if (opts.execute_immediately && questions.qs[gui.page].answered) {
        messenger("Cannot unanswer page whose decision has been executed.");
        return;
    }

    questions.qs[gui.page].answered = 0;
    if (STREQ(opts.paradigm, MULTI_CHOICE)) {
        mc_unanswer(gui.page);
    } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
        sa_unanswer(gui.page);
    } else if (STREQ(opts.paradigm, FUZZY_FINDER)) {
        ff_unanswer(gui.page);
    }
}

void handle_key(int key) {
    switch (key) {
        case ERR:
            syscall_err("wgetch", 1);
            break;
        case 'q':
            ask_exit();
            break;
        case KEY_LEFT:
        case 'h':
            nav_prev();
            break;
        case KEY_RIGHT:
        case 'l':
            nav_next();
            break;
        case 'v':
            stat_pager();
            break;
        case 'd':
            display_file();
            break;
        case 'p':
            progress_pager();
            break;
        case 'o':
            log_pager();
            break;
        case 'u':
            unanswer();
            break;
        case 'w':
            ask_write_out();
            break;
        case '?':
            help_pager();
            break;
        case CONTROL('r'):
            rename_current_file();
            break;
        case KEY_RESIZE:
            gui.resized = 1;
            break;
        default:
            /* becoz each paradigm has effectively its own gui */
            if (STREQ(opts.paradigm, MULTI_CHOICE)) {
                mc_handle_key(key);
            } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
                sa_handle_key(key);
            } else if (STREQ(opts.paradigm, FUZZY_FINDER)) {
                ff_handle_key(key);
            }
            break;
    }
}

void gui_loop(void) {
    int key;
    for (;;) {
        /* print state */
        if (gui.resized) {
            curses_resize();
            gui.resized = 0;
        }
        print_main_win();
        print_msg_win();

        /* get input */
        key = wgetch(curses.msg_win);

        /* change state */
        messenger("");
        handle_key(key);

        /* exit */
        if (gui.shall_exit)
            break;
    }
}
