#include "manhandle.h"

char *log_fpath = NULL;

void logger_initialize(void) {
    char *log_dir = opts.log_dir;
    if (!log_dir)
        log_dir = get_xdg_data_dir();

    if (log_dir) {
        char *tstamp = get_timestamp(1);
        SAFE_NEG_NE(asprintf, &log_fpath, "%s/%s.log", log_dir, tstamp);
        free(tstamp);

        if (!opts.log_dir)
            free(log_dir);

        log_debug("logger initialized");
    }
}

void logger_free(void) {
    if (log_fpath)
        free(log_fpath);
}

void log_base(char *level, char *fmt, va_list args) {
    FILE *fp = safe_fopen(log_fpath, "a");
    SAFE_NEG_NE(fprintf, fp, "%s %s: ", get_timestamp(0), level);
    SAFE_NEG_NE(vfprintf, fp, fmt, args);
    SAFE_NEG_NE(fprintf, fp, "\n");
    SAFE_VAL(EOF, fclose, fp);
}

void log_debug(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (log_fpath && opts.log_level >= 2)
        log_base("DEBUG", fmt, args);
}

void log_info(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (log_fpath && opts.log_level >= 1)
        log_base("INFO", fmt, args);
}
