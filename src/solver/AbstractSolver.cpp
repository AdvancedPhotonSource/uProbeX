/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <solver/AbstractSolver.h>

/*---------------------------------------------------------------------------*/

AbstractSolver::AbstractSolver()
{

}

/*---------------------------------------------------------------------------*/

AbstractSolver::~AbstractSolver()
{

}

/*--------------------------------------------------------------------------*/

QString AbstractSolver::getLastErrorMessage()
{

   return m_lastErrorMsg;

}

/*--------------------------------------------------------------------------*/