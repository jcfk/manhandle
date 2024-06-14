struct mhreadline {
    int reading;
    int input_available;
    int resized;
    char ch;
    char *line;
};

extern struct mhreadline mhreadline;
