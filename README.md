# manhandle - human in the loop, literally

If you have a big list of files that need to be organized or acted upon via the
inspection or decision of a human, you'll probably end up wanting to do things
manually.

`manhandle` puts a decision-making curses interface inside a loop over such
files. It supports decision paradigms such as:

* multiple-choice
* short-answer
* fuzzy-finder

## Examples

### Multiple choice

```
manhandle multi-choice \
    --choice '1:mv "$MH_FILE" /home/jacob/good' \
    --choice '2:mv "$MH_FILE" /home/jacob/ok' \
    --choice '3:mv "$MH_FILE" /home/jacob/bad' \
    --choice '9:rm "$MH_FILE"' \
    *.pdf
```

This opens a multiple-choice interface looping over all `*.pdf`, where keys 1,
2, 3, and 9 are bound to run the corresponding command in a child shell (with
the current pdf exported as `$MH_FILE`).

### Fuzzy finder

```
manhandle fuzzy-finder \
    --fuzzy-finder 'find sync -maxdepth 5 -type d | fzf' \
    --action 'mv "$MH_FILE" "$MH_STR"' \
    *
```

This allows you to open and select a string from the given fzf interface. The
selection is exported as `$MH_STR` prior to executing the action.

Paradigm "fuzzy-finder" can be used with more than just fuzzy finders. It
essentially supports any process of decision making resulting in a string.

## Install

    ./bootstrap && ./configure && make
    sudo make install

