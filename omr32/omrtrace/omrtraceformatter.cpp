/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 1998, 2016
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

#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "omrtraceformat.h"
#include "omrtrace_internal.h"

#define ONEMILLION (1000000)

struct UtTracePointIterator {
	OMR_TraceBuffer *buffer;
	int32_t recordLength;
	uint64_t end;
	uint64_t start;
	uint32_t dataLength;
	uint32_t currentPos;
	uint64_t startPlatform;
	uint64_t startSystem;
	uint64_t endPlatform;
	uint64_t endSystem;
	uint64_t timeConversion;
	uint64_t currentUpperTimeWord;
	int32_t isBigEndian;
	int32_t isCircularBuffer;
	int32_t iteratorHasWrapped;
	char *tempBuffForWrappedTP;
	int32_t processingIncompleteDueToPartialTracePoint;
	uint32_t longTracePointLength;
	uint32_t numberOfBytesInPlatformUDATA;
	uint32_t numberOfBytesInPlatformPtr;
	uint32_t numberOfBytesInPlatformShort;
	OMRPortLibrary *portLib;
	FormatStringCallback getFormatStringFn;
};

struct UtTraceFileIterator {
	UtTraceFileHdr *header;
	UtTraceSection *traceSection;
	UtServiceSection *serviceSection;
	UtStartupSection *startupSection;
	UtActiveSection *activeSection;
	UtProcSection *procSection;
	FormatStringCallback getFormatStringFn;
	OMRPortLibrary *portLib;
	intptr_t traceFileHandle;
	intptr_t currentPosition;
};

omr_error_t
omr_trc_getTraceFileIterator(OMRPortLibrary *portLib, char *fileName, UtTraceFileIterator **iteratorPtr,
							 FormatStringCallback getFormatStringFn)
{
	OMRPORT_ACCESS_FROM_OMRPORT(portLib);
	UtTraceFileIterator *iterator = NULL;
	intptr_t traceFileHandle = -1;
	intptr_t bytesRead = -1;
	UtTraceFileHdr dummyHeader;
	UtTraceFileHdr *header = NULL;

	/* Open the trace file and copy out the header. */
	traceFileHandle = omrfile_open(fileName, EsOpenRead, 0);

	if (traceFileHandle < 0) {
		return OMR_ERROR_FILE_UNAVAILABLE;
	}

	bytesRead = omrfile_read(traceFileHandle, &dummyHeader, sizeof(UtTraceFileHdr));

	if (bytesRead != sizeof(UtTraceFileHdr)) {
		omrmem_free_memory(header);
		return OMR_ERROR_INTERNAL;
	}

	/* Check for a valid looking header. */
	if (dummyHeader.endianSignature != UT_ENDIAN_SIGNATURE) {
		/* TODO - If this is the wrong endianess we'll either need to fix up the header
		 * and change the formatter to cope with files from other platforms OR document
		 * that the native formatter can't cope with that.
		 */
		return OMR_ERROR_ILLEGAL_ARGUMENT;
	}

	/* Now we know how big the header really is, reset the file and read it again. */
	header = (UtTraceFileHdr *)omrmem_allocate_memory(dummyHeader.header.length, OMRMEM_CATEGORY_TRACE);

	if (NULL == header) {
		return OMR_ERROR_OUT_OF_NATIVE_MEMORY;
	}

	omrfile_seek(traceFileHandle, 0, EsSeekSet);
	bytesRead = omrfile_read(traceFileHandle, header, dummyHeader.header.length);

	if (bytesRead != dummyHeader.header.length) {
		omrmem_free_memory(header);
		return OMR_ERROR_INTERNAL;
	}

	/* Check for a valid looking header. */
	if (header->endianSignature != UT_ENDIAN_SIGNATURE) {
		/* TODO - If this is the wrong endianess we'll either need to fix up the header
		 * and change the formatter to cope with files from other platforms OR document
		 * that the native formatter can't cope with that.
		 */
		omrmem_free_memory(header);
		return OMR_ERROR_ILLEGAL_ARGUMENT;
	}

	iterator = (UtTraceFileIterator *)omrmem_allocate_memory(sizeof(UtTraceFileIterator), OMRMEM_CATEGORY_TRACE);

	if (NULL == iterator) {
		omrmem_free_memory(header);
		return OMR_ERROR_OUT_OF_NATIVE_MEMORY;
	}

	iterator->header = header;
	/* Fix up pointers to the initialisation data. */
	iterator->traceSection = ((UtTraceSection *)((char *)header + header->traceStart));
	iterator->serviceSection = ((UtServiceSection *)((char *)header + header->serviceStart));
	iterator->startupSection = ((UtStartupSection *)((char *)header + header->startupStart));
	iterator->activeSection = ((UtActiveSection *)((char *)header + header->activeStart));
	iterator->procSection = ((UtProcSection *)((char *)header + header->processorStart));
	iterator->getFormatStringFn = getFormatStringFn;
	iterator->currentPosition = bytesRead;
	iterator->portLib = OMRPORTLIB;
	iterator->traceFileHandle = traceFileHandle;

	*iteratorPtr = iterator;

	return OMR_ERROR_NONE;

}

/**
 * This frees a trace file iterator and closes the associated trace file.
 * Any UtTracePointIterators returned by this UtTraceFileIterator must be
 * freed first.
 */
omr_error_t
omr_trc_freeTraceFileIterator(UtTraceFileIterator *iter)
{
	if (NULL != iter) {
		OMRPORT_ACCESS_FROM_OMRPORT(iter->portLib);
		omrfile_close(iter->traceFileHandle);
		if (NULL != iter->header) {
			omrmem_free_memory(iter->header);
		}
		omrmem_free_memory(iter);
	}
	return OMR_ERROR_NONE;
}

