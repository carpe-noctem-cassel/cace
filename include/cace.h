/*
 * cace.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */
#ifndef CACE_H_
#define CACE_H_

#include <iostream>
#include <memory>
#include <vector>

#include "timeManager/TimeManager.h"
#include "communication/CaceCommunication.h"
#include "variableStore/CVariableStore.h"

using namespace std;

namespace cace
{
	class CVariableStore;

	class Cace
	{
	private:
		Cace();
		~Cace();
		Cace(string prefix, int id);
		Cace(string prefix, int id, bool quiet);

	protected:
		void init(string prefix, int id) {init(prefix, id, false);}
		void init(string prefix, int id, bool quiet);

		vector<string> localScope;
		vector<int> activeRobots;
		CaceCommunication* communication;

	public:
		Cace* get();
		void substituteCaceCommunication(CaceCommunication cc);
		void setQuiet(string rosNodePrefix, short id);
		void unsetQuiet(string rosNodePrefix, int id);
		CaceCommunication* getCommunication();

		TimeManager* timeManager;
		CVariableStore* variableStore;

		void step();
		void run();

		vector<int>* getActiveRobots();

		string& ownNetworkStatusString();
		string& printMessageQueueStates();
		string& printActiveRobots();
		string& getGlobalScope();
		vector<string>& getLocalScope();
		string getLocalScopeString();
		void agentEngangement(int id, bool sendBeliveUpdates);
		void agentDisengangement(int id);
		string& toStringActiveRobots();
	};
}

#endif /* CACE_H_ */
