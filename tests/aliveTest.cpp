#include <stdio.h>

#include <dbAddr.h>
#include <dbAccess.h>
#include <asDbLib.h>
#include <dbEvent.h>
#include <dbStaticLib.h>
#include <dbmf.h>
#include <dbStaticLib.h>
#include <dbUnitTest.h>

#include <registry.h>
#include <registryFunction.h>
#include <iocInit.h>
#include <iocsh.h>


#include <epicsExit.h>
#include <epicsThread.h>
#include <epicsStdio.h>
#include <epicsString.h>
#include <epicsUnitTest.h>
#include <epicsTypes.h>

#include <osiSock.h>
#include <osiFileName.h>

#include <envDefs.h>
#include <testMain.h>

#include <string>
#include <sstream>

extern "C" {
    int aliveTest_registerRecordDeviceDriver(struct dbBase *pdbbase);
}

static dbEventCtx evtctx;

extern "C" {
static void aliveTestCleanup(void* junk)
{
    dbFreeBase(pdbbase);
    registryFree();
    pdbbase=0;

    db_close_events(evtctx);
}
}

void startIOC()
{
	std::stringstream join;
	std::string path1;
	
	join << "." << OSI_PATH_LIST_SEPARATOR << ".." << OSI_PATH_LIST_SEPARATOR << "../O.Common" << OSI_PATH_LIST_SEPARATOR << "O.Common";
	join >> path1;
	
	dbReadDatabase(&pdbbase, "aliveTest.dbd", path1.c_str(), NULL);
			
	aliveTest_registerRecordDeviceDriver(pdbbase);
	
	std::string path2;
	
	join << "." << OSI_PATH_LIST_SEPARATOR << "..";
	join >> path2;
	
	dbReadDatabase(&pdbbase, "aliveTest.db", path2.c_str(), NULL);
	
	epicsAtExit(&aliveTestCleanup, NULL);
	
    iocInit();
    evtctx = db_init_events();
}

void testRecv(SOCKET& receiver, char buffer[])
{
	const char* test_name = "Receive Packet";
	
	DBADDR addr;
	
	long status;
	
	status = dbNameToAddr("test:alive.RPORT", &addr);
	
	if (status)
	{
		testFail(test_name);
		testDiag("Error getting port number: %ld", status);
		return;
	}
	
	long val;
	
	long options = 0;
	long nelm = 1;
	
	status = dbGetField(&addr, addr.dbr_field_type, &val, &options, &nelm, NULL);
	
	if (status)
	{
		testFail(test_name);
		testDiag("Error getting port number: %ld", status);
		return;
	}
	
	struct sockaddr_in port_bind;
	
	port_bind.sin_family = AF_INET;
	port_bind.sin_addr.s_addr = htonl(INADDR_ANY);
	port_bind.sin_port = htons(val);
	
	status = bind(receiver, (struct sockaddr*) &port_bind, sizeof(port_bind));
	
	if (status != 0)
	{
		testFail(test_name);
		testDiag("Couldn't bind to port. Error: %ld", status);
		return;
	}
	
	struct sockaddr recv_addr;
	socklen_t recv_len = sizeof(recv_addr);
	
	status = recvfrom(receiver, buffer, 200, 0, &recv_addr, &recv_len);
	
	if (! testOk(status >= 0, test_name))
	{
		testDiag("Error during packet reception: %ld", status);
	}
}

void testMagic(char buffer[])
{
	epicsUInt64 received = (epicsUInt64) ((unsigned char)(buffer[0]) << 24 | 
	                                      (unsigned char)(buffer[1]) << 16 |
	                                      (unsigned char)(buffer[2]) << 8 | 
	                                      (unsigned char)(buffer[3]));
	
	testdbGetFieldEqual("test:alive.HMAG", DBR_ULONG, received);
}

void testPeriod(char buffer[])
{
	epicsUInt32 received = (epicsUInt32) ((unsigned char)(buffer[18]) << 8 | 
	                                      (unsigned char)(buffer[19]));
	
	testdbGetFieldEqual("test:alive.HPRD", DBR_USHORT, received);
}

void testReturn(char buffer[])
{
	epicsUInt32 received = (epicsUInt32) ((unsigned char)(buffer[22]) << 8 | 
	                                      (unsigned char)(buffer[23]));

	testdbGetFieldEqual("test:alive.IPORT", DBR_USHORT, received);
}

void testMessage(char buffer[])
{
	epicsUInt64 received = (epicsUInt64) ((unsigned char)(buffer[24]) << 24 | 
	                                      (unsigned char)(buffer[25]) << 16 |
	                                      (unsigned char)(buffer[26]) << 8 | 
	                                      (unsigned char)(buffer[27]));
	
	testdbGetFieldEqual("test:alive.MSG", DBR_ULONG, received);
}

void testName(char buffer[])
{
	const char* received = &buffer[28];
	
	testdbGetFieldEqual("test:alive.IOCNM", DBF_STRING, received);
}



MAIN(aliveTest)
{
	testPlan(6);

	startIOC();

	SOCKET receiver = epicsSocketCreate(AF_INET, SOCK_DGRAM, 0);
	char buffer[200];
	
	testRecv(receiver, buffer);
	testMagic(buffer);
	testPeriod(buffer);
	testReturn(buffer);
	testMessage(buffer);
	testName(buffer);
	
	return testDone();
}
