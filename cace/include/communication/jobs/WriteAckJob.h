/*
 * WriteAckJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef WRITEACKJOB_H_
#define WRITEACKJOB_H_

#include<vector>
#include <memory>

#include "AbstractCommunicationJob.h"
#include "cace.h"
#include "cace/CaceCommand.h"
#include "CaceTypes.h"

using namespace std;

namespace cace
{

	class WriteAckJob : AbstractCommunicationJob
	{
	public:
		WriteAckJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids,
					unsigned long lamportTime, Cace* cace, CaceCommandPtr cmd);
		virtual ~WriteAckJob();

		virtual string toString();
		virtual bool process();

		CaceCommandPtr cmd;

	protected:
		shared_ptr<ConsensusVariable> doVariableUpdate();
	};

} /* namespace cace */

#endif /* WRITEACKJOB_H_ */
