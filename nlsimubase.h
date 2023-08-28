#ifndef _NLSIMUBASE_H
#define _NLSIMUBASE_H

#include "eventhandler.h"
class NLSimulatorBase
{
public:
    virtual void sendEvent(unsigned long)=0;
    virtual EventRouter& router()=0;
};

#endif
