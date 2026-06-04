TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

LIBRARY_IOC += alive

# aliveRecord.h will be created from aliveRecord.dbd
DBDINC += aliveRecord
# Install aliveSupport.dbd into <top>/dbd
DBD += aliveSupport.dbd

# Install cfg declaration file for downstream IOC consumption
CFG += ALIVE_DEPS

# Compile and add the code to the support library
alive_SRCS += aliveRecord.c

alive_SYS_LIBS_WIN32 += ws2_32
alive_SYS_LIBS_WIN32 += advapi32

alive_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# Parallel build sometimes fails.
# Make dependences on aliveRecord.h explicit.
aliveRecord$(OBJ):  $(COMMON_DIR)/aliveRecord.h

