#include "manhandle.h"

char *log_fpath = NULL;

void logger_initialize(void) {
    char *log_dir = opts.log_dir;
    if (!log_dir)
        log_dir = get_xdg_data_dir();

    if (log_dir) {
        char *tstamp = get_timestamp(1);
        safe_asprintf(&log_fpath, "%s/%s.log", log_dir, tstamp);
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

void log_debug(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (log_fpath && opts.log_level >= 2) {
        FILE *fp = fopen(log_fpath, "a");
        fprintf(fp, "%s DEBUG: ", get_timestamp(0));
        vfprintf(fp, fmt, args);
        fprintf(fp, "\n");
        fclose(fp);
    }
}

void log_info(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (log_fpath && opts.log_level >= 1) {
        FILE *fp = fopen(log_fpath, "a");
        fprintf(fp, "%s INFO: ", get_timestamp(0));
        vfprintf(fp, fmt, args);
        fprintf(fp, "\n");
        fclose(fp);
    }
}
