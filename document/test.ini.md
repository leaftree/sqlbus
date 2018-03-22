#
# Key 不区分大小写
# 注释符号是#或者;
# 目前只支持单行注释
#
## [ORACLE]
### Descriptor = Oracle Driver of Afc
### Driver     = /home/fylos/sqlbus/library/libora.so
### Server     = 
### Port       =
### UserName   = fzlc50db@afc
### Password   = fzlc50db

## [MySQL]
### Descriptor = driver for mysql # abc
### driver = /home/afc/a.so
### server = fylos.cn
### port = 3306
### username=abc
### password=123

## [SQLite]
### Descriptor = driver for mysql
### driver = /home/afc/a.so
### server =fylos.cn
### port= 3306
### username=abc
### password=123

[LOG]
Level=debug
FileName=app.log
Catalog=./