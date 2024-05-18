# manhandle - human in the loop, literally

If you have a big list of files that need to be organized or acted upon via the
inspection/decision of a human (ie "soft judgements"), then by definition it
will be hard to write a program to do so, and you'll probably end up wanting to
do things manually.

`manhandle` puts a human decision-making (curses) interface inside a loop over
such files. It currently supports decision paradigms such as:

* multiple-choice

## Example

```
manhandle multi-choice \
    --choice '1:mv "$MH_FILE" /home/jacob/good' \
    --choice '2:mv "$MH_FILE" /home/jacob/ok' \
    --choice '3:mv "$MH_FILE" /home/jacob/bad' \
    --choice '9:rm "$MH_FILE"' \
    *.pdf
```

This opens a multiple-choice interface looping over all `*.pdf`, where keys 1,
2, 3, and 9 are bound to run the corresponding command in a subshell (with the
current pdf exported as `$MH_FILE`).

## Usage

Compile: `gcc *.c -o manhandle -lncurses`.

