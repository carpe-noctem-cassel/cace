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
		AbstractCommunicationJob(string& name, shared_ptr<ConsensusVariable> variable, vector<int>& robotids, unsigned long lamportTime, Cace* cace);
		virtual ~AbstractCommunicationJob();

		virtual bool process() = 0;
		virtual string toString() = 0;
		virtual bool failed();


		unsigned long lamportTime;
		unsigned long startTime;
		int maxRetrys;
		Cace* cace;
		int attempts;
		vector<int> expectedRobotIDs;
		shared_ptr<ConsensusVariable> variable;
		string name;
		CaceCommunication* caceCommunication;


	};

} /* namespace cace */

#endif /* ABSTRACTCOMMUNICATIONJOB_H_ */
