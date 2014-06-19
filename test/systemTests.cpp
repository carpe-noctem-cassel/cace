#include <caceSpace.h>
#include <communication/CaceCommunication.h>
#include <timeManager/TimeManager.h>
#include <variableStore/CVariableStore.h>
#include <cace/CaceType.h>
#include <cace.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-internal.h>
#include <ros/forwards.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace cace;

//Cace* cace;
class SystemTests : public ::testing::Test
{
public:
	static int iteration;
protected:

	vector<Cace*> cace;
	vector<int> allCaces;
	vector<vector<int>> allCacePertubations;
	int round = 0;
	int participants = 3;

	SystemTests()
	{
		// You can do set-up work for each test here.
	}

	virtual ~SystemTests()
	{
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp()
	{
		vector<int> empty;
		computePertubations(allCacePertubations, empty);
		cout << "Possible Pertubations: " << allCacePertubations.size() << " Current Iteration: " << iteration << endl;
		round = iteration++;

		cace.clear();

		for (int i = 0; i < participants; i++)
		{
			char name = (char)((int)'A' + i);
			cace.push_back(Cace::getEmulated("" + name, i + 1));
			cace[i]->safeStepMode = true;
		}

		allCaces = allCacePertubations[round % allCacePertubations.size()];
		for (int i = 0; i < allCaces.size(); i++)
		{
			cout << allCaces.at(i) << " ";
		}
		cout << endl;
	}

	virtual void TearDown()
	{
		for (Cace* c : cace)
		{
			delete c;
		}
	}

	void Step(vector<int>& cacesToStep)
	{
		ros::TimerEvent e;
		for (int i : cacesToStep)
		{
			if (i >= 0)
				cace[i]->step(e);
		}
	}

	void computePertubations(vector<vector<int>>& allCacePertubations, vector<int> init)
	{
		if (init.size() == participants)
		{
			allCacePertubations.push_back(init);
			return;
		}

		for (int num = 0; num < participants; num++)
		{
			bool contains = false;

			for (int i : init)
			{
				if (i == num)
				{
					contains = true;
					break;
				}
			}
			if (!contains)
			{
				vector<int> newList(init);
				newList.push_back(num);
				computePertubations(allCacePertubations, newList);
				break;
			}

		}
	}

};

int SystemTests::iteration;

TEST_F(SystemTests, Request)
{
	cace[0]->activeRobots.clear();
	cace[0]->agentEngangement(1, false);
	cace[0]->agentEngangement(2, false);

	cace[1]->activeRobots.clear();
	cace[1]->agentEngangement(1, false);
	cace[1]->agentEngangement(2, false);
	string name = "req" + to_string(round);
	//this_thread::sleep_for(chrono::milliseconds(200));

	cace[0]->caceSpace->distributeValue(name, 1.0, acceptStrategy::NoDistribution);
	Step(allCaces);
	Step(allCaces);
	EXPECT_TRUE(cace[0]->caceSpace->getVariable(name).operator bool()) << "Variable not even locally available";

	EXPECT_FALSE(cace[1]->caceSpace->checkAvailableResponse(name)) << "Response Available before";
	cace[1]->caceSpace->requestVariable(name, 1);

	for (int i = 0; i < 10; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}
	EXPECT_TRUE(cace[1]->caceSpace->checkAvailableResponse(name)) << "Response Available after";

	shared_ptr<ConsensusVariable> cv = cace[1]->caceSpace->getRequestedVariable(name);
	EXPECT_TRUE(cv.operator bool());
	double o = 0;
	cv->getValue(&o);
	EXPECT_DOUBLE_EQ(1.0, o) << "Wrong value transfered";
}

TEST_F(SystemTests, WriteRequest)
{
	cace[0]->activeRobots.clear();
	cace[0]->agentEngangement(1, false);
	cace[0]->agentEngangement(2, false);

	cace[1]->activeRobots.clear();
	cace[1]->agentEngangement(1, false);
	cace[1]->agentEngangement(2, false);
	string name = "reqwrite" + to_string(round);
	this_thread::sleep_for(chrono::milliseconds(200));

	shared_ptr<ConsensusVariable> Wvar = make_shared<ConsensusVariable>(name, acceptStrategy::NoDistribution,
																		std::numeric_limits<long>::max(), 1,
																		cace[1]->timeManager->getDistributedTime(),
																		cace[1]->timeManager->lamportTime,
																		CaceType::CInt);
	Wvar->setValue((double)1);
	cace[1]->caceSpace->writeVariable(Wvar, 1);

	for (int i = 0; i < 10; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}

	EXPECT_TRUE(cace[0]->caceSpace->getVariable(name).operator bool())
			<< "Variable not even locally available: Write didn't work!\n" << cace[0]->variableStore->toString();

	EXPECT_FALSE(cace[1]->caceSpace->checkAvailableResponse(name)) << "Response Available before";
	cace[1]->caceSpace->requestVariable(name, 1);

	for (int i = 0; i < 10; i++)
	{
		std::chrono::milliseconds dura(10);
		std::this_thread::sleep_for(dura);
		Step(allCaces);
	}
	EXPECT_TRUE(cace[1]->caceSpace->checkAvailableResponse(name)) << "Response Available after";
	shared_ptr<ConsensusVariable> cv = cace[1]->caceSpace->getRequestedVariable(name);
	EXPECT_TRUE(cv.operator bool());
	double o = 0;
	cv->getValue(&o);
	EXPECT_DOUBLE_EQ(1, o) << "Wrong value transfered";
}

TEST_F(SystemTests, Command)
{
	for (int i = 0; i < cace.size(); i++)
	{
		cace[i]->activeRobots.clear();
		for (int j = 0; j < cace.size(); j++)
		{
			cace[i]->agentEngangement(j + 1, false);
		}
	}

	string varName = "A";
	double s = 1;
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::ThreeWayHandShake);

