# zstack-agriculture
Autohor:shanghai university
Author info:http://www.scie-ce.shu.edu.cn/Default.aspx?tabid=17377
Email: jinyanliang@staff.shu.edu.cn / blaine.yao.shu@gmail.com
***
这是一个农业物联网的项目，底层的传感器节点采用了TI的cc2530作为MCU+RF模块，主要在上面running的[ZStack](http://www.ti.com.cn/tool/cn/z-stack)协议栈，同时把采集的数据upload到server端。
整个项目的框架如下所示。
![Struct](/1.png)
如上图所示，所有的cc2530节点将采集到的数据发送到Getway,Getway再将数据通过ipv4将数据发送到Cloud Server中nginx,nginx通过负载均衡将数据转发到SRC-Server(send/receive Controller Server),SRC-Server主要是将采集的数据save至redis数据做缓存，同时负责讲redis-mysql的数据一致性，Webserver通过读取redis与mysql，将采集的数据以web形式展现。

### 文件说明
* ZStack-2.5.1a 包含了传感器节点和getway的source code.
* agriculture_v2.1包含了SRC-Server的source code.

