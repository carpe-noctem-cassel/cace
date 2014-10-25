/*
 * BelieveNotificationJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef BELIEVENOTIFICATIONJOB_H_
#define BELIEVENOTIFICATIONJOB_H_

#include "AbstractCommunicationJob.h"
#include "../JobStateEntity.h"
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace cace
{
	class ConsensusVariable;

	class BelieveNotificationJob : public AbstractCommunicationJob
	{
	public:
		BelieveNotificationJob(string name, shared_ptr<ConsensusVariable> variable, vector<uint8_t> value,
								vector<int> robotids, unsigned long lamportTime, Cace* cace);
		virtual ~BelieveNotificationJob();

		list<JobStateEntity> entities;
		short msgID;
		int maxRetrys;
		vector<uint8_t> value;
		int remainingRetrys = 1;

		virtual string toString();
		virtual bool process();
		virtual bool failed();
	};

} /* namespace cace */

#endif /* BELIEVENOTIFICATIONJOB_H_ */
