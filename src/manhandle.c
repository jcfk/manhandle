#include "manhandle.h"

struct options opts = { 
    .execute_immediately = 0,
    .log_level = 1,
    .log_dir = NULL,
    .editor = NULL,
    .file_display = NULL, 
    .file_pager = NULL,
    .paradigm = NULL
};

struct questions questions;

struct gui gui = {
    .resized = 0,
    .shall_exit = 0,
    .display_pid = 0,
    .rip_message = NULL
};

struct curses curses;

void err(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    SAFE_NEG_NE(vfprintf, stderr, fmt, ap);
    exit(1);
}

void print_help(void) {
    SAFE_NEG_NE(printf, USAGE);
    exit(0);
}

void print_version(void) {
    SAFE_NEG_NE(printf, VERSION_FMT, PACKAGE_STRING);
    exit(0);
}

int options_parse(int argc, char *argv[]) {
    if (argc < 2)
        err("paradigm required\n");

    /* defaults */
    opts.file_pager = "stat \"$MH_FILE\"";
    opts.editor = getenv("EDITOR");
#ifdef HAVE_NANO
    if (!opts.editor)
        opts.editor = "nano";
#endif

    int i = 1;
    while (i < argc && argv[i][0] == '-') {
        if (STREQ(argv[i], "--editor")) {
            i += 1;
            if (i == argc)
                err("option --editor takes an argument\n");
            opts.editor = argv[i];
        } else if (STREQ(argv[i], "--stat-pager")) {
            i += 1;
            if (i == argc)
                err("option --stat-pager takes an argument\n");
            opts.file_pager = argv[i];
        } else if (STREQ(argv[i], "--file-display")) {
            i += 1;
            if (i == argc)
                err("option --file-display takes an argument\n");
            opts.file_display = argv[i];
        } else if (STREQ(argv[i], "--execute-immediately")) {
            opts.execute_immediately = 1;
        } else if (STREQ(argv[i], "--log-level")) {
            i += 1;
            if (i == argc)
                err("option --log-level takes an argument\n");
            errno = 0;
            int log_level = strtol(argv[i], NULL, 10);
            if ((log_level == 0 && errno == EINVAL) || log_level < 0 || 2 < log_level)
                err("invalid --log-level\n");
            opts.log_level = log_level;
        } else if (STREQ(argv[i], "--log-dir")) {
            i += 1;
            if (i == argc)
                err("option --log-dir takes an argument\n");
            char *log_dir = argv[i];
            if (!is_dir(log_dir))
                err("--log-dir doesn't exist\n");
            opts.log_dir = log_dir;
        } else if (STREQ(argv[i], "--help")) {
            print_help();
        } else if (STREQ(argv[i], "--version")) {
            print_version();
        } else {
            err("unknown global option \"%s\"\n", argv[i]);
        }
        i += 1;
    }

    if (i == argc)
        err("paradigm required\n");

    opts.paradigm = argv[i];
    i += 1;

    if (STREQ(opts.paradigm, MULTI_CHOICE)) {
        mc_options_parse(argc, argv, &i);
    } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
        sa_options_parse(argc, argv, &i);
    } else if (STREQ(opts.paradigm, FUZZY_FINDER)) {
        ff_options_parse(argc, argv, &i);
    } else {
        err("unknown paradigm \"%s\"\n", opts.paradigm);
    }

    if (i == argc)
        err("no files provided\n");

    return i;
}

int execute_decision(int page) {
    if (STREQ(opts.paradigm, MULTI_CHOICE)) {
        return mc_execute_decision(page);
    } else if (STREQ(opts.paradigm, SHORT_ANSWER)) {
        return sa_execute_decision(page);
    } else if (STREQ(opts.paradigm, FUZZY_FINDER)) {
        return ff_execute_decision(page);
    }
    return 0;
}

int all_files_complete(void) {
    for (int i = 0; i < gui.page_count; i++) {
        if (!questions.qs[i].answered)
            return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    int i = options_parse(argc, argv);
    char **files = argv+i;

    logger_initialize();
    state_initialize(files, argc-i);
    curses_initialize();

    gui_loop();

    curses_free();
    clear();
    refresh();
    endwin();

    if (gui.rip_message)
        SAFE_NEG_NE(printf, "%s", gui.rip_message);

    logger_free();
    state_free();
    exit(gui.exit_code);
}
