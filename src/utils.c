#include "manhandle.h"

char *strip_last_newline(char *str) {
    char *new_str = strdup(str);
    if (new_str[strlen(new_str)-1] == '\n')
        new_str[strlen(new_str)-1] = '\0';
    return new_str;
}

/* watch the encodings */
void read_whole_file(char *fpath, char **strp) {
    FILE *fp = fopen(fpath, "r");
    fseek(fp, 0L, SEEK_END);
    unsigned long length = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    /* remove final newline, if present */
    /* if (length > 0)
       length -= 1; */

    *strp = malloc(sizeof(char)*length + 1);
    if (!*strp)
        err("unable to malloc for reading %s", fpath);
    size_t transfered = fread(*strp, sizeof(char), length, fp);
    if (transfered != length)
        err("unable to read %s", fpath);
    (*strp)[length] = '\0';

    fclose(fp);
    return;
}


