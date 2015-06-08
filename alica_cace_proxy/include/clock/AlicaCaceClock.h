/*
 * AlicaROSClock.h
 *
 *  Created on: Jul 18, 2014
 *      Author: Paul Panin
 */

#ifndef ALICACACECLOCK_H_
#define ALICACACECLOCK_H_

#include <engine/IAlicaClock.h>
#include <cace.h>

using namespace cace;

namespace alicaCaceProxy
{

	class AlicaCaceClock : public virtual alica::IAlicaClock
	{
	public:
		AlicaCaceClock(Cace* cace);
		virtual ~AlicaCaceClock();
		virtual alica::AlicaTime now();
		virtual void sleep(long us);
	private:
		Cace* cace;
	};

} /* namespace supplementary */

#endif /* ALICAROSCLOCK_H_ */
