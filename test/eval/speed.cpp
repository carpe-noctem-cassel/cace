/*
 * speed.cpp
 *
 *  Created on: 25.06.2014
 *      Author: endy
 */


#include <boost/smart_ptr/make_shared_object.hpp>
#include <cace/CaceAcknowledge.h>
#include <cace/CaceType.h>
#include <cace.h>
#include <caceSpace.h>
#include <communication/CaceCommunication.h>
#include <communication/CommunicationWorker.h>
#include <communication/jobs/AbstractCommunicationJob.h>
#include <communication/jobs/CommandJob.h>
#include <communication/JobStateEntity.h>
#include <CaceTypes.h>
#include <gtest/gtest.h>
#include <gtest/gtest-message.h>
#include <gtest/internal/gtest-internal.h>
#include <gtest/internal/gtest-string.h>
#include <ros/init.h>
#include <timeManager/AgentCommunicationModel.h>
#include <timeManager/AgentTimeData.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <variableStore/CVariableStore.h>
#include <cstdint>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include <tuple>

using namespace cace;

class DelegateTest
{
public:
	string notName;
	void notifyChange(ConsensusVariable* v)
	{
		notName = v->getName();
	}
};

class SpeedEval : public ::testing::Test
{
public:
	DelegateTest delegateTest;

protected:
	Cace* mycace = nullptr;

	SpeedEval()
	{
		// You can do set-up work for each test here.
	}

	virtual ~SpeedEval()
	{
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp()
	{
		mycace = Cace::getEmulated("", 254);
		// Code here will be called immediately after the constructor (right
		// before each test).
	}

	virtual void TearDown()
	{
		delete mycace;
		// Code here will be called immediately after each test (right
		// before the destructor).
	}

	// Objects declared here can be used by all tests in the test case for Foo.

};

// Run all the tests that were declared with TEST()
int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	ros::init(argc, argv, "Cace");

	int ret = RUN_ALL_TESTS();
	cout << "All tests Completed" << endl;
	ros::shutdown();
	return ret;
}
