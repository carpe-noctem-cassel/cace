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

		/*!
		 * Synchronized Time
		 */
		unsigned long getDistributedTime();

		/*!
		 * Local Cace Time
		 */
		unsigned long getLocalTime();

		/*!
		 * Time difference between local time and distributed time
		 * should not be changed manually!
		 */
		long timeDiff;

		/*!
		 * Indicates the max time before resending a command
		 */
		unsigned long maxResendTime;

		/*!
		 * Indicates the min time before resending a command
		 */
		unsigned long minResendTime;

		/*!
		 * Indicates the max time before resending a command
		 */
		double resendArrivalPropability;

		/*!
		 * Local Lamport Time
		 */
		unsigned long lamportTime;

		/*!
		 * called whenever a new message is received to update the current lamport time
		 */
		void updateLamportTime(unsigned long seenTime);

		/*!
		 * Timemanager Iteration
		 */
		void step();

		/*!
		 * Returns a estimation for resendtimes for 'agentID' in ms
		 */
		unsigned long getEstimatedResendTime(int agentID);

		/*!
		 * Returns a estimation for packet loss to 'agentID'
		 */
		double getEstimatedPacketLoss(int agentID);

		/*!
		 * Returns a probability estimation for consensus
		 * depends on target 'agentID' and time since the last job has been startet
		 */
		double getPropabilityforConsensus(int agentID, unsigned long jobStart);

		/*!
		 * Returns the current communication delay estimation to 'agentID'
		 */
		unsigned long getAgentCommunicationDelay(int agentID);

		/*!
		 * Adds a new Timemessage
		 */
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
