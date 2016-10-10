/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 1991, 2015
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

#if !defined(ALLOCATEDESCRIPTION_HPP_)
#define ALLOCATEDESCRIPTION_HPP_

#include "omrcfg.h"
#include "omrcomp.h"
#include "modronbase.h"
#include "objectdescription.h"

#include "BaseVirtual.hpp"
#include "MemorySubSpace.hpp"

class MM_MemoryPool;
class MM_MemorySpace;

/**
 * @todo Provide class documentation
 * @ingroup GC_Base_Core
 */
class MM_AllocateDescription : public MM_BaseVirtual {
	/* N.B This class should only define non-VM specific properties of the 
	 * allocation request. Any VM specific properties should be defined 
	 * in MM_AllocateDescription
	 */ 
protected:

	uintptr_t _bytesRequested;

	uintptr_t _allocateFlags;
	uint32_t  _objectFlags;

	bool _allocationSucceeded; /**< Was the allocation successful */

	MM_MemorySpace *_memorySpace;
	MM_MemorySubSpace *_memorySubSpace;
	uintptr_t _allocationTaxSize;

	bool _tlhAllocation;
	bool _nurseryAllocation;
	bool _loaAllocation;
	uintptr_t _spineBytes;
	uintptr_t _numArraylets;
	bool _chunkedArray;
	omrarrayptr_t _spine; /**< field to store the arraylet spine allocated during an arraylet allocation */
	bool _threadAtSafePoint;
	MM_MemoryPool *_memoryPool;
		
	/* The following fields are applicable to collectorAllocate requests only; ignored on other allocate requests */
	bool _collectorAllocateExpandOnFailure; /**< if the allocation fails then expand heap if possible to satify the allocation */
	bool _collectorAllocateSatisfyAnywhere; /**< satify an allocate from any subpool */ 
	MM_MemorySubSpace::AllocationType _allocationType;	/**< Describes what type of work the allocation layer needs to perform to satisfy this request (TLH versus object allocate, for example) */
	bool  _collectAndClimb;
	bool  _climb;				/* indicates that current attempt to allocate should try parent, if current subspace failed */
	bool  _completedFromTlh;
		
public:

	MMINLINE uint32_t getObjectFlags() { return _objectFlags; }
	MMINLINE bool getTenuredFlag() { return (_allocateFlags & OMR_GC_ALLOCATE_OBJECT_TENURED) == OMR_GC_ALLOCATE_OBJECT_TENURED; }
	MMINLINE bool getPreHashFlag() { return OMR_GC_ALLOCATE_OBJECT_HASHED == (_allocateFlags & OMR_GC_ALLOCATE_OBJECT_HASHED); }

	/* NON_ZERO_TLH flag set means JIT requested to skip zero in it (not what its name suggests to allocate from non zero TLH).
	 * We internally, decide which TLH this gets allocated from (depending if its dual TLH mode or not), and whether we clear it or not */
	MMINLINE bool getNonZeroTLHFlag() { return (_allocateFlags & OMR_GC_ALLOCATE_OBJECT_NON_ZERO_TLH) == OMR_GC_ALLOCATE_OBJECT_NON_ZERO_TLH; }
	
	MMINLINE uintptr_t getAllocateFlags() { return _allocateFlags; }

	MMINLINE void setObjectFlags(uint32_t objectFlags) { _objectFlags = objectFlags; }


	void setSpineBytes(uintptr_t sb) { _spineBytes = sb; }
	uintptr_t getNumArraylets() { return _numArraylets; }
	void setNumArraylets(uintptr_t na) { _numArraylets = na; }
	bool isChunkedArray() { return _chunkedArray; }
	void setChunkedArray(bool ca) { _chunkedArray = ca; }
	omrarrayptr_t getSpine() { return _spine; }
	void setSpine(omrarrayptr_t spine) { _spine = spine; }
	MMINLINE bool isArrayletSpine() { return (0 != _spineBytes); }
	/**
	 * Contiguous bytes will only differ from bytes requested if this allocation is for an arraylet spine
	 */
	MMINLINE uintptr_t getContiguousBytes() { return (0 == _spineBytes) ? _bytesRequested : _spineBytes; }

	MMINLINE void setAllocationType(MM_MemorySubSpace::AllocationType allocationType) { _allocationType = allocationType; }
	MMINLINE MM_MemorySubSpace::AllocationType getAllocationType() { return _allocationType; }

	/**
	 * Total bytes requested for this allocation
	 * Note: this will include leaves if this is a spine allocation.
	 */
	MMINLINE uintptr_t getBytesRequested() { return _bytesRequested; }
	MMINLINE MM_MemorySpace *getMemorySpace() { return _memorySpace; }
	MMINLINE MM_MemorySubSpace *getMemorySubSpace() { return _memorySubSpace; }
	
