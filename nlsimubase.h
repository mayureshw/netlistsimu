#ifndef _NLSIMUBASE_H
#define _NLSIMUBASE_H

#include "eventhandler.h"
class NLSimulatorBase
{
public:
    virtual EventRouter& router()=0;
};

#endif
