## A trading order system with traffic flow control

#### 一种具有流量控制功能的交易订单系统

![](C:\Users\FL\OneDrive - HKUST Connect\GitHub\order-system\screenshots\order.gif)

### PROBLEM

This program simulates an order system with traffic flow control.

1. Consider a program trading company with 3 program traders who create 
   orders in a randomized, high-frequency, uncoordinated manner.

2. These orders should be sent to the stock exchange as soon as possible in 
   an first-in-first-out order.

3. However, if the company sent too many orders in too short a time, the 
   exchange would suspend the company's connection for a while, which the 
   company definitely would not like to see.

4. In particular, within any 1 second, no more than 10 orders should be 
   sent. The requirement is continuous in time. For example, it would be a 
   violation to send 11 orders in [09:05:27.642, 09:05:28.642).

5. This program designs an order system to meet the requirements yet avoid 
   violations.

6. In addition, this program simulates to create an exact 100 orders.

7. All the above specifications, such as 3 traders and 100 orders and 10
   orders in any 1 second, can be changed in the factory mode. For example,
     it altogether proper to have 10 traders to generate 10,000 orders
   subject to sending at most 1,000 orders in any interval of 1 minute.

### 问题

本程序设计模拟了一种具有流量控制功能的交易订单系统。

1. 设有一家程序交易公司，有3台自动化程序交易机器，各自以随机的方式高频下单。

2. 订单创建之后，公司应尽快按顺序向交易所提交订单。 

3. 不过，如果一段时间内，提交太过密集，交易所就会暂停该公司的下单连接。这显然是该公司
   不愿意见到的。

4. 具体而言，在任意1秒钟之内，不得下单超过10笔。注意，这种监控在时间上是连续的。比方
   说，如果在9点05分27秒642毫秒（含）至9点05分28秒642毫秒（不含）之间下了11单，即
   属违规。

5. 本程序设计了一个订单系统，既可以满足要求，同时避免违规。

6. 此外，本程序控制所有交易机器正好创建100笔订单。

7. 以上所有参数，例如3名交易员创建正好100笔订单，在任意1秒钟之内不得超过10单，都可
   以在工厂模式中重新设定。例如，完全可以设置为10台程序交易机器，共计产生正好10,000
   笔订单，限制条件为，不得在任意1分钟内下单超过1,000笔。

### DESIGN

We implement the system with OpenMP. Assuming 3 traders, we need 5 threads: 

1. Thread # 0 to print messages from all other threads;

2. Thread # 1 to # 3 for the 3 traders;

3. Thread # 4 to process the orders -- to let go or block in real time.

We need 3 queues. Each should be accessed by only 1 thread at a time:

1. a [to_print] queue of strings to print messages in progress;

2. a [to_send] queue of orders to schedule orders yet to send;

3. a [sent] queue of orders to record sent orders.

When an order is created, it is immediately enqueued to [to_send].

When it is sent, it is moved from [to_send] to [sent] (dequeue and enqueue).

### 设计

本程序使用OpenMP并行编程实现。若有3台程序化交易机器，则需要创建5条线程：

1. 第0号线程，用于打印输出其它线程产生的信息；

2. 第1 ~ 3号线程，对应第1 ~ 3号交易机器；

3. 第4号线程，用于处理订单，包括实时决定放行或禁行。

同时，本程序需要3条队列。每条队列的入队或出队在一个时刻只供一条线程操作：

1. [to_print] 队列，用于打印输出处理中的信息；

2. [to_send] 队列，用于计划未发送订单；

3. [sent] 队列，用于记录已发送订单。

当创建1笔订单是，该订单立刻进入 [to_send] 队列。

系统在发送该订单后，将该订单从 [to_send] 队列移至 [sent] 队列（先出队再入队）。
