/*
 * caceMonitor.cpp
 *
 *  Created on: 17.06.2014
 *      Author: endy
 */

#include <cace.h>
#include <caceSpace.h>
#include <communication/CaceCommunication.h>
#include <communication/CommunicationWorker.h>
#include <CaceTypes.h>
#include <ros/init.h>
#include <SystemConfig.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <variableStore/CVariableStore.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdbool>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace cace;

class CaceConsole
{

public:
	Cace* cace = nullptr;
	CaceConsole(bool quiet)
	{
		cace = Cace::getEmulated("", 0, quiet);
		cace->run();
	}

	int checkType(string& val)
	{
		stringstream ss(val);
		int ival;
		ss >> ival;
		if (!ss.fail() && val.find(".") != string::npos)
		{
			return CaceType::CDouble;
		}
		else if (!ss.fail())
		{
			return CaceType::CInt;
		}
		else
		{
			return CaceType::CString;
		}
	}

	vector<string> split(const string& str, int delimiter(int) = ::isspace)
	{
		vector<string> result;
		auto e = str.end();
		auto i = str.begin();
		while (i != e)
		{
			i = find_if_not(i, e, delimiter);
			if (i == e)
				break;
			auto j = find_if(i, e, delimiter);
			result.push_back(string(i, j));
			i = j;
		}
		return result;
	}

