# SQLBUS
SQL-BUS(SqlBus), forwarding SQL via Memory-Cache as a bus.
===
---

## SQLBUS 原理图
```c

+--------------------------------------------------------------------------------+
|   +------------------------------------------------------------------------+   |
|   |   ＳＴＲＵＣＴＵＲＡＬ ＳＣＨＥＭＡＴＩＣ ＤＩＡＧＲＡＭ ＯＦ ＳＱＬＢＵＳ   |   |
|   +------------------------------------------------------------------------+   |
|                                                                                |
|                  +------------------(1)-----------------+                      |
|                  |                                      |                      |
|                  |                                      v                      |
|        +---------+----------+                 +--------------------+           |
|        | TransactionProcess |<------(6)-------+    MemoryCahce     |           |
|        +--------------------+                 +------+-------------+           |
|           \         /                                |     ^                   |
|           [a]     [b]                               (2)    |                   |
|             \     /                                  |    (5)                  |
|              \   /                                   v     |                   |
|        +-------------------+                  +------------+-------+           |
|        |     DataBase      |<-------(3)-------|       SQLBUS       |           |
|        +--------+----------+                  +--------------------+           |
|                 |                                       ^                      |
|                 |                                       |                      |
|                 +-------------------(4)-----------------+                      |
|                                                                                |
+--------------------------------------------------------------------------------+
```

| 关键字                | 简称  | 名称       |
| ---                | --- | ---      |
| TransactionProcess | TP  | 业务处理进程   |
| MemoryCahce        | MC  | 内存数据库    |
| DataBase           | DB  | 数据库      |
| SQLBUS             | SB  | sqlbus服务 |

  在原来的架构中，TP通过直连DB进行数据库作，如结构图中[a]和[b]步骤；TP直接将SQL语句传输到DB中，然后等待DB返回操作结果。
然而有一些操作的结果不需要即可使用的，在这种结构下，TP会消耗大量时间在等待DB完成处理，这是不必要的等待。

  在SQLBUS的架构中，TP会将一些耗时并且不需要实时的数据库操作存储在MC中，然后继续处理后续业务流程；SB服务则会一直尝试读
