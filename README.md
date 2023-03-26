## Z语言编译器

zc是用C语言实现的Z语言编译器。 

zc工程使用C语言开发，暂时只支持Linux平台。

zc工程初始的代码是从[ChibiCC](https://github.com/rui314/chibicc)fork过来的，并根据Z语言的特性进行修改。

将来，zc会参考LCC、TCC等其他开源C编译器的实现，添加更多的特性。


## 感谢

- [ChibiCC](https://github.com/rui314/chibicc) - 本工程的灵感来源，也是本工程的代码基础。参看[引用License](#引用License)。
- [LCC](https://github.com/drh/lcc) - 一个开源的C编译器，并配套了对一个的书籍《》。我学习了这本书，并参考了它的部分实现。
- [TCC](https://www.bellard.org/tcc) - 大神Bellard开发的C编译器。本来我打算用它作为实现基础，但可惜没找到相对完整的文档。我打算学习完chibicc和LCC之后，再自己阅读TCC的源码，并融入它的部分特性。


### 引用License

[ChibiCC](https://github.com/rui314/chibicc)：[MIT License](LICENSE.chibicc)