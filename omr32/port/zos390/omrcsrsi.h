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
#ifndef OMRCSRSI_H_
#define OMRCSRSI_H_

#include <stdint.h>
#include "omrcomp.h"

typedef struct j9csrsi_t j9csrsi_t;

/** Init a j9csrsi session
 *  @param[out] ret Optional. If supplied, it will be used to return any error
 *  code CSRSI returns
 *  @return NULL in case of an error. Otherwise, an opaque pointer to a valid
 *  j9csrsi session
 */
const j9csrsi_t *j9csrsi_init(int32_t *ret);

/** If j9csrsi_init() fails, some of the error codes may indicate that hardware
 *  info could not be retrieved because this particular system does not satisfy
 *  all the requirements (e.g., CSRSI is not available). This function can be
 *  used in order to test whether this was the case.
 *  @param[in] ret The error code returned by j9csrsi_init().
 *  @return TRUE if ret indicates that j9csrsi_init() failed because this
 *  system does not satisfy the requirements. FALSE otherwise.
 */
BOOLEAN j9csrsi_is_hw_info_available(int32_t ret);

/** Shutdown a j9csrsi session
 *  @param[in] session Pointer to j9csrsi session
 */
void j9csrsi_shutdown(const j9csrsi_t *session);

/** Shutdown a j9csrsi session
 *  @param[in] session Pointer to j9csrsi session
 *  @return TRUE if system is running on an LPAR. FALSE otherwise
 */
BOOLEAN j9csrsi_is_lpar(const j9csrsi_t *session);

/** Shutdown a j9csrsi session
 *  @param[in] session Pointer to j9csrsi session
 *  @return TRUE if system is running on a VM hypervisor. FALSE otherwise.
 */
BOOLEAN j9csrsi_is_vmh(const j9csrsi_t *session);

/** Get vmhpidentifier (i.e., name of L-3 Hypervisor)
 *  @param[in] session Pointer to j9csrsi session
 *  @param[in] position Number of the entry that should be retrieved
 *  @param[out] buf buffer where vmhpidentifier will be written to. The string
 *  returned is in ASCII
 *  @param[in] len Length of buf. buf must be sufficiently large to accomodate
 *  the whole string and a '\0'. Otherwise, an error will be returned.
 *  @return A number greater than 0 in case of success. This is the number
 *  of chars written to buf. Any other return value indicates an error.
 */
int32_t j9csrsi_get_vmhpidentifier(const j9csrsi_t *session, uint32_t position,
								   char *buf, uint32_t len);

/** Get cpctype (i.e., hw model number)
 *  @param[in] session Pointer to j9csrsi session
 *  @param[out] buf buffer where cpctype will be written to. The string
 *  returned is in ASCII
 *  @param[in] len Length of buf. buf must be sufficiently large to accomodate
 *  the whole string and a '\0'. Otherwise, an error will be returned.
 *  @return A number greater than 0 in case of success. This is the number
 *  of chars written to buf. Any other return value indicates an error.
 */
int32_t j9csrsi_get_cpctype(const j9csrsi_t *session, char *buf,
							uint32_t len);

#endif /* OMRCSRSI_H_ */