	MMINLINE void setBytesRequested(uintptr_t bytesRequested) { _bytesRequested = bytesRequested; }
	MMINLINE void setMemorySpace(MM_MemorySpace *memorySpace) { _memorySpace = memorySpace; }
	MMINLINE void setMemorySubSpace(MM_MemorySubSpace *memorySubSpace) { _memorySubSpace = memorySubSpace; }

	MMINLINE void setAllocationTaxSize(uintptr_t size)	{ _allocationTaxSize = size; }
	MMINLINE uintptr_t getAllocationTaxSize() 			{ return _allocationTaxSize; }
	MMINLINE void payAllocationTax(MM_EnvironmentBase *env) { _memorySubSpace->payAllocationTax(env, this); }

	MMINLINE void setTLHAllocation(bool tlhAlloc) 						{ _tlhAllocation = tlhAlloc; }
	MMINLINE bool isTLHAllocation()										{ return _tlhAllocation; }
	
	MMINLINE void setNurseryAllocation(bool nurseryAlloc)				{ _nurseryAllocation = nurseryAlloc; }
	MMINLINE bool isNurseryAllocation()									{ return _nurseryAllocation; }
	
	MMINLINE void setLOAAllocation(bool loaAlloc)						{ _loaAllocation = loaAlloc; }
	MMINLINE bool isLOAAllocation()										{ return _loaAllocation; }
	
	MMINLINE void setThreadIsAtSafePoint(bool safe)						{_threadAtSafePoint = safe; }
	MMINLINE bool isThreadAtSafePoint()									{ return _threadAtSafePoint; }
	
	MMINLINE void setMemoryPool(MM_MemoryPool *memoryPool)				{_memoryPool = memoryPool; }
	MMINLINE MM_MemoryPool *getMemoryPool()								{ return _memoryPool; }
	
	MMINLINE void setCollectorAllocateExpandOnFailure(bool expand) { _collectorAllocateExpandOnFailure = expand; }
	MMINLINE bool isCollectorAllocateExpandOnFailure() { return _collectorAllocateExpandOnFailure; }
	
	MMINLINE void setCollectorAllocateSatisfyAnywhere(bool anywhere) { _collectorAllocateSatisfyAnywhere = anywhere; }
	MMINLINE bool isCollectorAllocateSatisfyAnywhere() { return _collectorAllocateSatisfyAnywhere; }

	MMINLINE bool shouldCollectAndClimb() { return _collectAndClimb; }

	MMINLINE bool shouldClimb() { return _climb; }
	MMINLINE bool setClimb() { return _climb = true; }

	MMINLINE bool isCompletedFromTlh() { return _completedFromTlh; }
	MMINLINE void completedFromTlh() { _completedFromTlh = true; }

	/**
	 * Set whether the allocation succeeded
	 * @param suceeded - true if the allocation succeeded, false otherwise
	 */
	MMINLINE void setAllocationSucceeded(bool succeeded) {_allocationSucceeded = succeeded;}
	/**
	 * Get whether the allocation succeeded
	 * @return true if the allocation succeeded, false otherwise
	 */
	MMINLINE bool getAllocationSucceeded() {return _allocationSucceeded;}


	/**
	 * Save the spine to this thread's "saved object" slot so that it will be used as a root and will be updated if the spine moves.
	 * NOTE:  This call must be balanced by a following restoreObjects call
	 */
	virtual void saveObjects(MM_EnvironmentBase* env);

	/**
	 * Restore the spine from this thread's "saved object" slot where it was stored for safe marking and update.
	 * NOTE:  This call must be balanced by a preceding saveObjects call
	 */
	virtual void restoreObjects(MM_EnvironmentBase* env);

	/**
	 * Create an AllocateDescriptionCore object.
	 */
	MM_AllocateDescription(uintptr_t bytesRequested, uintptr_t allocateFlags, bool collectAndClimb, bool threadAtSafePoint) :
		MM_BaseVirtual()
		, _bytesRequested(bytesRequested)
		, _allocateFlags(allocateFlags)
		, _allocationSucceeded(false)
		,_memorySpace(NULL)
		,_memorySubSpace(NULL)
		,_allocationTaxSize(0)
		,_tlhAllocation(false)
		,_nurseryAllocation(false)
		,_loaAllocation(false)
		,_spineBytes(0)
		,_numArraylets(0)
		,_chunkedArray(false)
		,_spine(NULL)
		,_threadAtSafePoint(threadAtSafePoint)
		,_memoryPool(NULL)
		,_collectorAllocateExpandOnFailure(false)
		,_collectorAllocateSatisfyAnywhere(false)
		, _allocationType(MM_MemorySubSpace::ALLOCATION_TYPE_INVALID)
		, _collectAndClimb(collectAndClimb)
		, _climb(false)
		, _completedFromTlh(false)
	{
		_typeId = __FUNCTION__;
	};
};

#endif /* ALLOCATEDESCRIPTION_HPP_ */
