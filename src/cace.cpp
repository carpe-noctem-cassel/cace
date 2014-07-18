#include <cace/CaceBelieveNotification.h>
#include <cace/CaceCommand.h>
#include <cace.h>
#include <caceSpace.h>
#include <communication/CaceCommunicationQuiet.h>
#include <communication/CaceCommunicationMultiCast.h>
#include <communication/CaceCommunicationRos.h>
#include <communication/CommunicationWorker.h>
#include <communication/jobs/BelieveAcknowledgeJob.h>
#include <communication/jobs/CommandAcknowledgeJob.h>
#include <ros/duration.h>
//#include <ros/rate.h>
#include <ros/spinner.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <variableStore/CVariableStore.h>
#include <iterator>
#include <list>
#include <memory>
#include <sstream>
#include <fstream>
#include <chrono>

//#include <string>

//#define USE_ROS

namespace cace
{

	Cace* Cace::get()
	{
		static Cace instance;
		return &instance;
	}

	Cace::~Cace()
	{
#ifndef USE_ROS
		if (isActive && timer != nullptr)
		{
			isActive = false;
			timer->join();
			delete timer;
		}
#endif
		//cout << "Killing Communication ..." << endl;
		destroy();
		delete communication;
		//cout << "Deleting Variable Store ..." << endl;
		delete caceSpace;
		delete variableStore;
		//cout << "Delteing worker ..." << endl;
		delete worker;
		//cout << "Deleting TimeManage ..." << endl;
		delete timeManager;
		//cout << "Cace Cleanup: Done" << endl;
	}

	Cace::Cace(string prefix, int id, bool quiet)
	{
#ifndef USE_ROS
		timer = nullptr;
		isActive = false;
#endif
		communication = nullptr;
		variableStore = nullptr;
		worker = nullptr;
		timeManager = nullptr;
		init(prefix, id, quiet);
	}

	void Cace::init(string prefix, int id, bool quiet)
	{
		quietMode = false;
		safeStepMode = false;
		string nodeprefix = prefix;

		//setup communication
		if (variableStore == nullptr)
		{
			variableStore = new CVariableStore(this);
		}

		if (communication == nullptr)
		{
			this->worker = new CommunicationWorker();
			if (quiet)
			{
				communication = new CaceCommunicationQuiet(worker, nodeprefix, this, (short)id);
			}
			else
			{
#ifdef USE_ROS
				communication = new CaceCommunicationRos(worker, nodeprefix, this);
#else
				communication = new CaceCommunicationMultiCast(worker, nodeprefix, this);
#endif
			}
			if (id != 0)
				communication->setOwnID((short)id);
			timeManager = new TimeManager(communication);

			caceSpace = new CaceSpace(variableStore, worker, this);
		}
		activeRobots.clear();
		localScope.clear();
	}

	void Cace::substituteCaceCommunication(CaceCommunication* cc)
	{
		communication->cleanUp();
		delete communication;
		communication = cc;
		delete timeManager;
		timeManager = new TimeManager(communication);
	}

	void Cace::setQuiet(string rosNodePrefix, short id)
	{
		quietMode = true;
		substituteCaceCommunication(new CaceCommunicationQuiet(worker, rosNodePrefix, this, id));
	}

	void Cace::unsetQuiet(string rosNodePrefix, int id)
	{
		communication->cleanUp();
		delete communication;
		quietMode = false;
#ifdef USE_ROS
		communication = new CaceCommunicationRos(worker, rosNodePrefix, this);
#else
		communication = new CaceCommunicationMultiCast(worker, rosNodePrefix, this);
#endif
		communication->setOwnID((short)id);
		timeManager = new TimeManager(communication);
	}

#ifdef USE_ROS
	void Cace::rosStep(const ros::TimerEvent& e)
	{
		step();
	}
#else
	void Cace::timerThread()
	{
		int t = sleepTime * 1000.0;
		while (isActive)
		{
			this_thread::sleep_for(chrono::milliseconds(t));
			step();
		}
	}
#endif

