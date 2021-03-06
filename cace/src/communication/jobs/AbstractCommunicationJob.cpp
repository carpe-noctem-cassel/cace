/*
 * AbstractCommunicationJob.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <cace.h>
#include <communication/jobs/AbstractCommunicationJob.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <string>

namespace cace
{

	AbstractCommunicationJob::AbstractCommunicationJob(string& name, shared_ptr<ConsensusVariable> variable, vector<int>& robotids, unsigned long lamportTime, Cace* cace)
	{
		this->cace = cace;
		this->caceCommunication = cace->communication;
		this->name = name;
		this->variable = variable;
		this->expectedRobotIDs = robotids;
		this->lamportTime = lamportTime;
		this->startTime = cace->timeManager->getDistributedTime();
		attempts = 0;
	}

	AbstractCommunicationJob::~AbstractCommunicationJob()
	{
	}

	bool AbstractCommunicationJob::failed()
	{
		return false;
	}

} /* namespace cace */
