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

#### 类似C的语法，参考Go/Kotlin/D/Rust等语言略作调整

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
hello.h hello.c hello.z
> zc clean && ls
hello.z
> # 编译为静态库
> zc build -c hello.z
hello.h hello.o hello.z
> zc clean && ls
hello.z
> # 编译为动态链接库
> zc build -so hello.z
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
fn main {
	mut a = 10
	let p = &a
	*p = 15 // 效果和`a = 15`一致
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

1. 动静皆宜。增加动态语言子系统，类似Javascript，我称之为ZJ。
1. 面向场景编程。根据不同的实用场景，编译器提供不同的语言特性组合（即不同的语言子集）。例如，在嵌入式场景中，编译器将限制只能使用ZC子集；而在脚本或前端场景下，支持ZJ子集。
2. 动态编程。添加新的语法，支持动态解释、动态类型、自动垃圾回收，和Javascript语言的互操作，添加JS后端，接入JS生态。
3. 编译期执行。把动态解释器集成到编译流程之中，实现编译器的代码执行。这个特性也可以用来实现元编程、宏等功能。
4. 编译服务。将编译的各个阶段打散，实现分布式地可持续的编译服务。实现增量话编译、自动优化、热更新。
5. 添加更多的后端，如LLVM，WebAssembly等。

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