/*
 * utiltrace.hpp
 *
 *  Created on: Oct 11, 2016
 *      Author: zhanggan
 */

#ifndef SRC_GLUE_UTILTRACE_HPP_
#define SRC_GLUE_UTILTRACE_HPP_
#include "VMFrame.h"
#include "VMMethod.h"

void __trace(const char * ,const char *,int  ,char * ,char * );

#define PRETRACE(trcs)   char trcs[200]; memset(trcs,0,200);
#define TRACE(trcp,trcs)  __trace(__FILE__,__FUNCTION__,__LINE__,(char *)trcp,(char *)trcs); //if (trcs != NULL) memset(trcs,0,200);
#define TRACEOBJ(trcp,trcs,obj) \
			if (strcmp("Frame",GETOBJCLASSNAME(obj)) == 0){ \
				sprintf(trcs,"object=%p,objClsName=%s,FrameMethodSig=%s",(obj),GETOBJCLASSNAME(obj),((pVMFrame)obj)->GetMethod()->GetSignature()->GetChars()); 	}\
else if (strcmp("Method",GETOBJCLASSNAME(obj)) == 0){ \
	sprintf(trcs,"object=%p,objClsName=%s,MethodSig=%s",(obj),GETOBJCLASSNAME(obj),((pVMMethod)obj)->GetSignature()->GetChars()); 	}\
	else if (strcmp("Symbol",GETOBJCLASSNAME(obj)) == 0){ \
		sprintf(trcs,"object=%p,objClsName=%s,Symbol=%s",(obj),GETOBJCLASSNAME(obj),((pVMSymbol)obj)->GetChars()); 	}\
		else if (strcmp("Integer",GETOBJCLASSNAME(obj)) == 0){ \
			sprintf(trcs,"object=%p,objClsName=%s,Integer=%d",(obj),GETOBJCLASSNAME(obj),((pVMInteger)obj)->GetEmbeddedInteger()); 	}\
		else {			sprintf(trcs,"object=%p,objClsName=%s",(obj),GETOBJCLASSNAME(obj)); } \
			__trace(__FILE__,__FUNCTION__,__LINE__,(char *)trcp,(char *)trcs);

#define GETOBJCLASSNAME(obj) ((((pVMObject)(obj))->GetClass())->GetName()->GetChars())
#define TRCOBJ(trcp,obj) {\
		char trcs[200]; memset(trcs,0,200); \
			if (strcmp("Frame",GETOBJCLASSNAME(obj)) == 0){ \
				sprintf(trcs,"object=%p,objClsName=%s,FrameMethodSig=%s",(obj),GETOBJCLASSNAME(obj),((pVMFrame)obj)->GetMethod()->GetSignature()->GetChars()); 	}\
else if (strcmp("Method",GETOBJCLASSNAME(obj)) == 0){ \
	sprintf(trcs,"object=%p,objClsName=%s,MethodSig=%s",(obj),GETOBJCLASSNAME(obj),((pVMMethod)obj)->GetSignature()->GetChars()); 	}\
	else if (strcmp("Symbol",GETOBJCLASSNAME(obj)) == 0){ \
		sprintf(trcs,"object=%p,objClsName=%s,Symbol=%s",(obj),GETOBJCLASSNAME(obj),((pVMSymbol)obj)->GetChars()); 	}\
		else if (strcmp("Integer",GETOBJCLASSNAME(obj)) == 0){ \
			sprintf(trcs,"object=%p,objClsName=%s,Integer=%d",(obj),GETOBJCLASSNAME(obj),((pVMInteger)obj)->GetEmbeddedInteger()); 	}\
		else {			sprintf(trcs,"object=%p,objClsName=%s",(obj),GETOBJCLASSNAME(obj)); } \
	     __trace(__FILE__,__FUNCTION__,__LINE__,(char *)trcp,(char *)trcs); }
#define TRCADDR(trcp,addr) {char trcs[50];\
		memset(trcs,0,50); \
		 if (NULL!=addr){\
			 sprintf(trcs,"%s=%p",#addr,(addr)); \
		 }\
		 __trace(__FILE__,__FUNCTION__,__LINE__,(char *)trcp,(char *)trcs); }
#define TRCCHRS(trcp,addr) {char trcs[100];\
		memset(trcs,0,100); \
		 if (NULL!=addr){\
			 sprintf(trcs,"%s=%s",#addr,(char *)(addr)); \
		 }\
		 __trace(__FILE__,__FUNCTION__,__LINE__,(char *)trcp,(char *)trcs); }
#endif /* SRC_GLUE_UTILTRACE_HPP_ */
