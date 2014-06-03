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

	class CommunicationWorker
	{
	public:
		CommunicationWorker();
		virtual ~CommunicationWorker();

		list<int> agentsToRemove;
		void appendJob(AbstractCommunicationJob* j);
		int count();
		void clearJobs();
		AbstractCommunicationJob* getNewestVariableJob(string name, acceptStrategy strategy);
		void processJobs();
		string toString();
		string ToStringNoNewLine();
	protected:
		mutex jobMutex;
		list<AbstractCommunicationJob*> jobs;

	};

} /* namespace cace */

#endif /* COMMUNICATIONWORKER_H_ */
