# SQLBUS
SQL-BUS, forwarding SQL via Memory-Cache as a bus.
===

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

### 协议字段说明
|字段|含义|备注|
|---|---|---|
|uuid|唯一标识|反馈数据中的uuid取自请求中的uuid|
|app|进程名称|客户端或者服务器的进程名称|
|pid|进程标识|发送请求或者应答请求的进程标识|
|rchannel|响应数据通道|响应数据在redis中的队列名称，由请求端告知服务端|
|statement|SQL语句|可以是一个单一的SQL语句，也可以是两个SQL语句组合|
|sync|阻塞方式|0为异步 1为同步，用来确定是否等待服务器响应数据|
|timestamp|时间戳|发送请求或者响应请求时的时间戳|
|type|请求或响应类型|请求为request 响应为response|
|errorid|错误编号|0 成功, 1 失败, 100 找不到数据|
|message|错误消息||
|fields|查询列中的字段数量||
|field|查询列名称列表||
|rows|查询结果行记录数||
|result|查询结果行列表||

### SQLBUS请求格式
#### 普通请求格式
```json
{
  "uuid": "0f9af891-d9e8-4285-bff3-5c5025f61c6e",
  "app": "client",
  "pid": 6367,
  "sync": true,
  "type": "request",
  "rchannel": "0f9af891-d9e8-4285-bff3-5c5025f61c6e",
  "timestamp": 1527817431,
  "statement": "select * from basi_station_info"
}
```
#### 业务相关请求格式（***包含一定业务逻辑相关的***）
```json
{
  "uuid": "5e2d5090-efea-4618-9202-e8fc63cfe621",
  "app": "acctranp",
  "pid": 6394,
  "sync": false,
  "type": "request",
  "timestamp": 1527831642,
  "statement": {
      "update": "update fylos set fylos1 = 4 where fylos3='5'",
      "insert": "insert into fylos values(4,4,'5','4',sysdate)"
  }
}
```
该类型业务相关的请求，所隐含的业务是：
***当要更新数据库数据，但查找不到对应记录，应该向数据库中插入一条记录***
所以服务端对这种类型的请求的处理方式是：
***先执行update语句，只有当update返回结果是Data not found时才执行insert语句***

### SQLBUS响应格式
#### 查询语句响应格式
```json
{
    "uuid": "64d4e873-4224-4967-b920-7b7522b67a15",
    "app": "SQLBUS-SERVER",
    "dbtype": "Oracle",
    "pid": 15119,
    "type": "response",
    "timestamp": 1527832090,
    "errorid": 0,
    "message": "",
    "fields": 9,
    "field": [
        "LINE_ID",
        "STATION_ID",
        "STATION_CN_NAME",
        "STATION_EN_NAME",
        "LOCATION_TYPE",
        "LOCATION_ID",
        "LOCATION_NUMBER",
        "STATION_IP",
        "DEVICE_ID"
    ],
    "rows": 2,
    "result": [ 
        [ "02", "0206", "Frist", "CenterServer", "1", "", "0", "1", "02000001" ],
        [ "02", "0201", "Second", "StationOne", "2", "", "", "12", "02010101" ]
    ]
}
```

#### 执行语句出错响应格式
```json
{
    "app": "SQLBUS-SERVER",
    "dbtype": "Oracle",
    "errorid": -1, 
    "field": [], 
    "fields": 0,
    "message": "[ORA-DRIVER] Not connected to database now.",
    "pid": 15119,
    "result": [], 
    "rows": 0,
    "timestamp": 1527832089,
    "type": "response",
    "uuid": "0f9af891-d9e8-4285-bff3-5c5025f61c6e"
}
```

## 配置文件
配置文件默认存放在/etc/下，文件名为sqlbus.ini；
但可以通过命令行参数-f，-c，--config，--file来指定要读取的配置文件。

配置文件采用ini格式存储，主要包含4个section，分别是default,log,消息队列和数据库驱动的配置。

### default段
|key|value|备注|
|---|---|---|
|database|数据库段配置名称||
|memcache|消息队列段配置名称|指定消息队列服务配置名|
|pidfile|进程ID存储文件|如果不指定则使用默认配置/var/run/sqlbus.pid，需要root权限|

