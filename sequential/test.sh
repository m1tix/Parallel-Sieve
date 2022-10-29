#!/usr/bin/env bash

R=5

for ((i = 3; i <= 9; i++)); do
    echo -n "$((10 ** i)) "
        for ((j = 0; j < R; j++)); do
            RESULT=$(./main $((10 ** i)))
            echo -n "$RESULT"
            if [ $((j + 1)) -ge $R ]; then
                echo ""
            else
                echo -n " "
            fi
        done
done
