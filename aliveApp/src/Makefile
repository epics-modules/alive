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

# Compile and add the code to the support library
alive_SRCS += aliveRecord.c

alive_SYS_LIBS_WIN32 += ws2_32
alive_SYS_LIBS_WIN32 += advapi32

alive_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Build the IOC application

PROD_IOC = aliveEx
# aliveEx.dbd will be created and installed
DBD += aliveEx.dbd

# aliveEx.dbd will be made up from these files:
aliveEx_DBD += base.dbd
aliveEx_DBD += aliveSupport.dbd

# aliveEx_registerRecordDeviceDriver.cpp derives from alive.dbd
aliveEx_SRCS += aliveEx_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
aliveEx_SRCS_DEFAULT += aliveExMain.cpp
aliveEx_SRCS_vxWorks += -nil-

# Link in the code from the support library
aliveEx_LIBS += alive

# Finally link to the EPICS Base libraries
aliveEx_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# Parallel build sometimes fails.
# Make dependences on aliveRecord.h explicit.
aliveRecord$(OBJ):  $(COMMON_DIR)/aliveRecord.h

