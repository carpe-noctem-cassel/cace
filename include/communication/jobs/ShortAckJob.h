/*
 * ShortAckJob.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef SHORTACKJOB_H_
#define SHORTACKJOB_H_

#include <cace/CaceAcknowledge.h>
#include <memory>
#include <vector>
#include "AbstractCommunicationJob.h"

using namespace std;

namespace cace
{
	class ConsensusVariable;

	class ShortAckJob : public AbstractCommunicationJob
	{
	public:
		ShortAckJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids,
					unsigned long lamportTime, Cace* cace, CaceAcknowledgePtr ack);
		virtual ~ShortAckJob();

		virtual string toString();
		virtual bool process();
		CaceAcknowledgePtr ack;

	protected:
		shared_ptr<ConsensusVariable> doVariableUpdate();
	};

} /* namespace cace */

#endif /* SHORTACKJOB_H_ */
