#include "manhandle.h"

void syscall_err(char *syscall, int reports_errno) {
    if (reports_errno)
        err("%s() failed with errno %d: %s\n", syscall, errno, strerror(errno));
    err("%s() failed without setting errno\n", syscall);
}

void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) syscall_err("malloc", 1);
    return ptr;
}

void *safe_realloc(void *ptr, size_t size) {
    void *nptr = realloc(ptr, size);
    if (!nptr) syscall_err("realloc", 1);
    return nptr;
}

char *safe_strdup(char *str) {
    char *str2 = strdup(str);
    if (!str2) syscall_err("strdup", 1);
    return str2;
}

char *safe_strndup(char *str, size_t n) {
    char *str2 = strndup(str, n);
    if (!str2) syscall_err("strndup", 1);
    return str2;
}

pid_t safe_fork(void) {
    pid_t pid = fork();
    if (pid < 0) syscall_err("fork", 1);
    return pid;
}

int safe_system(char *command) {
    int status = system(command);
    if (status < 0 || status == 127) syscall_err("system", 1);
    return status;
}

int safe_mkstemp(char *template) {
    int tempfd = mkstemp(template);
    if (tempfd < 0) syscall_err("mkstemp", 1);
    return tempfd;
}

FILE *safe_fopen(char *pathname, char *mode) {
    FILE *fp = fopen(pathname, mode);
    if (!fp) syscall_err("fopen", 1);
    return fp;
}

FILE *safe_popen(char *command, char *type) {
    FILE *fp = popen(command, type);
    if (!fp) syscall_err("popen", 1);
    return fp;
}

char *safe_fgets(char *s, int size, FILE *stream) {
    s = fgets(s, size, stream);
    if (!s)
        if (ferror(stream)) syscall_err("fgets", 0);
    return s;
}

void strip_last_newline(char *str) {
    if (str[strlen(str)-1] == '\n')
        str[strlen(str)-1] = '\0';
}

void read_whole_file(char *fpath, char **strp) {
    FILE *fp = safe_fopen(fpath, "r");
    SAFE_NEG(fseek, fp, 0L, SEEK_END);

    long length = ftell(fp);
    if (length < 0) syscall_err("ftell", 1);

    SAFE_NEG(fseek, fp, 0L, SEEK_SET);

    *strp = safe_malloc(length + 1);
    size_t transfered = fread(*strp, 1, length, fp);
    if (transfered != (size_t)length)
        err("unable to read file %s", fpath);
    (*strp)[length] = '\0';

    SAFE_VAL(EOF, fclose, fp);
}

char *make_tmp_name(void) {
    char *tmpdir = getenv("TMPDIR");
    if (!tmpdir)
        tmpdir = "/tmp";
    /* todo how much format checking to do here? */
    char *tempfile = safe_malloc(strlen(tmpdir)+strlen("/manhandle.XXXXXX")+1);
    strcpy(tempfile, tmpdir);
    strcat(tempfile, "/manhandle.XXXXXX");
    return tempfile;
}

int get_cmd_stdout(char *cmd, char **strp) {
    FILE *fp = safe_popen(cmd, "r");

    /* what block size? */
    int bs = GET_CMD_STDOUT_BS;
    size_t alloced = 1;
    char *str = safe_malloc(alloced);
    str[0] = '\0';
    char block[bs];
    while (1) {
        if (!safe_fgets(block, bs, fp))
            break;
        if (strlen(str)+strlen(block)+1 > alloced) {
            alloced += bs;
            str = safe_realloc(str, alloced);
        }
        strcat(str, block);
    }

    *strp = str;
    int status = pclose(fp);
    if (status < 0) syscall_err("pclose", 1);
    return status;
}

int is_dir(char *path) {
    struct stat s;
    int e = stat(path, &s);
    if (e < 0) {
        if (errno == EACCES || errno == ENOENT || errno == ENOTDIR) return 0;
        syscall_err("stat", 1);
    }

    if (S_ISDIR(s.st_mode)) return 1;
    return 0;
}

char *get_xdg_data_dir(void) {
    char *xdg_data_dir = getenv("XDG_DATA_HOME");
    if (!xdg_data_dir || !is_dir(xdg_data_dir)) return (char *)NULL;

    char *manhandle_data_dir = NULL;
    SAFE_NEG_NE(asprintf, &manhandle_data_dir, "%s/manhandle", xdg_data_dir);

    if (!is_dir(manhandle_data_dir))
        SAFE_NEG(mkdir, manhandle_data_dir, 0755);

    return manhandle_data_dir;
}

// ISO 8601
char *get_timestamp(int for_filename) {
    time_t current_time = time(NULL);
    if (current_time == (time_t)-1) syscall_err("time", 1);

    struct tm *current_tm = localtime(&current_time);
    if (!current_tm) syscall_err("localtime", 1);

    char *current_tstamp = safe_malloc((size_t)24);
    char *fmt = "%Y-%m-%dT%H:%M:%S";
    if (for_filename)
        fmt = "%Y%m%dT%H%M%S";
    SAFE_VAL((size_t)0, strftime, current_tstamp, 23, fmt, current_tm);
    return current_tstamp;
}