	this_thread::sleep_for(chrono::milliseconds(10));
	Step(allCaces);
	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = cace[1]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";
	EXPECT_TRUE(v2.operator bool()) << "3: Didn't receive Variable";

	double val1 = 0, val2 = 0, val3 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	v2->getValue(&val3);
	EXPECT_DOUBLE_EQ(s, val1) << "Own Believe1 is Not Correct";
	EXPECT_DOUBLE_EQ(s, val2) << "Own Believe2 is Not Correct";
	EXPECT_DOUBLE_EQ(s, val3) << "Own Believe3 is Not Correct";

	this_thread::sleep_for(chrono::milliseconds(10));
	Step(allCaces);
	EXPECT_EQ(v0->proposals.size(), cace.size()-1) << "v0: Wrong numer of Robot Believes";
	EXPECT_EQ(v1->proposals.size(), cace.size()-1) << "v1: Wrong numer of Robot Believes";
	EXPECT_EQ(v2->proposals.size(), cace.size()-1) << "v2: Wrong numer of Robot Believes";
}

TEST_F(SystemTests, CommandUpdate)
{
	for (int i = 0; i < cace.size(); i++)
	{
		cace[i]->activeRobots.clear();
		for (int j = 0; j < cace.size(); j++)
		{
			cace[i]->agentEngangement(j + 1, false);
		}
	}

	string varName = "AU";
	double s = 1;
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::ThreeWayHandShake);

	this_thread::sleep_for(chrono::milliseconds(10));
	Step(allCaces);
	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = cace[1]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";
	EXPECT_TRUE(v2.operator bool()) << "3: Didn't receive Variable";

	double val1 = 0, val2 = 0, val3 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	v2->getValue(&val3);
	EXPECT_DOUBLE_EQ(s, val1) << "Own Believe1 is Not Correct";
	EXPECT_DOUBLE_EQ(s, val2) << "Own Believe2 is Not Correct";
	EXPECT_DOUBLE_EQ(s, val3) << "Own Believe3 is Not Correct";
	EXPECT_EQ(cace.size() - 1, cace[0]->activeRobots.size()) << "1";
	EXPECT_EQ(cace.size() - 1, cace[1]->activeRobots.size()) << "1";
	EXPECT_EQ(cace.size() - 1, cace[2]->activeRobots.size()) << "2";

	for (int i = 0; i < 10; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}
	EXPECT_EQ(cace.size() - 1, cace[0]->activeRobots.size()) << "4:" + cace[0]->printActiveRobots();
	EXPECT_EQ(cace.size() - 1, cace[1]->activeRobots.size()) << "5:" + cace[1]->printActiveRobots();
	EXPECT_EQ(cace.size() - 1, cace[2]->activeRobots.size()) << "6:" + cace[2]->printActiveRobots();
	EXPECT_EQ(v0->proposals.size(), 2) << "v0: Wrong numer of Robot Believes";
	EXPECT_EQ(v1->proposals.size(), 2) << "v1: Wrong numer of Robot Believes";
	EXPECT_EQ(v2->proposals.size(), 2) << "v2: Wrong numer of Robot Believes";

	EXPECT_TRUE(v0->getType() == CaceType::CDouble) << "v0 Wrong Variable Type";
	EXPECT_TRUE(v1->getType() == CaceType::CDouble) << "v1 Wrong Variable Type";
	EXPECT_TRUE(v2->getType() == CaceType::CDouble) << "v2 Wrong Variable Type";

	EXPECT_EQ(cace.size() - 1, cace[0]->activeRobots.size()) << "7";
	EXPECT_EQ(cace.size() - 1, cace[1]->activeRobots.size()) << "8";
	EXPECT_EQ(cace.size() - 1, cace[2]->activeRobots.size()) << "9";
	double s2 = 2;
	cace[1]->caceSpace->distributeValue(varName, s2, acceptStrategy::ThreeWayHandShake);

	for (int i = 0; i < 10; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}
	EXPECT_EQ(cace.size() - 1, cace[0]->activeRobots.size()) << "10";
	EXPECT_EQ(cace.size() - 1, cace[1]->activeRobots.size()) << "11";
	EXPECT_EQ(cace.size() - 1, cace[2]->activeRobots.size()) << "12";

	double curVal = -3.0;
	EXPECT_TRUE(v0->getType() == CaceType::CDouble) << "v0 Wrong Variable Type";
	v0->getValue(&curVal);
	EXPECT_DOUBLE_EQ(s2, curVal) << "B. Own Believe1 is Not Correct\n" << v0->toString();
	EXPECT_TRUE(v1->getType() == CaceType::CDouble) << "v1 Wrong Variable Type";
	v1->getValue(&curVal);
	EXPECT_DOUBLE_EQ(s2, curVal) << "B. Own Believe1 is Not Correct\n" << v1->toString();
	EXPECT_TRUE(v2->getType() == CaceType::CDouble) << "v2 Wrong Variable Type";
	v2->getValue(&curVal);
	EXPECT_DOUBLE_EQ(s2, curVal) << "B. Own Believe1 is Not Correct\n" << v2->toString();

	EXPECT_DOUBLE_EQ(s2, curVal) << "B. Own Believe1 is Not Correct\n" << v0->toString();
	EXPECT_DOUBLE_EQ(s2, curVal) << "B. Own Believe1 is Not Correct\n" << v1->toString();
	EXPECT_TRUE(v2->getType() == CaceType::CDouble) << "v2 Wrong Variable Type";

	v0->proposals.at(0)->getValue(&curVal);
	EXPECT_EQ(s2, curVal) << "1Wrong other Believe0 is Not Correct\n" << v0->toString();
	v0->proposals.at(1)->getValue(&curVal);
	EXPECT_EQ(s2, curVal) << "1Wrong other Believe1 is Not Correct\n" << v0->toString();

	v1->proposals.at(0)->getValue(&curVal);
	EXPECT_EQ(s2, curVal) << "2Wrong other Believe0 is Not Correct\n" << v1->toString();
	v1->proposals.at(1)->getValue(&curVal);
	EXPECT_EQ(s2, curVal) << "2Wrong other Believe1 is Not Correct\n" << v1->toString();

	v2->proposals.at(0)->getValue(&curVal);
	EXPECT_EQ(s2, curVal) << "3Wrong other Believe0 is Not Correct\n" << v2->toString();
	v2->proposals.at(1)->getValue(&curVal);
	EXPECT_EQ(s2, curVal) << "3Wrong other Believe1 is Not Correct\n" << v2->toString();
}

