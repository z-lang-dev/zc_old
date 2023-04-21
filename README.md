## Z语言编译器

zc是Z语言的编译器和解释器。 

zc的初版用C语言开发，将来会改为用Z语言本身实现。
zc支持暂时只支持Linux，将来会添加其他平台的支持。

本工程有一本配套开源书籍[《脚踏实地Z语言》](https://gitee.com/jiaota/jiaota-z)，详细记录Z语言设计与开发的每一个步骤。

本工程还处于非常早期的筹备和尝试阶段。但由于本工程与配套书籍都遵循**增量开发**方法，每一步都是相对完整的。

zc早期代码同时参考[ChibiCC](https://github.com/rui314/chibicc)
和[clox](https://github.com/munificent/craftinginterpreters/tree/master/c)，
分别用于实现类C的静态特性和类Lox的动态特性。

## Z语言特性

我把Z语言设计为一门通用编程语言，它的主要独特之处在于：

- *动静皆宜*。Z语言同时支持静态编译（AOT）、动态解释（Interpreter），未来还会添加及时编译（JIT）功能。Z语言既支持静态类型检查，也支持动态类型数据。
- *编译期脚本*。利用动静结合的特性，将解释器嵌入到编译流程之中，实现编译期脚本执行功能。这个特性可以用来实现泛型、宏和模板等高阶语言特性。
- *面向场景编程*。针对不同的开发场景（Scenario），编译期提供不同的语言特性组合（即支撑集，Support Set）。
- *组合与接口*。Z语言的类型体系由组合（Composition）和行为接口（Duck Typing）配合完成。Z不支持继承，而是利用编译期脚本来实现编译期多态和运行时多态。
- *层次化内存管理*。利用场景编程特性，在不同场景中支持不同的内存管理策略。包括：手动管理、自动释放、智能指针、引用计数、垃圾回收和时间线管理。

Z语言既可以用于底层的高性能开发，也可以用于应用层的动态应用开发。
这是我的初始设计目标，也就是说，Z应当同时具备Python+C或Javascript+JIT的组合能力。

Z语言的**驱动应用**是类似[RT-Thread](https://www.rt-thread.org/)的嵌入式系统（筹备中），以及系统内的应用开发框架。

## 语法展示

注：下面的代码尚未实现，只是我的初步设计的展示。

#### 基本语法

```c
// 模块化采用D的方式，引用模块的关键字是use。
use io.[println, open, W]

// 常量定义用const关键字。
// 浮点数的字面值需要加上f后缀。
const PI = 3.1415926f

// 函数定义采用类似Rust的fn关键字。
// 参数为空时不需要'()'。
mfn main { 
  // 函数调用与C一致。
  println("Hello, world!") // 语句结尾不需要';'。

  // Z语言的字符串支持嵌入。这里PI值被直接嵌入到字符串里了。
  println("Here is pi: $PI")

  // 标量是不可变的，用let声明。
  // 标量的类型标识和Go一致，不需要冒号。
  let a int = 10 
  a = 12 // 错误！a是不可变的量。

  // 变量用mut声明。
  mut b = 5 // 支持基本的类型推导。
  b = b * 2 // 正确！b是可变量。
  b = "Z语言" // 错误！不能改变变量的类型。

  // 动态类型变量，在Z语言里被称为“幻量”
  // 幻量用var声明。
  var c = 5
  c = "Z语言" // 正确！c是幻量，类型和值都可以改变

  println("a+b is $add(a, b)")
}

// 函数定义和Go比较像。Z语言的函数是“纯函数”，参数和返回值都必须是不可变幻的，且内部不能有副作用，即只能调用纯函数。所有的纯函数都可以在编译期调用。函数的关键字是`fn`。
fn add(a int, b int) int {
  // 代码块的最后一个语句即是返回值
  a + b
}

// 如果想添加副作用，就使用“变函数”（mutable function），关键字是 `mfn`，即Mutable Function。
mfn writeFile(name str, s str) bool {
  let f = open(name, W) 
  on(exit) {
    f.close()
    return false
  }
  f.write(s)
  return true
}

// 幻函数则相当于JS之类动态语言里的函数，不需要指定参数或返回类型。关键字是`vfn`，即Variant Function。
vfn alert(message) {
  message = "Alert!! $message"
  println(message)
}
```

#### C语言互操作

Z语言可以直接调用C语言的代码，只要引用对应的模块即可。
模块到头文件和动静态库文件的关联由编译器搞定（主要订制的时候，可以在工程配置文件里指定，具体格式待设计）。

```c
// 调用C的库函数需要引用c模块下的各个子模块
use c.stdio.printf

mfn main {
  // 调用C语言代码时，需要用unsafe代码块包起来
  unsafe {
    printf("Hello, world!\n")
  }
}
```

在C语言里调用Z语言的功能，有两种方式：

1. Z语言可以直接翻译成C语言源码，且翻译保留可读性。这样的话，C语言工程可以直接引入翻译的结果。
2. Z语言编译成动态库时，可以指定生成对应的C头文件(-c)。

```bash
> ls
hello.z
> # 直接翻译，得到.h和.c文件
> zc trans -c hello.z
> ls
hello.h hello.c hello.z
> zc clean && ls
hello.z
> # 编译为静态库
> zc build -c hello.z
> ls
hello.h hello.o hello.z
> zc clean && ls
hello.z
> # 编译为动态链接库
> zc build -so hello.z
> ls
hello.h libhello.so hello.z
```

注意，默认情况下，编译的结果不会包含Z语言的基本库文件，需要在C语言编译时手动指定。
Z语言的基本库默认放在：

```bash
/usr/include/z/z.h
/usr/lib/libz.a
/usr/lib/libz.so
```

#### 基础的类型系统

Z语言的基本类型有：

- i8, i16, i32, i64：基本整数类型
- u8, u16, u32, u64：对应的正整数
- f32, f64：浮点类型
- int(i32), uint(u32), byte(i8), char(u8), float(f32), double(f64)：数类型的别名
- dec：精确十进制

- bool：布尔类型，true/false
- rune: 和Go一样，是一个i32的别名，用于表示Unicode字符。
- str：字符串类型，类似C++和D语言的string，包含长度信息。
- cstr: C的字符串类型，用于与C互操作。

下面是一些基本类型的示例：

```go
mfn main {
  let a = 10 // 整数字面量的默认类型是int
  let b i64 = 655537 // 可以指定具体的整数类型
  let c u64 = 0x123128ABCD // 0x开头的数字表示十六进制格式

  let d dec = 1_0000_0000_0000 // 大整数里可以添加下划线作为分隔提示。这里使用的是方便中文用户的四位分隔

  let e f32 = 3.14 // 浮点数里必须有小数点
  let f f64 = .8e15 // 支持科学计数法

  let g = true // 布尔字面量


  let r rune = '好' // 支持Unicode字符
  let r1 rune = '\u4F60' // 也可以用Unicode编码

  let s str = "你好Z语言" // 字符串字面量
  let cs cstr = c"你好C语言" // C字符串前面有个c开头，这样会默认添加'\0'
  let cs1 = s.cstr() // 支持转换
  let s1 = cs.str() // 反过来也可以

  let len = s.len() // 字符串长度
  let len1 = cs.len() // cstr也支持
}
```

也可以一定定义多个量值：

```go
mfn main {
  // 组合定义
  let [a = 1, b = 2, c = 3]

  // 模式匹配
  let [a, b, c] = 0..3

}
```

Z语言的复合类型有： 

- 数组（array），支持静态和动态两种。
- 切片（slice），是动态数组的摘要引用。
- 映射（map），是键值对的集合。
- 类型（type），用来定义自己使用的数据结构。默认情况下type和C语言的typdef struct一样。

下面是一些示例：

```go
mfn main {
  // 数组
  let ar [int|5] = [int| 1, 2, 3, 4, 5] // 静态数组，长度固定为5的数组
  let ar1 [int] = [1, 2, 3, 4, 5, 6] // 动态数组，长度可变

  let sl = ar[1:3] // 和Python一样，用:分隔开始和结尾。slice的范围是左闭右开的，即不包含3，[1, 3)

  // 切片
  let s2 = ar[1:] // 如果不指定，则默认到结尾
  let s3 = ar[:2] // 如果不指定，则默认从0开始
  let s4 = ar[] // 整个数组的切片
  let s5 = ar[:-1] // 可以用负数表示从后往前数

  // 数组操作
  let ar2 = [6, 7, 8]
  let ar3 = ar1 + ar2 // 数组相加相当于concatenate
  let ar4 = ar2 ~ 9 // ~相当于append
  
  mut ar4 = [1, 2]
  ar4 ~= 3 // 可变append，注意这里可能导致copy

  // 切片可以变回数组（有内存分配）
  let ar5 = sl.array() // 可以直接变化


  // 映射，形式和D差不多，类型标识是`{键类型:值类型}`
  let m1 {string:int} = {
    "hello": 1,
    "world": 2,
  }
  // map的操作
  // 添加元素
  m1["now"] = 3 
  // 修改
  m1["world"] = 4
  // 获取元素
  let v = m1["world"]
  // 带标默认值的获取
  let v = m1["world"] ? 0
  // 删除
  m1.del("hello")


  // 自定义类型
  type Message {
    id int,
    mut content str,
  }
  mut m = Message{id: 1, content: "hello"}
  m.id = 2 // 错误，m是标量
  m.message = "Now" // OK, content是变量

   // type也可以用来指定类型别名
  type MyInt = int
}
```

Z的自定义类型可以指定方法，类似Go。例如：

```c
use io.println
use fmt.format

// 自定义类型
type Message {
  id int,
  mut content str,
}

// 自定义方法
fn Message.str() str {
  format("id: %d, content: %s", .id, .content)
}

mfn main {
  mut m = Message{id: 1, content: "hello"}
  println(m.str()) // id: 1, content: hello
}
```

注意，和Go不同的是，Z的方法不需要指定receiver，而是直接在方法名前面指定类型。
在方法内部，不需要`this`或者`self`之类的关键字来访问receiver，而是直接使用`.`来访问。

与Go类似，Z的自定义类型也有简单的复用机制，即组合模式：

```c
use io.println

type Thing {
  name str,
}

type Hand {
  left str,
  right str,
  color str,
}

fn Hand.grab(thing Thing) {
  // ...
}

fn Hand.touch(thing Thing) {
  // ...
}

type Nose {
  color str,
}

fn Nose.sniff() {
  println("sniff")
}

fn Nose.touch(thing Thing) {
  // ...
}

type Person {
  :Hand, // Person是一个组合类型，装配了Hand类型，相当于把Hand的定义在Person里重写一遍，包括相应的方法
  :Nose, // 装配Nose类型
  name str,
}

mfn main {
  let p = Person{name: "Zack"}
  p.grab(Thing{name: "pen"}) // Person实例可以直接调用Hand.grab方法
  p.sniff() // 也可以直接调用Nose.sniff方法
  p.Hand.touch(Thing{name: "pen"}) // 当有同名的方法时，必须通过类型名来访问
  println(p.left) // 可以直接访问Hand的字段，因为没有重名。
  println(p.Nose.color) // Hand和Nose都有color字段，必须通过类型名来访问
}
```

上述的组合模式实际上是没有运行时消耗的，而是一种代码级别的复制。
这里Person类型里并没有一个字段叫Hand，而是把Hand的定义直接复制到Person里了。
所以实际上的Person的定义相当于：

```c
type Person {
  name str,
  left str,
  right str,
  Hand_color str,
  Nose_color str,
}

fn Person.grab(thing Thing) {
  // ...
}

fn Person.Hand_touch(thing Thing) {
  // ...
}

fn Person.sniff() {
  println("sniff")
}

fn Person.Nose_touch(thing Thing) {
  // ...
}
```

然后编译器再把调用和访问的代码翻译成这些具体字段和方法的访问。

#### 指针

Z语言也支持指针类型，但是不支持指针运算。

```c
type Point {
  x int,
  y int,
}

mfn main {
  mut a = 10
  let p = &a
  *p = 15 // 效果和`a = 15`一致

  let p2 = &Point{x: 1, y: 2}
  println(p2.x) // 指针访问字段时直接用`.`，不需要`->`
  println(*p2.x) // 这样也可以，但是不推荐
}
```

#### 控制流

Z语言支持三种控制流语句：`if`、`for`和`when`。

`if`语句与Go类似：

```c
use io.println
mfn main {
  let a = 1
  if a > 0 {
    println("a > 0")
  } else if a == 0 {
    println("a == 0")
  } else {
    println("a <= 0 ")
  }
}
```
`when`相当于C的`switch`，但是借鉴了Kotln的`when`模式匹配语句：

```c
use os
use io.println

mfn main {
  when os.GetOS() {
  is os.WINDOWS:
    println("windows")
  is os.LINUX:
    println("linux")
  is os.Android:
    println("android")
  else:
    println("unkown os")
  }
}
```

和Go语言类似，`when`语句的每个分支默认情况是隔离的，相当于自带break。
但是ZC暂时不提供类似`fallthrough`的语句，而是在同一个`is`判断里可以支持多个条件。

`for`有三种形式：

```c
use io.println
mfn main {
  // range
  for i: 0..5 {
    println(i)
  }

  // 相当于while
  mut a = 0
  for a < 5 {
    println(a++)
  }

  // 无限循环+break
  for {
    if a > 100 {
      break
    }
  }
}
```

其中，第二种相当于`while`的形式其实就是散开些的三段式`for`循环：
- 初始条件写在`for`语句之前
- 结束条件在`for`的判断表达式里
- 逐步语句直接写在循环体内

因此，我考虑不设计三段式`for`循环了。

Go语言的控制流语句本身也都是表达式，其返回值为：

- `if`语句返回值就是执行分支的值，若分支为语句块则实际上是语句块的最后一行
- `when`返回对一个分支的语句块值。
- `for`返回一个slice，其中每个元素是语句块的最后一行的值

```c
use io.println

mfn main {
  let a = if 1>0 {true} else {false} // 相当于C的三元表达式

  // when语句取分支结果
  let name = "Michael"
  let nickName = when name {
  is "William": "Bill"
  is "Michael": "Mike"
  is "Elizabeth": "Lisa"
  else: "Bob"
  } 

  // for语句会构造出一个slice
  let arr = for i: 0..5 { i*i } // arr == [0, 1, 4, 9, 16]

  // 如果只想要最后一个元素，则用[-1]即可：
  println(arr[-1])
}
```

#### 接口


Go的接口是一套复杂的机制，而且需要很多运行时支持。

考虑到我的编码能力，以及C的编码难度，我也在犹豫接口是不是要放在ZC里实现。

也许等实现了Z语言的编译期执行特性之后，再来实现接口功能，会简单很多。不过那样的话就需要先自举，然后还有等很久了。毕竟编译期执行可不是个简单的功能。

但是还是现在设计文档里写下关于接口的思考吧。

Z的接口的原则是没有运行时消耗，一切的判断都在编译期完成。这样的话就不能直接复用Go的设计了。

初步设计如下： 

- 支持DuckTyping。
- 接口的关键字叫`like`，名字取“if you look like a duck, walk like a duck, and quack like a duck, then you must be a duck”。
- 接口只支持指针类型。

```c
use io.println

like Bird {
  flap()
}

type Duck {}
Duck.flap() {
  println("flap~~")
}

type Chicken {}
Chicken.flap() {
  println("cluck~~")
}

type Eagle {}
Eagle.flap() {
  println("phew~~")
}

mfn main {
  let birds []Bird = [&Duck{}, &Chicken{}, &Eagle{}]
  birds[0].flap() // flap~~
  birds[1].flap() // cluck~~
  birds[2].flap() // phew~~
}
```

#### 动态脚本

Z语言也支持脚本格式，文件名后缀为`.zs`，即Z Script。


```zs
#!/usr/bin/env zc

// Z脚本允许在文件里直接写语句和表达式，不需要main函数
var a = 1 // ZJ中，数字默认是num类型，num可以是任意长度的dec，也可以是f64
a = "hello" // 动态变量可以随时改变类型，这里a变成了字符串类型
var typ = a.type // 可以直接获取变量的类型，需要运行时反射支持
println(a.type)  // println这样的常用函数会默认导入，即有一个导入的preset

var len = a.len // 当a是字符串时，len是字符串的长度，当a是数组时，len是数组的长度；当a是数字时，len都是1

var b = " world"
println(a+b) // 调用动态函数add，不需要判断类型，具体类型会在运行时判断

// 动态函数，参数和返回值都是默认var。相当于 fn add(var a, var b) var { a + b }
// 和JS一样，函数定义可以放在后面
var fn add(a, b) {
  a + b
}

// 数组类型
a = [1, 2, 3]

// 对象类型
a = {name: "z", age: 18}
```

为了方便使用，ZJ在用动态类型与静态函数交互时，会自动进行类型转换（而静态类型之间没有任何隐式转换）。例如：

```javascript

// 这是一个接收int类型的静态函数
fn pow(a int, b int) int {
  mut r = a
  for 0..b {
    r *= a
  }
  r
}

var a = 64
var b = pow(a, 4) // a会自动转换为int类型，然后调用pow函数
```

另外，为了能够配合JS生态，标准库也需要实现对应的模块。例如Dom操作等。
JS桥接库的设计在具体实现ZJ子集时再探讨。

#### 面向场景编程

Z语言在语言层面和编译器内部提出两个概念：场景（Scenario）和特性（Feature）。

不同的场景对应不同的特性集合（FeatureSet），称为支撑集（SupportSet）。
支撑集之外的其他特性都出于关闭状态，如果调用，编译器会报错。

Z语言提供的默认场景有：

- `app`：一般用来写主应用程序。
- `lib`：一般用来写库。
- `web`：一般用来写Web前端。
- `server`：一般用来写Web后端。
- `script`: 一般用来写脚本程序。
- `repl`：一般用来写交互式开发程序，如探索式编程、科学计算等。
- `test`：一般用来写测试程序。
- `sys`：一般用来写关键系统模块。
- `debug`：一般用来写调试程序。
- `ui`：一般用来写UI程序。
- `game`：一般用来写游戏程序。

Z语言提供的特性有：

- `core`：Z语言的核心特性，包括基本类型、基本语法、基本函数等。其他的特性都与`core`正交。
- `unsafe`：不安全的特性，包括指针、内存管理、C互操作等。
- `var`：动态变量和动态函数的支持。
- `pub`：模块间访问的限制。
- `mem`：内存管理，有几种选择
  - `mem:manual`：手动内存管理。
  - `mem:session`：会话内存管理。
  - `mem:gc`：自动垃圾回收
  - `mem:autofree`：自动内存管理。

每个场景对应的支撑集不同，相应的特性开关也不同。可以用如下表格展示：

| 场景 | core | unsafe | var | pub | mem |
| --- | --- | --- | --- | --- | --- |
| app | √ | × | √ | √ | mem:autofree |
| lib | √ | × | × | √ | mem:autofree |
| script | √ | × | √ | √ | mem:gc |
| web | √ | × | √ | √ | mem:gc |
| server | √ | × | × | √ | mem:session |
| game | √ | × | × | √ | mem:session |
| sys | √ | √ | × | √ | mem:manual |
| test | √ | × | √ | × | mem:autofree |
| repl | √ | × | √ | √ | mem:autofree |
| debug | √ | × | √ | √ | mem:autofree |
| ui | √ | × | √ | √ | mem:autofree |

当然，上述只是一个初步的设计，实际实现中估计会进行更细粒度的划分，把特性划分为几十种，那么这个表格将会非常大。

未来还会支持自定义场景，并且可以订制其支撑集。

总体来说，面向场景编程的效果是在不同的开发场景中，使用的语言子集是不一样的，而编译器可以帮助开发者避免越界使用不当的特性。

最典型的例子就是`unsafe`特性，它可能导致内存安全问题，所以在除了`sys`之外的场景，都不允许使用。

再例如`var`特性，它的易用性很高，但是执行效率太低，因此`lib`、`server`、`game`、`sys`等对性能要求高的场景都不允许使用。

同样的特性还有`mem:gc`。游戏每帧的时间都是固定的，而垃圾回收则是不定的，且会一起全局冻结，因此在`game`场景中不允许使用，而可以使用Z语言特有的`mem:session`会话回收机制，或者手动管理内存。

#### 编译期执行

Z语言最有野心的设计就是编译期执行（Compile-Time Code Execution，CTCE）。

Z语言的目标是可以在编译期执行“任何Z代码”，包括动态和静态的代码（前提是当前场景允许，且不破坏安全性）。

要是先这一点，我需要考虑将一个完整的解释器塞到编译流程中。这样，对于任何想要执行的代码，只要单独调用解释器去解释它就行了。

为此我设计了“编译期执行”的概念。传统的代码放在函数的代码块中，在运行时才会执行。而编译期的代码需要用不同的语法去标识，这样才能在编译期执行。

编译期执行的代码块用`#`表示。

例如，可以在编译器调用任何函数：

```c
const PI = 3.1415926
const PI_SQUARE = #pow(PI, 2)

fn pow(x int, y int) int {
  mut r
  for 0..y {
    r *= x
  }
  r
}
```

如果需要多条语句，可以用`#{}`代码块：

```c
const MAX_BUF_SIZE = #{
  let info = getSystemInfo()
  let memSize = info.mem.size
  memSzie / 256
}
```

`#{}`中的代码都会在编译期执行。
如果想在`#{}`代码块中生成运行期才会执行的代码，则可以用`%{}`包括。

这种方法类似于用模板去生产代码。

```c
use os
#{
  if os.OS == os.WINDOWS {
    %{
      // call windows api OpenFile()
    }
  } else if os.OS == os.LINUX {
    %{
      // call linux api open()
    }
  }
}
```

这段代码在Windows操作系统下相当于直接写`OpenFile()`，而在Linux系统下相当于直接写`open()`。
这样，就通过类似模板生成的技术实现了条件编译。

不过这样写比较乱，所以我们设计了更简单的编译期条件语句：`#if`、`#match`和`#for`。

这三个语句的条件表达式里的运算都是编译期执行，相当于放在`#{}`里，而他们的分支代码块里的代码则默认留到运行期，相当于放在`%{}`里：

```c
use os
#if os.OS == os.WINDOWS {
  // 这里的代码是运行期代码
  // call windows api OpenFile()
} else if os.OS = os.LINUX {
  // 之类的代码是运行期代码
  // call linux api open()
}
```

这段代码和上一个例子的代码功能相同。

`#when `也是一样的情况：

```c
use os

fn open(name str) {

#when os.OS {
is os.WINDOWS:  
  // call windows api OpenFile()
is os.LINUX:
  // call linux api open()
is os.MACOS:
  // call macos api open()
else:
  // OSDoesNotSupport()
}
```

`#for`则可以用来生成有规律的大量代码：

```c
enum {
  #for let i in ["A", "B", "C", "D", "E"] {
    MEMBER_#i
  }
}
```

相当于手写代码：

```c
enum {
  MEMBER_A
  MEMBER_B
  MEMBER_C
  MEMBER_D
  MEMBER_E
}
```


编译器执行可以用来实现类型推导和泛型：

```c
type ns = int or str

fn add(x ns, y ns) ns {
  x + y
}
```


#### 内存管理

Z语言内存管理的原则是：

- 尽量安全
- 尽量方便
- 尽量快速

这三者一般认为是不可兼得的，因此我们只能分情况去处理。
幸好Z语言有面向场景编程的功能，程序员可以根据场景去较为自由地选择。

为了支持不同场景的需求，我们设计了几种内存管理方式：

- mem.gc：基础的垃圾回收功能，性能一般，但能确保内存不会堆积或泄露。且程序员只需要分配内存，不需要管回收。
- mem.autofree：仿照Nim和V语言的方式，依靠编译器的静态分析，在跳出作用域时将为些较为简单的内存分配情形自动添加free函数。这样可以大大提前内存被回收的几率，从而减少延迟，并减少垃圾回收的压力。mem.autofree是mem.gc的上位替代，如果失效的情况，会回退到垃圾回收。据Nim的测试，70%以上的对象都可以通过autofree清理。
- mem.session：会话级回收是Z语言自己创新的内存回收机制。下面会详细描述。它是一个介于autofree和gc之间的回收机制，但相比于autofree，它能确保回收，因此不需要和gc配合。
- mem.manual：在unsafe的场景里，Z语言允许手动管理内存，即不用`new`关键字，而是用`mem.alloc`和`mem.free`（或者其他C的实现）。在与C交互的场景里，这也是必须的。但手动内存管理的缺陷是可能会导致内存安全问题。

要选择不同的内存管理机制，只需要在不同的场景下编程即可。

如果需要自己订制，可以设计新的场景。

#### 会话

会话（Session）本质上是一个时间上的概念。在代码上展现它，就一定是零散的。一个会话往往要运行多个函数，并从一个函数传递到另一个函数，直到结束。

有的时候，系统会存在多个会话，并行运行，并可能会争夺资源。

Z语言的会话必须保持独立，即内存访问的隔离性，这样才有机会实现会话级别的内存回收。

因此，Z语言的会话更像是一个协程，但它的内存不跟别的协程共享，而是要有独立的内存。

会话提供以下接口：

- session.start()：会话启动，并向线程申请一片独立的内存池
- session.new(obj)：向会话申请一个新对象。如果内存池不够用了，会自动增长。
- session.stop()：会话结束，此时该会话所有对象都被释放，内存池回归。

由于Z语言编译器会保障一个会话的对象不能够传递给其他会话，所以会话内存总是能保证回收成功。

这样的话，只要会话的平均时间不太长，大部分情况下都能做到较快的回收。如果我们把会话的粒度做得再细一些，例如一个用户读取一次页面算一个会话，那么内存回收的效率就会比GC高。

TODO：设计一个具体的示例，如HTTP会话。


#### 编译服务

Z语言的编译器本身就是一个持续运行的服务器，提供如下服务：

- 上传一段文件（或地址），如`z build hello.z`，服务器将它进行编译，生成编译结果。
- 上传一个执行命令（如`z run hello.z`，服务器运行并返回结果）。
- 上传一句表达式（如REPL中的一次对话），服务器执行并返回结果。

这样，不论是编译单个文件、整个目录，还是动态交互的命令，Z语言都可以统一执行。

Z编译服务器实现增量编译。

首先，Z语言有一个“编译单元”的概念。如REPL的一行表达式，是一个编译单元；一个函数，也是一个编译单元；一个文件或包，也是一个更大的编译单元。

Z语言会根据需求对编译单元的编译结果进行缓存和比照，这样在修改文件时，只有修改的函数所在的编译单元会被重新编译。

为了实现既能动态执行又能静态编译的功能，以及为了实现编译期的代码执行功能，Z编译期不但缓存代码的摘要，还会缓存其AST、ByteCode甚至对象文件。这样的话，不论是需要重新解释、还是需要把动态代码转换为JIT、还是需要重新link不同的对象时，都可以节省缓存单元的重编译时间。

Z编译服务器未来还会实现分布式编译集群的功能。这样的话，就能彻底避免传统C/C++的喝咖啡时间了。

#### 多后端

Z语言支持动态解释器、二进制机器码（或汇编）、C语言代码和JS代码4个后端输出格式。

未来会增加LLVM的LLIR格式，以便于利用LLVM拓展新的生态空间。

考虑到WASM的潜力，也有可能直接生成WASM输出，而不是通过C语言或LLVM间接转换。

## 开发计划

1. 参考ChibiCC和clox，实现一个和C/lox差不多的语言子集。
1. 学习LCC、TCC和Mir，根据需求修改或添加静态特性。
1. 学习QuickJS、Wren和Lua，根据需求添加动态特性。
1. 添加基础的模块化。
1. 实现和C语言的互相翻译。
1. 实现自举：用Z语言实现zc编译器。
1. 实现和Javascript的互相翻译。
1. 添加类型系统，实现自定义类型、方法、组合。

1.0之后的规划等实现自举之后再详细规划，现在规划过早了。

## 感谢

- [ChibiCC](https://github.com/rui314/chibicc) - 本工程的灵感来源，也是本工程的代码基础。参看[引用License](#引用License)。
- [LCC](https://github.com/drh/lcc) - 一个开源的C编译器，并配套了的书籍《A Retargetable C Compiler: Design and Implementation》。我会学习这本书，并参考它的实现。
- [TCC](https://www.bellard.org/tcc) - 大神Bellard开发的C编译器。本来我打算用它作为实现基础，但可惜没找到相对完整的文档。我打算学习完chibicc和LCC之后，再自己阅读TCC的源码，并融入它的部分特性。
- [Mir](https://github.com/vnmakarov/mir) - 一个C语言解释器和JIT编译期。
- [QuickJS](https://bellard.org/quickjs/) - 一个开源的Javascript引擎。我会学习它的实现，用来实现Z语言动态子系统以及与Javascript的互操作。
- [Wren](wren.io) - clox作者Bob Nystrom开发的一个小巧精致的动态嵌入式语言。
- [Lua](lua.org) - 设计精巧的脚本语言，在游戏编程行业很常用。


### 引用License

- [ChibiCC](https://github.com/rui314/chibicc)：[MIT License](LICENSE.chibicc)
- [clox](https://github.com/munificent/craftinginterpreters/tree/master/c)：[MIT License](LICENSE.clox)