/*
 * WriteJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef WRITEJOB_H_
#define WRITEJOB_H_

#include <vector>
#include <memory>

#include "cace/CaceShortAck.h"

#include "AbstractCommunicationJob.h"
#include "../../variables/ConsensusVariable.h"
#include "../../cace.h"
#include "../../timeManager/AgentTimeData.h"
#include "../JobStateEntity.h"

using namespace std;

namespace cace
{

	class WriteJob : public AbstractCommunicationJob
	{
	public:
		WriteJob(string& name, shared_ptr<ConsensusVariable> variable, vector<char> value, vector<int>& robotids, ctime lamportTime, Cace* cace);
		virtual ~WriteJob();

		short msgID;
		short target;
		vector<char> value;
		vector<CaceShortAck> acks;
		vector<JobStateEntity> entities;
		int remainingRetrys = 1;
		int maxRetrys;
		string name;
		ctime lamportTime = 0;

		virtual string toString();
		virtual bool process();
		virtual bool failed ();
	};

} /* namespace cace */

#endif /* WRITEJOB_H_ */