TEST_F(SystemTests, DefaultConflictHandling3)
{
	for (int i = 0; i < cace.size(); i++)
	{
		cace[i]->activeRobots.clear();
		for (int j = 0; j < cace.size(); j++)
		{
			cace[i]->agentEngangement(j + 1, false);
		}
		cace[i]->worker->clearJobs();
		cace[i]->variableStore->deleteAllVariables();
	}
	ros::TimerEvent e;

	string varName = "CH3";
	double s = 1;
	double s2 = 2;
	cace[1]->timeManager->updateLamportTime(cace[0]->timeManager->lamportTime + 1000);
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::ThreeWayHandShake);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[0]->step(e);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[2]->step(e);
	cace[1]->caceSpace->distributeValue(varName, s2, acceptStrategy::ThreeWayHandShake);

	this_thread::sleep_for(chrono::milliseconds(10));
	Step(allCaces);
	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = cace[1]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";
	EXPECT_TRUE(v2.operator bool()) << "3: Didn't receive Variable";

	for (int i = 0; i < 12; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}

	double val1 = 0, val2 = 0, val3 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	v2->getValue(&val3);
	EXPECT_DOUBLE_EQ(s2, val1) << "Own Believe1 is Not Correct\n" << v0->toString();
	EXPECT_DOUBLE_EQ(s2, val2) << "Own Believe2 is Not Correct\n" << v1->toString();
	EXPECT_DOUBLE_EQ(s2, val3) << "Own Believe3 is Not Correct\n" << v2->toString();

	EXPECT_EQ(2, v0->proposals.size()) << "v0: Wrong numer of Robot Believes" << v0->toString();
	EXPECT_EQ(2, v1->proposals.size()) << "v1: Wrong numer of Robot Believes" << v1->toString();
	EXPECT_EQ(2, v2->proposals.size()) << "v2: Wrong numer of Robot Believes" << v2->toString();
}

