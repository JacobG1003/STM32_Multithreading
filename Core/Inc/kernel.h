/*
 * kernel.h
 *
 *  Created on: Oct 18, 2023
 *      Author: Jacob Gordon
 */

#ifndef SRC_KERNEL_H_
#define SRC_KERNEL_H_
#define MAX_STACK_SIZE 0x4000
#define THREAD_SIZE 0x400
#define SHPR2 *(uint32_t*)0xE000ED1C //for setting SVC priority, bits 31-24
#define SHPR3 *(uint32_t*)0xE000ED20 // PendSV is bits 23-16
#define _ICSR *(uint32_t*)0xE000ED04 //This lets us trigger PendSV
#define RRT 300
int lastKernelLocation;

typedef struct k_thread{
	uint32_t* sp; //stack pointer
	void (*thread_function)(void*); //function pointer
	uint32_t timeslice;
	uint32_t runtime;
}thread;

#endif /* SRC_KERNEL_H_ */
