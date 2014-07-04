/*
 * AbstractCommunicationJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef ABSTRACTCOMMUNICATIONJOB_H_
#define ABSTRACTCOMMUNICATIONJOB_H_

#include <iostream>
#include <memory>
#include <vector>

using namespace std;

namespace cace
{
	class cace;
	class CaceCommunication;
	class ConsensusVariable;
	class Cace;

	class AbstractCommunicationJob
	{
	public:
		AbstractCommunicationJob(string& name, shared_ptr<ConsensusVariable> variable, vector<int>& robotids,
									unsigned long lamportTime, Cace* cace);
		virtual ~AbstractCommunicationJob();

		/*!
		 * Executed every step to perform job.
		 * Return value indicates whether the job is finished
		 */
		virtual bool process() = 0;

		virtual string toString() = 0;

		/*!
		 * Indicates whether job failed
		 */
		virtual bool failed();

		/*!
		 * Associated Job Lamporttime
		 */
		unsigned long lamportTime;

		/*!
		 * Distributed Job StartTime
		 */
		unsigned long startTime;
		int maxRetrys;
		Cace* cace;

		/*!
		 * Number of attempts to process the Jobs
		 */
		int attempts;

		/*!
		 * List of Robots that havent yet responedet
		 */
		vector<int> expectedRobotIDs;

		/*!
		 * Number of attempts to process the Jobs
		 */
		shared_ptr<ConsensusVariable> variable;

		/*!
		 * String of target variable
		 */
		string name;
		CaceCommunication* caceCommunication;

	};

} /* namespace cace */

#endif /* ABSTRACTCOMMUNICATIONJOB_H_ */
