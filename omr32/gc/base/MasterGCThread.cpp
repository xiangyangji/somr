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
 ******************************************************************************/

#include "modronopt.h"
#include "ModronAssertions.h"
#include "omrport.h"

#include "MasterGCThread.hpp"

#include "CollectorLanguageInterfaceImpl.hpp"
#include "ConcurrentGMPStats.hpp"
#include "ParallelDispatcher.hpp"
#include "EnvironmentBase.hpp"
#include "Collector.hpp"

/**
 * Initialization
 */
MM_MasterGCThread::MM_MasterGCThread(MM_EnvironmentBase *env)
	: MM_BaseNonVirtual()
	, _collectorControlMutex(NULL)
	, _masterThreadState(STATE_ERROR)
	, _masterGCThread(NULL)
	, _incomingCycleState(NULL)
	, _allocDesc(NULL)
	, _extensions(env->getExtensions())
	, _collector(NULL)
{
	_typeId = __FUNCTION__;
}

uintptr_t
MM_MasterGCThread::master_thread_proc2(OMRPortLibrary* portLib, void *info)
{
	MM_MasterGCThread *masterGCThread = (MM_MasterGCThread*)info;
	/* jump into the master thread procedure and wait for work.  This method will NOT return */
	masterGCThread->masterThreadEntryPoint();
	Assert_MM_unreachable();
	return 0;
}

int J9THREAD_PROC
MM_MasterGCThread::master_thread_proc(void *info)
{
	MM_MasterGCThread *masterGCThread = (MM_MasterGCThread*)info;
	MM_GCExtensionsBase *extensions = masterGCThread->_extensions;
	OMR_VM *omrVM = extensions->getOmrVM();
	OMRPORT_ACCESS_FROM_OMRVM(omrVM);
	uintptr_t rc = 0;
	omrsig_protect(master_thread_proc2, info,
			((MM_ParallelDispatcher *)extensions->dispatcher)->getSignalHandler(), omrVM,
		OMRPORT_SIG_FLAG_SIGALLSYNC | OMRPORT_SIG_FLAG_MAY_CONTINUE_EXECUTION,
		&rc);
	return 0;
}



bool
MM_MasterGCThread::initialize(MM_Collector *collector)
{
	bool success = true;
	if(omrthread_monitor_init_with_name(&_collectorControlMutex, 0, "MM_MasterGCThread::_collectorControlMutex")) {
		success = false;
	}

	_collector = collector;

	return success;
}

void
MM_MasterGCThread::tearDown(MM_EnvironmentBase *env)
{
	if (NULL != _collectorControlMutex) {
		omrthread_monitor_destroy(_collectorControlMutex);
		_collectorControlMutex = NULL;
	}
	_collector = NULL;
}


bool
MM_MasterGCThread::startup()
{
	/* set the success flag to false and we will set it true if everything succeeds */
	bool success = false;

	if (_extensions->fvtest_disableExplictMasterThread) {
		/* GC should be able to act even if master thread is not created (or late) */
		_masterThreadState = STATE_DISABLED;
		success = true;
	} else {
		/* hold the monitor over start-up of this thread so that we eliminate any timing hole where it might notify us of its start-up state before we wait */
		omrthread_monitor_enter(_collectorControlMutex);
		_masterThreadState = STATE_STARTING;
		intptr_t forkResult = createThreadWithCategory(
			NULL,
			OMR_OS_STACK_SIZE,
			J9THREAD_PRIORITY_NORMAL,
			0,
			master_thread_proc,
			this,
			J9THREAD_CATEGORY_SYSTEM_GC_THREAD);
		if (forkResult == 0) {
			/* thread creation success */
			/* wait to find out if they started up, successfully */
			while (STATE_STARTING == _masterThreadState) {
				omrthread_monitor_wait(_collectorControlMutex);
			}
			if (STATE_ERROR != _masterThreadState) {
				/* the master thread managed to start up and is in the waiting state, ready for GC requests */
				success = true;
			}
		} else {
			_masterThreadState = STATE_ERROR;
		}
		omrthread_monitor_exit(_collectorControlMutex);
	}

	return success;
}

