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
 *    Multiple authors (IBM Corp.) - initial API and implementation and/or initial documentation
 *******************************************************************************/

#ifndef omrportptb_h
#define omrportptb_h

/*
 * @ddr_namespace: map_to_type=J9PortptbConstants
 */

/**
 * @file
 * @ingroup Port
 * @brief Per Thread Buffers
 *
 * Per thread buffers are used to store information that is not sharable among the threads.
 * For example when an OS system call fails the error code associated with that error is
 * relevant to the thread that called the OS function; it has no meaning to any other thread.
 *
 * This file contains the structure of the per thread buffers.  @see omrtlshelpers.c for details on accessing
 * these buffers..
 */

#include "omrport.h"

#define J9ERROR_DEFAULT_BUFFER_SIZE 256 /**< default customized error message size if we need to create one */

/**
 * @typedef
 * @brief The per thread buffer
 * Storage for data related to the threads state.
 */
typedef struct PortlibPTBuffers_struct {
	struct PortlibPTBuffers_struct *next; /**< Next per thread buffer */
	struct PortlibPTBuffers_struct *previous; /**< Previous per thread buffer */

	int32_t platformErrorCode; /**< error code as reported by the OS */
	int32_t portableErrorCode; /**< error code translated to portable format by application */
	char *errorMessageBuffer; /**< last saved error message, either customized or from OS */
	uint32_t errorMessageBufferSize; /**< error message buffer size */

	int32_t reportedErrorCode; /**< last reported error code */
	char *reportedMessageBuffer; /**< last reported error message, either customized or from OS */
	uint32_t reportedMessageBufferSize; /**< reported message buffer size */
} PortlibPTBuffers_struct;

/**
 * @typedef
 * @brief The per thread buffer
 * Storage for data related to the threads state.
 */
typedef struct PortlibPTBuffers_struct *PortlibPTBuffers_t;

void omrport_free_ptBuffer(struct OMRPortLibrary *portLibrary, PortlibPTBuffers_t ptBuffer);
int32_t port_convertToUTF8(OMRPortLibrary *portLibrary, const wchar_t *unicodeString, char *utf8Buffer, uintptr_t size);
wchar_t *port_convertFromUTF8(OMRPortLibrary *portLibrary, const char *string, wchar_t *unicodeBuffer, uintptr_t unicodeBufferSize);

#endif     /* omrportptb_h */


