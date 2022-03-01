# RDT
Reliable data transport protocol, lab 1 of (SE3356)云操作系统设计与实践

姓名：陶青筱
学号：519021910876
邮箱：tao_qingxiao@sjtu.edu.cn

## 运行环境

linux版本：Ubuntu 20.04.3
gcc版本：9.3.0

## 总体设计

本次lab要求实现一个可靠数据传输协议，我选择了GBN（Go-back-N）协议来实现。其中，ack是计算的数据与序列号之和（模1000），序列号为递增形式，以128进制存储于第1、2、3位。

## 对GBN协议的理解

为了更加方便地写代码，我梳理了一下GBN协议的具体内容。

#### GBN发送方

接收方需要响应的事件有：上层传输数据，收到下层的ack，计时器超时。

- 上层传输数据时，先将数据拆分成固定大小的包，并放入包序列中，之后根据发送窗口的情况做如下处理：
```c++
while(next_seq_num < base + WindowSize && next_seq_num < pkt_seq.size()) { // 发送窗口未满
        if(base == next_seq_num) Sender_StartTimer(0.3); //若发送的是窗口中第一个包，则启动timer
        Sender_ToLowerLayer(&(pkt_seq[next_seq_num ++])); // 发送包，并更改下次发送包的序号
}
// 发送窗口满时，则do nothing
```

- 收到下层ack时，修改base为ack_num + 1，跳过已被确认的包。若此时所有已发送都已确认，则停止timer；若只是确认了某个包，则重启timer，再次等待0.3秒。

- 计时器超时时，重启计时器，并重新发送窗口中的所有分组。

#### GBN接收方
接收方需要响应的事件有：收到未损坏的包，且序号为自己所期待的。

- 接收方收到未损坏的包后，需判断序号是否正确，若正确，则向上层交付数据，并向下层发送ack，修改期待的序号；若不正确，则do nothing。

## 实现过程中遇到的问题

- 一开始实现时，没有考虑清楚发送方在窗口已满时收到上层的数据该怎么办，做了两层缓存，实现过于复杂，思路不清晰。后来改成了用一个packet的vector来做缓存，收到上层的数据先预处理成packet再进行缓存，可以省去很多不必要的复杂逻辑。

- 实现过程中，有很多hard coding导致的bug浪费了很多时间。比如一开始没有把header_size设置成全局常量，有的地方直接hard code为5，导致之后修改数据排列方式时，要改的地方非常多。

- 一开始在存储序列号时，没注意到char的取值范围为-127 - 128，导致了愚蠢的bug。修改后，改为128进制，存储于包的第1、2、3位。

## 总结

- 常用的常量最好写成全局常量，方便修改。
- 常用的功能封装为函数，增加代码的可读性，方便debug。
- gdb真好用！！！尤其是backtrace这个功能，第一次用，用来对付segmentation fault相当方便。
