#ifndef _NLSIMU_H
#define _NLSIMU_H

#include <thread>
#include <mutex>
#include <queue>
#include <random>
#include <condition_variable>
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
    EventRouter _simuRouter; // Must be before _factory
    GateFactory _factory;
    using t_pair  = pair<double, unsigned>;
    // Saves the overhead of comparing 2nd member of the pair
    class PriorityLT
    {
    public:
        // Since delay is opposite of priority, we use >
        bool operator() (t_pair& l, t_pair& r) { return l.first > r.first; }
    };
    using t_queue = priority_queue<t_pair, vector<t_pair>, PriorityLT>;
    t_queue _rq;
    mutex _rqmutex;
    condition_variable _rq_cvar;
    random_device _rng;
    uniform_real_distribution<double> _udistr {-1,1};
    bool _quit = false;
    bool simuend() { return _quit and _rq.empty(); }
    bool waitover() { return not _rq.empty() or _quit; }
    void _simuloop()
    {
        while ( true )
        {
            _rqmutex.lock();
            if ( _rq.empty() )
            {
                _rqmutex.unlock();
                break;
            }
            auto eid = _rq.top().second;
            _rq.pop();
            _rqmutex.unlock();
            notify(); // For convenience of waitTillStable
            _simuRouter.route(eid,0);
            cout << "event:" << eid << endl;
        }
    }
    void simuloop() // TODO: This needs some work, here and in petrinet.h
    {
        do {
            _simuloop();
            {
                unique_lock<mutex> ulockq(_rqmutex);
                _rq_cvar.wait(ulockq, [this](){return waitover();});
            }
        } while ( not simuend() );
    }
    thread _simuthread {&NLSimulator::simuloop,this};
    void notify() { _rq_cvar.notify_all(); }
    void wait() { _simuthread.join(); }
public:
    void init() { _factory.init(); }
    void quit()
    {
        _quit = true;
        notify();
        wait();
    }
    void sendEvent(unsigned long eid)
    {
        auto prio = _udistr(_rng);
        _rqmutex.lock();
        _rq.push( { prio, eid } );
        _rqmutex.unlock();
        notify();
    }
    void waitTillStable()
    {
        unique_lock<mutex> ulockq(_rqmutex);
        _rq_cvar.wait(ulockq, [this](){return _rq.empty();});
    }
    template<unsigned W> void setPort(unsigned opid, string portname, bitset<W> val) { _factory.setPort<W>(opid,portname,val); }
    void watch(unsigned opid, string portname) { _factory.watch(opid,portname); }
    void unwatch(unsigned opid, string portname) { _factory.unwatch(opid,portname); }
    EventRouter& router() { return _simuRouter; }
    NLSimulator(string netlistir) : _factory(netlistir,this)
    {
    }
};

#endif
