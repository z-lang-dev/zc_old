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

# single number
assert 0 '0'
assert 1 1
assert 41 41
# add and sub
assert 55 '20+20+20-5'
assert 7 ' 10 - 3 + 0 '
# assert 8 ' 10 - 3 ++ 0 '
# mult and div
assert 4 '10-3*2'
assert 3 '(10-4)/2'
# unary
assert 25 '-5*-5'
assert 1 '-3+4'
# compare
assert 1 '1>0'
assert 0 '5>7'
assert 1 '4<8'
assert 0 '5>=10'
# assign
assert 13 'a=13;a'
assert 10 'a1=8;b2=2;a1+b2'
# if
assert 3 'if (0) 2; 3;'
assert 3 'if (1-1) 2; 3;'
assert 2 'if (1) return 2; 3;'
assert 2 'if (1+1) return 2; 3;'
# for
assert 10 'i=0; for(i<10) {i=i+1}; i'

echo "OK"