### log段
|key|value|备注|
|---|---|---|
|trace|打印源代码位置信息|off:不启用 on:启用|
|level|日志等级|debug/warn(warning)/error/info|
|spacelineafterlog|行后再换行|每行日志间追加空行|
|filename|日志名称|默认为sqlbus.log|
|catalog|日志存储目录||

### 消息队列段
|key|value|例子|
|---|---|---|
|port|端口号|6379|
|host|主机名|127.0.0.1|
|connectimeout|连接超时时间|10|
|operatetimeout|操作超时时间|30|
|password|认证密码||
|database|实例名|0|

### 数据库驱动段
|key|value|例子|
|---|---|---|
|Descriptor|描述|Oracle driver of afc|
|driver|驱动全路径|/usr/lib/libora.so|
|server|数据库服务主机名或者ip|10.15.121.77|
|port|端口号|1521|
|username|数据库用户名|fzlc50db|
|password|数据库用户密码|fzlc50db|
|database|数据库实例名|afc|

## 驱动接口
要详细了解的，可以查看源代码driver_manager.h和driver_manager/oracle下的代码实现

该驱动是基于相应数据库客户端库开发的，运行时需要配置数据库客户端库，比如oracle需要配置好动态库libclntsh.so和libsqlplus.so

|API|作用|备注|
|---|---|---|
|DBEnvInitialize|初始化数据库连接环境||
|DBEnvFinalize|释放环境句柄||
|DBConnectInitialize|初始化数据库连接句柄||
|DBConnectFinalize|释放数据库连接句柄||
|DBConnect|连接数据库||
|DBDisconnect|断开与数据库的连接，并卸载驱动||
|DBStmtInitialize|初始化数据库操纵句柄||
|DBStmtFinalize|数据库操纵结束，释放资源||
|DBExecute|执行SQL语句||
|DBGetFieldCount|获取字段数量||
|DBGetRowCount|获取记录行数||
|DBGetFieldNameIdx|根据下标获取字段名称||
|DBGetFieldLengthIdx|获取字段最大长度||
|DBGetNextRow|移动指示位置，获取下一行记录||
|DBGetFieldValue|获取当前行的字段值||
|DBGetFieldValueIdx|数据库操纵句柄||
|DBGetErrorMessage|获取错误信息||
|DBGetConnectionStatus|获取数据库连接状态||
|DBGetExecuteResultCode|获取SQL语句执行结果||

