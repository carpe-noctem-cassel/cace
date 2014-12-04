/*
 * AlicaROSClock.cpp
 *
 *  Created on: Jul 18, 2014
 *      Author: Paul Panin
 */

#include "clock/AlicaCaceClock.h"
#include <cace.h>
#include <timeManager/TimeManager.h>

namespace alicaCaceProxy
{

	AlicaCaceClock::AlicaCaceClock(Cace* cace)
	{
		this->cace = cace;
	}

	AlicaCaceClock::~AlicaCaceClock()
	{
	}

	alica::alicaTime AlicaCaceClock::now()
	{
		return cace->timeManager->getLocalTime();
	}

	void AlicaCaceClock::sleep(long us)
	{
		this_thread::sleep_for(std::chrono::microseconds(us));
	}

} /* namespace supplementary */