TEST_F(SystemTests, DefaultConflictHandlingReverseCommandOrder3)
{
	for (int i = 0; i < cace.size(); i++)
	{
		cace[i]->activeRobots.clear();
		for (int j = 0; j < cace.size(); j++)
		{
			cace[i]->agentEngangement(j + 1, false);
		}
		cace[i]->worker->clearJobs();
		cace[i]->variableStore->deleteAllVariables();
	}
	ros::TimerEvent e;

	string varName = "CHRv2";
	double s = 1;
	double s2 = 2;

	cace[1]->timeManager->updateLamportTime(cace[0]->timeManager->lamportTime + 1000);
	cace[1]->caceSpace->distributeValue(varName, s2, acceptStrategy::ThreeWayHandShake);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[1]->step(e);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[2]->step(e);
	this_thread::sleep_for(chrono::milliseconds(10));

	//Reseted because quieck response already updates lamport clock
	cace[0]->timeManager->lamportTime = 1;
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::ThreeWayHandShake);

	this_thread::sleep_for(chrono::milliseconds(10));
	Step(allCaces);

	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = cace[1]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";
	EXPECT_TRUE(v2.operator bool()) << "3: Didn't receive Variable";

	for (int i = 0; i < 10; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}

	double val1 = 0, val2 = 0, val3 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	v2->getValue(&val3);
	EXPECT_EQ(s2, val1) << "Own Believe1 is Not Correct\n" << v0->toString();
	EXPECT_EQ(s2, val2) << "Own Believe2 is Not Correct\n" << v1->toString();
	EXPECT_EQ(s2, val3) << "Own Believe3 is Not Correct\n" << v2->toString();

	EXPECT_EQ(2, v0->proposals.size()) << "v0: Wrong numer of Robot Believes" << v0->toString();
	EXPECT_EQ(2, v1->proposals.size()) << "v1: Wrong numer of Robot Believes" << v1->toString();
	EXPECT_EQ(2, v2->proposals.size()) << "v2: Wrong numer of Robot Believes" << v2->toString();
}

