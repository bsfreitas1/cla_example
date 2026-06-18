
//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "math.h"

#pragma DATA_SECTION(fVal,"CpuToCla1MsgRAM");
float fVal=0;
#pragma DATA_SECTION(fResult,"Cla1ToCpuMsgRAM");
float fResult=0;


void main(void)
{
        Device_init();

        Interrupt_initModule();

        Interrupt_initVectorTable();

        Board_init();
        EINT;
        ERTM;

        for(;;)
        {
            CLA_forceTasks(myCLA0_BASE,CLA_TASKFLAG_1);
            DEVICE_DELAY_US(100000);
        }

}

__interrupt void cla1Isr1 ()
{
    fVal +=0.1f;
    if(fVal>2*M_PI) fVal=0;
    Interrupt_clearACKGroup(INT_myCLA01_INTERRUPT_ACK_GROUP);
}

