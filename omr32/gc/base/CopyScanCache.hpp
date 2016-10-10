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

#if !defined(COPYSCANCACHEBASE_HPP_)
#define COPYSCANCACHEBASE_HPP_

/**
 * @ingroup GC_Base_Core
 * @name Scavenger Cache type
 * @{
 */
#define OMR_SCAVENGER_CACHE_TYPE_SEMISPACE 1
#define OMR_SCAVENGER_CACHE_TYPE_TENURESPACE 2
#define OMR_SCAVENGER_CACHE_TYPE_SPLIT_ARRAY 4
#define OMR_SCAVENGER_CACHE_TYPE_COPY 8
#define OMR_SCAVENGER_CACHE_TYPE_LOA 16
#define OMR_SCAVENGER_CACHE_TYPE_CLEARED 32
#define OMR_SCAVENGER_CACHE_TYPE_SCAN 64
#define OMR_SCAVENGER_CACHE_TYPE_HEAP 128
/* a mask which represents the flags which cannot change during the lifetime of a scan cache structure */
#define OMR_SCAVENGER_CACHE_MASK_PERSISTENT (OMR_SCAVENGER_CACHE_TYPE_HEAP)
/** @} */

/* TODO 108399: remove these after previous OMR_SCAVENGER_CACHE_TYPE_* have been propagated into J9 */
#define J9VM_MODRON_SCAVENGER_CACHE_TYPE_SEMISPACE OMR_SCAVENGER_CACHE_TYPE_SEMISPACE
#define J9VM_MODRON_SCAVENGER_CACHE_TYPE_TENURESPACE OMR_SCAVENGER_CACHE_TYPE_TENURESPACE
#define J9VM_MODRON_SCAVENGER_CACHE_TYPE_SPLIT_ARRAY OMR_SCAVENGER_CACHE_TYPE_SPLIT_ARRAY
#define J9VM_MODRON_SCAVENGER_CACHE_TYPE_COPY OMR_SCAVENGER_CACHE_TYPE_COPY
#define J9VM_MODRON_SCAVENGER_CACHE_TYPE_LOA OMR_SCAVENGER_CACHE_TYPE_LOA
#define J9VM_MODRON_SCAVENGER_CACHE_TYPE_CLEARED OMR_SCAVENGER_CACHE_TYPE_CLEARED
#define J9VM_MODRON_SCAVENGER_CACHE_TYPE_SCAN OMR_SCAVENGER_CACHE_TYPE_SCAN
#define J9VM_MODRON_SCAVENGER_CACHE_TYPE_HEAP OMR_SCAVENGER_CACHE_TYPE_HEAP
#define J9VM_MODRON_SCAVENGER_CACHE_MASK_PERSISTENT OMR_SCAVENGER_CACHE_MASK_PERSISTENT

#include "omrcomp.h"
#include "modronbase.h"

#include "Base.hpp"
#include "ObjectIteratorState.hpp"

/**
 * @todo Provide class documentation
 * @ingroup GC_Base_Core
 */

class MM_CopyScanCache : public MM_Base {
	/* Data Members */
private:
protected:
public:
	MM_CopyScanCache* next;
	uintptr_t flags;
	bool _hasPartiallyScannedObject; /**< whether the current object been scanned is partially scanned */
	void* cacheBase;
	void* cacheTop;
	void* cacheAlloc;
	void* scanCurrent;

	/* Members Function */
private:
protected:
public:
	/**
	 * Sets the flag on the cache which denotes it is currently in use as a scan cache
	 */
	MMINLINE void setCurrentlyBeingScanned()
	{
		flags |= OMR_SCAVENGER_CACHE_TYPE_SCAN;
	}
	/**
	 * Clears the flag on the cache which denotes it is currently in use as a scan cache
	 */
	MMINLINE void clearCurrentlyBeingScanned()
	{
		flags &= ~OMR_SCAVENGER_CACHE_TYPE_SCAN;
	}

	/**
	 * Checks the flag on the cache which denotes if it is currently in use as a scan cache
	 * @return True if the receiver is currently being used for scanning
	 */
	MMINLINE bool isCurrentlyBeingScanned() const
	{
		return (OMR_SCAVENGER_CACHE_TYPE_SCAN == (flags & OMR_SCAVENGER_CACHE_TYPE_SCAN));
	}
	/**
	 * @return whether there is scanning work in the receiver
	 */
	MMINLINE bool isScanWorkAvailable() const
	{
		return scanCurrent < cacheAlloc;
	}

	/**
	 * Create a CopyScanCache object.
	 */
	MM_CopyScanCache()
		: MM_Base()
		, next(NULL)
		, flags(0)
		, _hasPartiallyScannedObject(false)
		, cacheBase(NULL)
		, cacheTop(NULL)
		, cacheAlloc(NULL)
		, scanCurrent(NULL)
	{
	}

	MM_CopyScanCache(uintptr_t givenFlags)
		: MM_Base()
		, next(NULL)
		, flags(givenFlags)
		, _hasPartiallyScannedObject(false)
		, cacheBase(NULL)
		, cacheTop(NULL)
		, cacheAlloc(NULL)
		, scanCurrent(NULL)
	{
	}
};

#endif /* COPYSCANCACHEBASE_HPP_ */
