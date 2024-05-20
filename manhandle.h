#include <curses.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "multi_choice.h"
#include "short_answer.h"

struct options {
    int execute_immediately;
    char *file_display;
    char *file_pager;
    char *paradigm;
    union {
        struct multi_choice_options mc;
        struct short_answer_options sa;
    } pd_opts;
};

struct state_of_file {
    int complete;
    char *file;
    union {
        struct mc_decision mc;
        struct sa_decision sa;
    } decision;
};

struct state_of_decisions {
    struct state_of_file *files;
};

struct state_of_gui {
    int page;
    int page_count;
    int exit_code;
    int resized;
    int shall_exit;
    pid_t display_pid;
    char *message;
    char *rip_message;
};

struct state_of_curses {
    int rows;
    int cols;
    WINDOW *main_win;
    WINDOW *msg_win;
};

extern struct options opts;
extern struct state_of_decisions state_of_decisions;
extern struct state_of_gui state_of_gui;
extern struct state_of_curses state_of_curses;

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
void messenger(char *fmt, ...);
int ask(char *fmt, ...);
void nav_prev(void);
void nav_next(void);
void ask_write_out(void);
void ask_exit(void);
void handle_key(char key);
void gui_loop(void);
void err(char *fmt, ...);
void print_help(void);
int options_parse(int argc, char *argv[]);
int execute_decision(int page);
int all_files_complete(void);
int main(int argc, char *argv[]);
void mc_options_parse(int argc, char *argv[], int *i);
void mc_print_menu(void);
char *mc_progress(void);
int mc_execute_decision(int page);
void mc_handle_key(char key);
void sa_handle_key(char key);
