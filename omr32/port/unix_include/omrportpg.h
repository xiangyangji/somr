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
 *    Multiple authors (IBM Corp.) - initial API and implementation and/or initial documentation
 *******************************************************************************/

#ifndef omrportpg_h
#define omrportpg_h

/*
 * @ddr_namespace: default
 */

#include "omrcfg.h"

#include "omrport.h"
#if defined(OMR_ENV_DATA64)
#include "omrportpriv.h"
#include "omrmem32struct.h"
#endif

/** Number of pageSizes supported.  There is always 1 for the default size, and 1 for the 0 terminator.
 * The number of large pages supported determines the remaining size.
 * Responsibility of the implementation of omrvmem to initialize this table correctly.
 */
#define OMRPORT_VMEM_PAGESIZE_COUNT 5

#if defined(OMR_PORT_NUMA_SUPPORT) && defined (LINUX)
#include <numa.h>
#include <sched.h>

typedef struct J9PortNodeMask {
	unsigned long mask[1024 / 8 / sizeof(unsigned long)];
} J9PortNodeMask;
#endif

typedef struct OMRPortPlatformGlobals {
	uintptr_t numa_platform_supports_numa;
#if defined(OMR_PORT_NUMA_SUPPORT) && defined (LINUX)
	J9PortNodeMask numa_available_node_mask;
	unsigned long numa_max_node_bits;
	cpu_set_t process_affinity;
	J9PortNodeMask numa_mempolicy_node_mask;
	int numa_policy_mode; /* the NUMA policy returned by sys_get_mempolicy */
#endif
#if defined(OMR_ENV_DATA64)
	J9SubAllocateHeapMem32 subAllocHeapMem32;
#endif
	char *si_osType;
	char *si_osVersion;
	uintptr_t vmem_pageSize[OMRPORT_VMEM_PAGESIZE_COUNT]; /** <0 terminated array of supported page sizes */
	uintptr_t vmem_pageFlags[OMRPORT_VMEM_PAGESIZE_COUNT]; /** <0 terminated array of flags describing type of the supported page sizes */
#if defined(LINUX) && defined(S390)
	int64_t last_clock_delta_update;  /** hw clock microsecond timestamp of last clock delta adjustment */
	int64_t software_msec_clock_delta; /** signed difference between hw and sw clocks in milliseconds */
#endif
	BOOLEAN loggingEnabled;
	uintptr_t systemLoggingFlags;
	BOOLEAN globalConverterEnabled;
	char *si_executableName;
#if defined(RS6000) || defined (LINUXPPC) || defined (PPC)
	int32_t mem_ppcCacheLineSize;
#endif
#if defined(OMR_CONFIGURABLE_SUSPEND_SIGNAL)
	int32_t introspect_threadSuspendSignal;
#endif /* defined(OMR_CONFIGURABLE_SUSPEND_SIGNAL) */
} OMRPortPlatformGlobals;


#if defined(RS6000) || defined (LINUXPPC) || defined (PPC)
#define PPG_mem_ppcCacheLineSize (portLibrary->portGlobals->platformGlobals.mem_ppcCacheLineSize)
#endif
#define PPG_numa_platform_supports_numa (portLibrary->portGlobals->platformGlobals.numa_platform_supports_numa)
#if defined(OMR_PORT_NUMA_SUPPORT) && defined (LINUX)
#define PPG_numa_available_node_mask (portLibrary->portGlobals->platformGlobals.numa_available_node_mask)
#define PPG_numa_max_node_bits (portLibrary->portGlobals->platformGlobals.numa_max_node_bits)
#define PPG_process_affinity (portLibrary->portGlobals->platformGlobals.process_affinity)
#define PPG_numa_mempolicy_node_mask (portLibrary->portGlobals->platformGlobals.numa_mempolicy_node_mask)
#define PPG_numa_policy_mode (portLibrary->portGlobals->platformGlobals.numa_policy_mode)
#endif
#define PPG_si_osType (portLibrary->portGlobals->platformGlobals.si_osType)
#define PPG_si_osVersion (portLibrary->portGlobals->platformGlobals.si_osVersion)
#define PPG_vmem_pageSize (portLibrary->portGlobals->platformGlobals.vmem_pageSize)
#define PPG_vmem_pageFlags (portLibrary->portGlobals->platformGlobals.vmem_pageFlags)
#if defined(LINUX) && defined(S390)
#define PPG_last_clock_delta_update  (portLibrary->portGlobals->platformGlobals.last_clock_delta_update)
#define PPG_software_msec_clock_delta (portLibrary->portGlobals->platformGlobals.software_msec_clock_delta)
#endif
#if defined(OMR_ENV_DATA64)
#define PPG_mem_mem32_subAllocHeapMem32 (portLibrary->portGlobals->platformGlobals.subAllocHeapMem32)
#endif

#define PPG_syslog_enabled (portLibrary->portGlobals->platformGlobals.loggingEnabled)
#define PPG_syslog_flags (portLibrary->portGlobals->platformGlobals.systemLoggingFlags)

#define PPG_global_converter_enabled (portLibrary->portGlobals->platformGlobals.globalConverterEnabled)

#define PPG_si_executableName (portLibrary->portGlobals->platformGlobals.si_executableName)

#if defined(OMR_CONFIGURABLE_SUSPEND_SIGNAL)
#define PPG_introspect_threadSuspendSignal (portLibrary->portGlobals->platformGlobals.introspect_threadSuspendSignal)
#endif

#endif /* omrportpg_h */

