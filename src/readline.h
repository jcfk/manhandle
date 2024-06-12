struct mhreadline {
    int reading;
    int input_available;
    char ch;
    char *line;
};

extern struct mhreadline mhreadline;
