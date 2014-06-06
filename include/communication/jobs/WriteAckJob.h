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

#include "cace.h"
#include "cace/CaceCommand.h"
#include "CaceTypes.h"

using namespace std;

namespace cace
{

	class WriteAckJob
	{
	public:
		WriteAckJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids, unsigned long lamportTime, Cace* cace, CaceCommandPtr cmd);
		virtual ~WriteAckJob();
	};

} /* namespace cace */

#endif /* WRITEACKJOB_H_ */
