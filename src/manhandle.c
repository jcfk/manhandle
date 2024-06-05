#include "manhandle.h"

struct options opts = { 
    .execute_immediately = 0,
    .editor = NULL,
    .file_display = NULL, 
    .file_pager = NULL,
    .paradigm = NULL
};
struct state_of_decisions state_of_decisions;
struct state_of_gui state_of_gui = {
    .resized = 0,
    .shall_exit = 0,
    .display_pid = 0,
    .rip_message = NULL
};
struct state_of_curses state_of_curses;

void print_help(void) {
    printf(USAGE);
    exit(0);
}

/* todo: just use getopt */
int options_parse(int argc, char *argv[]) {
    if (argc < 2)
        err("paradigm required\n");

    /* defaults */
    opts.file_pager = "stat \"$MH_FILE\"";

    int i = 1;
    while (i < argc && argv[i][0] == '-') {
        if (STREQ(argv[i], "--editor")) {
            if (i+1 == argc)
                err("option --editor takes an argument\n");
            opts.editor = argv[i+1];
            i += 1;
        } else if (STREQ(argv[i], "--file-pager")) {
            if (i+1 == argc)
                err("option --file-pager takes an argument\n");
            opts.file_pager = argv[i+1];
            i += 1;
        } else if (STREQ(argv[i], "--file-display")) {
            if (i+1 == argc)
                err("option --file-display takes an argument\n");
            opts.file_display = argv[i+1];
            i+= 1;
        } else if (STREQ(argv[i], "--execute-immediately")) {
            opts.execute_immediately = 1;
        } else if (STREQ(argv[i], "--help")) {
            print_help();
        } else {
            err("unknown option \"%s\"\n", argv[i]);
        }
        i += 1;
    }

    if (i == argc)
        err("paradigm required\n");

    if (STREQ(argv[i], MULTI_CHOICE)) {
        mc_options_parse(argc, argv, &i);
    } else if (STREQ(argv[i], SHORT_ANSWER)) {
        sa_options_parse(argc, argv, &i);
    } else {
        err("unknown paradigm \"%s\"\n", argv[i]);
    }

    return i;
}

int execute_decision(int page) {
    if (STREQ(opts.paradigm, MULTI_CHOICE)) {
        return mc_execute_decision(page);
    } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
        return sa_execute_decision(page);
    }
    return 0;
}

int all_files_complete(void) {
    for (int i = 0; i < state_of_gui.page_count; i++) {
        if (!state_of_decisions.files[i].complete)
            return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    int i = options_parse(argc, argv);
    char **files = argv+i;

    state_initialize(files, argc-i);
    curses_initialize();

    gui_loop();

    curses_free();
    clear();
    refresh();
    endwin();

    if (state_of_gui.rip_message)
        printf("%s", state_of_gui.rip_message);

    state_free();
    exit(state_of_gui.exit_code);
}
