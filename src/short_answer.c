#include "manhandle.h"

void sa_options_parse(int argc, char *argv[], int *i) {
    int j = *i;
    opts.paradigm = argv[j];

    opts.pd_opts.sa.cmd = NULL;

    j += 1;
    while (j < argc && argv[j][0] == '-') {
        if (STREQ(argv[j], "--cmd")) {
            j += 1;
            if (j == argc)
                err("option --cmd takes an argument\n");
            opts.pd_opts.sa.cmd = argv[j];
        } else if (STREQ(argv[j], "--")) {
            j += 1;
            break;
        } else {
            err("unknown option \"%s\" for short-answer\n", argv[j]);
        }
        j += 1;
    }

    if (!opts.pd_opts.sa.cmd)
        err("short-answer requires a --cmd\n");

    *i = j;
}

void sa_state_initialize(int page) {
    questions.qs[page].answer.sa.str = NULL;
}

void sa_print_menu(void) {
    mvwaddstr(curses.main_win, GUI_MENU_Y, GUI_MENU_X, "MH_STR: ");
    if (!questions.qs[gui.page].answered) {
        waddstr(curses.main_win, "not set");
    } else {
        waddstr(curses.main_win, "\"");
        waddstr(curses.main_win, questions.qs[gui.page].answer.sa.str);
        waddstr(curses.main_win, "\"");
    }

    mvwaddstr(curses.main_win, GUI_MENU_Y+2, GUI_MENU_X,
              "(use 'e' for editor)");
}

char *sa_progress(void) {
    char *lines = strdup("");
    char *line;

    char *file;
    for (int i = 0; i < gui.page_count; i++) {
        file = questions.qs[i].file;
        if (questions.qs[i].answered) {
            safe_asprintf(&line, "Page: %d\nFile: '%s'\nMH_STR: \"%s\"\n\n",
                          i, file, questions.qs[i].answer.sa.str);
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

/* todo ugly */
int sa_execute_decision (int page) {
    char *file = questions.qs[page].file;
    char *str = questions.qs[page].answer.sa.str;
    char *cmd = opts.pd_opts.sa.cmd;

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

void sa_unanswer(int page) {
    if (questions.qs[page].answer.sa.str)
        free(questions.qs[page].answer.sa.str);
    questions.qs[page].answer.sa.str = NULL;
}

void sa_handle_key(char key) {
    if (key == 'e') {
        if (opts.execute_immediately && questions.qs[gui.page].answered) {
            messenger("Decision for page %d already executed.", gui.page+1);
            return;
        }

        editor(&questions.qs[gui.page].answer.sa.str);
        strip_last_newline(questions.qs[gui.page].answer.sa.str);

        /* editor cannot leave str as NULL */
        questions.qs[gui.page].answered = 1;

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
