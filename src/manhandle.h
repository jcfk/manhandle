#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "gui.h"
#include "multi_choice.h"
#include "short_answer.h"
#include "fuzzy_finder.h"

/* See https://www.gnu.org/software/autoconf-archive/ax_with_curses.html */
#if defined HAVE_NCURSESW_CURSES_H
#include <ncursesw/curses.h>
#elif defined HAVE_NCURSESW_H
#include <ncursesw.h>
#elif defined HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#elif defined HAVE_NCURSES_H
#include <ncurses.h>
#elif defined HAVE_CURSES_H
#include <curses.h>
#else
#error "SysV or X/Open-compatible Curses header file required"
#endif

#define GET_CMD_STDOUT_BS 1000
#define REASONABLE_ESCDELAY 50
#define STREQ(str1, str2) strcmp(str1, str2) == 0
#define CONTROL(c) (c & 0x1f)

struct options {
    int execute_immediately;
    int log_level;
    char *log_dir;
    char *editor;
    char *file_display;
    char *file_pager;
    char *paradigm;
    union {
        struct mc_options mc;
        struct sa_options sa;
        struct ff_options ff;
    } pd_opts;
};

struct question {
    int answered;
    char *file;
    union {
        struct mc_answer mc;
        struct sa_answer sa;
        struct ff_answer ff;
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
extern char *log_fpath;

/* */

/* r!cat *.c | grep -E "^\w.*) {" | sed 's/) {/);/g' */

void ff_options_parse(int argc, char *argv[], int *i);
void ff_state_initialize(int page);
void ff_print_menu(void);
char *ff_progress(void);
int ff_execute_decision(int page);
void ff_fuzzy_find_current_answer();
void ff_unanswer(int page);
void ff_handle_key(char key);
void state_initialize(char *files[], int file_count);
void state_free();
void curses_initialize(void);
void curses_resize(void);
void curses_free(void);
void print_menubar(void);
void print_file_meta(void);
void print_menu(void);
void print_main_win(void);
void print_msg_win(void);
void display_file(void);
void stat_pager(void);
void help_pager(void);
void progress_pager(void);
void log_pager(void);
void string_pager(char *fmt, ...);
void file_pager(char *fpath);
void editor(char **strp);
void messenger(char *fmt, ...);
int ask(char *fmt, ...);
void nav_prev(void);
void nav_next(void);
void ask_write_out(void);
void ask_exit(void);
void rename_current_file(void);
void unanswer(void);
void handle_key(char key);
void gui_loop(void);
void logger_initialize(void);
void logger_free(void);
void log_debug(char *fmt, ...);
void log_info(char *fmt, ...);
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
void mc_unanswer(int page);
void mc_handle_key(char key);
int mhrl_startup_hook(void);
int mhrl_getc_function(FILE *dummy);
int mhrl_input_available_hook(void);
int mhrl_handle_esc(int count, int key);
void mhrl_line_handler(char *line);
void print_readline(void);
char *msg_win_readline(char *prompt, char *default_line);
void sa_options_parse(int argc, char *argv[], int *i);
void sa_state_initialize(int page);
void sa_print_menu(void);
char *sa_progress(void);
int sa_execute_decision (int page);
void sa_unanswer(int page);
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
void strip_last_newline(char *str);
void read_whole_file(char *fpath, char **strp);
char *make_tmp_name(void);
int get_cmd_stdout(char *cmd, char **strp);
int is_dir(char *path);
char *get_xdg_data_dir(void);
char *get_timestamp(int for_filename);
