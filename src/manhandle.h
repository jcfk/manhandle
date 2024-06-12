#include <curses.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "gui.h"
#include "multi_choice.h"
#include "short_answer.h"
#include "readline.h"

#define REASONABLE_ESCDELAY 50
#define STREQ(str1, str2) strcmp(str1, str2) == 0
#define CONTROL(c) (c & 0x1f)

struct options {
    int execute_immediately;
    char *editor;
    char *file_display;
    char *file_pager;
    char *paradigm;
    union {
        struct mc_options mc;
        struct sa_options sa;
    } pd_opts;
};

struct question {
    int answered;
    char *file;
    union {
        struct mc_answer mc;
        struct sa_answer sa;
    } answer;
};

struct questions {
    struct question *qs;
};

struct gui {
    int page;
    int page_count;
    int exit_code;
    int resized;
    int shall_exit;
    pid_t display_pid;
    char *message;
    char *rip_message;
};

struct curses {
    int rows;
    int cols;
    WINDOW *main_win;
    WINDOW *msg_win;
};

extern struct options opts;
extern struct questions questions;
extern struct gui gui;
extern struct curses curses;

/* */

/* r!cat *.c | grep -E "^\w.*) {" | sed 's/) {/);/g' */

void state_initialize(char *files[], int file_count);
void state_free();
void curses_initialize(void);
void curses_resize(void);
void curses_free(void);
void print_menubar(void);
void print_file_meta(void);
void print_menu(void);
void display_file(void);
void pager_file(void);
void pager_help(void);
void pager_progress(void);
void pager(char *fmt, ...);
void editor(char **strp);
void messenger(char *fmt, ...);
void readline_handler(char *line);
int readline_getc(FILE *dummy);
int readline_input_available(void);
void readline_read_char(char ch);
void quit_readline(void);
char *msg_win_readline(char *prompt, char *default_value);
int ask(char *fmt, ...);
void nav_prev(void);
void nav_next(void);
void ask_write_out(void);
void ask_exit(void);
void rename_current_file(void);
void handle_key(char key);
void gui_loop(void);
void err(char *fmt, ...);
void print_help(void);
void print_version(void);
int options_parse(int argc, char *argv[]);
int execute_decision(int page);
int all_files_complete(void);
int main(int argc, char *argv[]);
void mc_options_parse(int argc, char *argv[], int *i);
void mc_state_initialize(int page);
void mc_print_menu(void);
char *mc_progress(void);
int mc_execute_decision(int page);
void mc_handle_key(char key);
void readline_handler(char *line);
int readline_getc(FILE *dummy);
int readline_input_available(void);
void readline_read_char(char ch);
void quit_readline(void);
char *msg_win_readline(char *prompt, char *default_value);
void sa_options_parse(int argc, char *argv[], int *i);
void sa_state_initialize(int page);
void sa_print_menu(void);
char *sa_progress(void);
int sa_execute_decision (int page);
void sa_handle_key(char key);
void syscall_err(char *syscall);
void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t size);
void safe_asprintf(char **strp, char *fmt, ...);
void safe_vasprintf(char **strp, char *fmt, va_list ap);
int safe_system(char *command);
pid_t safe_fork(void);
void safe_setenv(char *name, char *value, int overwrite);
void safe_unsetenv(char *name);
char *strip_last_newline(char *str);
void read_whole_file(char *fpath, char **strp);
char *make_tmp_name(void);

