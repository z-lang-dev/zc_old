#pragma once

// 版本号
static const char *ZC_VERSION = "0.0.1";

// 表达式求值
int interpret(char *src);

// 表达式编译
void compile(char *src);
