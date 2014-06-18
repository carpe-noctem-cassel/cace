/*
 * WriteJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef WRITEJOB_H_
#define WRITEJOB_H_



#include "AbstractCommunicationJob.h"
#include <cace/CaceShortAck.h>
#include <communication/JobStateEntity.h>
#include <CaceTypes.h>
#include <variables/ConsensusVariable.h>
//#include "../../cace.h"
//#include "../../timeManager/AgentTimeData.h"
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

using namespace std;

namespace cace
{

	class WriteJob : public AbstractCommunicationJob
	{
	public:
		WriteJob(string& name, shared_ptr<ConsensusVariable> variable, vector<uint8_t> value, vector<int>& robotids, unsigned long lamportTime, Cace* cace);
		virtual ~WriteJob();

		short msgID;
		short target;
		vector<char> value;
		list<CaceShortAckPtr> acks;
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
