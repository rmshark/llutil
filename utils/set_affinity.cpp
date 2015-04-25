/*
* Copyright (C) 2015 Rishabh Mukherjee
*
* This program is free software: you can redistribute it and/or modify it under the terms of the 
* GNU General Public License as published by the Free Software Foundation, either version 3 of the License, 
* or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program. 
* If not, see http://www.gnu.org/licenses/.
*/

#include "set_affinity.h"

int SetAffinity(int nCoreID_, int nPid_, std::string &strStatus)
{
  int nRet = 0;
  char strErrMsg[512] = {0};
  unsigned int nThreads = std::thread::hardware_concurrency();
  cpu_set_t set;
  CPU_ZERO(&set);

  if(nCoreID_ >= nThreads)
  {
    sprintf(strErrMsg, "Error : Supplied core id %d is invalid. Valid range is 0 - %d", nCoreID_, nThreads - 1);
    strStatus = strErrMsg;
    nRet = -1;
  }
  else
  {
    CPU_SET(nCoreID_,&set);

    if(sched_setaffinity(nPid_,sizeof(cpu_set_t), &set) < 0)
    {
      sprintf(strErrMsg, "Error : %d returned while setting process affinity for process ID . Valid range is 0 - %d", errno,nPid_);
      strStatus = strErrMsg;
      nRet = -1;
    }
  }
  return 0;
};