void
MM_MasterGCThread::shutdown()
{
	Assert_MM_true(NULL != _collectorControlMutex);
	if ((STATE_ERROR != _masterThreadState) && (STATE_DISABLED != _masterThreadState)) {
		/* tell the background thread to shut down and then wait for it to exit */
		omrthread_monitor_enter(_collectorControlMutex);
		while(STATE_TERMINATED != _masterThreadState) {
			_masterThreadState = STATE_TERMINATION_REQUESTED;
			omrthread_monitor_notify(_collectorControlMutex);
			omrthread_monitor_wait(_collectorControlMutex);
		}
		omrthread_monitor_exit(_collectorControlMutex);
		
		/* don't NULL _collector as RAS could still trigger a collection after we've started shutting down */
	}
}

void
MM_MasterGCThread::masterThreadEntryPoint()
{
	OMR_VMThread *omrVMThread = NULL;
	Assert_MM_true(NULL != _collectorControlMutex);
	Assert_MM_true(NULL == _masterGCThread);
	MM_CollectorLanguageInterface *cli = _extensions->collectorLanguageInterface;

	/* Attach the thread as a system daemon thread */	
	/* You need a VM thread so that the stack walker can work */
	omrVMThread = cli->attachVMThread(_extensions->getOmrVM(), "Dedicated GC Master", MM_CollectorLanguageInterfaceImpl::ATTACH_GC_MASTER_THREAD);
	if (NULL == omrVMThread) {
		/* we failed to attach so notify the creating thread that we should fail to start up */
		omrthread_monitor_enter(_collectorControlMutex);
		_masterThreadState = STATE_ERROR;
		omrthread_monitor_notify(_collectorControlMutex);
		omrthread_exit(_collectorControlMutex);
	} else {
		/* thread attached successfully */
		MM_EnvironmentBase *env = MM_EnvironmentBase::getEnvironment(omrVMThread);
		env->setThreadType(GC_MASTER_THREAD);

		/* Begin running the thread */
		omrthread_monitor_enter(_collectorControlMutex);
		
		_collector->preMasterGCThreadInitialize(env);
		
		/* first, notify the thread that started us that we are ready to enter the waiting state */
		_masterThreadState = STATE_WAITING;
		_masterGCThread = omrthread_self();
		omrthread_monitor_notify(_collectorControlMutex);
		do {
			if (STATE_GC_REQUESTED == _masterThreadState) {
				/* Note that we might want to give up the _collectorControlMutex around the call to run the collect but it might allow
				 * for some bugs to become harder to track down (consider the case of an additional thread entering the monitor during
				 * the collect - this can't happen but allowing it to access the monitor would make the bug harder to find) and holding
				 * the monitor does expose the thread dependency to the OS so it can potentially give the master GC thread priority
				 * inheritance from the thread which requested the collect.
				 */
				Assert_MM_true(NULL != _incomingCycleState);
				env->_cycleState = _incomingCycleState;

				/* this thread effectively inherits exclusive access from the mutator thread -- set its state to indicate this */
				env->assumeExclusiveVMAccess(1);

				_collector->masterThreadGarbageCollect(env, _allocDesc);

				/* clear the exclusive access state */
				uintptr_t exclusiveCount = env->relinquishExclusiveVMAccess();
				Assert_MM_true(1 == exclusiveCount);

				env->_cycleState = NULL;
				_incomingCycleState = NULL;
				_masterThreadState = STATE_WAITING;
				omrthread_monitor_notify(_collectorControlMutex);
			}
			if (STATE_WAITING == _masterThreadState) {
				/* now try the concurrent operation */
				if (_collector->isConcurrentWorkAvailable(env)) {
					_masterThreadState = STATE_RUNNING_CONCURRENT;
					// todo: remove this VLHGC specific structure
					MM_ConcurrentGMPStats stats;
					_collector->preConcurrentInitializeStatsAndReport(env, &stats);
					/* ensure that we get out of this monitor so that the mutator can wake up */
					omrthread_monitor_exit(_collectorControlMutex);
					/* note that we currently run the concurrent mark without VM access since it could deadlock us with another thread trying to acquire exclusive to invoke a GC */
					uintptr_t bytesConcurrentlyScanned = _collector->masterThreadConcurrentCollect(env);
					omrthread_monitor_enter(_collectorControlMutex);
					_collector->postConcurrentUpdateStatsAndReport(env, &stats, bytesConcurrentlyScanned);
					if (STATE_RUNNING_CONCURRENT == _masterThreadState) {
						/* we are still in the concurrent state (might have changed if a GC was requested) so set it back to waiting */
						_masterThreadState = STATE_WAITING;
					}
				} else {
					omrthread_monitor_wait(_collectorControlMutex);
				}
			}
		} while (STATE_TERMINATION_REQUESTED != _masterThreadState);
		/* notify the other side that we are active so that they can continue running */
		_masterThreadState = STATE_TERMINATED;
		_masterGCThread = NULL;
		omrthread_monitor_notify(_collectorControlMutex);
		cli->detachVMThread(_extensions->getOmrVM(), omrVMThread, MM_CollectorLanguageInterfaceImpl::ATTACH_GC_MASTER_THREAD);
		omrthread_exit(_collectorControlMutex);
	}
}

