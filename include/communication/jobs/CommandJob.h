/*
 * CommandJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef COMMANDJOB_H_
#define COMMANDJOB_H_

#include "AbstractCommunicationJob.h"
#include "../JobStateEntity.h"
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

using namespace std;

namespace cace
{
	class ConsensusVariable;

	class CommandJob : public AbstractCommunicationJob
	{
	public:
		CommandJob(string name, shared_ptr<ConsensusVariable> variable, vector<uint8_t> value, vector<int> robotids,
					unsigned long lamportTime, Cace* cace);
		virtual ~CommandJob();

		list<JobStateEntity> entities;
		short msgID;
		int maxRetrys;
		vector<uint8_t> value;
		int remainingRetrys=1;

		virtual string toString();
		virtual bool process();
		virtual bool failed();
	};

} /* namespace cace */

#endif /* COMMANDJOB_H_ */
