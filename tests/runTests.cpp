#include <epicsUnitTest.h>

int aliveTest(void);

void runAliveTests(void)
{
	testHarness();
	
	runTest(aliveTest);
}
