#!/bin/bash

assert() {
    want="$1"
    input="$2"
    got="$3"

    if [ "$got" = "$want" ]; then
        echo "OK: $input => $got"
    else
        echo "Error! Input = $input, Wanted $want, but got $got"
        exit 1
    fi
}

test() {
    want="$1"
    input="$2"

    echo "... testing interpreter ..."
    ./zi.exe "$input"
    got="$?"

    assert "$want" "$input" "$got"

    echo "... testing compiler ..."
    ./zc.exe "$input"
    ./app.exe
    got="$?"
    assert "$want" "$input" "$got"
}

# 单个数字
test 41 "41"
test 255 "255"
test 0 "0"

echo "====== Done ======"
echo "All Good."