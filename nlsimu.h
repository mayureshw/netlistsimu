#ifndef _NLSIMU_H
#define _NLSIMU_H

#include "gates.h"

// TODO: Complete simulator without CEP first.
// When introducing CEP:
// IntervalManager requires StateIf, for now use NLSimulator as StateIf, decide later
// Like in vc2pn, _intervalManager->route will be a listener to the events in the simulator
// IntervalManager has its own EventRouter instance, which is different from _simuRouter
//
// NLSimulator flow:
// - Decide who wants to be the event handler: pin/port?
// - They can instantiate EventHandler for each event (register separately for 0 and 1)
// - On an event on IPin, trigger eval() of the gate
// - On state change on OPin send new event
// - Net will register for source event and in the handler create target events
// - When creating new events, don't send them to router directly, send them to simulator
// - The simulator will pick events from a priority queue and send them to the router
// - It is convenient to send just NLSimulatorBase to factory and use it for both
//   registering and sending events. Begin NLSimulatorBase with just 2 APIs
class NLSimulator : public NLSimulatorBase
{
    GateFactory _factory;
    EventRouter _simuRouter;
public:
    void sendEvent(unsigned long eid) { _simuRouter.route(eid,0); }
    EventRouter& router() { return _simuRouter; }
    NLSimulator(string netlistir) : _factory(netlistir,this)
    {
    }
};

#endif
