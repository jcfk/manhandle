#include "manhandle.h"

void syscall_err(char *syscall) {
    err("%s() failed with errno %d: %s.\n", syscall, errno, strerror(errno));
}

void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr)
        syscall_err("malloc");
    return ptr;
}

void *safe_realloc(void *ptr, size_t size) {
    void *nptr = realloc(ptr, size);
    if (!nptr)
        syscall_err("realloc");
    return nptr;
}

void safe_asprintf(char **strp, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    safe_vasprintf(strp, fmt, ap);
}

void safe_vasprintf(char **strp, char *fmt, va_list ap) {
    if (vasprintf(strp, fmt, ap) < 0) syscall_err("vasprintf");
}

int safe_system(char *command) {
    int status = system(command);
    if (status < 0) syscall_err("system");
    return status;
}

pid_t safe_fork(void) {
    pid_t pid = fork();
    if (pid < 0)
        syscall_err("fork");
    return pid;
}

void safe_setenv(char *name, char *value, int overwrite) {
    if (setenv(name, value, overwrite) < 0) syscall_err("setenv");
}

void safe_unsetenv(char *name) {
    if (unsetenv(name) < 0) syscall_err("unsetenv");
}

void strip_last_newline(char *str) {
    if (str[strlen(str)-1] == '\n')
        str[strlen(str)-1] = '\0';
}

void read_whole_file(char *fpath, char **strp) {
    FILE *fp = fopen(fpath, "r");
    if (!fp)
        syscall_err("fopen");

    if (fseek(fp, 0L, SEEK_END) < 0) syscall_err("fseek");

    long length = ftell(fp);
    if (length < 0)
        syscall_err("ftell");

    if (fseek(fp, 0L, SEEK_SET) < 0) syscall_err("fseek");

    *strp = safe_malloc(sizeof(char)*length + 1);
    size_t transfered = fread(*strp, sizeof(char), length, fp);
    if (transfered != (size_t)length)
        err("unable to read file %s", fpath);
    (*strp)[length] = '\0';

    if (fclose(fp) < 0) syscall_err("fclose");
}

char *make_tmp_name(void) {
    char *tmpdir = getenv("TMPDIR");
    if (!tmpdir)
        tmpdir = "/tmp";
    /* todo how much format checking to do here? */
    char *tempfile = malloc(sizeof(char)*
                            (strlen(tmpdir)+strlen("/manhandle.XXXXXX")+1));
    strcpy(tempfile, tmpdir);
    strcat(tempfile, "/manhandle.XXXXXX");
    return tempfile;
}

int get_cmd_stdout(char *cmd, char **strp) {
    FILE *fp = popen(cmd, "r");

    /* what block size? */
    int bs = GET_CMD_STDOUT_BS;
    size_t alloced = 1;
    char *str = malloc(sizeof(char)*alloced);
    str[0] = '\0';
    char block[bs];
    while (1) {
        if (!fgets(block, sizeof(char)*bs, fp))
            break;
        if (strlen(str)+strlen(block)+1 > alloced) {
            alloced += bs;
            str = realloc(str, sizeof(char)*alloced);
        }
        strcat(str, block);
    }

    *strp = str;
    return pclose(fp);
}

int is_dir(char *path) {
    struct stat s;
    int e = stat(path, &s) < 0;
    // check errno?
    if (e < 0 || !S_ISDIR(s.st_mode))
        return 0;
    return 1;
}

char *get_xdg_data_dir(void) {
    char *home = getenv("HOME");
    char *data_dir_tail = "/.local/share";
    char *data_dir = malloc(strlen(home) + strlen(data_dir_tail) + 1);
    memcpy(data_dir, home, strlen(home));
    memcpy(data_dir + strlen(home), data_dir_tail, strlen(data_dir_tail) + 1);

    if (is_dir(data_dir)) {
        char *manhandle_tail = "/manhandle";
        char *manhandle_data_dir = malloc(strlen(data_dir) + strlen(manhandle_tail) + 1);
        memcpy(manhandle_data_dir, data_dir, strlen(data_dir));
        memcpy(manhandle_data_dir + strlen(data_dir), manhandle_tail, strlen(manhandle_tail) + 1);
        free(data_dir);

        mkdir(manhandle_data_dir, 0755);
        return manhandle_data_dir;
    }

    free(data_dir);
    return (char *)NULL;
}

// ISO 8601
char *get_timestamp(int for_filename) {
    time_t current_time = time(NULL);
    struct tm *current_tm = localtime(&current_time);
    char *current_tstamp = malloc((size_t)24);
    char *fmt = "%Y-%m-%dT%H:%M:%S";
    if (for_filename)
        fmt = "%Y%m%dT%H%M%S";
    strftime(current_tstamp, 23, fmt, current_tm);
    return current_tstamp;
}
