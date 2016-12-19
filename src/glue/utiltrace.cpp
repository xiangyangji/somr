/*
 * utiltrace.cpp
 *
 *  Created on: Oct 12, 2016
 *      Author: zhanggan
 */
#include "utiltrace.hpp"
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <pthread.h>

void __trace(const char * file,const char * func,int line,char * tp,char * other){
	//pid_t tid = syscall(SYS_gettid);
	pthread_t ptid = pthread_self();
	char * filename = (char *)strrchr(file,'/');
	if(filename == NULL) {filename =(char *) file ; filename--;}
	printf("ptid=%ld,%s:%d.(%s):%s,%s\n",ptid,++filename,line,func,tp,other);
}


