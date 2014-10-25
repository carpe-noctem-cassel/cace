/*
 * AlicaROSClock.cpp
 *
 *  Created on: Jul 18, 2014
 *      Author: Paul Panin
 */

#include "clock/AlicaCaceClock.h"
#include "ros/time.h"

namespace alicaRosProxy
{

	AlicaCaceClock::AlicaCaceClock()
	{
		ros::Time::init();
	}

	AlicaCaceClock::~AlicaCaceClock()
	{
	}

	alica::alicaTime AlicaCaceClock::now()
	{
		ros::Time t = ros::Time::now();
		alica::alicaTime ret = (alica::alicaTime)(t.sec * 1000000000UL + t.nsec);
		return ret;
	}
	void AlicaCaceClock::sleep(long us)
	{
		int sec = us/1000000;
		int nsec = (us%1000000)*1000;
		ros::Duration(sec, nsec).sleep();
		//ros::Duration(us/1000).sleep();
	}

} /* namespace supplementary */