	void run()
	{
		while (ros::ok())
		{
			cout << cace->getGlobalScope() << "> ";
			string cmd;
			std::getline(std::cin, cmd);
			std::transform(cmd.begin(), cmd.end(), cmd.begin(),
							std::bind2nd(std::ptr_fun(&std::tolower<char>), std::locale("")));

			stringstream cmds(cmd);

			string tmp;
			cmds >> tmp;
			int numberOfWords = 1;
			for (int i = 0; i < cmd.length(); i++)
			{
				if (cmd[i] == ' ')
				{
					numberOfWords++;
				}
			}

			if (tmp == string("cmd") || tmp == string("set"))
			{
				acceptStrategy level = acceptStrategy::ThreeWayHandShake;
				if (tmp == string("set"))
					level = acceptStrategy::NoDistribution;
				int temp = 0;
				if (numberOfWords > 2)
				{
					string ctxt = cace->getLocalScopeString();
					string s;
					cmds >> s;
					string name = ctxt.substr(1, ctxt.length() - 1) + s;

					vector<string> cmdVector;
					while (!cmds.fail())
					{
						string next;
						cmds >> next;
						cmdVector.push_back(next);
					}

					auto v1 = make_shared<ConsensusVariable>(name, level, std::numeric_limits<long>::max(),
																cace->communication->getOwnID(),
																cace->timeManager->getDistributedTime(),
																cace->timeManager->lamportTime, CaceType::Custom);

					if (numberOfWords == 3)
					{
						//distinguish all non list types:
						if (checkType(cmdVector.at(0)) == CaceType::CDouble)
						{
							v1->setValue(stod(cmdVector.at(0)));
						}
						else if (checkType(cmdVector.at(0)) == CaceType::CInt)
						{
							v1->setValue(stoi(cmdVector.at(0)));
						}
						else
						{
							v1->setValue(&cmdVector.at(0));
						}
					}

					cmdVector.pop_back();
					if (numberOfWords > 3)
					{
						//distinguish all list types by the first value:
						if (checkType(cmdVector[0]) == CaceType::CDouble)
						{
							vector<double> list;
							for (string s : cmdVector)
							{
								list.push_back(stod(s));
							}
							v1->setValue(&list);
						}
						else if (checkType(cmdVector[0]) == CaceType::CInt)
						{
							vector<int> list;
							for (string s : cmdVector)
							{
								list.push_back(stoi(s));
							}
							v1->setValue(&list);
						}
						else
						{
							v1->setValue(&cmdVector);
						}
					}
					cout << "CMD " << name << "\t" << v1->valueAsString() << endl;
					cace->caceSpace->distributeVariable(v1);
				}
				else
				{
					cout << "Syntax: CMD/SET [Address/Name] [Value]" << endl;
				}
			}
			else if (tmp == string("add"))
			{
				if (numberOfWords > 1)
				{
					for (int i = 1; i < numberOfWords; i++)
					{
						string s;
						cmds >> s;
						cace->agentEngangement(stoi(s), false);
					}
				}
				else
				{
					cout << "Syntax: ADD RobotID [RobotID2 ...]" << endl;
				}
			}
			else if (tmp == string("addb"))
			{
				if (numberOfWords > 1)
				{
					for (int i = 1; i < numberOfWords; i++)
					{
						string s;
						cmds >> s;
						cace->agentEngangement(stoi(s), true);
					}
				}
				else
				{
					cout << "Syntax: ADDB RobotID" << endl;
				}
			}
			else if (tmp == string("rem"))
			{
				if (numberOfWords > 1)
				{
					for (int i = 1; i < numberOfWords; i++)
					{
						string s;
						cmds >> s;
						cace->agentDisengangement(stoi(s));
					}
				}
				else
				{
					cout << "Syntax: REM RobotID [RobotID2 ...]";
				}
			}
			else if (tmp == string("time") || tmp == string("t"))
			{
				cout << "Distributed Time: " << cace->timeManager->getDistributedTime() << "\tTimeOffest: "
						<< cace->timeManager->timeDiff << endl;
			}
			else if (tmp == string("store") || tmp == string("s"))
			{
				cout << cace->variableStore->toString() << endl;
			}
			else if (tmp == string("scope") || tmp == string("ls"))
			{
				string scope = cace->getLocalScopeString();
				cout << cace->variableStore->toString(scope);
			}
			else if (tmp == string("robots") || tmp == string("r"))
			{
				cout << "Active Robots: -";
				for (int i : cace->activeRobots)
				{
					cout << i << "-";
				}
				cout << endl;
			}
			else if (tmp == string("jobs") || tmp == string("j"))
			{
				cout << cace->worker->toString() << endl;
			}
			else if (tmp == string("queues") || tmp == string("q"))
			{
				cout << cace->printMessageQueueStates() << endl;
			}
			else if (tmp == string("req"))
			{
				if (numberOfWords > 2)
				{
					string name;
					int agentId;
					cmds >> name;
					cmds >> agentId;
					cace->caceSpace->requestVariable(name, agentId);
					this_thread::sleep_for(chrono::milliseconds(250));
					shared_ptr<ConsensusVariable> cv = cace->caceSpace->getRequestedVariable(name);
					if (cv.operator bool())
					{
						cout << cv->toString();
					}
				}
				else
				{
					cout << "Syntax: REQ Variablename AgentID" << endl;
				}
			}
			else if (tmp == string("write") || tmp == string("w"))
			{
				acceptStrategy level = acceptStrategy::NoDistribution;
				int temp = 0;
				if (numberOfWords > 2)
				{
					string ctxt = cace->getLocalScopeString();
					int targetAgent;
					cmds >> targetAgent;
					string s;
					cmds >> s;
					string name = ctxt.substr(1, ctxt.length() - 1) + s;

					shared_ptr<ConsensusVariable> writeVar = make_shared<ConsensusVariable>(
							name, level, std::numeric_limits<long>::max(), targetAgent,
							cace->timeManager->getDistributedTime(), cace->timeManager->lamportTime, 0);

					if (numberOfWords == 4)
					{
						string firstVal;
						cmds >> firstVal;
						//distinguish all non list types:
						if (firstVal.find(".") != string::npos)
						{
							string val;
							cmds >> val;
							cout << "WRITE to " << targetAgent << ": " << name << "\t" << stod(val) << "d" << endl;
							writeVar->setValue(stod(val));
						}
						/*else if (Int32.TryParse(cmds[3], out temp))
						 {
						 Console.WriteLine("WRITE to " + cmds[1] + ": " + cmds[2] + "\t" + temp + "i");
						 writeVar.SetValue(temp);
						 }
						 else if (cmds[3].Split(new char[] {','}, StringSplitOptions.None).Length == 2)
						 {
						 string[] p = cmds[3].Split(new char[]
						 {	','}, StringSplitOptions.None);
						 Console.WriteLine("WRITE to " + cmds[1] + ": " + "\t(" + p[0] + "," + p[1] + ")");
						 writeVar.SetValue(double.Parse(p[0]), double.Parse(p[1]));
						 */
					}
					else
					{
						string val;
						cmds >> val;
						cout << "WRITE to " << targetAgent << ": \t" << val << endl;
						//cace->caceSpace->distributeValue(name, val, level);
						//writeVar->setValue(cmds[3]);
					}

					if (numberOfWords > 4)
					{
						/*//distinguish all list types by the first value:
						 if (cmds[3].Contains("."))
						 {
						 List<double> dlist = new List<double>();
						 Console.Write("WRITE to " + cmds[1] + ": " + cmds[2]);
						 for (int i = 3; i < cmds.Length; i++)
						 {
						 dlist.Add(double.Parse(cmds[i]));
						 Console.Write("\t" + Double.Parse(cmds[i]) + "d");
						 }
						 Console.WriteLine();
						 writeVar.SetValue(dlist);
						 }
						 else if (Int32.TryParse(cmds[3], out temp))
						 {
						 List<int> dlist = new List<int>();
						 Console.Write("WRITE to " + cmds[1] + ": " + cmds[2]);
						 for (int i = 3; i < cmds.Length; i++)
						 {
						 dlist.Add(Int32.Parse(cmds[i]));
						 Console.Write("\t" + Int32.Parse(cmds[i]) + "i");
						 }
						 Console.WriteLine();
						 writeVar.SetValue(dlist);
						 }
						 else
						 {
						 List<string> dlist = new List<string>();
						 Console.Write("WRITE to " + cmds[1] + ": " + cmds[2]);
						 for (int i = 3; i < cmds.Length; i++)
						 {
						 dlist.Add(cmds[i]);
						 Console.Write("\t" + cmds[i]);
						 }
						 Console.WriteLine();
						 writeVar.SetValue(dlist);
						 }
						 */
					}
					cace->caceSpace->writeVariable(writeVar, targetAgent);
				}
				else
				{
					cout << "Syntax: WRITE AgentID [Address/Name] [Value]" << endl;
				}
			}
			else if (tmp == string("cd"))
			{
				if (numberOfWords <= 1)
				{
					cace->localScope.clear();
				}
				else
				{
					string newScope;
					cmds >> newScope;
					if (newScope.length() > 0 && newScope[0] == '/')
					{
						cace->localScope.clear();
					}
					vector<string> sc = split(newScope);
					for (int i = 0; i < sc.size(); i++)
					//while(newScope.find("/")!=string::npos)
					{
						if (sc.size() == 0)
						{
							continue;
						}
						if (sc[i] == string(".."))
						{
							if (cace->localScope.size() > 0)
							{
								cace->localScope.erase(cace->localScope.end());
							}
						}
						else
						{
							cace->localScope.push_back(sc[i]);
						}
					}
				}
			}
			else if (tmp == string("quiet"))
			{
				if (numberOfWords > 1)
				{
					int id;
					cmds >> id;
					cace->setQuiet("", id);
				}
				else
				{
					cout << "Syntax: QUIET <RobotID>" << endl;
				}
			}
			else
			{
				cout << "Commands:" << endl;
				cout << "\tSTORE: Shows Current Variable Store" << endl;
				cout << "\tCMD: Command a Variable Value" << endl;
				cout << "\tTIME: Returns the Current Distributed Time" << endl;
				cout << "\tSET: Sets a local Variable Value (ConsensusLevel 0)" << endl;
				cout << "\tREQ: Command a Variable Value" << endl;
				cout << "\tWRITE: Write a Variable Value to an remote agent" << endl;
				cout << "\tADD: Enganges a robot locally (without believe updates)" << endl;
				cout << "\tADDB: Enganges a robot locally (with believe updates)" << endl;
				cout << "\tREM: Disenganges a robot locally" << endl;
				cout << "\tJOBS: List of Open Jobs" << endl;
				cout << "\tQUEUES: Current Message Queue Sizes" << endl;
				cout << "\tTIME: Shows Distributed Time" << endl;
				cout << "\tQUIET: Sets Monitor to pure observing mode" << endl;
				cout << "\tCD: Changes Local Context" << endl;
			}

		}
	}
};

int main(int argc, char **argv)
{
	bool quiet = false;
	for (int i = 0; i < argc; i++)
	{
		if (string(argv[i]) == string("quiet"))
		{
			quiet = true;
			break;
		}
	}

	supplementary::SystemConfig *sc = supplementary::SystemConfig::getInstance();
	ros::init(argc, argv, sc->getHostname() + string("Cace"));

	CaceConsole cc(quiet);
	cc.run();

	return 1;
}
