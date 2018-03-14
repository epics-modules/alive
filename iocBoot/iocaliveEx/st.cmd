#!../../bin/linux-x86_64/aliveEx

## You may have to change dsh to something else
## everywhere it appears in this file

< envPaths

epicsEnvSet("ENGINEER","Tester")
epicsEnvSet("LOCATION","Office")
epicsEnvSet("GROUP","BCDA")

epicsEnvSet("ALIVE_SERVER","xxx.xxx.xxx.xxx")

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/aliveEx.dbd"
aliveEx_registerRecordDeviceDriver pdbbase


## Load record examples (look in alive.db for more settable options)
#    simplest example
dbLoadRecords("db/alive.db", "P=aliveEx:,RHOST=$(ALIVE_SERVER)" )
#    if you want to directly name an IOC
# dbLoadRecords("db/alive.db", "P=aliveEx:,RHOST=$(ALIVE_SERVER),IOCNM=iocLab")
#    if you want to use a custom environment variable, such as IOCVAR
# dbLoadRecords("db/alive.db", "P=aliveEx:,RHOST=$(ALIVE_SERVER),NMVAR=IOCVAR")

## If you want to set up a calcout to automatically write to the MSG field
#    based on PV fieldsvalues
# dbLoadRecords("db/aliveMSGCalc.db", "P=aliveEx:" )


## Run this to trace the stages of iocInit
#traceIocInit
cd ${TOP}/iocBoot/${IOC}
iocInit

