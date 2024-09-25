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

char *strip_last_newline(char *str) {
    char *new_str = strdup(str);
    if (new_str[strlen(new_str)-1] == '\n')
        new_str[strlen(new_str)-1] = '\0';
    return new_str;
}

void read_whole_file(char *fpath, char **strp) {
    FILE *fp = fopen(fpath, "r");
    if (!fp)
        syscall_err("fopen");

    if (fseek(fp, 0L, SEEK_END) < 0) syscall_err("fseek");

    unsigned long length = ftell(fp);
    if (length < 0)
        syscall_err("ftell");

    if (fseek(fp, 0L, SEEK_SET) < 0) syscall_err("fseek");

    *strp = safe_malloc(sizeof(char)*length + 1);
    size_t transfered = fread(*strp, sizeof(char), length, fp);
    if (transfered != length)
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
    int alloced = 1;
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
