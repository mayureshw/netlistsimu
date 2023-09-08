#ifndef _NLSIMU_H
#define _NLSIMU_H

#include <thread>
#include <mutex>
#include <queue>
#include <random>
#include <condition_variable>
#include <filesystem>
#include "gates.h"
#include "intervals.h"

class StateIfProxy : public CEPStateIf
{
public:
    void* getStatePtr(vector<int>&)
    {
        cout << "CEPStateIf::getStatePtr not implemented" << endl;
        exit(1);
    }
    Etyp getStateTyp(vector<int>&)
    {
        cout << "CEPStateIf::getStateTyp not implemented" << endl;
        exit(1);
    }
    void quit()
    {
        cout << "CEPStateIf::quit not implemented" << endl;
        exit(1);
    }
};

class NLSimulator : public NLSimulatorBase
{
    StateIfProxy _stateif;
    IntervalManager *_intervalManager;
    EventRouter _simuRouter; // Must be before _factory
    GateFactory _factory;
    string _basename;
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
    bool _blocked = false;
    bool simuend() { return _quit and _rq.empty(); }
    bool waitover() { return not _rq.empty() or _quit; }
    bool blocked() { return _rq.empty() and _blocked; }
    void route(Event eid)
    {
        cout << "event:" << eid << endl;
        _intervalManager->route(eid,0);
        _simuRouter.route(eid,0);
    }
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
            route(eid);
        }
    }
    void simuloop()
    {
        do {
            _simuloop();
            {
                unique_lock<mutex> ulockq(_rqmutex);
                _blocked = true;
                notify(); // For waitTillStable
                _rq_cvar.wait(ulockq, [this](){return waitover();});
                _blocked = false;
            }
        } while ( not simuend() );
    }
    thread _simuthread {&NLSimulator::simuloop,this};
    void notify() { _rq_cvar.notify_all(); }
    void wait() { _simuthread.join(); }
public:
    void init()
    {
        _factory.init();
        waitTillStable();
    }
    void quit()
    {
        _quit = true;
        notify();
        wait();
    }
    void sendEventImmediate(Event eid) { route(eid); }
    void sendEvent(Event eid)
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
        _rq_cvar.wait(ulockq, [this](){return blocked();});
    }
    template<unsigned W> void setPort(unsigned opid, string portname, bitset<W> val) { _factory.setPort<W>(opid,portname,val); }
    void watch(unsigned opid, string portname) { _factory.watch(opid,portname); }
    void unwatch(unsigned opid, string portname) { _factory.unwatch(opid,portname); }
    EventRouter& router() { return _simuRouter; }
    NLSimulator(string netlistir) : _factory(netlistir,this)
    {
        filesystem::path irpath(netlistir);
        _basename = irpath.stem();
        string cepfile = _basename + ".cep";
        _intervalManager = new IntervalManager( _basename, &_stateif );
    }
    ~NLSimulator() { delete _intervalManager; }
};

#endif
