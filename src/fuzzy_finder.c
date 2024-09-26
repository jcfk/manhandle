#include "manhandle.h"

void ff_options_parse(int argc, char *argv[], int *i) {
    int j = *i;
    opts.paradigm = argv[j];
    j += 1;

    opts.pd_opts.ff.fuzzy_finder_cmd = NULL;
    opts.pd_opts.ff.action_cmd = NULL;

    while (j < argc && argv[j][0] == '-') {
        if (STREQ(argv[j], "--fuzzy-finder")) {
            j += 1;
            if (j == argc)
                err("option --fuzzy-finder takes an argument\n");
            opts.pd_opts.ff.fuzzy_finder_cmd = argv[j];
        } else if (STREQ(argv[j], "--action")) {
            j += 1;
            if (j == argc)
                err("option --action takes an argument\n");
            opts.pd_opts.ff.action_cmd = argv[j];
        } else if (STREQ(argv[j], "--")) {
            j += 1;
            break;
        } else {
            err("unknown option \"%s\" for fuzzy-finder\n", argv[j]);
        }
        j += 1;
    }

    if (!opts.pd_opts.ff.fuzzy_finder_cmd)
        err("fuzzy-finder requires option --fuzzy-finder\n");
    if (!opts.pd_opts.ff.action_cmd)
        err("fuzzy-finder requires option --action\n");

    *i = j;
}

void ff_state_initialize(int page) {
    questions.qs[page].answer.ff.str = NULL;
}

void ff_print_menu(void) {
    mvwaddstr(curses.main_win, GUI_MENU_Y, GUI_MENU_X, "MH_STR: ");
    if (!questions.qs[gui.page].answered) {
        waddstr(curses.main_win, "not set");
    } else {
        waddstr(curses.main_win, "\"");
        waddstr(curses.main_win, questions.qs[gui.page].answer.ff.str);
        waddstr(curses.main_win, "\"");
    }

    mvwaddstr(curses.main_win, GUI_MENU_Y+2, GUI_MENU_X,
              "(use 'f' for fuzzy finder)");
}

char *ff_progress(void) {
    char *lines = strdup("");
    char *line;

    char *file;
    char *str;
    for (int i = 0; i < gui.page_count; i++) {
        file = questions.qs[i].file;
        if (questions.qs[i].answered) {
            safe_asprintf(&line, "Page: %d\nFile: '%s'\nMH_STR: \"%s\"\n\n",
                          i, file, questions.qs[i].answer.ff.str);
        } else {
            safe_asprintf(&line, "Page: %d\nFile: '%s'\nMH_STR: not set\n\n",
                          i, file);
        }
        lines = realloc(lines, (strlen(lines)+strlen(line)+1)*sizeof(char));
        strcat(lines, line);
        free(line);
    }

    return lines;
}

int ff_execute_decision(int page) {
    char *file = questions.qs[page].file;
    char *str = questions.qs[page].answer.ff.str;
    char *cmd = opts.pd_opts.ff.action_cmd;

    safe_setenv("MH_FILE", file, 1);
    safe_setenv("MH_STR", str, 1);
    int status = safe_system(cmd);
    if (status)
        safe_asprintf(&gui.rip_message,
                      "Failure during execution of page %d/%d. Shell:\n"
                      "  $ MH_FILE='%s'\n"
                      "  $ MH_STR='%s'\n"
                      "  $ %s\n"
                      "  exit code: %d\n",
                      page+1, gui.page_count, file, str, cmd, status);
    safe_unsetenv("MH_FILE");
    safe_unsetenv("MH_STR");

    return status;
}

void ff_fuzzy_find_current_answer() {
    def_prog_mode();
    endwin();

    char *file = questions.qs[gui.page].file;
    char *str = questions.qs[gui.page].answer.ff.str;
    char *new_str;

    safe_setenv("MH_FILE", file, 1);
    int status = get_cmd_stdout(opts.pd_opts.ff.fuzzy_finder_cmd, &new_str);
    strip_last_newline(new_str);
    safe_unsetenv("MH_FILE");

    if (status) {
        free(new_str);
        messenger("Fuzzy finder exited with code %d. No changes saved.", status);
    } else {
        if (str) free(str);
        questions.qs[gui.page].answer.ff.str = new_str;
        questions.qs[gui.page].answered = 1;
    }

    reset_prog_mode();
    wrefresh(curses.main_win);
    wrefresh(curses.msg_win);
}

void ff_unanswer(int page) {
    if (questions.qs[gui.page].answer.ff.str)
        free(questions.qs[gui.page].answer.ff.str);
    questions.qs[gui.page].answer.ff.str = NULL;
}

void ff_handle_key(char key) {
    if (key == 'f') {
        if (opts.execute_immediately && questions.qs[gui.page].answered) {
            messenger("Decision for page %d already executed.", gui.page+1);
            return;
        }

        ff_fuzzy_find_current_answer();

        if (questions.qs[gui.page].answered) {
            if (opts.execute_immediately) {
                if (execute_decision(gui.page)) {
                    gui.shall_exit = 1;
                    return;
                }
                if (all_files_complete()) {
                    gui.shall_exit = 1;
                    gui.rip_message = strdup("Successfully executed decisions.\n");
                    return;
                }
            } else {
                if (all_files_complete())
                    messenger("All pages complete. Check progress pager and write out.");
            }
        }
    }
}