/**
 * This returns a structure for iterating over a trace buffer for
 * use with omr_trc_formatNextTracePoint.
 *
 * Important: Unlike trcGetTracePointIteratorForBuffer this does not hold the global trace lock until
 * trcFreeTracePointIterator is called as we assume we are working on data from a file that trace is
 * not currently accessing.
 */
omr_error_t
omr_trc_getTracePointIteratorForNextBuffer(UtTraceFileIterator *fileIterator, UtTracePointIterator **bufferIteratorPtr)
{
	UtTracePointIterator *iterator = NULL;
	intptr_t bytesRead = -1;
	uint64_t spanPlatform, spanSystem;

	OMRPORT_ACCESS_FROM_OMRPORT(fileIterator->portLib);

	iterator = (UtTracePointIterator *)omrmem_allocate_memory(sizeof(UtTracePointIterator), OMRMEM_CATEGORY_TRACE);
	if (iterator == NULL) {
		UT_DBGOUT_CHECKED(1, ("<UT> trcGetTracePointIteratorForBuffer cannot allocate iterator\n"));
		*bufferIteratorPtr = NULL;
		return OMR_ERROR_OUT_OF_NATIVE_MEMORY;
	}

	iterator->buffer = (OMR_TraceBuffer *)omrmem_allocate_memory(
			fileIterator->header->bufferSize + offsetof(OMR_TraceBuffer, record), OMRMEM_CATEGORY_TRACE);
	if (iterator->buffer == NULL) {
		UT_DBGOUT_CHECKED(1, ("<UT> trcGetTracePointIteratorForBuffer cannot allocate iterator's buffer\n"));
		omrmem_free_memory(iterator);
		*bufferIteratorPtr = NULL;
		return OMR_ERROR_OUT_OF_NATIVE_MEMORY;
	}

	/* set up the iterator */
	bytesRead = omrfile_read(fileIterator->traceFileHandle, &iterator->buffer->record, fileIterator->header->bufferSize);
	if (fileIterator->header->bufferSize != bytesRead) {
		omrmem_free_memory(iterator->buffer);
		omrmem_free_memory(iterator);
		*bufferIteratorPtr = NULL;
		if (-1 == bytesRead) {
			/* End of file, not an error! */
			return OMR_ERROR_NONE;
		} else {
			/* Unexpectedly reached the end of the file. */
			return OMR_ERROR_INTERNAL;
		}
	}

	iterator->recordLength = fileIterator->header->bufferSize;
	iterator->end = iterator->buffer->record.nextEntry;
	iterator->start = iterator->buffer->record.firstEntry;
	iterator->dataLength = iterator->buffer->record.nextEntry - iterator->buffer->record.firstEntry;
	iterator->currentUpperTimeWord = (uint64_t)(iterator->buffer->record.sequence) & J9CONST64(0xFFFFFFFF00000000);
	iterator->currentPos = iterator->buffer->record.nextEntry;
	iterator->startPlatform = fileIterator->traceSection->startPlatform;
	iterator->startSystem = fileIterator->traceSection->startSystem;
	iterator->endPlatform = omrtime_hires_clock(); /* TODO - Is there a better timestamp we can use here? */
	iterator->endSystem = ((uint64_t) omrtime_current_time_millis()); /* TODO - Is there a better timestamp we can use here? */
	iterator->portLib = fileIterator->portLib;
	iterator->getFormatStringFn = fileIterator->getFormatStringFn;

	spanPlatform = iterator->endPlatform - iterator->startPlatform;
	spanSystem = iterator->endSystem - iterator->startSystem;

	iterator->timeConversion = spanPlatform / spanSystem;
	if (iterator->timeConversion == 0) {
		/* this will be used as the divisor in formatting time stamps */
		iterator->timeConversion = 1;
	}

#ifdef OMR_ENV_LITTLE_ENDIAN
	iterator->isBigEndian = FALSE;
#else
	iterator->isBigEndian = TRUE;
#endif
	iterator->isCircularBuffer = TRUE;
	iterator->iteratorHasWrapped = FALSE;
	iterator->processingIncompleteDueToPartialTracePoint = FALSE;
	iterator->longTracePointLength = 0;

	iterator->numberOfBytesInPlatformUDATA = (uint32_t)sizeof(uintptr_t);
	iterator->numberOfBytesInPlatformPtr = (uint32_t)sizeof(char *);
	iterator->numberOfBytesInPlatformShort = (uint32_t)sizeof(short);

	UT_DBGOUT_CHECKED(4,
			("<UT> firstEntry: %d, offset of record: %ld buffer size: %d endianness %s\n", iterator->start, offsetof(OMR_TraceBuffer, record), fileIterator->header->bufferSize, (iterator->isBigEndian)?"bigEndian":"littleEndian"));
	UT_DBGOUT_CHECKED(2,
			("<UT> omr_trc_getTracePointIteratorForNextBuffer: Thread %s returning iterator %p\n", iterator->buffer->record.threadName, iterator));

	*bufferIteratorPtr = iterator;
	return OMR_ERROR_NONE;

}

uint64_t
omr_trc_getBufferIteratorThreadId(UtTracePointIterator *iter)
{
	return iter->buffer->record.threadId;
}

uint32_t
omr_trc_getBufferIteratorThreadName(UtTracePointIterator *iter, char *buffer, uint32_t buffLen)
{
	memset(buffer, 0, buffLen);
	strncpy(buffer, iter->buffer->record.threadName, buffLen - 1);
	return (uint32_t)strlen(buffer);
}

