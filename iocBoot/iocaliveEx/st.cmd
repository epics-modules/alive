#!../../bin/linux-x86_64/aliveEx

## You may have to change dsh to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/aliveEx.dbd"
aliveEx_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadRecords "db/alive.db", "P=aliveEx:,RHOST=xxx.xxx.xxx.xxx"


## Run this to trace the stages of iocInit
#traceIocInit
cd ${TOP}/iocBoot/${IOC}
iocInit

