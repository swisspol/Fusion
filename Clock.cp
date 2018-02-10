#include			"Structures.h"
#include			"Editor Header.h"

//MACROS:

#define				Rate2Ms(r) (1000 / r)

//VARIABLES:

TMTask				clockTask;
unsigned long		clockTime;

//ROUTINES:

static pascal void Clock_Task(TMTaskPtr theTask)
{
	++clockTime;
	PrimeTime((QElemPtr) &clockTask, Rate2Ms(kTimeUnit));
}

OSErr Clock_Install()
{
	clockTask.tmAddr		= NewTimerProc(Clock_Task);
	clockTask.tmWakeUp		= 0;
	clockTask.tmReserved	= 0;
	
	clockTime = 0;
	
	InsXTime((QElemPtr) &clockTask);
	PrimeTime((QElemPtr) &clockTask, Rate2Ms(kTimeUnit));
	
	return noErr;
}

void Clock_Dispose()
{
	RmvTime((QElemPtr) &clockTask);
}