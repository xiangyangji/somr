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

#if !defined(VERBOSEWRITERFILELOGGING_HPP_)
#define VERBOSEWRITERFILELOGGING_HPP_
 
#include "omrcfg.h"

#include "VerboseWriter.hpp"

class MM_VerboseManager;

/**
 * Ouptut agent which directs verbosegc output to file.
 */
class MM_VerboseWriterFileLogging : public MM_VerboseWriter
{
	/*
	 * Data members
	 */
public:
protected:
	char *_filename; /**< the filename template supplied from the command line */
	uintptr_t _numFiles; /**< number of files to rotate through */
	uintptr_t _numCycles; /**< number of cycles in each file */
	
	uintptr_t _mode; /**< rotation mode -- single_file or rotating_files */
	uintptr_t _currentFile; /**< zero-based index of current rotating file */
	uintptr_t _currentCycle; /**< current GC cycle within this file */

	J9StringTokens *_tokens; /**< tokens used during filename expansion */

	MM_VerboseManager *_manager; /**< Verbose manager */
private:

	/*
	 * Function members
	 */
public:
	virtual bool reconfigure(MM_EnvironmentBase *env, const char* filename, uintptr_t fileCount, uintptr_t iterations);

	void closeStream(MM_EnvironmentBase *env);

	virtual void endOfCycle(MM_EnvironmentBase *env);

	virtual void outputString(MM_EnvironmentBase *env, const char* string) = 0;

protected:
	MM_VerboseWriterFileLogging(MM_EnvironmentBase *env, MM_VerboseManager *manager, WriterType type);

	virtual bool initialize(MM_EnvironmentBase *env, const char *filename, uintptr_t numFiles, uintptr_t numCycles);
	virtual void tearDown(MM_EnvironmentBase *env);

	virtual bool openFile(MM_EnvironmentBase *env) = 0;
	virtual void closeFile(MM_EnvironmentBase *env) = 0;

	intptr_t findInitialFile(MM_EnvironmentBase *env);
	bool initializeFilename(MM_EnvironmentBase *env, const char *filename);
	bool initializeTokens(MM_EnvironmentBase *env);
	char* expandFilename(MM_EnvironmentBase *env, uintptr_t currentFile);
private:
};

#endif /* VERBOSEWRITERFILELOGGING_HPP_ */
