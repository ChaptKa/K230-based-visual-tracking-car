本视觉代码硬件使用为嘉立创的庐山派开发板\n
参考API手册：https://wiki.lckfb.com/zh-hans/lushan-pi-k230/api/\n
不同厂商的开发板调用的API可能不同\n
\n
协议格式:(发送ASCII码 格式为：#误差,模式!)\n
示例:\n
#10,0!->误差10，直行\n
#-50,1!->误差-50，检测到左岔路\n
#5,3!->误差5，检测到十字路口\n
\n
模式定义:\n
0:直行(Straight)\n
1:左岔路(Left)\n
2:右岔路(Right)\n
3:十字路口(Cross)\n
