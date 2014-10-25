/*
 * BelieveAcknowledgeJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef BELIEVEACKNOWLEDGEJOB_H_
#define BELIEVEACKNOWLEDGEJOB_H_

#include <cace/CaceBelieveNotification.h>
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

	class BelieveAcknowledgeJob : public AbstractCommunicationJob
	{
	public:
		BelieveAcknowledgeJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids, unsigned long lamportTime, Cace* cace,  CaceBelieveNotificationPtr notification);
		virtual ~BelieveAcknowledgeJob();

		virtual string toString();
		virtual bool process();
		virtual bool failed();

		short msgID;
		CaceBelieveNotificationPtr notification;
		int remainingRetrys = 1;
		bool updatedOwnBelieve = false;
		list<JobStateEntity> entities;
		unsigned long lamportTime = 0;
		int maxRetrys;

	protected:
		shared_ptr<ConsensusVariable> doVariableUpdate();
	};

} /* namespace cace */

#endif /* BELIEVEACKNOWLEDGEJOB_H_ */