```c
/**
 * DBEnvInitialize - 初始化数据库连接环境
 *
 * @henv: Env句柄
 * @catalog: 配置文件路径
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者加载配置文件失败
 *  RETURN_SUCCESS: 加载配置文件成功
 */
int DBEnvInitialize(HENV *henv, char *catalog);

/**
 * DBEnvFinalize - 释放环境句柄
 *
 * @henv: Env句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 释放成功
 */
int DBEnvFinalize(HENV henv);

/**
 * DBConnectInitialize - 初始化数据库连接句柄
 *
 * @henv: Env句柄
 * @hdbc: 数据库连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者OOM
 *  RETURN_SUCCESS: 成功
 */
int DBConnectInitialize(HENV henv, HDBC *hdbc);

/**
 * DBConnectFinalize - 释放数据库连接句柄
 *
 * @hdbc: 连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int DBConnectFinalize(HDBC hdbc);

/**
 * DBConnect - 连接数据库
 *
 * @hdbc: 连接句柄
 * @dsn: Data Source Name, same as section
 * @username: 数据库用户名
 * @password: 数据库用户密码
 * @hostname: 数据库通讯地址
 * @database: 数据库实例名
 * @port    : 数据库监听端口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int DBConnect(HDBC hdbc, char *dsn, char *username, char *password, char *hostname, char *database, int port);

/**
 * DBDisconnect - 断开与数据库的连接，并卸载驱动
 *
 * @hdbc: 数据库连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者断开连接失败
 *  RETURN_SUCCESS: 成功断开连接
 */
int DBDisconnect(HDBC hdbc);

/**
 * DBStmtInitialize - 初始化数据库操纵句柄
 *
 * @hdbc: 数据库连接句柄
 * @hstmt: 数据库操纵句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者申请资源失败(包括驱动申请资源)
 *  RETURN_SUCCESS: 初始化成功
 */
int DBStmtInitialize(HDBC hdbc, HSTMT *hstmt);

/**
 * DBStmtFinalize - 数据库操纵结束，释放资源
 *
 * @hstmt: 操作句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者释放失败
 *  RETURN_SUCCESS: 释放成功
 */
int DBStmtFinalize(HSTMT hstmt);

/**
 * DBExecute - 执行SQL语句
 *
 * @hstmt: 数据库操纵句柄
 * @statement: SQL语句
 *
 * return value:
 *  RETURN_FAILURE: SQL执行失败
 *  RETURN_SUCCESS: SQL执行成功
 */
int DBExecute(HSTMT hstmt, char *statement);

/**
 * DBGetFieldCount - 获取字段数量
 *
 * @hstmt: 数据库操纵句柄
 * @counter: 出参，字段数量
 *
 * return value:
 *  RETURN_FAILURE: 参数无效，非查询SQL语句
 *  RETURN_SUCCESS: 获取字段数量成功
 */
int DBGetFieldCount(HSTMT hstmt, int *counter);

/**
 * DBGetRowCount - 获取记录行数
 *
 * @hstmt: 数据库操纵句柄
 * @counter: 出参，记录行数
 *
 * return value:
 *  RETURN_FAILURE: 参数无效，非查询SQL语句
 *  RETURN_SUCCESS: 获取字段数量成功
 */
int DBGetRowCount(HSTMT hstmt, int *counter);

/**
 * DBGetFieldNameIdx - 根据下标获取字段名称
 *
 * @hstmt: 数据库操纵句柄
 * @index: 下标，范围[0, field_counter)
 * @value: 字段的名称
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 获取字段名称成功
 */
int DBGetFieldNameIdx(HSTMT hstmt, int index, char *value);

/**
 * DBGetFieldLengthIdx - 获取字段最大长度
 *
 * @hstmt: 数据库操纵句柄
 * @index: 下标，范围[0, field_counter)
 * @length: 字段最大长度
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 获取字段长度成功
 */
int DBGetFieldLengthIdx(HSTMT hstmt, int index, int *length);

/**
 * DBGetNextRow - 移动指示位置，获取下一行记录
 *
 * @hstmt: 数据库操纵句柄
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 *  SQLBUS_DB_DATA_NOT_FOUND: 找不到数据
 */
int DBGetNextRow(HSTMT hstmt);

/**
 * DBGetFieldValue - 获取当前行的字段值
 *
 * @hstmt: 数据库操纵句柄
 * @value: 字段值
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 *  SQLBUS_DB_DATA_NOT_FOUND: 找不到数据
 */
int DBGetFieldValue(HSTMT hstmt, char *value);

/**
 * DBGetFieldValueIdx - 根据指定的行和列读取字段值
 *
 * @hstmt: 数据库操纵句柄
 * @row: 指定行数
 * @field: 指定列数
 * @value: 字段值
 *
 * 当@row为-1时，表示从当前行获取字段值
 * 当@field为-1时，表示从当前列获取字段值
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 *  SQLBUS_DB_DATA_NOT_FOUND: 找不到数据
 */
int DBGetFieldValueIdx(HSTMT hstmt, int row, int field, char *value);

/**
 * DBGetErrorMessage - 获取错误信息
 *
 * @handle: 句柄，真实含义由type来决定
 * @type: 表示@handle的类型，其取值由数据库操作句柄的类型决定
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者获取信息错误，或者无错误信息
 *  RETURN_SUCCESS: 成功获取到错误信息
 */
int DBGetErrorMessage(HDMHANDLE handle, int type);

/**
 * DBGetConnectionStatus - 获取数据库连接状态
 *
 * @hdbc: 数据库连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 */
int DBGetConnectionStatus(HDBC hdbc);

/**
 * DBGetExecuteResultCode - 获取SQL语句执行结果
 *
 * @hstmt: 数据库操纵句柄
 * @rcode: 结果代码
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 */
int DBGetExecuteResultCode(HSTMT hstmt, int *rcode);
```

## TODO LIST
- BUG，尝试连接时存在内存泄露问题
- 接收SIGHUP信号时重新读取配置文件
- 接收退出信号时正常退出sqlbus，现在只能通过kill -9退出
- 日志文件被改动时sqlbus无法写入日志记录
- 启动sqlbus时指定连接数据库
- 由业务进程指定要连接的数据库
- 封装业务层接口
- 编写说明及驱动接口编写规则
- 驱动出错时，SQLBUS不能正常重新加载

