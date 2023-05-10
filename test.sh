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

    echo "---- testing compiler ----" 
    echo "$input" | ./zc.exe -
    ./app.exe
    got="$?"
    assert "$want" "$input" "$got"


    echo "---- testing interpreter ----"
    echo "$input" | ./zi.exe -
    got="$?"

    assert "$want" "$input" "$got"
}

# 指针类型
test 1 "let a=1;let b *int=&a;*b"

# Hello
test 1 'use puts; let s="Happy Birthday!"; puts(s); 1;'

# 作用域
test 2 'let x=2; {let x =3}; x;'
test 2 'let x=2; {let x=3}; {let y=4; x}'
test 3 '{let x=2; {x=3}; x}'

# 简单字符串
test 97 '"a"[0]'
test 98 '"abc"[1]'
test 99 '"abc"[2]'
test 98 'let s="abc"; s[1]' 


# 简单字符
test 97 "'a'"
test 122 "'z'"
test 65 "'A'"
test 90 "'Z'"
test 48 "'0'"
test 57 "'9'"

# 基本数组
test 4 "let a [1]int = [4]; a[0]"
test 6 "let a [2]int = [3,6]; a[1]"
test 9 "let a [3]int = [3,6,9]; a[2]"

# 指针加减法
test 13 "let a=13;let b=14;let p=&a; *p"
test 3 "let a=2;let b=3; let p=&a; p=p+1; *p"
test 22 "let a=22;let b=23;let p=&b; p=p-1; *p"

# 指针
test 13 "let a=13;let b=&a;*b"

# 指定类型
test 5 "let a int=5;a"
test 4 "let b char=3;b+1"


# 带参数的函数
test 10 "fn db(a int){a*2};db(5)"
test 12 "fn add(a int, b int){a+b};add(5, 7)"
test 21 "fn sum(a int,b int,c int,d int,e int,f int){a+b+c+d+e+f};sum(1,2,3,4,5,6)"

# 无参数的函数
test 12 "fn b{let a=8;a+4};b()"
test 5 "fn a{2+3};a()"

# 负数
test 3 "-1+4"

# for
test 5 "let a=0; for a<5 {a=a+1}; a"
test 30 "let a=0; for a<15 {a=a+1}; a*2"


# if-else
test 2 "if 1<0 {1} else {2}"
test 2 "if 1 {2} else {3}"
test 3 "if 0 {2} else {3}"
test 4 "if 0 {2} else if 0 {3} else {4}"

# 空语句
test 4 ";;4"
test 5 ";;{;;5};"

# 代码块
test 3 "{3}"
test 4 "{1;2;3;4}"
test 13 "let a={1;2;3+10}"

# 多个字符的变量
test 2 "let frog=3; let fox=5; fox-frog"
test 41 "let abc=41;abc"

# 单个字符的变量
test 41 "let a=41;a"
test 5 "let a=3;let b=a+2;b"

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