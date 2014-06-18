/*
 * RequestJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef REQUESTJOB_H_
#define REQUESTJOB_H_

#include "AbstractCommunicationJob.h"
#include "cace.h"
#include "cace/CaceCommand.h"
#include "CaceTypes.h"

using namespace std;

namespace cace
{
	class JobStateEntity;

	class RequestJob : public AbstractCommunicationJob
	{
	public:
		RequestJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids, unsigned long lamportTime,
					Cace* cace);
		virtual ~RequestJob();

		virtual string toString();
		virtual bool process();
		virtual bool failed();

		short msgID;
		short target;
		vector<JobStateEntity> entities;
		int remainingRetrys = 1;
		string name = "";
		unsigned long lamportTime = 0;
	};

} /* namespace cace */

#endif /* REQUESTJOB_H_ */