bool
MM_MasterGCThread::garbageCollect(MM_EnvironmentBase *env, MM_AllocateDescription *allocDescription)
{
	Assert_MM_mustHaveExclusiveVMAccess(env->getOmrVMThread());
	bool didAttemptCollect = false;
	
	if (NULL != _collector) {
		/* the collector has started up so try to run */
		/* once the master thread has stored itself in the _masterGCThread, it should never need to collect - this would hang */
		Assert_MM_true(omrthread_self() != _masterGCThread);
		if (NULL == _masterGCThread) {
			/* We might not have _masterGCThread in the startup phase or late in the shutdown phase.
			 * For example, there may be a native out-of-memory during startup or RAS may 
			 * trigger a GC after we've shutdown the master thread.
			 */
			Assert_MM_true(0 == env->getSlaveID());
			_collector->preMasterGCThreadInitialize(env);
			_collector->masterThreadGarbageCollect(env, allocDescription);
			/* Another thread may act a Master GC => be nice and clean up our environment */
			/* we should clean up _rememberedSetCardBucketPool, but we keep it for debugging purpose */
			/* env->_rememberedSetCardBucketPool = NULL; */
		} else {
			/* this is the general case, when the master thread is running internally */
			omrthread_monitor_enter(_collectorControlMutex);
			/* The variable assignments below are safe because we hold Xaccess.  Otherwise, it is possible (based on the wait/notify mechanism here)
			 * that another thread could come in under this mutex and stomp on the "parameters" while another thread is waiting.
			 */
			_allocDesc = allocDescription;
			_incomingCycleState = env->_cycleState;
			MasterGCThreadState previousState = _masterThreadState;
			_masterThreadState = STATE_GC_REQUESTED;
			if (STATE_WAITING == previousState) {
				omrthread_monitor_notify(_collectorControlMutex);
			} else if (STATE_RUNNING_CONCURRENT == previousState) {
				_collector->forceConcurrentFinish();
			} else {
				Assert_MM_unreachable();
			}
			
			/* The master thread will claim exclusive VM access. Artificially give it up in this thread so that tools like -Xcheck:vm continue to work. */
			uintptr_t savedExclusiveCount = env->relinquishExclusiveVMAccess();
			while (STATE_GC_REQUESTED == _masterThreadState) {
				omrthread_monitor_wait(_collectorControlMutex);
			}
			env->assumeExclusiveVMAccess(savedExclusiveCount);

			Assert_MM_true(NULL == _incomingCycleState);
			omrthread_monitor_exit(_collectorControlMutex);
		}
		
		didAttemptCollect = true;
	}
	return didAttemptCollect;
}
