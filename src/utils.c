#include "manhandle.h"

void err(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    exit(1);
}

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
    SYSCALL(vasprintf(strp, fmt, ap));
}

int safe_system(char *command) {
    int status = system(command);
    if (status < 0)
        syscall_err("system");
    return status;
}

pid_t safe_fork(void) {
    pid_t pid = fork();
    if (pid < 0)
        syscall_err("fork");
    return pid;
}

void safe_setenv(char *name, char *value, int overwrite) {
    SYSCALL(setenv(name, value, overwrite));
}

void safe_unsetenv(char *name) {
    SYSCALL(unsetenv(name));
}

char *strip_last_newline(char *str) {
    char *new_str = strdup(str);
    if (new_str[strlen(new_str)-1] == '\n')
        new_str[strlen(new_str)-1] = '\0';
    return new_str;
}

/* watch the encodings */
void read_whole_file(char *fpath, char **strp) {
    FILE *fp = fopen(fpath, "r");
    if (!fp)
        syscall_err("fopen");

    SYSCALL(fseek(fp, 0L, SEEK_END));

    unsigned long length = ftell(fp);
    if (length < 0)
        syscall_err("ftell");

    SYSCALL(fseek(fp, 0L, SEEK_SET));

    *strp = safe_malloc(sizeof(char)*length + 1);
    size_t transfered = fread(*strp, sizeof(char), length, fp);
    if (transfered != length)
        err("unable to read file %s", fpath);
    (*strp)[length] = '\0';

    SYSCALL(fclose(fp));
}
