#!/usr/bin/env bash

R=5

for ((i = 1; i <= 10; i++)); do
    for p in 1 2 4 6 8 12; do
        echo -n "$((i * 10 ** 9)) $p "
        for ((j = 0; j < R; j++)); do
            RESULT=$(./bitpack $((i * 10 ** 9)) $p)
            echo -n "$RESULT"
            if [ $((j + 1)) -ge $R ]; then
                echo ""
            else
                echo -n " "
            fi
        done
    done
done
