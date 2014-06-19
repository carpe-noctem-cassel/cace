#include <variables/CaceMutex.h>
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
class ApplicationTests : public ::testing::Test
{
public:
	static int iteration;
protected:

	vector<Cace*> cace;
	Cace* quietcace;
	int participants = 3;

	ApplicationTests()
	{
		// You can do set-up work for each test here.
	}

	virtual ~ApplicationTests()
	{
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp()
	{
		vector<int> empty;
		cace.clear();

		for (int i = 0; i < participants; i++)
		{
			char name = (char)((int)'A' + i);
			cace.push_back(Cace::getEmulated("def" + name, i + 1));
			cace[i]->safeStepMode = true;
		}
		quietcace = Cace::getEmulated("Quiet", 101);
		quietcace->setQuiet("Quiet", 101);
		quietcace->safeStepMode = true;

	}

	virtual void TearDown()
	{
		for (Cace* c : cace)
		{
			delete c;
		}
		delete quietcace;
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
};

TEST_F(ApplicationTests, QuietMode)
{
	cace[0]->activeRobots.clear();

	quietcace->activeRobots.clear();
	quietcace->agentEngangement(1, false);

	string varName = "quiet";
	double s = 1;
	cace[0]->caceSpace->distributeValue(varName, s, acceptStrategy::ThreeWayHandShake);
	ros::TimerEvent e;

	for (int i = 0; i < 10; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		cace[0]->step(e);
		quietcace->step(e);
	}

	shared_ptr<ConsensusVariable> v0 = cace[0]->caceSpace->getVariable(varName);
	shared_ptr<ConsensusVariable> v1 = quietcace->caceSpace->getVariable(varName);
	EXPECT_TRUE(v0.operator bool()) << "1: Didn't receive Variable";
	EXPECT_TRUE(v1.operator bool()) << "2: Didn't receive Variable";

	double val1 = 0, val2 = 0;
	v0->getValue(&val1);
	v1->getValue(&val2);
	EXPECT_DOUBLE_EQ(s, val1) << "A. Own Believe1 is Not Correct";
	EXPECT_DOUBLE_EQ(s, val2) << "A. Own Believe2 is Not Correct";

	EXPECT_EQ(0, v0->proposals.size()) << "v0: Wrong numer of Robot Believes\n" << v0->toString();
	EXPECT_EQ(1, v1->proposals.size()) << "v1: Wrong numer of Robot Believes\n" << v1->toString();
}

TEST_F(ApplicationTests, MutexStrictAlone)
{
	cace[0]->activeRobots.clear();
	cace[0]->variableStore->deleteAllVariables();
	CaceMutex* m = new CaceMutex(cace[0], "simple", std::numeric_limits<long>::max(), cace[0]->timeManager->getDistributedTime(),
				cace[0]->timeManager->lamportTime, true);
	shared_ptr<ConsensusVariable> mutexptr(m);
	cace[0]->caceSpace->addVariable(mutexptr, false);
	EXPECT_TRUE(m->getName()==(CaceMutex::mutexNamespace + string("simple"))) << "Name and Context";
	EXPECT_TRUE(m->isFree(cace[0]->activeRobots)) << "Before";
	m->p();

	EXPECT_FALSE(m->isFree(cace[0]->activeRobots)) << "Freeness After P";
	EXPECT_TRUE(m->isOwner(cace[0]->communication->getOwnID(), cace[0]->activeRobots)) << "IsOwner";
	EXPECT_TRUE(m->v()) << "V";
	EXPECT_TRUE(m->isFree(cace[0]->activeRobots)) << "After V";
	//mutexptr.reset();
}

TEST_F(ApplicationTests, MutexStrictNotAlone)
{
	cace[0]->activeRobots.clear();

	vector<int> agents;
	vector<int> empty;
	agents.push_back(101);

	cace[0]->variableStore->deleteAllVariables();
	CaceMutex* m= new CaceMutex(cace[0], "simple3", std::numeric_limits<long>::max(), cace[0]->timeManager->getDistributedTime(),
				cace[0]->timeManager->lamportTime, true);
	shared_ptr<ConsensusVariable> mutexptr(m);
	cace[0]->caceSpace->addVariable(mutexptr, false);
	EXPECT_TRUE(m->isFree(agents)) << "Before";
	EXPECT_FALSE(m->pNoBlock(agents)) << "No P in one step";

	EXPECT_FALSE(m->isFree(agents)) << "1Freeness After P";
	EXPECT_FALSE(m->isOwner(cace[0]->communication->getOwnID(), agents)) << "1IsOwner";

	auto v = make_shared<ConsensusVariable>(CaceMutex::mutexNamespace + string("simple3"),
											acceptStrategy::ThreeWayHandShake, numeric_limits<long>::max(), 101,
											cace[0]->timeManager->getDistributedTime(),
											cace[0]->timeManager->lamportTime, CaceType::CInt);
	m->proposals.push_back(v);

	EXPECT_FALSE(m->isFree(agents)) << "2Freeness After P";
	EXPECT_FALSE(m->isOwner(cace[0]->communication->getOwnID(), agents)) << "2IsOwner";
	EXPECT_FALSE(m->isOwner(cace[0]->communication->getOwnID(), empty)) << "2IsOwner empty";

	EXPECT_FALSE(m->pNoBlock(agents)) << "No P in here";
	EXPECT_FALSE(m->isFree(agents)) << "3Freeness After P";
	EXPECT_FALSE(m->isOwner(cace[0]->communication->getOwnID(), agents)) << "3IsOwner";

	v->setValue(cace[0]->communication->getOwnID());

	EXPECT_FALSE(m->isFree(agents)) << "4Freeness After P";
	EXPECT_TRUE(m->isOwner(cace[0]->communication->getOwnID(), agents)) << "4IsOwner";
	EXPECT_TRUE(m->v()) << "V";
	EXPECT_FALSE(m->isFree(agents)) << "After V 1";
	EXPECT_FALSE(m->isFree(empty)) << "After V 1 empty";

	v->setValue((int)-1);
	EXPECT_TRUE(m->isFree(agents)) << "After V 2";
}

TEST_F(ApplicationTests, MutexNonStrictNotAlone)
{
	cace[0]->activeRobots.clear();

	vector<int> agents;
	vector<int> empty;
	agents.push_back(103);

	cace[0]->variableStore->deleteAllVariables();
	CaceMutex* m = new CaceMutex(cace[0], "simpleNonStrict", numeric_limits<long>::max(), cace[0]->timeManager->getDistributedTime(),
				cace[0]->timeManager->lamportTime, false);
	shared_ptr<ConsensusVariable> mutexptr(m);
	cace[0]->caceSpace->addVariable(mutexptr, false);
	EXPECT_TRUE(m->isFree(agents)) << "Before";
	EXPECT_FALSE(m->pNoBlock(agents)) << "No P in one step";

	EXPECT_FALSE(m->isFree(agents)) << "1Freeness After P";
	EXPECT_FALSE(m->isOwner(cace[0]->communication->getOwnID(), agents)) << "1IsOwner";

	auto v = make_shared<ConsensusVariable>(CaceMutex::mutexNamespace + string("simpleNonStrict"),
											acceptStrategy::ThreeWayHandShake, numeric_limits<long>::max(), 103,
											cace[0]->timeManager->getDistributedTime(),
											cace[0]->timeManager->lamportTime, CaceType::CInt);
	m->proposals.push_back(v);

	EXPECT_FALSE(m->isFree(agents)) << "2Freeness After P";
	EXPECT_FALSE(m->isOwner(cace[0]->communication->getOwnID(), agents)) << "2IsOwner";
	EXPECT_TRUE(m->isOwner(cace[0]->communication->getOwnID(), empty)) << "2IsOwner empty";

	EXPECT_FALSE(m->pNoBlock(agents)) << "No P in here";
	EXPECT_FALSE(m->isFree(agents)) << "3Freeness After P";
	EXPECT_FALSE(m->isOwner(cace[0]->communication->getOwnID(), agents)) << "3IsOwner";

	v->setValue(cace[0]->communication->getOwnID());

	EXPECT_FALSE(m->isFree(agents)) << "4Freeness After P";
	EXPECT_TRUE(m->isOwner(cace[0]->communication->getOwnID(), agents)) << "4IsOwner";
	EXPECT_TRUE(m->v()) << "V";
	EXPECT_FALSE(m->isFree(agents)) << "After V 1";
	EXPECT_TRUE(m->isFree(empty)) << "After V 1 empty";

	v->setValue((int)-1);
	EXPECT_TRUE(m->isFree(agents)) << "After V 2";
}

TEST_F(ApplicationTests, MutexNonStrictAlone)
{
	cace[0]->activeRobots.clear();
	cace[0]->variableStore->deleteAllVariables();
	CaceMutex* m = new CaceMutex(cace[0], "simple2", numeric_limits<long>::max(), cace[0]->timeManager->getDistributedTime(), cace[0]->timeManager->lamportTime, false);
	shared_ptr<ConsensusVariable> mutexptr(m);
	cace[0]->caceSpace->addVariable(mutexptr, false);

	EXPECT_TRUE(m->isFree(cace[0]->activeRobots))<< "Before";
	m->p();

	EXPECT_FALSE(m->isFree(cace[0]->activeRobots)) << "Freeness After P";
	EXPECT_TRUE(m->isOwner(cace[0]->communication->getOwnID(), cace[0]->activeRobots)) << "IsOwner";
	EXPECT_TRUE(m->v()) << "V";
	EXPECT_TRUE(m->isFree(cace[0]->activeRobots)) << "After V";
}
