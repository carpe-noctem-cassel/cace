/*
 * TimeManager.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef TIMEMANAGER_H_
#define TIMEMANAGER_H_

#include <iostream>
#include <map>
#include <mutex>
#include <algorithm>

#include "ros/ros.h"
#include "cace/CaceTime.h"


#include "AgentCommunicationModel.h"


using namespace std;

namespace cace
{
	class CaceCommunication;
	class AgentCommunicationModel;

	class TimeManager
	{
	public:
		TimeManager(CaceCommunication* com);
		virtual ~TimeManager();

		unsigned long getDistributedTime();
		unsigned long getLocalTime();
		long timeDiff;
		unsigned long maxResendTime;
		unsigned long minResendTime;
		double resendArrivalPropability;
		unsigned long lamportTime;

		void updateLamportTime(unsigned long seenTime);
		void step();
		unsigned long getEstimatedResendTime(int agentID);
		double getEstimatedPacketLoss(int agentID);
		double getPropabilityforConsensus(int agentID, unsigned long jobStart);
		unsigned long getAgentCommunicationDelay(int agentID);

		void addTimeMessage(CaceTimePtr ct, unsigned long receivedTime);

		string toString();


		map<int, AgentCommunicationModel> agentModels;
		static unsigned long timeMessageInterval;
		static unsigned long timeResolutionDevisor;

	protected:
		unsigned long lastSent=0;
		mutex modelMutex;
		CaceCommunication* com;
	};

} /* namespace cace */

#endif /* TIMEMANAGER_H_ */
