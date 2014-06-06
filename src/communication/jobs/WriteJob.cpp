/*
 * WriteJob.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include "communication/jobs/WriteJob.h"

namespace cace
{

	WriteJob::WriteJob(string& name, shared_ptr<ConsensusVariable> variable, vector<char> value, vector<int>& robotids,
						ctime lamportTime, Cace* cace) :
			AbstractCommunicationJob(name, variable, robotids, lamportTime, cace)
	{
		this->name = name;
		this->target = robotids.at(0);
		msgID = (short)((int)(cace->timeManager->lamportTime + 100) * caceCommunication->getOwnID());

		//#include "SystemConfig.h"
		//supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		//maxRetrys = (*sc)["Cace"]->get<int>("Cace.MaxCommandRetrys", NULL);

		//caceCommunication.SendWriteCommand(variable, msgID, target);
		lamportTime = cace->timeManager->lamportTime;
		for (int& id : robotids)
		{
			entities.push_back(
					JobStateEntity(id, maxRetrys,
									cace->timeManager->getLocalTime() / TimeManager::timeResolutionDevisor));
		}

	}

	WriteJob::~WriteJob()
	{
	}

	string WriteJob::toString()
	{
	}

	bool WriteJob::process()
	{
	}

	bool WriteJob::failed()
	{
	}

} /* namespace cace */
