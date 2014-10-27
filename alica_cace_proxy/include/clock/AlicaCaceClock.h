/*
 * AlicaROSClock.h
 *
 *  Created on: Jul 18, 2014
 *      Author: Paul Panin
 */

#ifndef ALICAROSCLOCK_H_
#define ALICAROSCLOCK_H_

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
		virtual alica::alicaTime now();
		virtual void sleep(long us);
	private:
		Cace* cace;
	};

} /* namespace supplementary */

#endif /* ALICAROSCLOCK_H_ */