omr_error_t
omr_trc_freeTracePointIterator(UtTracePointIterator *iter)
{
	if (iter != NULL) {
		OMRPORT_ACCESS_FROM_OMRPORT(iter->portLib);
		omrmem_free_memory(iter->buffer);
		UT_DBGOUT_CHECKED(2, ("<UT> trcFreeTracePointIterator freeing iterator %p\n", iter));
		omrmem_free_memory(iter);
	}
	return OMR_ERROR_NONE;
}

static unsigned char
getUnsignedByteFromBuffer(UtTraceRecord *record, uint32_t offset)
{
	char *tempPtr = (char *)record;
	return tempPtr[offset];
}

static uint32_t
getTraceIdFromBuffer(UtTraceRecord *record, uint32_t offset)
{
	uint32_t ret = 0;
	unsigned char data[3];

	data[0] = getUnsignedByteFromBuffer(record, offset);
	data[1] = getUnsignedByteFromBuffer(record, offset + 1);
	data[2] = getUnsignedByteFromBuffer(record, offset + 2);

	/* trace ID's are stored in hard coded big endian order */
	ret = (data[0] << 16) | (data[1] << 8) | (data[2]);

	/* remove comp Id's */
	ret &= 0x3FFF;
	return ret;
}

static uint16_t
getU_16FromBuffer(UtTraceRecord *record, uint32_t offset, int32_t isBigEndian)
{
	uint16_t ret = 0;
	unsigned char data[2];

	data[0] = getUnsignedByteFromBuffer(record, offset);
	data[1] = getUnsignedByteFromBuffer(record, offset + 1);

	/* integer values are written out in platform endianess */
	if (isBigEndian) {
		ret = (data[0] << 8) | (data[1]);
	} else {
		ret = (data[1] << 8) | (data[0]);
	}
	return ret;
}

static uint32_t
getU_32FromBuffer(UtTraceRecord *record, uint32_t offset, int32_t isBigEndian)
{
	uint32_t ret = 0;
	unsigned char data[4];

	data[0] = getUnsignedByteFromBuffer(record, offset);
	data[1] = getUnsignedByteFromBuffer(record, offset + 1);
	data[2] = getUnsignedByteFromBuffer(record, offset + 2);
	data[3] = getUnsignedByteFromBuffer(record, offset + 3);

	/* integer values are written out in platform endianess */
	if (isBigEndian) {
		ret = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
	} else {
		ret = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0]);
	}
	return ret;
}

static uint64_t
getU_64FromBuffer(UtTraceRecord *record, uint32_t offset, int32_t isBigEndian)
{
	uint64_t ret = 0;
	unsigned char data[8];

	data[0] = getUnsignedByteFromBuffer(record, offset);
	data[1] = getUnsignedByteFromBuffer(record, offset + 1);
	data[2] = getUnsignedByteFromBuffer(record, offset + 2);
	data[3] = getUnsignedByteFromBuffer(record, offset + 3);
	data[4] = getUnsignedByteFromBuffer(record, offset + 4);
	data[5] = getUnsignedByteFromBuffer(record, offset + 5);
	data[6] = getUnsignedByteFromBuffer(record, offset + 6);
	data[7] = getUnsignedByteFromBuffer(record, offset + 7);

	/* integer values are written out in platform endianess */
	if (isBigEndian) {
		ret = ((uint64_t)data[0] << 56) | ((uint64_t)data[1] << 48) | ((uint64_t)data[2] << 40) | ((uint64_t)data[3] << 32)
			  | ((uint64_t)data[4] << 24) | ((uint64_t)data[5] << 16) | ((uint64_t)data[6] << 8) | ((uint64_t)data[7]);
	} else {
		ret = ((uint64_t)data[7] << 56) | ((uint64_t)data[6] << 48) | ((uint64_t)data[5] << 40) | ((uint64_t)data[4] << 32)
			  | ((uint64_t)data[3] << 24) | ((uint64_t)data[2] << 16) | ((uint64_t)data[1] << 8) | ((uint64_t)data[0]);
	}
	return ret;
}

#define UT_TRACE_FORMATTER_64BIT_DATA		64
#define UT_TRACE_FORMATTER_32BIT_DATA		32
#define UT_TRACE_FORMATTER_8BIT_DATA		8
#define UT_TRACE_FORMATTER_STRING_DATA	-1

