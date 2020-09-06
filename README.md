# pdulib

## 1、介绍

这是一个PDU格式短信内容解析库。

### 1.1 目录结构



| 名称 | 说明 |
| ---- | ---- |
| figure  | 示例图片 |
| examples | 例子目录，并有相应的一些说明 |
| include  | 头文件目录 |
| src  | 源代码目录 |


### 1.2 许可证



pdulib 遵循 LGPLv2.1 许可，详见 `LICENSE` 文件。

### 1.3 依赖

- RT-Thread 3.0+

## 2、如何打开 pdulib


使用 pdulib package 需要在 RT-Thread 的包管理器中选择它，具体路径如下：

```
RT-Thread online packages
    IoT - internet of things --->
        [*] pdulib: A PDU SMS parse Library for RT-Thread
            [*] Enable pdulib sample
            [*] Enable pdulib debug
                Version(v1.0.0)
```
`Enable pdulib sample`用于使能的示例，`Enable pdulib debug`用于使能DEBUG调试信息。

然后让 RT-Thread 的包管理器自动更新，或者使用 `pkgs --update` 命令更新包到 BSP 中。

## 3、使用 pdulib

在打开 pdulib package 后，当进行 bsp 编译时，它会被加入到 bsp 工程中进行编译。

更多编程示例见examples。

## 4、注意事项

短信解析过程中，默认过滤短信中的中文内容，如果短信中包含有效中文，请注释`get_UD_info`中的`usc2str2uint8_tstr`，解注释下面的`memcpy`，直接将短信内容的转发给用户自行解析；
中文格式发送默认发送USC2编码的英文内容，如果用户的短信内容是支持中文的USC2编码字符串，请注释`create_UD_info`的`uint8_tstr2usc2str`，解注释下面的`memcpy`，直接使用用户编码的内容。

## 5、联系方式 & 感谢

* 维护：ShineRoyal
* 主页：https://github.com/ShineRoyal/pdulib
