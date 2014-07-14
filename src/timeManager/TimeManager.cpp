/*
 * TimeManager.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

//#define USE_ROS
#include "timeManager/TimeManager.h"
#include "communication/CaceCommunication.h"
#include "SystemConfig.h"

namespace cace
{
	unsigned long TimeManager::timeResolutionDevisor = 1;
	unsigned long TimeManager::timeMessageInterval = 1;

	TimeManager::TimeManager(CaceCommunication* com)
	{
		timeEvalOffeset = 0;
		lastSent = 0;
		lamportTime = 1;
		timeDiff = 0;
		timeReceiveEvalOffeset = 0;
		this->com = com;

		supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		timeMessageInterval = (*sc)["Cace"]->get<unsigned long>("Cace.TimeMessageInterval", NULL);
		maxResendTime = (*sc)["Cace"]->get<unsigned long>("Cace.MaxResendTime", NULL);
		minResendTime = (*sc)["Cace"]->get<unsigned long>("Cace.MinResendTime", NULL);
		this->resendArrivalPropability = (*sc)["Cace"]->get<double>("Cace.ResendArrivalPropability", NULL);

		AgentCommunicationModel::defaultDelay = (*sc)["Cace"]->get<unsigned long>("Cace.DefaultDelay", NULL);
		AgentCommunicationModel::defaultDelayVariance = (*sc)["Cace"]->get<unsigned long>("Cace.DefaultDelayVariance",
		NULL);
	}

	TimeManager::~TimeManager()
	{
	}

	unsigned long TimeManager::getDistributedTime()
	{
#ifdef USE_ROS
		ros::Time now = ros::Time::now();
		return timeDiff + ((unsigned long)now.sec) * 1000000000ul + (unsigned long)now.nsec + timeEvalOffeset;
#else
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = now.time_since_epoch();
		return timeDiff + std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() + timeEvalOffeset;
#endif
	}

	unsigned long TimeManager::getLocalTime()
	{
#ifdef USE_ROS
		ros::Time now = ros::Time::now();
		return ((unsigned long)now.sec) * 1000000000ul + (unsigned long)now.nsec + timeEvalOffeset;
#else
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = now.time_since_epoch();
		return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() + timeEvalOffeset;
#endif
	}

	void TimeManager::updateLamportTime(unsigned long seenTime)
	{
		if (lamportTime < seenTime)
			lamportTime = seenTime + 1;
	}

	void TimeManager::step()
	{
		//Send every 'TimeMessageInterval'ms
		if (lastSent + timeMessageInterval < getLocalTime() / ((unsigned long)1000000))
		{
			com->sendTime(this);
			lastSent = getLocalTime() / ((unsigned long)1000000);
		}
	}

	unsigned long TimeManager::getEstimatedResendTime(int agentID)
	{
		AgentCommunicationModel* model = nullptr;
		lock_guard<std::mutex> lock(this->modelMutex);
		if (agentModels.find(agentID) == agentModels.end())
		{
			//if we never received a timemessage: The Agent is propably new and just joining communication range
			// -> max resendtime
			return maxResendTime;
		}
		return max(
				min(agentModels[agentID].getEstimatedResendTime(resendArrivalPropability) / timeResolutionDevisor,
					maxResendTime),
				minResendTime);
	}

	double TimeManager::getEstimatedPacketLoss(int agentID)
	{
		AgentCommunicationModel* model = nullptr;
		lock_guard<std::mutex> lock(this->modelMutex);
		if (agentModels.find(agentID) == agentModels.end())
		{
			return 1.0;
		}
		return agentModels[agentID].getProbabilityForPacketLoss();
	}

	double TimeManager::getPropabilityforConsensus(int agentID, unsigned long jobStart)
	{
		AgentCommunicationModel* model = nullptr;
		lock_guard<std::mutex> lock(this->modelMutex);
		if (agentModels.find(agentID) == agentModels.end())
		{
			return 0.0;
		}
		return agentModels[agentID].getPropabilityConsensus(jobStart, resendArrivalPropability);
	}

	unsigned long TimeManager::getAgentCommunicationDelay(int agentID)
	{
		AgentCommunicationModel* model = nullptr;
		lock_guard<std::mutex> lock(this->modelMutex);
		if (agentModels.find(agentID) == agentModels.end())
		{
			return 0.0;
		}
		return agentModels[agentID].getMaxLikelihoodDelay();
	}

	void TimeManager::addTimeMessage(CaceTimePtr ct, unsigned long receivedTime)
	{
		receivedTime += timeReceiveEvalOffeset;
		map<int, AgentCommunicationModel>::iterator it;

		lock_guard<std::mutex> lock(this->modelMutex);
		//cout << "please implement this! (TimeManager.cpp:AddTimeMessage)" << endl;
		AgentTimeData* agt = new AgentTimeData(ct->localtime, ct->distributedTime,
												(ctime)((long)receivedTime + timeDiff), receivedTime, ct->senderID);

		if (agentModels.find(ct->senderID) == agentModels.end())
		{
			agentModels[ct->senderID].robotID = ct->senderID;
		}
		AgentCommunicationModel* model = &agentModels[ct->senderID];

		//Comment out for fixed values:
		model->addData(agt);
		//ctime mu = model->getMaxLikelihoodDelay();
		//Console.WriteLine("Sender: "+ct.SenderID+" mu:" + mu +" b: "+model.GetMaxLikelihoodDelayVariance(mu) + " P(L): "+model.GetProbabilityForPacketLoss() + " DataCount: "+model.Data.Count);

		//cout << ct->localtime << "\t" << ct->distributedTime << "\t" << receivedTime << "\t" << timeDiff << "\t"<< mu << endl;

		long diffSum = 0;
		long count = 0;
		for (it = agentModels.begin(); it != agentModels.end(); it++)
		{
			if (it->second.data.size() > 1)
			{
				diffSum += (long)it->second.getEstimatedTimeDifference();
				count++;
			}
		}
		timeDiff += ((diffSum / (count + 1)) - timeDiff) * 0.3; // / 3;

	}

	string TimeManager::toString()
	{
		string acmstring = "";
		map<int, AgentCommunicationModel>::iterator it;
		lock_guard<std::mutex> lock(this->modelMutex);
		for (it = agentModels.begin(); it != agentModels.end(); it++)
		{
			acmstring += "> " + it->second.toString() + " <\t";
		}

		return "Distributed Time: " + to_string(getDistributedTime()) + " LocalTime " + to_string(getLocalTime())
				+ " LamportTime " + to_string(lamportTime) + " Models(" + to_string(agentModels.size()) + ") "
				+ acmstring;
	}

	void TimeManager::clearQueues()
	{
		agentModels.clear();
	}

} /* namespace cace */

