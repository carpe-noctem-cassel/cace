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
		/*!
		 * Constructor required for simulating different agents
		 */
		Cace(string prefix = "", int id = 0, bool quiet = false);

		/*!
		 * Setup communication
		 */
		void init(string prefix, int id, bool quiet);

	public:
		~Cace();

		/*!
		 * Communication Module
		 */
		CaceCommunication* communication;

		/*!
		 * Factory for cace instances
		 */
		static Cace* getEmulated(string prefix = "", int id = 0, bool quiet = false);

		/*!
		 * Singleton
		 */
		static Cace* get();

		/*!
		 * Destroy communication safely
		 */
		void destroy();

		/*!
		 * Replaces CaceCommunication module e.g. for tests, quiet mode or different network models
		 */
		void substituteCaceCommunication(CaceCommunication* cc);

		/*!
		 * Sets Quiet Mode (Listening but not talking)
		 * Commanding is possible but only with fire and forget
		 * required to observe variable values
		 * Note: not guarantees are given anymore, when commanding a value
		 */
		void setQuiet(string rosNodePrefix, short id);

		/*!
		 * Unsets Quiet Mode
		 * required to observe variable values
		 */
		void unsetQuiet(string rosNodePrefix, int id);

		/*!
		 * DS Time Manager and Network Monitor
		 */
		TimeManager* timeManager = nullptr;

		/*!
		 * Store for Consensus Variables
		 */
		CVariableStore* variableStore = nullptr;

		/*!
		 * Interface to manipulate the cace variable space
		 */
		CaceSpace* caceSpace = nullptr;

		/*!
		 * Interface that realizes the consistency protocol
		 */
		CommunicationWorker* worker = nullptr;

		//double sleepTime = 0.033;
		double sleepTime = 0.01;
#ifdef USE_ROS
		ros::Timer timer;
		void rosStep(const ros::TimerEvent&);
#else
		bool isActive;
		thread* timer;
		void timerThread();
#endif

		/// <summary>
		/// List of Active Robots
		/// </summary>
		vector<string> localScope;
		vector<int> activeRobots;

		/*!
		 * Performs one processing iteration (should be called each robot iteration if cace does not run asynchronously)
		 */
		void step();

		/*!
		 * Lets Cace Run asynchronously
		 */
		void run();

		/*!
		 * Halts asynchronous run
		 */
		void halt();

		vector<int>* getActiveRobots();

		/*!
		 * When Set to true all Jobs send Messageupdates und Messages only after Step
		 * Note: Command Messages are still sent immidiatly
		 */
		bool safeStepMode;

		/*!
		 * Indicates whether quietmode is set
		 */
		bool quietMode;

		string ownNetworkStatusString();

		/*!
		 * Returns printable string of message queue states
		 */
		string printMessageQueueStates();

		/*!
		 * Returns printable string of robots within the team
		 */
		string printActiveRobots();

		/*!
		 * Returns the Global address to determine valid variables
		 */
		string getGlobalScope();

		/*!
		 * Changes the "own" scope and restores consistency for all variables of the new scope
		 * w.r.t. their consistency level
		 */
		void changeScopeAndRestoreConsistency(vector<string>& localScope);

		/*!
		 * returns "own" context
		 */
		vector<string>& getLocalScope();

		/*!
		 * Returns the local address to determine valid variables
		 */
		string getLocalScopeString();

		/*!
		 * should be called to notify the presence of a new agent with 'id'.#
		 * If sendBelieveUpdates is true the agents database will be restored
		 * multiple calls do not have any effect
		 */
		void agentEngangement(int id, bool sendBeliveUpdates);

		/*!
		 * Handles disengagement of Agent 'id'
		 */
		void agentDisengangement(int id);
	};
}

#endif /* CACE_H_ */
