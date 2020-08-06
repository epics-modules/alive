#!../../bin/linux-x86_64/aliveEx

## You may have to change dsh to something else
## everywhere it appears in this file

< envPaths

epicsEnvSet("ENGINEER","Tester")
epicsEnvSet("LOCATION","Office")
epicsEnvSet("GROUP","BCDA")

epicsEnvSet("ALIVE_SERVER","xxx.aps.anl.gov")


cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/aliveEx.dbd"
aliveEx_registerRecordDeviceDriver pdbbase


## Load record examples (look in alive.db for more settable options)

#    if you don't specify IOCNM, the name defaults to the value of the IOC
#       environment variable, which is explicitly done here
dbLoadRecords("db/alive.db", "P=aliveEx:,IOCNM=$(IOC),RHOST=$(ALIVE_SERVER)" )

## If you want to set up a calcout to automatically write to the MSG field
#    based on PV values
# dbLoadRecords("db/aliveMSGCalc.db", "P=aliveEx:" )


## Run this to trace the stages of iocInit
#traceIocInit
cd ${TOP}/iocBoot/${IOC}
iocInit

