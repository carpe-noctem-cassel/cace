/*
 * ShortAckJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef SHORTACKJOB_H_
#define SHORTACKJOB_H_

#include <cace/CaceAcknowledge.h>
#include <variables/ConsensusVariable.h>


using namespace std;

namespace cace
{

	class ShortAckJob
	{
	public:
		ShortAckJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids, unsigned long lamportTime, Cace* cace, CaceAcknowledgePtr ack);
		virtual ~ShortAckJob();
	};

} /* namespace cace */

#endif /* SHORTACKJOB_H_ */
