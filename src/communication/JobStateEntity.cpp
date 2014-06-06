/*
 * JobStateEntity.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include "communication/JobStateEntity.h"

namespace cace
{

	JobStateEntity::JobStateEntity(int robotID, int retrys, ctime lastSent)
	{
		this->robotID = robotID;
		this->retrys = retrys;
		this->lastSent = lastSent;
	}

	JobStateEntity::~JobStateEntity()
	{
	}

} /* namespace cace */
