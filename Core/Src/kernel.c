/*
 * kernel.c
 *
 *  Created on: Oct 18, 2023
 *      Author: Jacob Gordon
 */

#include <stdio.h>
#include "main.h"
#include "kernel.h"
uint32_t threadCount;
uint32_t MSP_INIT_VAL;
thread thread1;
thread threadArray[5];
uint32_t currentThread;
uint32_t software_timer = 1000;
int started = 0;
void SVC_Handler_Main( unsigned int *svc_args )
{
	unsigned int svc_number;
	/*
	* Stack contains:
	* r0, r1, r2, r3, r12, r14, the return address and xPSR
	* First argument (r0) is svc_args[0]
	*/
	svc_number = ( ( char * )svc_args[ 6 ] )[ -2 ] ;
	switch( svc_number )
	{
	case 0:
			__set_PSP((int32_t)threadArray[0].sp);
			runFirstThread();
			break;
	case 1:
		printf("Success!\r\n");
		break;
	case 2:
			printf("Fail!\r\n");
			break;
	case 3:
			printf("Other?\r\n");
			break;
	case 4:
		//Pend an interrupt to do the context switch
		_ICSR |= 1<<28;
		__asm("isb");
		break;
	default: /* unknown SVC */
	break;
	}
}

uint32_t* findThread()
{
	uint32_t newAdress = MSP_INIT_VAL - (threadCount+1)*THREAD_SIZE;

	if(newAdress < (MSP_INIT_VAL - MAX_STACK_SIZE - THREAD_SIZE))
		return (uint32_t*)(0x0);
	threadCount++;
	return (uint32_t *)newAdress;
}


char osCreateThread(void (*func_ptr)(void*))
{
	uint32_t* thread_ptr = findThread();
	if(thread_ptr == 0x0)
		return 0;
	uint32_t threadNum = threadCount-1;
	threadArray[threadNum].sp = thread_ptr;
	threadArray[threadNum].thread_function = func_ptr;
	threadArray[threadNum].timeslice = RRT;
	threadArray[threadNum].runtime = RRT;

	*(--(threadArray[threadNum].sp)) = 1<<24; //A magic number, this is xPSR
	*(--(threadArray[threadNum].sp)) = (uint32_t)func_ptr; //PC
	*(--threadArray[threadNum].sp) = 0xA; //LR
	*(--threadArray[threadNum].sp) = 0xA; //R12
	*(--threadArray[threadNum].sp) = 0xA; //R3
	*(--threadArray[threadNum].sp) = 0xA; //R2
	*(--threadArray[threadNum].sp) = 0xA; //R1
	*(--threadArray[threadNum].sp) = 0xA; //R0
	for(int i = 0; i < 8; i++) //R11-4
	{
		*(--threadArray[threadNum].sp) = 0xA;
	}

	return 1;
}

char osCreateThreadArgs(void (*func_ptr)(void*), uint32_t timeout, void* args)
{
	uint32_t* thread_ptr = findThread();
	if(thread_ptr == 0x0)
		return 0;
	uint32_t threadNum = threadCount-1;
	threadArray[threadNum].sp = thread_ptr;
	threadArray[threadNum].thread_function = func_ptr;
	threadArray[threadNum].timeslice = timeout;
	threadArray[threadNum].runtime = timeout;


	*(--(threadArray[threadNum].sp)) = 1<<24; //A magic number, this is xPSR
	*(--(threadArray[threadNum].sp)) = (uint32_t)func_ptr; //PC
	*(--threadArray[threadNum].sp) = 0xA; //LR
	*(--threadArray[threadNum].sp) = 0xA; //R12
	*(--threadArray[threadNum].sp) = 0xA; //R3
	*(--threadArray[threadNum].sp) = 0xA; //R2
	*(--threadArray[threadNum].sp) = 0xA; //R1
	*(--threadArray[threadNum].sp) = args; //R0
	for(int i = 0; i < 8; i++) //R11-4
	{
		*(--threadArray[threadNum].sp) = 0xA;
	}

	return 1;
}

void osKernelInitialize()
{
	MSP_INIT_VAL = (uint32_t)(*(uint32_t**)0x0);
	threadCount = 0;
	SHPR3 |= 0xFE << 16; //shift the constant 0xFE 16 bits to set PendSV priority
	SHPR2 |= 0xFDU << 24; //Set the priority of SVC higher than PendSV
}

void osSched()
{
	threadArray[currentThread].sp = (uint32_t*)(__get_PSP() - 8*4);
	currentThread = (currentThread+1)%threadCount;
	__set_PSP((int32_t)threadArray[currentThread].sp);
	return;
}


void osKernelStart()
{
	started = 1;
	__asm("SVC #0");
}
void osYield(void)
{
	threadArray[currentThread].runtime = threadArray[currentThread].timeslice;
	__asm("SVC #4");
}

void pre_emptive()
{
	if(started == 0) return;

	threadArray[currentThread].runtime--;
	if(threadArray[currentThread].runtime == 0)
	{
		threadArray[currentThread].runtime = threadArray[currentThread].timeslice;
		_ICSR |= 1<<28;
		__asm("isb");
	}


}
