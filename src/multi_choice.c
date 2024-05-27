#include "manhandle.h"

void mc_options_parse(int argc, char *argv[], int *i) {
    int n;
    int j = *i;
    int choice_exists = 0;
    opts.paradigm = argv[j];

    for (int k = 0; k < 10; k++)
        opts.pd_opts.mc.choices[k] = NULL;

    j += 1;
    while (j < argc && argv[j][0] == '-') {
        if (STREQ(argv[j], "--choice")) {
            j += 1;
            if (j == argc)
                err("option --choice takes an argument\n");

            n = argv[j][0] - '0';
            if (n < 0 || 9 < n)
                err("badly formed --choice \"%s\"\n", argv[j]);

            if (strlen(argv[j]) < 3 || argv[j][1] != ':')
                err("badly formed --choice \"%s\"\n", argv[j]);

            opts.pd_opts.mc.choices[n] = argv[j]+2;
            choice_exists = 1;
        } else {
            err("unknown option \"%s\" for multi-choice", argv[j]);
        }
        j += 1;
    }

    if (!choice_exists)
        err("multi-choice requires at least one --choice\n");

    if (j == argc)
        err("no files provided\n");
    *i = j;
}

void mc_state_initialize(int page) {
    state_of_decisions.files[page].decision.mc.n = 0;
}

void mc_print_menu(void) {
    int keys[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
    int key;
    char c;
    int j = 0;
    for (int i = 0; i < 10; i++) {
        key = keys[i];
        if (opts.pd_opts.mc.choices[key]) {
            if (state_of_decisions.files[state_of_gui.page].complete && 
                state_of_decisions.files[state_of_gui.page].decision.mc.n == key
            ) {
                c = '*';
            } else {
                c = ' ';
            }
            mvwprintw(state_of_curses.main_win, GUI_MENU_Y+j, GUI_MENU_X,
                      " %c%d) '%s'", c, key, opts.pd_opts.mc.choices[key]);
            j += 1;
        }
    }
}

/* todo: even chatgpt's version looks much better than this */
char *mc_progress(void) {
    char *lines;
    char *line;
    asprintf(&lines, "Page  Choice  File\n");
    for (int i = 0; i < state_of_gui.page_count; i++) {
        if (state_of_decisions.files[i].complete) {
            asprintf(&line, "%-5d %-7d %s\n", i+1,
                state_of_decisions.files[i].decision.mc.n,
                state_of_decisions.files[i].file);
        } else {
            asprintf(&line, "%-5d         %s\n", i+1,
                     state_of_decisions.files[i].file);
        }
        lines = realloc(lines, strlen(lines) + strlen(line)*sizeof(char) + 1);
        strcat(lines, line);
        free(line);
    }
    return lines;
}

int mc_execute_decision(int page) {
    int n = state_of_decisions.files[page].decision.mc.n;
    char *file = state_of_decisions.files[page].file;
    char *cmd = opts.pd_opts.mc.choices[n];

    setenv("MH_FILE", file, 1);
    int exit_code = system(cmd);
    
    if (exit_code) {
        asprintf(&state_of_gui.rip_message, 
            "Failure during execution of page %d/%d. Subshell:\n"
            "  $ MH_FILE='%s'\n"
            "  $ %s\n"
            "  exit_code: %d\n",
            page+1, state_of_gui.page_count, file, cmd, exit_code);
    }

    unsetenv("MH_FILE");

    return exit_code;
}

/* allow deselect all */
void mc_handle_key(char key) {
    int n = key - '0';

    if (n < 0 || 9 < n)
        return;

    if (!opts.pd_opts.mc.choices[n]) {
        messenger("%d) is not a choice.", n);
        return;
    }

    if (opts.execute_immediately && state_of_decisions.files[state_of_gui.page].complete) {
        messenger("Decision for page %d already executed.", state_of_gui.page+1);
        return;
    }

    state_of_decisions.files[state_of_gui.page].complete = 1;
    state_of_decisions.files[state_of_gui.page].decision.mc.n = n;

    if (opts.execute_immediately) {
        if (execute_decision(state_of_gui.page)) {
            state_of_gui.shall_exit = 1;
            return;
        }
    }

    if (state_of_gui.page < state_of_gui.page_count-1)
        nav_next();

    if (all_files_complete()) {
        if (opts.execute_immediately) {
            state_of_gui.shall_exit = 1;
            return;
        }
        messenger("All pages complete. Check progress pager and write out.");
    }
}
