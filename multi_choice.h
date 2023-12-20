#define MULTI_CHOICE "multi-choice"

struct mc_decision {
    int n;
};

struct mc_choice {
    char *cmd;
};

struct multi_choice_options {
    struct mc_choice choices[10];
};