static uint32_t
readConsumeAndSPrintfParameter(OMRPortLibrary *portLib, char *rawParameterData, uint32_t rawParameterDataLength,
							   uint32_t *offsetInParameterData, char *destBuffer, uint32_t destBufferLength, uint32_t *offsetInDestBuffer,
							   const char *format, int32_t dataType, uint32_t nWidthAndPrecisions, int32_t isBigEndian)
{
	uintptr_t temp = 0;
	uint64_t u64data = 0;
	uint32_t u32data = 0;
	int16_t i16data = 0;
	unsigned char u8data = 0;
	char *strValue = NULL;
	uint32_t precisionOrWidth1 = 0;
	uint32_t precisionOrWidth2 = 0;
	OMRPORT_ACCESS_FROM_OMRPORT(portLib);

	if (rawParameterData == NULL || rawParameterDataLength == 0) {
		/* this must be minimal tracing */
		if ((destBufferLength - (*offsetInDestBuffer)) < 3) {
			/* error - just return */
			return 0;
		} else {
			/* give an indication that we haven't filled in the format on purpose */
			destBuffer[*offsetInDestBuffer] = '?';
			destBuffer[*offsetInDestBuffer + 1] = '?';
			destBuffer[*offsetInDestBuffer + 2] = '?';
			*offsetInDestBuffer += 3;
			return 3;
		}
	}

	if (nWidthAndPrecisions == 1) {
		precisionOrWidth1 = getU_32FromBuffer((UtTraceRecord *)rawParameterData, *offsetInParameterData, isBigEndian);
		*offsetInParameterData += 4;
	} else if (nWidthAndPrecisions == 2) {
		precisionOrWidth1 = getU_32FromBuffer((UtTraceRecord *)rawParameterData, *offsetInParameterData, isBigEndian);
		precisionOrWidth2 = getU_32FromBuffer((UtTraceRecord *)rawParameterData, *offsetInParameterData, isBigEndian);
		*offsetInParameterData += 8;
	}

	if (dataType == UT_TRACE_FORMATTER_64BIT_DATA) {
		/* handling a generic 64 bit integer */
		u64data = getU_64FromBuffer((UtTraceRecord *)rawParameterData, *offsetInParameterData, isBigEndian);
		if (nWidthAndPrecisions == 2) {
			temp = omrstr_printf(NULL, 0, format, precisionOrWidth1, precisionOrWidth2, u64data);
			if (temp > destBufferLength) {
				/* can't write data to dest buffer - it's too full, be cautious and just return */
				UT_DBGOUT_CHECKED(1,
						("<UT> readConsumeAndSPrintfParameter destination buffer exhausted: %d [%s]\n", dataType, format));
				return 0;
			}
			temp = omrstr_printf(destBuffer + (*offsetInDestBuffer), destBufferLength - (*offsetInDestBuffer),
								 format, precisionOrWidth1, precisionOrWidth2, u64data);
		} else if (nWidthAndPrecisions == 1) {
			temp = omrstr_printf(NULL, 0, format, precisionOrWidth1, u64data);
			if (temp > destBufferLength) {
				/* can't write data to dest buffer - it's too full, be cautious and just return */
				UT_DBGOUT_CHECKED(1,
						("<UT> readConsumeAndSPrintfParameter destination buffer exhausted: %d [%s]\n", dataType, format));
				return 0;
			}
			temp = omrstr_printf(destBuffer + (*offsetInDestBuffer), destBufferLength - (*offsetInDestBuffer),
								 format, precisionOrWidth1, u64data);
		} else {
			temp = omrstr_printf(NULL, 0, format, u64data);
			if (temp > destBufferLength) {
				/* can't write data to dest buffer - it's too full, be cautious and just return */
				UT_DBGOUT_CHECKED(1,
						("<UT> readConsumeAndSPrintfParameter destination buffer exhausted: %d [%s]\n", dataType, format));
				return 0;
			}
			temp = omrstr_printf(destBuffer + (*offsetInDestBuffer), destBufferLength - (*offsetInDestBuffer),
								 format, u64data);
		}
		*offsetInParameterData += 8;
	} else if (dataType == UT_TRACE_FORMATTER_32BIT_DATA) {
		/* handling a generic 32 bit integer */
		u32data = getU_32FromBuffer((UtTraceRecord *)rawParameterData, *offsetInParameterData, isBigEndian);
		if (nWidthAndPrecisions == 2) {
			temp = omrstr_printf(NULL, 0, format, precisionOrWidth1, precisionOrWidth2, u32data);
			if (temp > destBufferLength) {
				/* can't write data to dest buffer - it's too full, be cautious and just return */
				UT_DBGOUT_CHECKED(1,
						("<UT> readConsumeAndSPrintfParameter destination buffer exhausted: %d [%s]\n", dataType, format));
				return 0;
			}
			temp = omrstr_printf(destBuffer + (*offsetInDestBuffer), destBufferLength - (*offsetInDestBuffer),
								 format, precisionOrWidth1, precisionOrWidth2, u32data);
		} else if (nWidthAndPrecisions == 1) {
			temp = omrstr_printf(NULL, 0, format, precisionOrWidth1, u32data);
			if (temp > destBufferLength) {
				/* can't write data to dest buffer - it's too full, be cautious and just return */
				UT_DBGOUT_CHECKED(1,
						("<UT> readConsumeAndSPrintfParameter destination buffer exhausted: %d [%s]\n", dataType, format));
				return 0;
			}
			temp = omrstr_printf(destBuffer + (*offsetInDestBuffer), destBufferLength - (*offsetInDestBuffer),
								 format, precisionOrWidth1, u32data);
		} else {
			temp = omrstr_printf(NULL, 0, format, u32data);
			if (temp > destBufferLength) {
				/* can't write data to dest buffer - it's too full, be cautious and just return */
				UT_DBGOUT_CHECKED(1,
						("<UT> readConsumeAndSPrintfParameter destination buffer exhausted: %d [%s]\n", dataType, format));
				return 0;
			}
			temp = omrstr_printf(destBuffer + (*offsetInDestBuffer), destBufferLength - (*offsetInDestBuffer),
								 format, u32data);
		}
		*offsetInParameterData += 4;
	} else if (dataType == UT_TRACE_FORMATTER_8BIT_DATA) {
		/* handling a char */
		u8data = getUnsignedByteFromBuffer((UtTraceRecord *)rawParameterData, *offsetInParameterData);
		temp = omrstr_printf(NULL, 0, format, u8data);
		if (temp > destBufferLength) {
			/* can't write data to dest buffer - it's too full, be cautious and just return */
			UT_DBGOUT_CHECKED(1,
					("<UT> readConsumeAndSPrintfParameter destination buffer exhausted: %d [%s]\n", dataType, format));
			return 0;
		}
		temp = omrstr_printf(destBuffer + (*offsetInDestBuffer), destBufferLength - (*offsetInDestBuffer),
							 format, u8data);
		*offsetInParameterData += 1;
	} else if (dataType == UT_TRACE_FORMATTER_STRING_DATA) {
		/* handling a string */
		/* any length already read is a false one - the trace engine will have modified the utf8 length field */
		if (nWidthAndPrecisions == 1) {
			*offsetInParameterData -= 4;
			i16data = (int16_t)getU_16FromBuffer((UtTraceRecord *)rawParameterData, *offsetInParameterData, isBigEndian);
			*offsetInParameterData += 2;
			strValue = rawParameterData + (*offsetInParameterData);
			temp = omrstr_printf(NULL, 0, format, i16data, strValue);
			if (temp > destBufferLength) {
				/* can't write data to dest buffer - it's too full, be cautious and just return */
				UT_DBGOUT_CHECKED(1,
						("<UT> readConsumeAndSPrintfParameter destination buffer exhausted: %d [%s]\n", dataType, format));
				return 0;
			}
			temp = omrstr_printf(destBuffer + (*offsetInDestBuffer), destBufferLength - (*offsetInDestBuffer),
								 format, i16data, strValue);
		} else {
			/* it's just a plain string */
			strValue = rawParameterData + (*offsetInParameterData);
			temp = omrstr_printf(NULL, 0, format, strValue);
			if (temp > destBufferLength) {
				/* can't write data to dest buffer - it's too full, be cautious and just return */
				UT_DBGOUT_CHECKED(1,
						("<UT> readConsumeAndSPrintfParameter destination buffer exhausted: %d [%s]\n", dataType, format));
				return 0;
			}
			temp = omrstr_printf(destBuffer + (*offsetInDestBuffer), destBufferLength - (*offsetInDestBuffer),
								 format, strValue);
			/* for the null in the parm data */
			*offsetInParameterData += 1;
		}
		*offsetInParameterData += (uint32_t)temp;
	} else {
		UT_DBGOUT_CHECKED(1, ("<UT> bad byte width in readConsumeAndSPrintfParameter: %d [%s]\n", dataType, format));
	}

	*offsetInDestBuffer += (uint32_t)temp;
	return (uint32_t)temp;
}

