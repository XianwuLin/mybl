# README  

## 前言

这个项目的功能是封禁某一个IP的请求。

实现的方式是通过netfilter识别IP头的src_ip，并与配置文件中的src_ip比较，如果一致，则封禁。

配置文件的下发使用了netlink。

目的是为了熟悉用户态和内核态的通信和netfilter的应用，一个练手项目。

## 代码结构

代码分成两部分，一部分是内核空间代码，文件为：kernel/mybl.c；

第二部分是用户空间代码，文件为userspace/mybl_cli.c；

两者需要相互搭配。

## 编译

开发测试环境： centos 7， 内核版本：3.10.0-1160.el7.x86_64
```shell
# 准备环境
yum groupinstall "Development Tools"
yum install kernel-devel

# 编译并加载内核模块
cd kernel
make 
insmod mybl.ko

# 编译cli程序
cd ../userspace
make 

# 查看帮助
./mybl-ctl -h
# 封禁IP
./mybl-ctl on 192.168.0.23
# 取消封禁
./mybl-ctl off

# 卸载内核模块
rmmod mybl.ko
```

## 注意事项

1. 如果是使用虚拟机开发的话，不要封禁本地IP，小心自己被踢出，如果已经无法连接，请重启服务器；
2. 本项目只是实验性质代码，没有太多的参数校验，小心内核panic；

## 下一步

* [ ] 增加多IP匹配
* [ ] 计数功能
