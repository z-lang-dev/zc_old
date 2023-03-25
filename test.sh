#!/bin/bash

assert() {
    expected="$1"
    input="$2"

    ./zc "$input" > tmp.s
    cc -static -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 0
assert 1 1
assert 41 41
assert 55 '20+20+20-5'
assert 7 ' 10 - 3 + 0 '
# assert 8 ' 10 - 3 ++ 0 '

echo "OK"