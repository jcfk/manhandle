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

    if (j == argc)
        err("no files provided\n");

    *i = j;
}

void sa_state_initialize(int page) {
    state_of_questions.qs[page].answer.sa.str = NULL;
}

void sa_print_menu(void) {
    mvwaddstr(state_of_curses.main_win, GUI_MENU_Y, GUI_MENU_X, "MH_STR: ");
    if (!state_of_questions.qs[state_of_gui.page].answered) {
        waddstr(state_of_curses.main_win, "not set");
    } else {
        char* str_short = strip_last_newline(state_of_questions.qs[state_of_gui.page].answer.sa.str);
        waddstr(state_of_curses.main_win, "\"");
        waddstr(state_of_curses.main_win, str_short);
        waddstr(state_of_curses.main_win, "\"");
        free(str_short);
    }

    mvwaddstr(state_of_curses.main_win, GUI_MENU_Y+2, GUI_MENU_X,
              "(use 'e' for editor)");
}

char *sa_progress(void) {
    char *lines = strdup("");
    char *line;

    char *file;
    for (int i = 0; i < state_of_gui.page_count; i++) {
        file = state_of_questions.qs[i].file;
        if (state_of_questions.qs[i].answered) {
            char *str_short = strip_last_newline(state_of_questions.qs[i].answer.sa.str);
            safe_asprintf(&line, "Page: %d\nFile: '%s'\nMH_STR: \"%s\"\n\n",
                          i, file, str_short);
            free(str_short);
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

int sa_execute_decision (int page) {
    char *file = state_of_questions.qs[page].file;
    char *str = strdup(state_of_questions.qs[page].answer.sa.str);
    char *cmd = opts.pd_opts.sa.cmd;

    safe_setenv("MH_FILE", file, 1);
    safe_setenv("MH_STR", strip_last_newline(str), 1);
    int status = safe_system(cmd);
    if (status)
        safe_asprintf(&state_of_gui.rip_message, 
                      "Failure during execution of page %d/%d. Shell:\n"
                      "  $ MH_FILE='%s'\n"
                      "  $ MH_STR='%s'\n"
                      "  $ %s\n"
                      "  exit code: %d\n",
                      page+1, state_of_gui.page_count, file, str, cmd, status);
    safe_unsetenv("MH_FILE");
    safe_unsetenv("MH_STR");

    free(str);
    return status;
}

/* todo handle --execute-immediately */
void sa_handle_key(char key) {
    if (key == 'e') {
        editor(&state_of_questions.qs[state_of_gui.page].answer.sa.str);
        if (state_of_questions.qs[state_of_gui.page].answer.sa.str) {
            state_of_questions.qs[state_of_gui.page].answered = 1;
        } else {
            state_of_questions.qs[state_of_gui.page].answered = 0;
        }
    }

    return;
}
