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

# 下面的测试用例测，为了方便测试最新的功能，顺序是倒着放的，即最新的功能在最上方。

# 简单的for循环
assert 10 'i=0; for i<10 {i=i+1}; i'

# 简单的if条件语句，不包括else分支
assert 3 'if 0 {2}; 3;'
assert 3 'if 1-1 {2}; 3;'
assert 2 'if 1 {return 2};3;'
assert 2 'if 1+1 {return 2};3;'

# 变量赋值
assert 13 'a=13;a'
assert 10 'a1=8;b2=2;a1+b2'

# 比较运算
assert 1 '1>0'
assert 0 '5>7'
assert 1 '4<8'
assert 0 '5>=10'

# 负号
assert 25 '-5*-5'
assert 1 '-3+4'

# 乘除法
assert 4 '10-3*2'
assert 3 '(10-4)/2'

# 加减法
assert 55 '20+20+20-5'
assert 7 ' 10 - 3 + 0 '

# 单数
assert 0 '0'
assert 1 1
assert 41 41

echo "OK"