取MC中的请求数据，当接收到一个"合法"的请求时，就会将请求传输到DB中进行处理。

  合法的请求，是指符合既定SQLBUS格式的请求数据，请查看[SQLBUS请求格式](#sqlbus请求格式)

### 请求数据与响应数据的流转
1、TP需要执行SQL语句时，只需构建一个请求，发送至MC，如果需要等待响应，TP则会阻塞，否则TP的请求结束；
2、SQLBUS从MC中读取该请求数据，截取请求中的SQL语句，发送至DB；
3、DB执行完成后，将结果返回给SQLBUS，如果TP的请求是同步的，即需要等待响应，SQLBUS则构建响应数据，发送至MC；
4、当MC接收到SQLBUS的响应数据后，TP会从当中取走。至此，TP的请求结束。

TP发送的请求数据在MC中会以队列的形式存在，默认的队列名称是：SqlBusDefaultQueue。
当TP以同步的形式等待SQLBUS响应数据，则使用另一个队列，名字由TP来自动生成。不同请求最好生成不同的响应队列名称。

## SQLBUS数据格式
无论是请求数据还是响应数据，都采用JSON格式进来传输。

|Key|Name|Values|Notes|
|---|---|---|---|
|UUID|唯一标识||取自/proc/sys/kernel/random/uuid|
|APP|进程名称|各业务进程名，<br>SQLBUS-SERVER|TP端由进程名表示<br>SQLBUS端为SQLBUS-SERVER|
|PID|进程ID标识|||
|TYPE|请求响应标识|REQUEST<br>RESPONSE|TP发起的为REQUEST<br>SB发起的为RESPONSE|
|SYNC|同步标志|true<br>false|TP等待SB响应的为true，否则为false|
|RCHANNEL|响应频道||由TP设定，SB向该频道响应请求数据|
|TIMESTAMP|时间戳||用于标识请求或者响应的时间|
|STATEMENT|请求SQL语句|||
|MESSAGE|消息||只有当SQL语句执行出现错误时使用|
|ERRORID|错误代码|||
|FIELDS|选择列数|||
|FILED|列名称列表|||
|ROWS|查询结果集记录数|||
|RESULT|查询结果集|||

### SQLBUS请求格式
```c
{
  "UUID": "f8a6fb8c-31a7-4183-a4e3-fc1d931d860b",
  "APP": "Client",
  "PID":  29322,
  "TYPE": "REQUEST",
  "SYNC":  true,
  "RCHANNEL": "Client",
  "TIMESTAMP": 1521620927,
  "STATEMENT": "SELECT TO_CHAR(SYSDATE, 'YYYY/MM/DD HH24:MM:SS') AS Cur_Date_Time FROM DUAL"
}
```

### SQLBUS响应格式

#### 查询语句响应格式

```c
{
  "UUID": "f8a6fb8c-31a7-4183-a4e3-fc1d931d860b",
  "APP": "SQLBUS-SERVER",
  "PID": 31003,
  "TYPE": "RESPONSE",
  "DBTYPE": "ORACLe",
  "TIMESTAMP": 1522315216,
  "MESSAGE": "",
  "ERRORID": 0,
  "FIELDS": 1,
  "FIELD": [
    "CUR_DATE_TIME"
  ],
  "ROWS": 1,
  "RESULT": [
    [
      "2018/03/29 17:03:54"
    ]
  ]
}
```

#### 非查询语句响应格式
```c
{
  "UUID": "f8a6fb8c-31a7-4183-a4e3-fc1d931d860b",
  "APP": "SQLBUS-SERVER",
  "PID": 31003,
  "TYPE": "RESPONSE",
  "DBTYPE": "ORACLe",
  "TIMESTAMP": 1522315216,
  "MESSAGE": "",
}
```

#### 执行语句出错响应格式
```c
{
  "UUID": "f8a6fb8c-31a7-4183-a4e3-fc1d931d860b",
  "APP": "SQLBUS-SERVER",
  "PID": 31003,
  "TYPE": "RESPONSE",
  "DBTYPE": "ORACLe",
  "TIMESTAMP": 1522315216,
  "STATEMENT": "SELECT TO_CHAR(SYSDATE, 'YYYY/MM/DD HH24:MM:SS') AS Cur_Date_Time FROM DUAL_Wrong_Table_Name",
  "MESSAGE": "ORA-00942: table or view does not exist",
  "ERRORID": 0,
}
```

## SQLBUS配置
SQLBUS可以在服务启动的时候通过命令"--config"或者"--file"指定配置文件，如果不通过命令参数指定配置文件的路径，则会使用默认的配置文件/etc/sqlbus.ini。
如果不指令配置文件路径，或者默认配置文件不存在时SQLBUS无法启动。

SQLBUS的配置文件使用INI格式的配置方法，例子：
```ini
#
# Key 不区分大小写
# 注释符号是#或者;
# 目前只支持单行注释
#

[Default]
; 数据库类型
database=oracle

[oracle]
; oracle数据库驱动配置
descriptor = Oracle Driver of Afc
driver     = /home/fylos/sqlbus/library/libora.so
server     = 
port       =
username   = fzlc50db@afc
password   = fzlc50db
```

Default为节Section，database为键，oracle为值，键值使用"="分隔，每个键值对都属于某个Section。
SQLBUS的键不区分大小写，但值区分大小写。
SQLBUS的INI使用英文的"#"或者";"作为注释符，当前仅支持单行注释。

SQLBUS的配置内容可以查看：[Example](document/sqlbus.ini)

Default节必须要配置上，并且必须指定键database的值，它指定了要连接数据库的类型；也可以通过键memcache指定缓存数据库，默认是redis。它们是指定后续Section中数据库或者内存缓存的配置的项。

键是固定的，值是可变的。上例子中，指定database的类型为oracle，


## SQLBUS的缺点
- 增加了需要即时响应的数据的等待时间
- 响应数据体积大大增加

## SQLBUS的优点
- 理想中的SQLBUS应该会有不少的好处。

## TODO LIST
- BUG，尝试连接时存在内存泄露问题
- 接收SIGHUP信号时重新读取配置文件
- 接收退出信号时正常退出sqlbus，现在只能通过kill -9退出
- 日志文件被改动时sqlbus无法写入日志记录
- 启动sqlbus时指定连接数据库
- 由业务进程指定要连接的数据库
- 封装业务层接口
- 编写说明及驱动接口编写规则

