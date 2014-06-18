#include <cace/CaceBelieveNotification.h>
#include <cace/CaceCommand.h>
#include <cace.h>
#include <caceSpace.h>
#include <communication/CaceCommunicationQuiet.h>
#include <communication/CommunicationWorker.h>
#include <communication/jobs/BelieveAcknowledgeJob.h>
#include <communication/jobs/CommandAcknowledgeJob.h>
#include <ros/rate.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <variableStore/CVariableStore.h>
#include <list>
#include <memory>

//#include <string>

namespace cace
{

	Cace* Cace::get()
	{
		static Cace instance;
		return &instance;
	}

	Cace::~Cace()
	{
		//cout << "Killing Communication ..." << endl;
		destroy();
		//cout << "Deleting Variable Store ..." << endl;
		delete variableStore;
		//cout << "Delteing worker ..." << endl;
		delete worker;
		//cout << "Deleting TimeManage ..." << endl;
		delete timeManager;
		//cout << "Cace Cleanup: Done" << endl;
	}


	Cace::Cace(string prefix, int id, bool quiet)
	{
		communication=nullptr;
		variableStore=nullptr;
		worker=nullptr;
		timeManager=nullptr;
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
				communication = new CaceCommunication(worker, nodeprefix, this);
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
		communication = new CaceCommunication(worker, rosNodePrefix, this);
		communication->setOwnID((short)id);
		timeManager = new TimeManager(communication);
	}

	void Cace::step(const ros::TimerEvent& e)
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
		timer = communication->rosNode->createTimer(ros::Duration(0.033), &Cace::step, this, false);
		communication->spinner->start();
	}

	vector<int>* Cace::getActiveRobots()
	{
		return &activeRobots;
	}

	string Cace::ownNetworkStatusString()
	{
		return string("Not implemented");
		/*		string allines = null;
		 try {
		 StringBuilder sb = new StringBuilder();
		 using (StreamReader sr = new StreamReader("/proc/net/wireless"))
		 {
		 String line;
		 line = sr.ReadLine();
		 if(line!=null) line = sr.ReadLine();
		 // Read and display lines from the file until the end of
		 // the file is reached.
		 while ((line = sr.ReadLine()) != null)
		 {
		 sb.AppendLine(line);
		 }
		 }
		 allines = sb.ToString().Replace("\n","");
		 } catch (Exception e)
		 {
		 //Inter-| sta-|   Quality        |   Discarded packets               | Missed | WE
		 // face | tus | link level noise |  nwid  crypt   frag  retry   misc | beacon | 22
		 return " wlan0: 0001   0  0  0        0      0      0    0     0        0 ";
		 }*/
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
			variableStore->sendBelieveUpdates(id, scope, *worker, timeManager->lamportTime);
		}

	}

	void Cace::agentDisengangement(int id)
	{
		for (int i = 0; i < activeRobots.size(); i++)
		{
			if (activeRobots.at(i) == id)
			{
				activeRobots.erase(activeRobots.begin() + i);
				i--;
			}
		}
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
}

