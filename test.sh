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