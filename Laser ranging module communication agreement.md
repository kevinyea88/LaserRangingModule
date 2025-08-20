激光测距模块通讯协议

Laser ranging module communication agreement

波特率 9600bps, 8 位数据位，1 位起始位，一位停止位,无奇偶校验.

Baud rate of 9600bps, 8-bit data bits, 1 start bit, one stop bit, unremarkable parity.

功能描述

命令代码

返回代码

备注

FA 06 84 “DAT1 DAT2……DAT16”

CS

FA 04 81 81

DATn 为 ASCII 格式

操作成功

FA 84 81 02 FF

写入地址错误返回

读取机器号

FA 06 04 FC

设置地址

FA 04 01 ADDR CS

FA 04 06 符号（正或者负，负为

距离修改

0x2d，正为 0x2b)，0xXX(修正值，

一个字节)，CS

连续测量时设

置数据返回时

FA 04 05 MeaInterver CS

间间隔

设置距离起止

FA 04 08 Position CS

FA 04 8B 77

FA 84 8B 01 F6

FA 04 85 7D

FA 84 85 01 FC

FA 84 85 01 FA

FA 04 88 7A

FA 84 88 01 F9

点

设定频率

注释：起始点可前、后端选择

FA 04 0A Freq CS

Freq : 05 10 20

FA 04 8A 78

FA 84 8A 01 F7

FA 04 0C Resolution CS

FA 04 8C 76

设定分辨率

Resolution :

1(1mm),2(0.1mm)

FA 04 0D Start CS

Start : 0(关闭),1(开启)

FA 84 8C 01 F5

FA 04 8D 75

FA 84 8D 01 F4

FA 06 06 FA

无返回返回代码，模块测量后将结果

存入缓存

设定上电即测

单次测量

（广播命令，返

回结果存入模

块缓存）

操作成功

操作失败

操作成功

写入时间间隔错误

操作失败

操作成功

操作失败

操作成功

操作失败

操作成功

操作失败

操作成功

操作失败

读取缓存

ADDR 06 07 CS

同 单次测量 返回代码

单次测量（1mm）

ADDR 06 02 CS

ADDR 06 82”3X 3X 3X 2E 3X 3X 3X”

CS

ADDR 06 82”’E’ ’R’ ’R’ ’

-’ ’-’ ’3X’ ’3X’ ”CS

正确返回

错误返回

单次测量

(0.1mm)

ADDR 06 82”3X 3X 3X 2E 3X 3X 3X

3X”CS

正确返回

ADDR 06 02 CS

ADDR 06 82”’E’ ’R’ ’R’ ’

-’ ’-’ ‘-‘’3X’ ’3X’ ”

错误返回

连续测量（1mm）

ADDR 06 03 CS

连续测量

（0.1mm）

ADDR 06 03 CS

控制激光打开

ADDR 06 05 LASER CS

或关闭

(LASER : 00 关闭，01 开启）

CS

ADDR 06 83” 3X 3X 3X 2E 3X 3X 3X”

CS

ADDR 06 83” ’E’ ’R’ ’R’ ’

-’ ’-’ ’3X’ ’3X’”CS

ADDR 06 83” 3X 3X 3X 2E 3X 3X 3X

3X”CS

ADDR 06 83” ’E’ ’R’ ’R’ ’

-’ ’-’ ‘-‘’3X’ ’3X’”CS

正确返回

错误返回

正确返回

错误返回

ADDR 06 85 01 CS

正确返回

ADDR 06 85 00 CS

错误返回

关机

ADDR 04 02 CS

ADDR 04 82 CS

注：以上命令及返回数据均为 16 进制格式

· ADDR 为机器地址

·Postion 为 1 时由顶端算起，为 0 时由尾端算起，默认设置为尾端（程序中有

测距仪长度，由距离修正到顶端后，再加此长度即可设置到尾端）

·CS 为校验字节，其为前面所有字节求和，返回取反加 1

·在单次测量和连续测量返回数据中，引号中为数据部分，其格式为 ASCII 格式

如：123.456 米 显示为 31 32 33 2E 34 35 36

·ADDR 默认值为 80(128)

下面的命令是常用的命令，可直接使用，跟表格是一样的。

单次测量：80 06 02 78
连续测量：80 06 03 77
关机：80 04 02 7A

设置地址 ： FA 04 01 80 81
距离修改 ： FA 04 06 2D 01 CE
FA 04 06 2B 01 D0

-1
+1

设置量程 ：

时间间隔(1S) ： FA 04 05 01 FC
时间间隔(0S) ：FA 04 05 00 FD
设置起始点 ： FA 04 08 01 F9 顶端
FA 04 08 00 FA 尾端
FA 04 09 05 F4
FA 04 09 0A EF
FA 04 09 1E DB
FA 04 09 32 C7
FA 04 09 50 A9
FA 04 0A 00 F8 最低频率，3Hz 左右
5
FA 04 0A 05 F3
10
FA 04 0A 0A EE
20
FA 04 0A 14 E4
1mm
设定分辨率 ： FA 04 0C 01 F5
0.1mm
FA 04 0C 02 F4

5m
10m
30m
50m
80m

设置频率 ：

设定上电就测 ： FA 04 0D 00 F5 关闭
FA 04 0D 01 F4 开启

单次测量（广播）FA 06 06 FA
： 80 06 07 73
读取缓存

控制激光 ：

80 06 05 01 74 开启
80 06 05 00 75 关闭

数据返回错误码：（错误码是 ASCII 码展示，请以字符形式查看）

电量低

计算错误

超出量程

信号弱或者测量时间过长

环境光太强

超出显示范围

ERR-10

ERR-14

ERR-15

ERR-16

ERR-18

ERR-26

接线图：

关于排线接口，从左到右，VCC VCC GND GND VCC VCC TX RX