static uint32_t
formatTracePointParameters(UtTracePointIterator *iter, char *destBuffer, uint32_t destBufferLength, const char *format,
						   char *rawParameterData, uint32_t rawParameterDataLength)
{
	uint32_t formatLength;
	uint32_t offsetInFormat = 0, offsetInDestBuffer = 0, offsetInParameterData = 0;
	int numberOfStars = 0;
	char type = '\0';
	int32_t longModifierFound = FALSE;
	int32_t platformUDATAWidthDataFound = FALSE;
	char formatBuffer[20];
	uint32_t offsetInFormatBuffer = 0;
	uint32_t offsetOfType = 0;
	int traceDataType = 0;

	if (destBuffer == NULL || destBufferLength == 0) {
		UT_DBGOUT_CHECKED(1,
				("<UT> formatTracePointParameters called with destination buffer %p and destination buffer length %u\n", destBuffer, destBufferLength));
		return 0;
	}

	if (format == NULL) {
		UT_DBGOUT_CHECKED(1, ("<UT> formatTracePointParameters called with no format string\n"));
		return 0;
	}
	formatLength = (uint32_t)strlen(format);

	/* walk the format string copying it char by char into the destination buffer */
	for (offsetInFormat = 0, offsetInDestBuffer = 0; offsetInFormat <= formatLength;
		 offsetInFormat++, offsetInDestBuffer++) {
		if (format[offsetInFormat] == '%') {
			if (format[offsetInFormat + 1] == '%') {
				destBuffer[offsetInDestBuffer] = format[offsetInFormat];
				offsetInFormat++; /* skip the percent, just write a single percent to the destination buffer */
			} else {
				int32_t foundType = FALSE;
				offsetInFormatBuffer = 0;
				platformUDATAWidthDataFound = longModifierFound = FALSE;
				numberOfStars = 0;
				traceDataType = 0;

				for (offsetOfType = 0; foundType == FALSE; offsetOfType++) {
					/* have we read off the end? */
					if (format[offsetInFormat + offsetOfType] == 'x' || format[offsetInFormat + offsetOfType] == 'X'
						|| format[offsetInFormat + offsetOfType] == 'u'
						|| format[offsetInFormat + offsetOfType] == 'i'
						|| format[offsetInFormat + offsetOfType] == 'd'
						|| format[offsetInFormat + offsetOfType] == 'f'
						|| format[offsetInFormat + offsetOfType] == 'p'
						|| format[offsetInFormat + offsetOfType] == 'P'
						|| format[offsetInFormat + offsetOfType] == 'c'
						|| format[offsetInFormat + offsetOfType] == 's'
					) {
						/* we found the type */
						foundType = TRUE;
						type = format[offsetInFormat + offsetOfType];
					} else if (format[offsetInFormat + offsetOfType] == '*') {
						numberOfStars++;
					} else if (format[offsetInFormat + offsetOfType] == 'l') {
						/* 64bit number - its actually 'll' but we'll give it benefit of doubt */
						longModifierFound = TRUE;
					} else if (format[offsetInFormat + offsetOfType] == 'z') {
						/* platform udata width data */
						platformUDATAWidthDataFound = TRUE;
					}
					/* this is both default behaviour and always needs to happen */
					formatBuffer[offsetInFormatBuffer] = format[offsetInFormat + offsetInFormatBuffer];
					offsetInFormatBuffer++;
				}
				offsetInFormat += (offsetOfType - 1);
				formatBuffer[offsetInFormatBuffer] = '\0';
				/*increment offsetInFormatBuffer here if it needs to be used further*/

				switch (type) {
				case 'x':
				case 'X':
				case 'u':
				case 'i':
				case 'd':
					if (longModifierFound == TRUE) {
						traceDataType = UT_TRACE_FORMATTER_64BIT_DATA;
					} else if (platformUDATAWidthDataFound == TRUE) {
						if (iter->numberOfBytesInPlatformUDATA == 8) {
							traceDataType = UT_TRACE_FORMATTER_64BIT_DATA;
						} else {
							traceDataType = UT_TRACE_FORMATTER_32BIT_DATA;
						}
					} else {
						/* normal integer */
						traceDataType = UT_TRACE_FORMATTER_32BIT_DATA;
					}
					readConsumeAndSPrintfParameter(iter->portLib, rawParameterData, rawParameterDataLength,
												   &offsetInParameterData, destBuffer, destBufferLength, &offsetInDestBuffer,
												   (const char *)formatBuffer, traceDataType, numberOfStars, iter->isBigEndian);
					break;
				case 'p':
				case 'P':
					if (iter->numberOfBytesInPlatformPtr == 8) {
						traceDataType = UT_TRACE_FORMATTER_64BIT_DATA;
					} else {
						traceDataType = UT_TRACE_FORMATTER_32BIT_DATA;
					}

					readConsumeAndSPrintfParameter(iter->portLib, rawParameterData, rawParameterDataLength,
												   &offsetInParameterData, destBuffer, destBufferLength, &offsetInDestBuffer,
												   (const char *)formatBuffer, traceDataType, numberOfStars, iter->isBigEndian);
					break;
				case 'c':
					traceDataType = UT_TRACE_FORMATTER_8BIT_DATA;
					readConsumeAndSPrintfParameter(iter->portLib, rawParameterData, rawParameterDataLength,
												   &offsetInParameterData, destBuffer, destBufferLength, &offsetInDestBuffer,
												   (const char *)formatBuffer, traceDataType, numberOfStars, iter->isBigEndian);
					break;
				case 's':
					traceDataType = UT_TRACE_FORMATTER_STRING_DATA;
					readConsumeAndSPrintfParameter(iter->portLib, rawParameterData, rawParameterDataLength,
												   &offsetInParameterData, destBuffer, destBufferLength, &offsetInDestBuffer,
												   (const char *)formatBuffer, traceDataType, numberOfStars, iter->isBigEndian);
					break;
				case 'f':
					/* Floats are promoted to doubles by convention, so all %fs are 64bit */
					traceDataType = UT_TRACE_FORMATTER_64BIT_DATA;
					readConsumeAndSPrintfParameter(iter->portLib, rawParameterData, rawParameterDataLength,
												   &offsetInParameterData, destBuffer, destBufferLength, &offsetInDestBuffer,
												   (const char *)formatBuffer, traceDataType, numberOfStars, iter->isBigEndian);
					break;
				default:
					UT_DBGOUT_CHECKED(1,
							("<UT> formatTracePointParameters unknown trace format type: %c [%s]\n", type, formatBuffer));
					break;
				}
				offsetInDestBuffer--;

			}
		} else {
			if (offsetInDestBuffer > destBufferLength) {
				/* return now before writing off the end */
				UT_DBGOUT_CHECKED(1,
							("<UT> formatTracePointParameters truncated output due to buffer exhaustion at format type: %c [%s]\n", type, formatBuffer));
				return offsetInDestBuffer;
			}
			destBuffer[offsetInDestBuffer] = format[offsetInFormat];
		}
	}

	/* the number of characters successfully written to destination buffer */
	return offsetInDestBuffer;
}

