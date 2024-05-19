#include "manhandle.h"

struct options opts = { 
    .execute_immediately = 0,
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

void err(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);

    state_free();
    options_free();
    exit(1);
}

void print_help(void) {
    printf("\n"
        "  manhandle provides an interface to expedite decision-making over files.\n"
        "\n"
        "USAGE:\n"
        "  manhandle [OPTION...] multi-choice [--choice CHOICE]... FILE...\n"
        "\n"
        "OPTIONS:\n"
        "  --file-pager _cmd_     Run this in a subshell and open stdout in a pager\n"
        "                         (default: stat \"$MH_FILE\").\n"
        "  --file-display _cmd_   Run this in a subshell (default: none).\n"
        "  --execute-immediately  Execute decisions upon entry, instead of waiting to\n"
        "                         write out.\n"
        "  --help                 Print this message.\n"
        "\n"
        "GUI:\n"
        "  key  function\n"
        "  q    Quit\n"
        "  h    Prev page\n"
        "  l    Next page\n"
        "  w    Write out decisions\n"
        "  v    Open --file-pager\n"
        "  d    Open --file-display\n"
        "  p    Progress pager\n"
        "\n"
        "PARADIGM multi-choice:\n"
        "  Multiple choice binds keys 0-9 to commands through the --choice option.\n"
        "\n"
        "  OPTIONS:\n"
        "    --choice _choice_  Format _choice_ like \"n:cmd\" (eg. 1:rm \"$MH_FILE\") to\n"
        "                       bind digit n ([0-9]) to cmd.\n"
    );

    options_free();
    exit(0);
}

/* todo: implement -- (and other standards) */
int options_parse(int argc, char *argv[]) {
    if (argc < 2)
        err("paradigm required\n");

    /* defaults */
    opts.file_pager = strdup("stat \"$MH_FILE\"");

    int i = 1;
    while (i < argc && argv[i][0] == '-') {
        if (strcmp(argv[i], "--file-pager") == 0) {
            if (i+1 == argc)
                err("option --file-pager takes an argument\n");
            free(opts.file_pager);
            opts.file_pager = strdup(argv[i+1]);
            i += 1;
        } else if (strcmp(argv[i], "--file-display") == 0) {
            if (i+1 == argc)
                err("option --file-display takes an argument\n");
            opts.file_display = strdup(argv[i+1]);
            i+= 1;
        } else if (strcmp(argv[i], "--execute-immediately") == 0) {
            opts.execute_immediately = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
        } else {
            err("unknown option \"%s\"\n", argv[i]);
        }
        i += 1;
    }

    if (i == argc)
        err("paradigm required\n");

    if (strcmp(argv[i], MULTI_CHOICE) == 0) {
        mc_options_parse(argc, argv, &i);
    } else {
        err("unknown paradigm \"%s\"\n", argv[i]);
    }

    return i;
}

void options_free(void) {
    if (opts.paradigm && strcmp(opts.paradigm, MULTI_CHOICE) == 0)
        mc_options_free();
    free(opts.file_display);
    free(opts.file_pager);
    free(opts.paradigm);
}

int execute_decision(int page) {
    if (strcmp(opts.paradigm, MULTI_CHOICE) == 0) {
        return mc_execute_decision(page);
    }
    return 0;
}

int all_files_complete(void) {
    for (int i = 0; i < state_of_gui.page_count; i++) {
        if (!state_of_decisions.files[i].complete) {
            return 0;
        }
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
    options_free();
    exit(state_of_gui.exit_code);
}

