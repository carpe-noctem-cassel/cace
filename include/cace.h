/*
 * cace.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */
#ifndef CACE_H_
#define CACE_H_

//#include "caceSpace.h"
//#include "communication/CaceCommunication.h"
//#include "timeManager/TimeManager.h"
//#include "variableStore/CVariableStore.h"
//#include <iostream>
//#include <memory>

#ifdef USE_ROS
#include <ros/node_handle.h>
#include <ros/timer.h>
#endif
#include <string>
#include <vector>
#include <thread>

namespace cace
{
	class CaceCommunication;
	class CaceSpace;
	class CommunicationWorker;
	class TimeManager;
} /* namespace cace */
namespace ros
{
	class Timer;
	struct TimerEvent;
} /* namespace ros */

using namespace std;

namespace cace
{
	class CVariableStore;

	class Cace
	{
	protected:
		Cace(string prefix = "", int id = 0, bool quiet = false);
		void init(string prefix, int id, bool quiet);

	public:
		~Cace();
		CaceCommunication* communication;
		static Cace* getEmulated(string prefix = "", int id = 0, bool quiet = false);
		static Cace* get();
		void destroy();
		void substituteCaceCommunication(CaceCommunication* cc);
		void setQuiet(string rosNodePrefix, short id);
		void unsetQuiet(string rosNodePrefix, int id);

		TimeManager* timeManager = nullptr;
		CVariableStore* variableStore = nullptr;
		CaceSpace* caceSpace = nullptr;
		CommunicationWorker* worker = nullptr;

		double sleepTime = 0.033;
#ifdef USE_ROS
		ros::Timer timer;
		void rosStep(const ros::TimerEvent&);
#else
		bool isActive;
		thread* timer;
		void timerThread();
#endif

		vector<string> localScope;
		vector<int> activeRobots;

		void step();
		void run();

		vector<int>* getActiveRobots();

		bool safeStepMode;
		bool quietMode;

		string ownNetworkStatusString();
		string printMessageQueueStates();
		string printActiveRobots();
		string getGlobalScope();
		vector<string>& getLocalScope();
		string getLocalScopeString();
		void agentEngangement(int id, bool sendBeliveUpdates);
		void agentDisengangement(int id);
	};
}

#endif /* CACE_H_ */
