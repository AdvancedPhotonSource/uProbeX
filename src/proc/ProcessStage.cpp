/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <proc/ProcessStage.h>

using gstar::Array;

/*---------------------------------------------------------------------------*/

ProcessStage::ProcessStage(Stage* parent)
: Stage(parent)
{

   m_stageType = Stage::STAGE_PROCESS;

}

/*---------------------------------------------------------------------------*/

ProcessStage::~ProcessStage()
{

}

/*---------------------------------------------------------------------------*/