TEST_F(SystemTests, ElectionConflictHandling2)
{
	for (int i = 0; i < cace.size(); i++)
	{
		cace[i]->activeRobots.clear();
		for (int j = 0; j < cace.size(); j++)
		{
			cace[i]->agentEngangement(j + 1, false);
		}
		cace[i]->worker->clearJobs();
		cace[i]->variableStore->deleteAllVariables();
	}
	ros::TimerEvent e;

	string varName = "CH2";
	double s = 1;
	double s2 = 2;
	cace[1]->timeManager->updateLamportTime(cace[0]->timeManager->lamportTime + 1000);
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::TwoWayHandShakeElection);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[0]->step(e);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[2]->step(e);
	cace[1]->caceSpace->distributeValue(varName, s2, acceptStrategy::TwoWayHandShakeElection);

	this_thread::sleep_for(chrono::milliseconds(10));
	Step(allCaces);

	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = cace[1]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";
	EXPECT_TRUE(v2.operator bool()) << "3: Didn't receive Variable";

	for (int i = 0; i < 10; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}

	double val1 = 0, val2 = 0, val3 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	v2->getValue(&val3);
	EXPECT_EQ(s, val1) << "Own Believe1 is Not Correct\n" << v0->toString();
	EXPECT_EQ(s2, val2) << "Own Believe2 is Not Correct\n" << v1->toString();
	EXPECT_NE(s2, val3) << "Own Believe3 is Not Correct\n" << v2->toString();
	EXPECT_NE(s, val3) << "Own Believe3 is Not Correct\n" << v2->toString();

	EXPECT_EQ(2, v0->proposals.size()) << "v0: Wrong numer of Robot Believes" << v0->toString();
	EXPECT_EQ(2, v1->proposals.size()) << "v1: Wrong numer of Robot Believes" << v1->toString();
	EXPECT_EQ(2, v2->proposals.size()) << "v2: Wrong numer of Robot Believes" << v2->toString();
}

TEST_F(SystemTests, DefaultConflictHandling2)
{
	for (int i = 0; i < cace.size(); i++)
	{
		cace[i]->activeRobots.clear();
		for (int j = 0; j < cace.size(); j++)
		{
			cace[i]->agentEngangement(j + 1, false);
		}
		cace[i]->worker->clearJobs();
		cace[i]->variableStore->deleteAllVariables();
	}
	ros::TimerEvent e;

	string varName = "CH2";
	double s = 1;
	double s2 = 2;
	cace[1]->timeManager->updateLamportTime(cace[0]->timeManager->lamportTime + 1000);
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::TwoWayHandShake);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[0]->step(e);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[2]->step(e);
	cace[1]->caceSpace->distributeValue(varName, s2, acceptStrategy::TwoWayHandShake);

	this_thread::sleep_for(chrono::milliseconds(10));
	Step(allCaces);
	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = cace[1]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";
	EXPECT_TRUE(v2.operator bool()) << "3: Didn't receive Variable";

	for (int i = 0; i < 12; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}

	double val1 = 0, val2 = 0, val3 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	v2->getValue(&val3);
	EXPECT_DOUBLE_EQ(s2, val1) << "Own Believe1 is Not Correct\n" << v0->toString();
	EXPECT_DOUBLE_EQ(s2, val2) << "Own Believe2 is Not Correct\n" << v1->toString();
	EXPECT_DOUBLE_EQ(s2, val3) << "Own Believe3 is Not Correct\n" << v2->toString();

	EXPECT_EQ(2, v0->proposals.size()) << "v0: Wrong numer of Robot Believes" << v0->toString();
	EXPECT_EQ(2, v1->proposals.size()) << "v1: Wrong numer of Robot Believes" << v1->toString();
	EXPECT_EQ(2, v2->proposals.size()) << "v2: Wrong numer of Robot Believes" << v2->toString();
}

