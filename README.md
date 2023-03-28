## Z语言编译器

zc是用C语言实现的Z语言编译器。 

zc工程使用C语言开发，暂时只支持Linux平台。

zc工程初始的代码是从[ChibiCC](https://github.com/rui314/chibicc)fork过来的，并根据Z语言的特性进行修改。

将来，zc会参考LCC、TCC等其他开源C编译器的实现，添加更多的特性。

## 语言特性

Z语言规划为两个版本：

- ZC：更好的C语言。用C语言实现。
- Z：更好的D语言。用ZC语言实现。

ZC是Z语言的一个子集，类似于C语言和C++的关系。
因此我仍然总称为Z语言。

语言特性规划如下：

### ZC

#### 基本语法

ZC的语法类似C，但会参考Go/Kotlin/D/Rust等语言略作调整。

下面是一段ZC语法的示例，可以初步感受一下Z语言的风格。

```c
// 模块化采用D的方式，引用模块的关键字是use。
use io.println

// 常量定义用const关键字。
// 浮点数的字面值需要加上f后缀。
const PI = 3.1415926f

// 函数定义采用类似Rust的fn关键字。
// 参数为空时不需要'()'。
fn main { 
	// 函数调用与C一致。
	println("Hello, world!") // 语句结尾不需要';'。

	// println和printf差不多，但会自动添加换行符。
	println("Here is pi: %f", PI)

	// 标量是不可变的，用let声明。
	// 标量的类型标识和Go一致，不需要冒号。
	let a int = 10 
	a = 12 // 错误！a是不可变的量。

	// 变量用mut声明。
	mut b = 5 // 支持基本的类型推导。
	b = b * 2 // 正确！b是可变量。

	println("a+b is %d:", add(a, b))
}

// 函数定义和Go比较像
fn add(a int, b int) int {
	// 代码块的最后一个语句即是返回值
	a + b
}
```

#### 和C语言实现ABI级别的互操作。

Z语言可以直接调用C语言的代码，只要引用对应的模块即可。
模块到头文件和动静态库文件的关联由编译器搞定（主要订制的时候，可以在工程配置文件里指定，具体格式待设计）。

```c
// 调用C的库函数需要引用c模块下的各个子模块
use c.stdio.printf

fn main {
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

```c
fn main {
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

Z语言的复合类型有： 

- array，即数组，支持静态和动态两种。
- slice，即切片，是动态数组的引用。
- map，即字典，是键值对的集合。
- type，用来自己定义类型。默认情况下，和C语言的typdef struct一样。

下面是一些示例：

```c
fn main {
	// array
	let ar [5]int = int[1, 2, 3, 4, 5] // 静态数组，类型标识和Go一致，字面量不同
	let ar1 []int = [1, 2, 3, 4, 5] // 动态数组

	let sl = ar[1:3] // 和Python一样，用:分隔开始和结尾。slice的范围是左闭右开的，即不包含3，[1, 3)

	// slice
	let s2 = ar[1:] // 如果不指定，则默认到结尾
	let s3 = ar[:2] // 如果不指定，则默认从0开始
	let s4 = ar[] // 整个数组的切片
	let s5 = ar[:-1] // 可以用负数表示从后往前数

	// array 操作
	let ar2 = [6, 7, 8]
	let ar3 = ar1 + ar2 // 数组相加相当于concatenate
	let ar4 = ar2 ~ 9 // ~相当于append
	
	mut ar4 = [1, 2]
	ar4 ~= 3 // 可变append，注意这里可能导致copy

	// slice可以变回array
	let ar5 = sl.array() // 可以直接变化


	// map，形式和D差不多，类型标识是{key-type:value-type}
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

fn main {
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

fn main {
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

fn main {
	mut a = 10
	let p = &a
	*p = 15 // 效果和`a = 15`一致

	let p2 = &Point{x: 1, y: 2}
	println(p2.x) // 指针访问字段时直接用`.`，不需要`->`
	println(*p2.x) // 这样也可以，但是不推荐
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

fn main {
	let birds []Bird = [&Duck{}, &Chicken{}, &Eagle{}]
	birds[0].flap() // flap~~
	birds[1].flap() // cluck~~
	birds[2].flap() // phew~~
}
```

### Z

等ZC编译器实现了完整的功能之后，尝试将它的C语言代码翻译为ZC，编译通过之后，ZC就实现自举了。

之后，用ZC语言来扩充ZC编译器，使之实现下述的Z语言高阶特性：

1. **动静皆宜**（Static and Dynamic Synergy）。增加动态语言子系统，类似Javascript或Python，我称之为ZJ。为ZJ增加Javascript后端，可以直接介入JS生态。
1. **面向场景编程**（Scenario Oriented Programming, SOP）。根据不同的实用场景，编译器提供不同的语言特性组合（即不同的语言子集）。例如，在嵌入式场景中，编译器将限制只能使用ZC子集；而在脚本或前端场景下，支持ZJ子集。
3. **编译期执行**。（Compile Time Code Execution）。把动态解释器集成到编译流程之中，实现编译器的代码执行。增加编译期和运行期的信息交流特性。有了这两个特性，可以实现模板元编程、宏、泛型等语言特性。
4. **多级内存管理**。（Leveled Memory Management）。仿照V等语言，实现多级内存管理，从自动到手动都可以支持。
4. **编译服务**。（Compiler as a Service）。将编译的各个阶段打散，实现分布式地可持续的编译服务。集成解释器、JIT、AOT、CodeGen等各个子系统，并提供API。实现增量编译、自动优化、热更新。
5. **多后端**，如LLVM，WebAssembly等。

下面详细介绍这几个特性。

#### ZJ动态后端

ZJ是Z语言的另一个子集，主要支持动态类型和动态解释。ZJ和ZJ在Z语言编译器内是共生的，他们共用一个前端，源码被解析为AST之后，根据不同的场景，进入不同的编译通道。

- 静态编译通道：ZC的传统编译通道，即AOT编译，AST->IR->CodeGen->ASM/Obj。
- 动态编译通道：ZJ->IR(ByteCode)->动态解释器->JIT->虚拟机。

其中，静态编译通道负责打通Z语言和C语言的边界，可以简称为ZC通道；
动态编译通道负责打通Z语言和JS语言的边界，可以简称为ZJ通道。

ZJ通道的输出目标，ZJ虚拟机，同时也是一个JS虚拟机。我初步计划采用QuickJS来实现。
也就是说，ZJ需要把Z语言的AST转换为JS的AST，然后再交给QuickJS来执行。

ZJ语法子集新增的语法主要是**动态变量**`var`和动态函数`var fn`。

```javascript
// ZJ允许在文件里直接写语句和表达式，不需要main函数
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

ZJ的基本类型有：

- `num`：内部实现是dec和f64的一种联合体，可以表示任意精度的数字
- `jstr`: JS字符串，内部实现是QuickJS的JSString，可以和`str`与`cstr`相互转换
- `jarr`: JS数组，内部实现是QuickJS的JSArray，可以和`arr`相互转换
- `jobj`: JS对象，内部实现是QuickJS的JSObject，可以和`obj`相互转换

为了方便使用，ZJ在用动态类型与静态函数交互时，会自动进行类型转换。例如：

```javascript

// 这是一个接收int类型的静态函数
fn pow(a int, b int) int {
	mut r = a
	for ..b {
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

编译期执行的代码块用`#{}`括起来：

```c
const PI = 3.1415926
const 2PI = #(2*PI)
const PI_SQUARE = #pow(PI, 2)

fn pow(x int, y int) int {
	mut r
	for ..y {
		r *= x
	}
	r
}
```


#### 内存管理

#### 编译服务

#### 多后端

## 开发计划

- 0.1：参考ChibiCC，实现C的基本特性。
- 0.2：学习LCC，根据需求修改或添加特性。
- 0.3：学习TCC，根据需求修改或添加特性。
- 0.4：实现和C语言的互相翻译。
- 0.5：添加类型系统，实现自定义类型与方法。
- 0.6：添加基础的模块化。
...
- 1.0：实现自举：用Z语言实现zz编译器。

1.0之后的规划等实现了zz编译器，再在zz编译器工程里讨论。

## 感谢

- [ChibiCC](https://github.com/rui314/chibicc) - 本工程的灵感来源，也是本工程的代码基础。参看[引用License](#引用License)。
- [LCC](https://github.com/drh/lcc) - 一个开源的C编译器，并配套了的书籍《A Retargetable C Compiler: Design and Implementation》。我会学习这本书，并参考它的实现。
- [TCC](https://www.bellard.org/tcc) - 大神Bellard开发的C编译器。本来我打算用它作为实现基础，但可惜没找到相对完整的文档。我打算学习完chibicc和LCC之后，再自己阅读TCC的源码，并融入它的部分特性。
- [QuickJS](https://bellard.org/quickjs/) - 一个开源的Javascript引擎。我会学习它的实现，用来实现Z语言动态子系统以及与Javascript的互操作。


### 引用License

[ChibiCC](https://github.com/rui314/chibicc)：[MIT License](LICENSE.chibicc)