	void Cace::step()
	{
		//Node this is only required in stepwise mode! Nevertheless it doesn't hurt!
		communication->step();

		//Module Steps
		timeManager->step();

		//process arrived messages

		//first commands
		list<CaceCommandPtr> commands = communication->getCommands();
		for (CaceCommandPtr cc : commands)
		{
			vector<int> nall = activeRobots;
			shared_ptr<ConsensusVariable> np;
			worker->appendJob(
					new CommandAcknowledgeJob(cc->variableName, np, nall, timeManager->lamportTime, this, cc));
		}

		//believe notifcations
		list<CaceBelieveNotificationPtr> notifications = communication->getCaceBelieveNotifications();
		for (CaceBelieveNotificationPtr cbn : notifications)
		{
			vector<int> nall;
			nall.push_back(cbn->senderID);
			shared_ptr<ConsensusVariable> np;
			worker->appendJob(
					new BelieveAcknowledgeJob(cbn->variableName, np, nall, timeManager->lamportTime, this, cbn));
		}

		//process own jobs
		worker->processJobs();

		//remove agents that didn't respond to jobs from list
		for (int id : worker->agentsToRemove)
		{
			agentDisengangement(id);
		}
		worker->agentsToRemove.clear();

		//only in quietmode: cleanup message queues, when no jobs have to be done
		if (quietMode)
		{
			if (worker->count() == 0)
			{
				communication->clearAllMessageLists();
			}
		}

		//check for expired variables
		variableStore->invalidate(*timeManager);

		//sendEvalString
		string evalString = timeManager->toString() + variableStore->toStringNoNewLine() + worker->toStringNoNewLine(); /*OwnNetworkStatusString()*/
		communication->sendEvalString(evalString);

	}

	void Cace::run()
	{
#ifdef USE_ROS
		timer = ((CaceCommunicationRos*)communication)->rosNode.createTimer(ros::Duration(sleepTime), &Cace::rosStep, this, false);
		communication->startAsynchronous();
#else
		isActive = true;
		timer = new std::thread(&Cace::timerThread, this);
#endif
	}

	vector<int>* Cace::getActiveRobots()
	{
		return &activeRobots;
	}

	string Cace::ownNetworkStatusString()
	{
		ifstream ifs("/proc/net/wireless");
		string line;
		//first two lines are just comments
		if (!ifs.eof())
		{
			std::getline(ifs, line);
		}
		if (!ifs.eof())
		{
			std::getline(ifs, line);
		}
		// Read and display lines from the file until the end of
		// the file is reached.
		stringstream ss;
		while (!ifs.eof())
		{
			std::getline(ifs, line);
			ss << line << endl;
		}

//Inter-| sta-|   Quality        |   Discarded packets               | Missed | WE
// face | tus | link level noise |  nwid  crypt   frag  retry   misc | beacon | 22
//" wlan0: 0001   0  0  0        0      0      0    0     0        0 ";
		return ss.str();
	}

	string Cace::printMessageQueueStates()
	{
		return worker->toString();
	}

	string Cace::printActiveRobots()
	{
		stringstream ss;
		for (int i : activeRobots)
		{
			ss << i << " ";
		}
		return ss.str();
	}

	string Cace::getGlobalScope()
	{
		stringstream ss;
		ss << communication->getOwnID();
		ss << "/";
		for (string& s : localScope)
		{
			ss << s << "/";
		}
		return ss.str();
	}

	vector<string>& Cace::getLocalScope()
	{
		return localScope;
	}

	string Cace::getLocalScopeString()
	{
		stringstream ss;
		ss << "/";
		for (string& s : localScope)
		{
			ss << s << "/";
		}
		return ss.str();
	}

	void Cace::agentEngangement(int id, bool sendBeliveUpdates)
	{
		if (id == communication->getOwnID())
			return;
		for (int i = 0; i < activeRobots.size(); i++)
		{
			if (activeRobots.at(i) == id)
			{
				return;
			}
		}
		activeRobots.push_back(id);
		if (sendBeliveUpdates)
		{
			string scope = getLocalScopeString();
			if (scope.length() > 0 && scope[0] == '/')
			{
				scope = scope.substr(1, scope.length() - 1);
			}
			if (scope.length() == 0 || (scope.length() > 0 && scope[scope.length() - 1] != '/'))
			{
				scope = scope.append("/");
			}
			variableStore->sendBelieveUpdates(id, scope, *worker, timeManager->lamportTime);
		}

	}

	void Cace::agentDisengangement(int id)
	{
		/*for (int i = 0; i < activeRobots.size(); i++)
		{
			if (activeRobots.at(i) == id)
			{
				activeRobots.erase(activeRobots.begin() + i);
				i--;
			}
		}*/
	}

	Cace* Cace::getEmulated(string prefix, int id, bool quiet)
	{
		return new Cace(prefix, id, quiet);
	}

	void Cace::destroy()
	{
		//destroy communication
		if (communication != nullptr)
		{
			communication->anounceDisengange();
			communication->cleanUp();
		}
	}

	void Cace::changeScopeAndRestoreConsistency(vector<string>& localScope)
	{
		this->localScope = localScope;
		string scope = getLocalScopeString();
		if (scope.length() > 0 && scope[0] == '/')
		{
			scope = scope.substr(1, scope.length() - 1);
		}
		if (scope.length() == 0 || (scope.length() > 0 && scope[scope.length() - 1] != '/'))
		{
			scope = scope.append("/");
		}
		for (int id : activeRobots)
		{
			variableStore->sendBelieveUpdates(id, scope, *worker, timeManager->lamportTime);
		}
	}
}
