/*
 * CommunicationWorker.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef COMMUNICATIONWORKER_H_
#define COMMUNICATIONWORKER_H_

#include <iostream>
#include <list>
#include <mutex>

#include "jobs/AbstractCommunicationJob.h"
#include "jobs/ShortAckJob.h"
#include "jobs/CommandJob.h"
#include "jobs/BelieveNotificationJob.h"
#include "../variables/ConsensusVariable.h"

using namespace std;

namespace cace
{
	class AbstractCommunicationJob;

	class CommunicationWorker
	{
	public:
		CommunicationWorker();
		virtual ~CommunicationWorker();

		list<int> agentsToRemove;

		/*!
		 * Add a new Job to task queue
		 */
		void appendJob(AbstractCommunicationJob* j);

		/*!
		 * Returns number of active jobs
		 */
		int count();

		/*!
		 * Clears all Jobs
		 */
		void clearJobs();

		/*!
		 * Returns the oldes job related to the given variable name that requires confirmation
		 * Note Type Depends on variable consensuslevel
		 */
		AbstractCommunicationJob* getNewestVariableJob(string name, acceptStrategy strategy);

		/*!
		 * Processes all Jobs
		 */
		void processJobs();

		string toString();
		string toStringNoNewLine();
	protected:
		mutex jobMutex;
		list<AbstractCommunicationJob*> jobs;

	};

} /* namespace cace */

#endif /* COMMUNICATIONWORKER_H_ */
