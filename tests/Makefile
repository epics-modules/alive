TOP=..

include $(TOP)/configure/CONFIG

SRC_DIRS += $(TOP)/aliveApp/src


TARGETS += $(COMMON_DIR)/aliveTest.dbd
DBDDEPENDS_FILES += aliveTest.dbd$(DEP)
aliveTest_DBD += base.dbd
aliveTest_DBD += aliveRecord.dbd
TESTPROD_HOST += aliveTest
aliveTest_SRCS += aliveTest.cpp
aliveTest_SRCS += aliveTest_registerRecordDeviceDriver.cpp
testHarness_SRCS += aliveTest.cpp
testHarness_SRCS += aliveTest_registerRecordDeviceDriver.cpp
TESTFILES += $(COMMON_DIR)/aliveTest.db ../aliveTest.db
TESTS += aliveTest

testHarness_SRCS += runTests.cpp

PROD_vxWorks = aliveTestHarness
aliveTestHarness_SRCS += $(testHarness_SRCS)
TESTSPEC_vxWorks = aliveTestHarness.munch; runTests

PROD_LIBS += alive
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)
PROD_SYS_LIBS_WIN32 += ws2_32

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

include $(TOP)/configure/RULES
