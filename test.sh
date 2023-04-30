#!/bin/bash

# 检测`want`和`got`是否相等，否则输出错误信息，提前退出测试流程
assert() {
    want="$1"
    input="$2"
    got="$3"

    if [ "$got" = "$want" ]; then
        echo "OK: $input => $got"
    else
        echo "【Error】! Input = $input, Wanted $want, but got $got"
        exit 1
    fi
}

# 测试一组用例，包括解释器和编译器的测试
test() {
    want="$1"
    input="$2"

    echo "---- testing interpreter ----"
    ./zi.exe "$input"
    got="$?"

    assert "$want" "$input" "$got"

    echo "---- testing compiler ----" 
    ./zc.exe "$input"
    ./app.exe
    got="$?"
    assert "$want" "$input" "$got"
}

# 多个字符的变量
test 2 "frog=3; fox=5; fox-frog"
test 41 "abc=41;abc"

# 单个字符的变量
test 41 "a=41;a"
test 5 "a=3;b=a+2;b"

# 多个表达式
test 3 "1;2;3"

# 括号
test 3 "(1+2)"
test 6 "(1+2)*2"
test 9 "(1+2)*(2+1)"
test 7 "1+(2*3)"
test 4 "(1+7)/(4-2)"

# 乘除法
test 6 "2*3"
test 2 "4/2"
test 12 "3*8/2"
test 8 "5*2 + 12/3 - 6"

# 空白字符
test 0 " 0 "
test 2 "1 + 1"
test 15 " 1 + 2+3 +4+ 5 "

# 多个加减法
test 7 "1+4+8-6"
test 10 "100-80-12+2"

# 简单加减法 
test 3 "1+2"
test 4 "8-4"

# 单个数字
test 41 "41"
test 255 "255"
test 0 "0"

echo "====== Done ======"
echo "All Good."