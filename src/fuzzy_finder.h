#define FUZZY_FINDER "fuzzy-finder"

struct ff_answer {
    char *str;
};

struct ff_options {
    char *fuzzy_finder_cmd;
    char *action_cmd;
};
