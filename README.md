# WebBench

Webbench是一个在Unix/Linux/OS X命令行下使用的非常简单的网站压测工具。它使用fork()模拟多个客户端同时访问我们设定的URL，测试网站在压力下工作的性能，最多可以模拟3万个并发连接去测试网站的负载能力。

##使用：

	sudo make && make install
  
##命令行选项：




| 短参        | 长参数           | 作用   |
| ------------- |:-------------:| -----:|
|-f     |--force                |不需要等待服务器响应               | 
|-r     |--reload               |发送重新加载请求                   |
|-t     |--time <sec>           |运行多长时间，单位：秒"            |
|-p     |--proxy <server:port>  |使用代理服务器来发送请求	    |
|-c     |--clients <n>          |创建多少个客户端，默认1个"         |
|-u     |--User-Agent <n>       |更改UserAgent，默认 WebBench + 程序版本"         |
|-d     |--data <n>             |读取文件，在request body中添加数据，计划支持json和csv（主要用于POST请求）"         |
|-F     |--Field <n>            |读取文件指定字段，必须和-d一块使用，"         |
|-9     |--http09               |使用 HTTP/0.9                      |
|-1     |--http10               |使用 HTTP/1.0 协议                 |
|-2     |--http11               |使用 HTTP/1.1 协议                 |
|       |--get                  |使用 GET请求方法                   |
|       |--head                 |使用 HEAD请求方                    |
|       |--options              |使用 OPTIONS请求方法               |
|       |--trace                |使用 TRACE请求方法                 |
|-?/-h  |--help                 |打印帮助信息                       |
|-V     |--version              |显示版本号                         |



##下一步开发计划，进一步增强POST请求，增加从CSV/JSON文件中读取表单的内容 开发中！
或者直接在命令后面跟进字段名，如： webbench http://localhost:3000 --post --data={"name":"webbench","phone":"13999990000"} 
或 webbench http://localhost:3000 --post -d pathToJsonOrCSVFile
本功能使用parson来解析json
现在使用方法 webbench  --post -d pathToJsonFile -F fieldOfJsonFile http://localhost:3000/
由于源程序的创建requestBody和发送request是分开的。现在写的程序实在创建request中循环，并不能达成读取一行json就直接发送
实际的逻辑应该称改成：创建一次request就发送一次request
所以新逻辑：

主函数
循环（do while）

｛
创建一个requestBody全局变量

调用读取函数（如果遇到文件尾，则继续循环读取）

->调用创建函数来修改requestBody

->调用发送函数（取消以前的循环发送）

将所有发送函数的返回值验证？

将requestBody清空

｝
将所有发送函数的返回值验证，如果出现异常的返回值就直接返回这个数值给main函数

####Todo增加https支持