TEST_F(SystemTests, DefaultConflictHandlingReverseCommandOrder2)
{
	for (int i = 0; i < cace.size(); i++)
	{
		cace[i]->activeRobots.clear();
		for (int j = 0; j < cace.size(); j++)
		{
			cace[i]->agentEngangement(j + 1, false);
		}
		cace[i]->worker->clearJobs();
		cace[i]->variableStore->deleteAllVariables();
	}
	ros::TimerEvent e;

	string varName = "CHRv2";
	double s = 1;
	double s2 = 2;

	cace[1]->timeManager->updateLamportTime(cace[0]->timeManager->lamportTime + 1000);
	cace[1]->caceSpace->distributeValue(varName, s2, acceptStrategy::TwoWayHandShake);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[1]->step(e);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[2]->step(e);
	this_thread::sleep_for(chrono::milliseconds(10));
	//Reseted because quieck response already updates lamport clock
	cace[0]->timeManager->lamportTime = 1;
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::TwoWayHandShake);

	this_thread::sleep_for(chrono::milliseconds(10));
	Step(allCaces);

	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = cace[1]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";
	EXPECT_TRUE(v2.operator bool()) << "3: Didn't receive Variable";

	for (int i = 0; i < 10; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}

	double val1 = 0, val2 = 0, val3 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	v2->getValue(&val3);
	EXPECT_EQ(s2, val1) << "Own Believe1 is Not Correct\n" << v0->toString();
	EXPECT_EQ(s2, val2) << "Own Believe2 is Not Correct\n" << v1->toString();
	EXPECT_EQ(s2, val3) << "Own Believe3 is Not Correct\n" << v2->toString();

	EXPECT_EQ(2, v0->proposals.size()) << "v0: Wrong numer of Robot Believes" << v0->toString();
	EXPECT_EQ(2, v1->proposals.size()) << "v1: Wrong numer of Robot Believes" << v1->toString();
	EXPECT_EQ(2, v2->proposals.size()) << "v2: Wrong numer of Robot Believes" << v2->toString();
}

TEST_F(SystemTests, DefaultConflictHandling1)
{
	for (int i = 0; i < cace.size(); i++)
	{
		cace[i]->activeRobots.clear();
		for (int j = 0; j < cace.size(); j++)
		{
			cace[i]->agentEngangement(j + 1, false);
		}
		cace[i]->worker->clearJobs();
		cace[i]->variableStore->deleteAllVariables();
	}
	ros::TimerEvent e;

	string varName = "CH1";
	double s = 1;
	double s2 = 2;
	cace[1]->timeManager->updateLamportTime(cace[0]->timeManager->lamportTime + 1000);
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::FireAndForget);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[0]->step(e);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[2]->step(e);
	cace[1]->caceSpace->distributeValue(varName, s2, acceptStrategy::FireAndForget);

	this_thread::sleep_for(chrono::milliseconds(10));
	Step(allCaces);

	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = cace[1]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";
	EXPECT_TRUE(v2.operator bool()) << "3: Didn't receive Variable";

	for (int i = 0; i < 12; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}

	double val1 = 0, val2 = 0, val3 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	v2->getValue(&val3);
	EXPECT_DOUBLE_EQ(s2, val1) << "Own Believe1 is Not Correct\n" << v0->toString();
	EXPECT_DOUBLE_EQ(s2, val2) << "Own Believe2 is Not Correct\n" << v1->toString();
	EXPECT_DOUBLE_EQ(s2, val3) << "Own Believe3 is Not Correct\n" << v2->toString();

	EXPECT_EQ(1, v0->proposals.size()) << "v0: Wrong numer of Robot Believes" << v0->toString();
	EXPECT_EQ(1, v1->proposals.size()) << "v1: Wrong numer of Robot Believes" << v1->toString();
	EXPECT_EQ(2, v2->proposals.size()) << "v2: Wrong numer of Robot Believes" << v2->toString();
}

