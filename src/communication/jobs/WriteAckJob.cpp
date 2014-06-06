/*
 * WriteAckJob.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include "communication/jobs/WriteAckJob.h"

namespace cace
{

	WriteAckJob::WriteAckJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids, unsigned long lamportTime, Cace* cace, CaceCommandPtr cmd)
	{
		// TODO Auto-generated constructor stub

	}

	WriteAckJob::~WriteAckJob()
	{
		// TODO Auto-generated destructor stub
	}

} /* namespace cace */
