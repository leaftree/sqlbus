#!/bin/bash

# startup.sh
#
# Copyright (C) 2018 by Liu YunFeng.
#
#        Create : 2018-06-08 13:01:45
# Last Modified : 2018-06-08 13:01:45
#

WORKDIR=$(cd `dirname $0`; pwd)

export NLS_LANG=american_america.ZHS16GBK
export ORACLE_CLIENT64_HOME=/usr/lib/oracle/11.2/client64
export ORACLE_HOME=$ORACLE_CLIENT64_HOME
export TNS_ADMIN=$ORACLE_CLIENT64_HOME/conf
export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib/:$ORACLE_CLIENT64_HOME/lib

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${WORKDIR}/lib

exec ${WORKDIR}/sqlbus --daemonize --config ${WORKDIR}/sqlbus.ini $@