TEST_F(SystemTests, DefaultConflictHandlingReverseCommandOrder1)
{
	for (int i = 0; i < cace.size(); i++)
	{
		cace[i]->activeRobots.clear();
		for (int j = 0; j < cace.size(); j++)
		{
			cace[i]->agentEngangement(j + 1, false);
		}
		cace[i]->worker->clearJobs();
		cace[i]->variableStore->deleteAllVariables();
	}
	ros::TimerEvent e;

	string varName = "CHRv0";
	double s = 1;
	double s2 = 2;

	cace[1]->timeManager->updateLamportTime(cace[0]->timeManager->lamportTime + 1000);
	cace[1]->caceSpace->distributeValue(varName, s2, acceptStrategy::FireAndForget);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[1]->step(e);
	this_thread::sleep_for(chrono::milliseconds(1));
	cace[2]->step(e);
	this_thread::sleep_for(chrono::milliseconds(10));
	//Reseted because quieck response already updates lamport clock
	cace[0]->timeManager->lamportTime = 1;
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::FireAndForget);

	this_thread::sleep_for(chrono::milliseconds(10));
	Step(allCaces);

	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = cace[1]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";
	EXPECT_TRUE(v2.operator bool()) << "3: Didn't receive Variable";

	for (int i = 0; i < 10; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}

	double val1 = 0, val2 = 0, val3 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	v2->getValue(&val3);
	EXPECT_EQ(s2, val1) << "Own Believe1 is Not Correct\n" << v0->toString();
	EXPECT_EQ(s2, val2) << "Own Believe2 is Not Correct\n" << v1->toString();
	EXPECT_EQ(s2, val3) << "Own Believe3 is Not Correct\n" << v2->toString();

	EXPECT_EQ(1, v0->proposals.size()) << "v0: Wrong numer of Robot Believes" << v0->toString();
	EXPECT_EQ(1, v1->proposals.size()) << "v1: Wrong numer of Robot Believes" << v1->toString();
	EXPECT_EQ(2, v2->proposals.size()) << "v2: Wrong numer of Robot Believes" << v2->toString();
}

TEST_F(SystemTests, Notify)
{
	for (int i = 0; i < cace.size(); i++)
	{
		cace[i]->activeRobots.clear();
		int off = (i == cace.size() - 1) ? 0 : 1;
		for (int j = 0; j < cace.size() - off; j++)
		{
			cace[i]->agentEngangement(j + 1, false);
		}
		cace[i]->worker->clearJobs();
		cace[i]->variableStore->deleteAllVariables();
	}
	ros::TimerEvent e;

	string varName = "Notify";
	double s = 1;
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::ThreeWayHandShake);

	this_thread::sleep_for(chrono::milliseconds(10));
	cace[0]->step(e);
	cace[1]->step(e);
	//CleanUp all other acks here
	cace[2]->communication->clearAllMessageLists();
	cace[2]->worker->clearJobs();
	cace[2]->variableStore->deleteAllVariables();

	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = cace[1]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";
	EXPECT_FALSE(v2.operator bool()) << "3: Did receive Variable but shoudn't";

	double val1 = 0, val2 = 0, val3 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	EXPECT_DOUBLE_EQ(s, val1) << "Own Believe1 is Not Correct";
	EXPECT_DOUBLE_EQ(s, val2) << "Own Believe2 is Not Correct";

	for (int i = 0; i < 5; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		cace[0]->step(e);
		cace[1]->step(e);
	}
	EXPECT_TRUE(v0->proposals.size() == 1) << "v0 before: Wrong numer of Robot Believes";
	EXPECT_TRUE(v1->proposals.size() == 1) << "v1 before: Wrong numer of Robot Believes";

	cace[0]->agentEngangement(3, true);
	cace[1]->agentEngangement(3, true);

	for (int i = 0; i < 12; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		Step(allCaces);
	}

	v2 = cace[2]->caceSpace->getVariable(varName);
	EXPECT_TRUE(v2.operator bool()) << "3: Didn't receive Variable";

	v0->getValue(&val1);
	v1->getValue(&val2);
	v2->getValue(&val3);
	EXPECT_DOUBLE_EQ(s, val1) << "Own Believe1 is Not Correct";
	EXPECT_DOUBLE_EQ(s, val2) << "Own Believe2 is Not Correct";
	EXPECT_DOUBLE_EQ(s, val3) << "Own Believe3 is Not Correct";

	EXPECT_TRUE(v0->proposals.size() == 2) << "v0 after: Wrong numer of Robot Believes " << v0->proposals.size() << "\n"
			<< v0->toString() << "\n" << v1->toString() << "\n" << v2->toString() << "\n" << cace[0]->worker->toString()
			<< "\n" << cace[1]->worker->toString() << "\n" << cace[2]->worker->toString();
	EXPECT_TRUE(v1->proposals.size() == 2) << "v1 after: Wrong numer of Robot Believes " << v1->proposals.size() << "\n"
			<< v1->toString();
	EXPECT_TRUE(v2->proposals.size() == 2) << "v2 after: Wrong numer of Robot Believes " << v2->proposals.size() << "\n"
			<< v2->toString();
}
