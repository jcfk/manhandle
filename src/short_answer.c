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
    state_of_decisions.files[page].decision.sa.str = NULL;
}

void sa_print_menu(void) {
    mvwaddstr(state_of_curses.main_win, GUI_MENU_Y, GUI_MENU_X, "MH_STR: ");
    if (!state_of_decisions.files[state_of_gui.page].complete) {
        waddstr(state_of_curses.main_win, "not set");
    } else {
        char* str_short = strip_last_newline(state_of_decisions.files[state_of_gui.page].decision.sa.str);
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
        file = state_of_decisions.files[i].file;
        if (state_of_decisions.files[i].complete) {
            char *str_short = strip_last_newline(state_of_decisions.files[i].decision.sa.str);
            asprintf(&line, "Page: %d\nFile: '%s'\nMH_STR: \"%s\"\n\n", i, file, str_short);
            free(str_short);
        } else {
            asprintf(&line, "Page: %d\nFile: '%s'\nMH_STR: not set\n\n", i, file);
        }
        lines = realloc(lines, (strlen(lines)+strlen(line)+1)*sizeof(char));
        strcat(lines, line);
        free(line);
    }
    return lines;
}

int sa_execute_decision (int page) {
    char *file = state_of_decisions.files[page].file;
    char *str = strdup(state_of_decisions.files[page].decision.sa.str);
    char *cmd = opts.pd_opts.sa.cmd;

    setenv("MH_FILE", file, 1);
    setenv("MH_STR", strip_last_newline(str), 1);
    int exit_code = system(cmd);

    if (exit_code) {
        asprintf(&state_of_gui.rip_message, 
            "Failure during execution of page %d/%d. Subshell:\n"
            "  $ MH_FILE='%s'\n"
            "  $ MH_STR='%s'\n"
            "  $ %s\n"
            "  exit_code: %d\n",
            page+1, state_of_gui.page_count, file, str, cmd, exit_code);
    }

    free(str);
    unsetenv("MH_FILE");
    unsetenv("MH_STR");

    return exit_code;
}

/* handle --execute-immediately */
void sa_handle_key(char key) {
    if (key == 'e') {
        editor(&state_of_decisions.files[state_of_gui.page].decision.sa.str);
        if (state_of_decisions.files[state_of_gui.page].decision.sa.str) {
            state_of_decisions.files[state_of_gui.page].complete = 1;
        } else {
            state_of_decisions.files[state_of_gui.page].complete = 0;
        }
    }

    return;
}
