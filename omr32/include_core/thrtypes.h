/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 1991, 2016
 *
 *  This program and the accompanying materials are made available
 *  under the terms of the Eclipse Public License v1.0 and
 *  Apache License v2.0 which accompanies this distribution.
 *
 *      The Eclipse Public License is available at
 *      http://www.eclipse.org/legal/epl-v10.html
 *
 *      The Apache License v2.0 is available at
 *      http://www.opensource.org/licenses/apache2.0.php
 *
 * Contributors:
 *    Multiple authors (IBM Corp.) - initial implementation and documentation
 *******************************************************************************/

#ifndef THRTYPES_H
#define THRTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "omrthread.h"
#include "omrmemcategories.h"
#include "thrdsup.h"

typedef struct J9Thread {
	J9_ABSTRACT_THREAD_FIELDS
	OSTHREAD handle;
	J9OSCond condition;
	J9OSMutex mutex;
	uintptr_t stacksize;
#if defined(WIN32)
	uintptr_t *tos;
#endif /* WIN32 */
#if defined(LINUX)
	void *jumpBuffer;
#endif /* LINUX */
#if defined(OMR_PORT_NUMA_SUPPORT)
	uint8_t numaAffinity[128];
#endif /* OMR_PORT_NUMA_SUPPORT */
	struct J9ThreadMonitor *destroyed_monitor_head;
	struct J9ThreadMonitor *destroyed_monitor_tail;
#if defined(J9ZOS390)
	omrthread_os_errno_t os_errno2;
#endif   /* J9ZOS390 */
#if !defined(WIN32)
	uintptr_t key_deletion_attempts;
#endif /* !WIN32 */
} J9Thread;

/*
 * @ddr_namespace: map_to_type=J9ThreadMonitor
 */

typedef struct J9ThreadMonitor {
	J9_ABSTRACT_MONITOR_FIELDS
	J9OSMutex mutex;
	struct J9Thread *notifyAllWaiting;
} J9ThreadMonitor;


#define J9THREAD_MONITOR_POOL_SIZE 64

/*
 * @ddr_namespace: map_to_type=J9ThreadMonitorPool
 */

typedef struct J9ThreadMonitorPool {
	struct J9ThreadMonitorPool *next;
	struct J9ThreadMonitor *next_free;
	struct J9ThreadMonitor entries[J9THREAD_MONITOR_POOL_SIZE];
} J9ThreadMonitorPool;

/* This constant required here for DDR */
#define MONITOR_POOL_SIZE  J9THREAD_MONITOR_POOL_SIZE


typedef struct J9ThreadGlobal {
	struct J9ThreadGlobal *next;
	char *name;
	uintptr_t data;
} J9ThreadGlobal;

typedef struct J9ThreadLibrary {
	uintptr_t spinlock;
	struct J9ThreadMonitorPool *monitor_pool;
	struct J9Pool *thread_pool;
	uintptr_t threadCount;
#if defined(WIN32)
	uintptr_t stack_usage;
#endif /* WIN32 */
	intptr_t initStatus;
	uintptr_t flags;
	struct J9ThreadGlobal *globals;
	struct J9Pool *global_pool;
	J9OSMutex global_mutex;
	TLSKEY self_ptr;
	J9OSMutex monitor_mutex;
	J9OSMutex tls_mutex;
	omrthread_tls_finalizer_t tls_finalizers[J9THREAD_MAX_TLS_KEYS];
	char *thread_weight;
#if defined(OMR_THR_JLM)
	struct J9Pool *monitor_tracing_pool;
	struct J9Pool *thread_tracing_pool;
	struct J9ThreadMonitorTracing *gc_lock_tracing;
	uint64_t clock_skew;
#endif /* OMR_THR_JLM */
#if defined(OMR_THR_THREE_TIER_LOCKING)
	uintptr_t defaultMonitorSpinCount1;
	uintptr_t defaultMonitorSpinCount2;
	uintptr_t defaultMonitorSpinCount3;
#endif /* OMR_THR_THREE_TIER_LOCKING */
	TLSKEY attachedLibKey;
#if defined(OMR_THR_ADAPTIVE_SPIN)
	uintptr_t adaptSpinSampleThreshold;
	uintptr_t adaptSpinHoldtime;
	uintptr_t adaptSpinSlowPercent;
	uintptr_t adaptSpinSampleStopCount;
	uintptr_t adaptSpinSampleCountStopRatio;
	uintptr_t adaptSpinKeepSampling;
#endif /* OMR_THR_ADAPTIVE_SPIN */
	uintptr_t secondarySpinForObjectMonitors;
	OMRMemCategory threadLibraryCategory;
	OMRMemCategory nativeStackCategory;
#if defined(OMR_THR_FORK_SUPPORT)
	OMRMemCategory mutexCategory;
	OMRMemCategory condvarCategory;
#endif /* defined(OMR_THR_FORK_SUPPORT) */
	omrthread_monitor_t globalMonitor;
	uintptr_t useRealtimeScheduling;
#if defined(OMR_THR_YIELD_ALG)
	uintptr_t yieldAlgorithm;
	uintptr_t yieldUsleepMultiplier;
#endif /* OMR_THR_YIELD_ALG */
	J9ThreadsCpuUsage cumulativeThreadsInfo;
	J9OSMutex resourceUsageMutex;
	uintptr_t threadWalkMutexesHeld;
#if defined(OMR_THR_FORK_SUPPORT)
	struct J9Pool *rwmutexPool;
#endif /* defined(OMR_THR_FORK_SUPPORT) */
	omrthread_attr_t systemThreadAttr;
#if defined(OSX)
	clock_serv_t clockService;
#endif /* defined(OSX) */
} J9ThreadLibrary;

/*
 * @ddr_namespace: map_to_type=J9Semaphore
 */

typedef struct J9Semaphore {
	OSSEMAPHORE sem;
} J9Semaphore;

#define STACK_DEFAULT_SIZE 0x8000

#define FREE_TAG ((uintptr_t)-1)
typedef struct J9ThreadMonitorPool *omrthread_monitor_pool_t;
typedef struct J9ThreadLibrary *omrthread_library_t;

#ifdef __cplusplus
}
#endif

#endif /* THRTYPES_H */