#undef UT_TRACE_FORMATTER_64BIT_DATA
#undef UT_TRACE_FORMATTER_32BIT_DATA
#undef UT_TRACE_FORMATTER_8BIT_DATA
#undef UT_TRACE_FORMATTER_STRING_DATA

static const char *
parseTracePoint(OMRPortLibrary *portLib, UtTraceRecord *record, uint32_t offset, int tpLength,
				uint64_t *timeStampMostSignificantBytes, UtTracePointIterator *iter, char *buffer, uint32_t bufferLength)
{
	uint32_t traceId;
	uint32_t timeStampLeastSignificantBytes;
	uint64_t tempUpper, tempLower;
	uint64_t timeStamp = 0;
	char *modName = NULL;
	const char *modNameString = NULL;
	uint32_t modNameLength;
	char *tempPtr = (char *)record;
	const char *formatString;
	char tempSwapChar = '\0';
	char tempSwapChar2 = '\0';
	char *tempSwapCharLoc;
	uint32_t nanos, millis, seconds, minutes, hours;
	uint64_t splitTime = 0, splitTimeRem = 0;
	uint32_t offsetOfParameters = 0;
	char *rawParameterData;
	uint32_t rawParameterDataLength;
	OMRPORT_ACCESS_FROM_OMRPORT(portLib);

	if (bufferLength == 0 || buffer == NULL) {
		UT_DBGOUT_CHECKED(1, ("<UT> parseTracePoint called with unpopulated iterator formatted tracepoint buffer\n"));
		return NULL;
	}

	if (record == NULL) {
		UT_DBGOUT_CHECKED(1, ("<UT> parseTracePoint called with NULL record\n"));
		return NULL;
	}

	traceId = getTraceIdFromBuffer(record, offset + TRACEPOINT_RAW_DATA_TP_ID_OFFSET);

	/* check for all the control/internal tracepoints */
	/* 0x0010nnnn8 == lost record tracepoint */
	if (traceId == 0x0010) {
		/* 	too much danger of looping to recurse here - return a warning string and let
		 omr_trc_formatNextTracePoint(thr, iter) make the call on whether it can extract
		 any further data */
		return "*** lost records found in trace buffer - use external formatting tools for details.\n";
	}

	/* 0x0000nnnn8 == sequence wrap tracepoint */
	if ((traceId == 0x0000) && (tpLength == UT_TRC_SEQ_WRAP_LENGTH)) {
		/* read in the timer upper word, write it into iterator struct, and (recursively) return the next tracepoint parsed */
		tempUpper = getU_32FromBuffer(record, offset + TRACEPOINT_RAW_DATA_TIMER_WRAP_UPPER_WORD_OFFSET,
									  iter->isBigEndian);
		tempUpper <<= 32;
		*timeStampMostSignificantBytes = tempUpper;
		return omr_trc_formatNextTracePoint(iter, buffer, bufferLength);
	}

	if (tpLength == 4) {
		/* this is a long tracepoint */
		char longTracePointLength;

		longTracePointLength = getUnsignedByteFromBuffer(record, offset + TRACEPOINT_RAW_DATA_TP_ID_OFFSET + 2);
		/* the next call to omr_trc_formatNextTracePoint will pick this up and use it appropriately */
		iter->longTracePointLength = (uint32_t)longTracePointLength;
		return omr_trc_formatNextTracePoint(iter, buffer, bufferLength);
	}

	if (tpLength == UT_TRC_SEQ_WRAP_LENGTH) {
		return omr_trc_formatNextTracePoint(iter, buffer, bufferLength);
	}

	/* otherwise all is well and we have a normal tracepoint */
	timeStampLeastSignificantBytes = getU_32FromBuffer(record, offset + TRACEPOINT_RAW_DATA_TIMESTAMP_OFFSET,
									 iter->isBigEndian);
	modNameLength = getU_32FromBuffer(record, offset + TRACEPOINT_RAW_DATA_MODULE_NAME_LENGTH_OFFSET,
									  iter->isBigEndian);

	/* Module name must not be longer than the tracepoint length - start of module name string */
	/* modNameLength == 0 -- no modname - most likely partially overwritten - its unformattable as a result! */
	if (modNameLength == 0 || tpLength < TRACEPOINT_RAW_DATA_MODULE_NAME_DATA_OFFSET
		|| modNameLength > (uint32_t)(tpLength - TRACEPOINT_RAW_DATA_MODULE_NAME_DATA_OFFSET)) {
		return omr_trc_formatNextTracePoint(iter, buffer, bufferLength);
	}

	modName = &tempPtr[offset + TRACEPOINT_RAW_DATA_MODULE_NAME_DATA_OFFSET]; /* the modName was written straight to buf as (char *) */
	if (!strncmp(modName, "INTERNALTRACECOMPONENT", 22)) {
		if (traceId == UT_TRC_CONTEXT_ID) {
			/* ignore and recurse - its just a thread id sitting in the buffer used for external trace tool */
			return omr_trc_formatNextTracePoint(iter, buffer, bufferLength);
		}
		modNameLength = 2;
		modNameString = "dg";
		formatString = "internal Trace Data Point";
	} else {
		if (traceId <= 256) {
			return omr_trc_formatNextTracePoint(iter, buffer, bufferLength);
		}
		traceId -= 257;
		/* temporarily add NULL terminator to the modName in copy of the buffer for getFormatString
		 * to be able to do look up on it - save the overwritten data and reinstate once finished. Doing
		 * this avoids allocing a temp buffer. We also need to check to see if this is a composite name,
		 * e.g. pool(j9mm). If it is then we need to take the string before the opening brace as the
		 * component name. We don't do this in getFormatString because that's in the fastpath for
		 * -Xtrace:print
		 */
		tempSwapChar = modName[modNameLength];
		modName[modNameLength] = '\0';

		tempSwapCharLoc = strchr(modName, '(');
		if (tempSwapCharLoc != NULL) {
			tempSwapChar2 = *tempSwapCharLoc;
			*tempSwapCharLoc = '\0';
		}
		formatString = iter->getFormatStringFn((const char *)modName, traceId);
		modName[modNameLength] = tempSwapChar;
		if (tempSwapCharLoc != NULL) {
			*tempSwapCharLoc = tempSwapChar2;
		}
		modNameString = modName;
	}

	tempLower = (uint64_t)timeStampLeastSignificantBytes;
	tempUpper = (uint64_t)*timeStampMostSignificantBytes;
	timeStamp = tempUpper | tempLower;

	/* this formula is taken directly from the trace formatter to maintain agreement between representations
	 *	made by this function and those made by the TraceFormat tool. */
	timeStamp -= iter->startPlatform;

	splitTime = timeStamp / iter->timeConversion;
	splitTimeRem = timeStamp % iter->timeConversion;

	getTimestamp(splitTime + iter->startSystem, &hours, &minutes, &seconds, &millis);

	nanos = (uint32_t)((millis * ONEMILLION) + (splitTimeRem * ONEMILLION / iter->timeConversion));

	offsetOfParameters = (uint32_t) omrstr_printf(NULL, 0, "%02u:%02u:%02u:%09u GMT %.*s.%u - ", hours, minutes,
						 seconds, nanos, modNameLength, modNameString, traceId);

	if (offsetOfParameters > bufferLength) {
		UT_DBGOUT_CHECKED(1,
				("<UT> parseTracePoint called with buffer length %d - too small for tracepoint details\n", bufferLength));
		return NULL;
	}

	offsetOfParameters = (uint32_t) omrstr_printf(buffer, bufferLength, "%02u:%02u:%02u:%09u GMT %.*s.%u - ", hours,
						 minutes, seconds, nanos, modNameLength, modNameString, traceId);

	rawParameterData = tempPtr + offset + TRACEPOINT_RAW_DATA_MODULE_NAME_DATA_OFFSET + modNameLength;
	rawParameterDataLength = tpLength - (TRACEPOINT_RAW_DATA_MODULE_NAME_DATA_OFFSET + modNameLength);

	/* the parameters will be formatted as question marks if there is no data to populate them with */
	if (!formatTracePointParameters(iter, buffer + offsetOfParameters, bufferLength - offsetOfParameters, formatString,
									rawParameterData, rawParameterDataLength)) {
		return NULL;
	}

	return buffer;
}

