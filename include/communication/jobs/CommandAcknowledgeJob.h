/*
 * CommandAcknowledgeJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef COMMANDACKNOWLEDGEJOB_H_
#define COMMANDACKNOWLEDGEJOB_H_

#include <cace/CaceCommand.h>
#include "AbstractCommunicationJob.h"
#include "../JobStateEntity.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <list>

namespace cace
{

	class ConsensusVariable;

	class CommandAcknowledgeJob : public AbstractCommunicationJob
	{
	public:
		CommandAcknowledgeJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids,
								unsigned long lamportTime, Cace* cace, CaceCommandPtr cmd);
		virtual ~CommandAcknowledgeJob();

		virtual string toString();
		virtual bool process();
		virtual bool failed();

		short msgID;
		CaceCommandPtr command;
		int remainingRetrys=1;
		shared_ptr<ConsensusVariable> cv;
		bool updatedOwnBelieve = false;
		list<JobStateEntity> entities;
		unsigned long lamportTime=0;
		int maxRetrys;

	protected:
		shared_ptr<ConsensusVariable> doVariableUpdate();
	};

} /* namespace cace */

#endif /* COMMANDACKNOWLEDGEJOB_H_ */
