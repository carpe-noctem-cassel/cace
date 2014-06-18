/*
 * CommunicationWorker.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include "communication/CommunicationWorker.h"

namespace cace
{

	CommunicationWorker::CommunicationWorker()
	{
	}

	CommunicationWorker::~CommunicationWorker()
	{
		for(AbstractCommunicationJob* j : jobs) {
			delete j;
		}
	}

	void CommunicationWorker::appendJob(AbstractCommunicationJob* j)
	{
		lock_guard<std::mutex> lock(jobMutex);
		jobs.push_back(j);
	}
	int CommunicationWorker::count()
	{
		lock_guard<std::mutex> lock(jobMutex);
		return jobs.size();
	}
	void CommunicationWorker::clearJobs()
	{
		lock_guard<std::mutex> lock(jobMutex);
		list<AbstractCommunicationJob*>::iterator it;
		for (it = jobs.begin(); it != jobs.end(); it++)
		{
			delete (*it);
		}
		jobs.clear();
	}
	AbstractCommunicationJob* CommunicationWorker::getNewestVariableJob(string name, acceptStrategy strategy)
	{
		AbstractCommunicationJob* ret = nullptr;
		unsigned long time = 0;
		list<AbstractCommunicationJob*>::iterator it;

		lock_guard<std::mutex> lock(jobMutex);
		for (it = jobs.begin(); it != jobs.end(); it++)
		{
			ShortAckJob* saj = dynamic_cast<ShortAckJob*>((*it));
			CommandJob* cj = dynamic_cast<CommandJob*>((*it));
			BelieveNotificationJob* bnj = dynamic_cast<BelieveNotificationJob*>((*it));
			if ((strategy > acceptStrategy::TwoWayHandShake && saj == 0)
					|| (strategy > acceptStrategy::FireAndForget && (cj != 0 | bnj != 0)))
			{
				if (name == (*it)->name && (*it)->startTime > time)
				{
					time = (*it)->startTime;
					ret = (*it);
				}

			}
		}
		return ret;
	}
	void CommunicationWorker::processJobs()
	{
		lock_guard<std::mutex> lock(jobMutex);
		list<AbstractCommunicationJob*>::iterator it = jobs.begin();

		while (it != jobs.end())
		{
			if ((*it)->process())
			{
				if ((*it)->failed())
				{
					agentsToRemove.insert(agentsToRemove.end(), (*it)->expectedRobotIDs.begin(),
											(*it)->expectedRobotIDs.end());
				}
				delete (*it);
				it = jobs.erase(it);
			}
			else
			{
				it++;
			}
		}
	}
	string CommunicationWorker::toString()
	{
		list<AbstractCommunicationJob*>::iterator it;

		lock_guard<std::mutex> lock(jobMutex);
		string ret = "OpenJobs: " + to_string((int) jobs.size()) + "\n";
		for (it = jobs.begin(); it != jobs.end(); it++)
		{
			ret += "\t" + (*it)->toString() + "\n";
		}

		return ret;
	}
	string CommunicationWorker::toStringNoNewLine()
	{
		list<AbstractCommunicationJob*>::iterator it;

		lock_guard<std::mutex> lock(jobMutex);
		string ret = "OpenJobs: " + to_string((int) jobs.size()) + "\n";
		for (it = jobs.begin(); it != jobs.end(); it++)
		{
			ret += "\t> " + (*it)->toString() + " <\t";
		}

		return ret;
	}

} /* namespace cace */