const char *
omr_trc_formatNextTracePoint(UtTracePointIterator *iter, char *buffer, uint32_t bufferLength)
{
	UtTraceRecord *record;
	int32_t recordDataStart;
	int32_t recordDataLength;
	uint32_t tpLength = 0;
	uint32_t offset = 0;

	OMRPORT_ACCESS_FROM_OMRPORT(iter->portLib);

	if (iter == NULL) {
		UT_DBGOUT_CHECKED(1, ("<UT> omr_trc_formatNextTracePoint called with NULL iterator\n"));
		return NULL;
	}

	if (iter->buffer == NULL) {
		UT_DBGOUT_CHECKED(1, ("<UT> omr_trc_formatNextTracePoint called with unpopulated iterator buffer\n"));
		return NULL;
	}

	if (iter->currentPos <= iter->start) {
		/* currentPosition is at or before the start of the data - not possible to continue
		 *	or the normal exit condition - either way NULL is the right thing to return */
		return NULL;
	}

	record = &iter->buffer->record;
	recordDataStart = record->firstEntry;
	recordDataLength = iter->recordLength;
	offset = iter->currentPos;

	tpLength = getUnsignedByteFromBuffer(record, offset);

	if (iter->longTracePointLength != 0) {
		/* the previous tracepoint was a long tracepoint marker, so it
		 * contained the upper word of a two byte length descriptor
		 */
		tpLength |= (iter->longTracePointLength << 8);
		iter->longTracePointLength = 0;
	}

	/* long trace points have length stored in 2bytes, so check we've not overflowed that
	 * limit
	 */
	if (tpLength == 0 || tpLength > UT_MAX_EXTENDED_LENGTH) {
		/* prevent looping in case of bad data */
		return NULL;
	}

	/* if this is a circular buffer, that we have alread moved to the end of, and
	 * the current tracepoint length takes us back before where formatting originally
	 * started in this buffer, then we have found the partially overwritten tracepoint
	 * that means we have got to the end of this buffer. */
	if ((iter->isCircularBuffer) && iter->iteratorHasWrapped && ((offset - tpLength) < iter->end)) {
		return NULL;
	}

	/* Check whether we are about to process a wrapped tracepoint.
	 *
	 * 'offset' points to location of 'tpLength' for the current tracepoint.
	 * 'tpLength' is the length of the tracepoint, including the length byte.
	 * 'offset - tpLength' points to the previous tracepoint's 'tpLength', if it exists.
	 * 'offset - tpLength + 1' points to the start of the current tracepoint's data, which
	 * must be at least 'record->firstEntry' or 'iter->start' if the tracepoint fits in this buffer.
	 */
	if ((tpLength > offset) || ((offset - tpLength + 1) < iter->start)) {
		if (iter->isCircularBuffer) {
			uint32_t remainder = 0;
			const char *returnString;
			char *tempChrPtr;
			/* the tracepoint wraps back into the end of the current buffer */
			iter->iteratorHasWrapped = TRUE;

			remainder = (uint32_t)(tpLength - (offset - iter->start));
			UT_DBGOUT_CHECKED(4,
					("<UT> getNextTracePointForIterator: remainder found at end of buffer: %d tplength = %d tracedata start: %llu end %llu\n", remainder, tpLength, iter->start, iter->end));

			/* set the iterator's position to be before the incomplete data at the end of the buffer */
			iter->currentPos =  iter->recordLength - remainder;

			/* check if the wrapped tracepoint takes us back past the tracepoint data end point (i.e. record.nextEntry) */
			if (iter->currentPos < iter->end) {
				return NULL; /* we are done, discard this partial tracepoint */
			}

			iter->tempBuffForWrappedTP = (char *)omrmem_allocate_memory(tpLength + 2, OMRMEM_CATEGORY_TRACE);
			if (iter->tempBuffForWrappedTP == NULL) {
				UT_DBGOUT_CHECKED(1, ("<UT> omr_trc_formatNextTracePoint: cannot allocate %d bytes\n", tpLength + 2));
				return NULL;
			}

			/* copy the two ends of tracepoints into the temporary buffer
			 * the beginning of the data is at the end of the buffer
			 * end of data is at the start of the buffer */
			tempChrPtr = (char *)record;
			/* get the start of the tracepoint from the end of the buffer */
			memcpy(iter->tempBuffForWrappedTP, tempChrPtr + recordDataLength - remainder, remainder);
			/* get the end of the tracepoint from the start of the buffer */
			memcpy(iter->tempBuffForWrappedTP + remainder, tempChrPtr + recordDataStart, tpLength - remainder);

			/* and parse the temp buffer */
			returnString = parseTracePoint(iter->portLib, (UtTraceRecord *)iter->tempBuffForWrappedTP, 0, tpLength,
										   &iter->currentUpperTimeWord, iter, buffer, bufferLength);

			UT_DBGOUT_CHECKED(1,
					("<UT> getNextTracePointForIterator: recombined a tracepoint into %s\n", returnString ? returnString : "NULL"));
			omrmem_free_memory(iter->tempBuffForWrappedTP);
			iter->tempBuffForWrappedTP = NULL;
			iter->processingIncompleteDueToPartialTracePoint = FALSE;
			return returnString;
		} else {
			/* the tracepoint is in another buffer and some magic is needed elsewhere to process */
			iter->processingIncompleteDueToPartialTracePoint = TRUE;
			return NULL;
		}
	}

	/* parse the relevant section into a tracepoint */
	iter->currentPos -= tpLength;
	return parseTracePoint(iter->portLib, record, offset - tpLength, tpLength, &iter->currentUpperTimeWord, iter,
						   buffer, bufferLength);
}

