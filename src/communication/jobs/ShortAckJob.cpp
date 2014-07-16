/*
 * ShortAckJob.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <boost/smart_ptr/shared_ptr.hpp>
#include <cace.h>
#include <communication/CaceCommunication.h>
#include <communication/jobs/ShortAckJob.h>
#include <CaceTypes.h>
#include <variables/ConsensusVariable.h>
#include <variableStore/CVariableStore.h>
#include <limits>
//#include <memory>
#include <string>
//#include <vector>

namespace cace
{

	ShortAckJob::ShortAckJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids,
								unsigned long lamportTime, Cace* cace, CaceAcknowledgePtr ack) :
			AbstractCommunicationJob(name, variable, robotids, lamportTime, cace)
	{
		this->ack = ack;

		if (!cace->safeStepMode)
		{
			shared_ptr<ConsensusVariable> cv = doVariableUpdate();

			if (cv->getAcceptStrategy() > acceptStrategy::TwoWayHandShake)
			{
				caceCommunication->sendCaceShortAck(cv->getName(), ack->msgID, ack->senderID);
			}
		}
	}

	ShortAckJob::~ShortAckJob()
	{
	}

	string ShortAckJob::toString()
	{
		return "SAck " + ack->variableName + "=" + to_string(ack->value.size()) + "\tattempts: " + to_string(attempts)
				+ "\tMsgID: " + to_string(ack->msgID);
	}

	bool ShortAckJob::process()
	{
		if (attempts == 0 && cace->safeStepMode)
		{
			shared_ptr<ConsensusVariable> cv = doVariableUpdate();
			if (cv->getAcceptStrategy() > acceptStrategy::TwoWayHandShake)
			{
				caceCommunication->sendCaceShortAck(cv->getName(), ack->msgID, ack->senderID);
			}
		}
		attempts++;

		return true;
	}

	shared_ptr<ConsensusVariable> ShortAckJob::doVariableUpdate()
	{
		CVariableStore* store = cace->variableStore;
		shared_ptr<ConsensusVariable> cv = store->getVariable(ack->variableName);
		if (store->existsVariable(ack->variableName))
		{
			cv = store->getVariable(ack->variableName);

			bool found = false;
			bool proposalsUpdated = false;
			for (auto var : cv->proposals)
			{
				if (var->getRobotID() == ack->senderID)
				{
					found = true;
					if (var->getLamportAge() < ack->lamportTime)
					{
						var->setValue(ack->value);
						var->setDecissionTime(std::numeric_limits<long>::max()-1);
						var->setValidityTime(std::numeric_limits<long>::max()-1);
						var->setLamportAge(ack->lamportTime);
						//var->setAcceptStrategy((acceptStrategy)3);
						var->setType(ack->type);
						proposalsUpdated = true;
					}
				}
			}
			if (!found)
			{
				//add believe
				auto var = make_shared<ConsensusVariable>(ack->variableName, cv->getAcceptStrategy(),
															cv->getValidityTime(), ack->senderID,
															cv->getDecissionTime(), ack->lamportTime, ack->type);
				var->setValue(ack->value);
				var->setRobotID(ack->senderID);
				cv->proposals.push_back(var);
				proposalsUpdated = true;
			}
			if (proposalsUpdated)
			{
				//cv->notify();
				cv->acceptProposals(*cace, nullptr);
			}
			//cv->acceptProposals(*cace, nullptr);
		}
		else
		{
			//If we don't know anything: Accept Command
			cv = make_shared<ConsensusVariable>(ack->variableName, acceptStrategy::ThreeWayHandShake,
												std::numeric_limits<long>::max()-1, caceCommunication->getOwnID(),
												std::numeric_limits<long>::max()-1, 0, ack->type);
			auto co = make_shared<ConsensusVariable>(ack->variableName, acceptStrategy::ThreeWayHandShake,
														std::numeric_limits<long>::max()-1, ack->senderID,
														std::numeric_limits<long>::max()-1, ack->lamportTime, ack->type);
			co->setValue(ack->value);
			cv->proposals.push_back(co);

			cv->acceptProposals(*cace, &ack->value);
			store->addVariable(cv);
		}

		return cv;
	}

} /* namespace cace */
