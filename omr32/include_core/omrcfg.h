/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2015, 2016
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

/*
 * include_core/omrcfg.h.  Generated from omrcfg.h.in by configure.
 */

#ifndef OMRCFG_H
#define OMRCFG_H

/*
 * @ddr_namespace: map_to_type=OmrBuildFlags
 * @ddr_options: buildflagsonly
 */

#ifdef __cplusplus
extern "C" {
#endif

#define OMR_GC
#define OMR_PORT
#define OMR_THREAD
#define OMR_OMRSIG
#undef OMR_EXAMPLE

#define OMR_GC_ALLOCATION_TAX
#undef OMR_GC_OBJECT_ALLOCATION_NOTIFY
#define OMR_GC_BATCH_CLEAR_TLH
#define OMR_GC_COMBINATION_SPEC
#undef OMR_GC_CONCURRENT_SWEEP
#define OMR_GC_DEBUG_ASSERTS
#define OMR_GC_HEAP_CARD_TABLE
#define OMR_GC_LARGE_OBJECT_AREA
#define OMR_GC_MINIMUM_OBJECT_SIZE
#undef OMR_GC_MODRON_COMPACTION
#undef OMR_GC_MODRON_CONCURRENT_MARK
#undef OMR_GC_MODRON_SCAVENGER
#undef OMR_GC_CONCURRENT_SCAVENGER
#define OMR_GC_MODRON_STANDARD
#define OMR_GC_NON_ZERO_TLH
#undef OMR_GC_SEGREGATED_HEAP
#define OMR_GC_THREAD_LOCAL_HEAP

/**
 * Tells if a platform has support for Semaphores.
 * ifRemoved: platform cannot use semaphore operations.
 */
#define OMR_INTERP_HAS_SEMAPHORES

/**
 * Use TDF based trace engine
 * ifRemoved: No tracepoints
 */
#define OMR_RAS_TDF_TRACE

/**
 * Support options to adapt spinning as controlled by options provided to the vm.
 * Requires flag: OMR_THR_JLM_HOLD_TIMES
 * ifRemoved: No adaptive spinning supported
 */
#define OMR_THR_ADAPTIVE_SPIN

/**
 * Enable support for lock monitor to profile locking behaviour - counts only
 * ifRemoved: Lock monitor cannot be enabled
 */
#define OMR_THR_JLM

/**
 * Enable support for lock monitor to profile locking behaviour - counts + timings only
 * ifRemoved: Lock monitor cannot be enabled
 */
#define OMR_THR_JLM_HOLD_TIMES

/**
 * object header does not contain the monitor word
 * ifRemoved: object header will contain the monitor word
 */
#define OMR_THR_LOCK_NURSERY

/**
 * Output lots of thread statistics (Pentium only)
 * ifRemoved: Turn off in production VMs
 */
#undef OMR_THR_TRACING

#undef OMRTHREAD_LIB_AIX
#define OMRTHREAD_LIB_UNIX 1
#undef OMRTHREAD_LIB_WIN32
#undef OMRTHREAD_LIB_ZOS

/**
 * This spec targets PPC processors.
 */
#undef OMR_ARCH_POWER

/**
 * This spec targets ARM processors.
 */
#undef OMR_ARCH_ARM

/**
 * This spec targets x86 processors.
 */
#undef OMR_ARCH_X86

/**
 * This spec targets S390 processors.
 */
#undef OMR_ARCH_S390

#undef OMR_ENV_DATA64
#undef OMR_ENV_GCC
#define OMR_ENV_LITTLE_ENDIAN
#define OMR_GC_ARRAYLETS
#undef OMR_GC_COMPRESSED_POINTERS
#undef OMR_GC_HYBRID_ARRAYLETS
#undef OMR_GC_OBJECT_MAP
#undef OMR_GC_REALTIME
#undef OMR_GC_STACCATO
#define OMR_GC_TLH_PREFETCH_FTA
#undef OMR_GC_VLHGC

/**
 * Flag to indicate that on 64-bit platforms, the monitor slot and class slot in object headers are U32 rather than UDATA.
 */
#undef OMR_INTERP_COMPRESSED_OBJECT_HEADER

/**
 * Flag to indicate that on 64-bit platforms, the monitor slot in object headers is a U32 rather than a UDATA.
 * ifRemoved: The monitor slot is a UDATA.
 */
#undef OMR_INTERP_SMALL_MONITOR_SLOT

/**
 * Add support for CUDA
 */
#undef OMR_OPT_CUDA

/**
 * Enabling this flag implies the platform's vmem implementation is able to allocate memory top down.
 * ifRemoved: Implies the platform's vmem implementation is not able to allocate memory top down
 */
#undef OMR_PORT_ALLOCATE_TOP_DOWN

/**
 * Enables async signal handler thread in the port library.
 * ifRemoved: Async signal handler thread is disabled.
 */
#undef OMR_PORT_ASYNC_HANDLER

/**
 * The platform is able to attempt to reserve virtual memory via a call to omrvmem_reserve_memory at the address specified by the user.
 * ifRemoved: The platform is not able to reserve virtual memory at a specific address
 */
#undef OMR_PORT_CAN_RESERVE_SPECIFIC_ADDRESS

/**
 * This platform is able to associate memory with a specific node, as is relevant when the system has a Non Uniform Memory Access configuration.
 * ifRemoved: This platform is not able to associate memory with a specific node.
 */
#define OMR_PORT_NUMA_SUPPORT

/**
 * If set, omrsig_protect will include support for registering a handler using CEEHDLR.
 * Pass OMRPORT_SIG_OPTIONS_ZOS_USE_CEEHDLR into omrsig_set_options() before the first call
 * to omrsig_protect() to turn on the use of CEEHDLR at runtime.
 * ifRemoved: omrsig_protect will not include support for CEEHDLR.
 */
#undef OMR_PORT_ZOS_CEEHDLRSUPPORT

/**
 * Enable the j9thread library to be used after a fork().
 * ifRemoved: The j9thread library may be corrupt and unusable after a fork().
 */
#define OMR_THR_FORK_SUPPORT

/**
 * Attempt to enter raw monitors using user-space spinlocks before reverting to an OS synchronization object
 * ifRemoved: All raw monitor enters will proceed directly to an OS synchronization object
 */
#undef OMR_THR_THREE_TIER_LOCKING

/**
 * Enable support for custom spin options.
 * Requires flag: OMR_THR_THREE_TIER_LOCKING.
 * ifRemoved: Custom spin options cannot be used.
 */
#undef OMR_THR_CUSTOM_SPIN_OPTIONS

/**
 * This enables the option to select different algorithms for yielding.
 */
#undef OMR_THR_YIELD_ALG

#ifdef __cplusplus
}
#endif

#endif /* OMRCFG_H */
