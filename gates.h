#ifndef _GATES_H
#define _GATES_H

#include <map>
#include <bitset>
#include <tuple>
#include <set>
#include <string>
#include <functional>

using namespace std;
#include "xsb2cpp.h"
#include "nlsimubase.h"

typedef map<string,string> Parmap;
typedef list<tuple<string,string>> Parlist;

class Pin;

class Gate
{
protected:
    unsigned _opid;
    NLSimulatorBase *_nlsimu;
public:
    virtual Pin* getIPin(string portname, unsigned pinindex)=0;
    virtual Pin* getOPin(string portname, unsigned pinindex)=0;
    virtual void setEventHandlers()=0;
    virtual void eval()=0;
    virtual ~Gate() {}
};

class Pin
{
public:
    inline static const unsigned EVENTTYPS = 2;
private:
    void validateIndex(unsigned index)
    {
        if ( index >= EVENTTYPS )
        {
            cout << "Event type exceeds allowed number " << index << " " << EVENTTYPS << endl;
            exit(1);
        }
    }
protected:
    unsigned _eids[EVENTTYPS];
public:
    void set(bool val, NLSimulatorBase *nlsimu) {}
    virtual void setEventHandlers(Gate *, NLSimulatorBase *) {}
    void setEid(unsigned index, unsigned eid)
    {
        validateIndex(index);
        _eids[index] = eid;
    }
    unsigned getEid(unsigned index)
    {
        validateIndex(index);
        return _eids[index];
    }
    virtual ~Pin() {}
};

template<unsigned W> class PinState : public Pin
{
protected:
    typename bitset<W>::reference _state;
public:
    PinState<W>( typename bitset<W>::reference state ) : _state(state) {}
};

template<unsigned W> class IPin : public PinState<W>
{
using PinState<W>::PinState;
    EventHandler *_eventHandlers[Pin::EVENTTYPS];
    template<int V> void handle(Gate *gate)
    {
        PinState<W>::_state = V;
        gate->eval();
    }
public:
    void setEventHandlers(Gate *gate, NLSimulatorBase *nlsimu)
    {
        _eventHandlers[0] = new EventHandler( nlsimu->router(), Pin::_eids[0],
            [this,gate](Event,unsigned long) { this->handle<0>(gate); } );
        _eventHandlers[1] = new EventHandler( nlsimu->router(), Pin::_eids[1],
            [this,gate](Event,unsigned long) { this->handle<1>(gate); } );
    }
    ~IPin()
    {
        for(int i=0; i<Pin::EVENTTYPS; i++) delete _eventHandlers[i];
    }
};

template<unsigned W> class OPin : public PinState<W>
{
using PinState<W>::PinState;
public:
    void set(bool val, NLSimulatorBase *nlsimu)
    {
        if ( PinState<W>::_state != val )
        {
            PinState<W>::_state = val;
            nlsimu->sendEvent( Pin::_eids[val] );
        }
    }
};

class PortBase
{
public:
    virtual void setEventHandlers(Gate *, NLSimulatorBase *) {}
    virtual Pin* getPin(unsigned index)=0;
};

template<unsigned W> class Port : public PortBase
{
protected:
    Pin *_pins[W];
    bitset<W> _state;
public:
    void set(bitset<W>& val, NLSimulatorBase *nlsimu)
    {
        for(int i=0; i<W; i++) _pins[i]->set(_state[i], nlsimu);
    }
    Pin* getPin(unsigned index)
    {
        if ( index >= W )
        {
            cout << "getPin received index > W " << index << " " << W << endl;
            exit(1);
        }
        return _pins[index];
    }
    void setEventHandlers(Gate *gate, NLSimulatorBase *nlsimu)
    {
        for(int i=0; i<W; i++) _pins[i]->setEventHandlers(gate,nlsimu);
    }
    ~Port()
    {
        for(int i=0; i<W; i++) delete _pins[i];
    }
};

template<unsigned W> class IPort : public Port<W>
{
public:
    IPort<W>()
    {
        for(int i=0; i<W; i++) Port<W>::_pins[i] = new IPin<W>( Port<W>::_state[i] );
    }
};

template<unsigned W> class OPort : public Port<W>
{
public:
    OPort<W>()
    {
        for(int i=0; i<W; i++) Port<W>::_pins[i] = new OPin<W>( Port<W>::_state[i] );
    }
};

typedef map<string,PortBase*> Portmap;
#define DEFPARM static inline const Parmap _defaults =
#define NODEFPARM static inline const set<string> _nodefaults =
#define PORT(PORTNAME) {#PORTNAME,&PORTNAME}
class CARRY4 : public Gate
{
protected:
    DEFPARM   { {"ADDER_THRESHOLD",""} };
    NODEFPARM {};
    IPort<1> CYINIT;
    IPort<1> CI;
    IPort<4> S;
    IPort<4> DI;
    Portmap _iportmap = {
        PORT(CYINIT),
        PORT(CI),
        PORT(S),
        PORT(DI),
        };
    OPort<4> CO;
    OPort<4> O;
    Portmap _oportmap = {
        PORT(CO),
        PORT(O),
        };
public:
    void eval()
    {
    }
};

// TODO: We'll need PassiveIPin class for pins that shouldn't  trigger eval call
class FDCE : public Gate
{
protected:
    DEFPARM   { {"IS_C_INVERTED","1'b0"} };
    NODEFPARM {"INIT"};
    IPort<1> C;
    IPort<1> CE;
    IPort<1> CLR;
    IPort<1> D;
    Portmap _iportmap = {
        PORT(C),
        PORT(CE),
        PORT(CLR),
        PORT(D),
        };
    OPort<1> Q;
    Portmap _oportmap = {
        PORT(Q),
        };
public:
    void eval()
    {
    }
};

class GND : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    Portmap _iportmap = {};
    OPort<1> G;
    Portmap _oportmap = {
        PORT(G),
        };
public:
    void eval()
    {
    }
    GND()
    {
        bitset<1> val = 0;
        G.set(val,_nlsimu);
    }
};

class IBUF : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {"CCIO_EN"};
    IPort<1> I;
    Portmap _iportmap = {
        PORT(I),
        };
    OPort<1> O;
    Portmap _oportmap = {
        PORT(O),
        };
public:
    void eval()
    {
    }
};

template<unsigned W> class LUT : public Gate
{
protected:
    DEFPARM   { { "SOFT_HLUTNM",""}, { "box_type",""} };
    NODEFPARM { "INIT" };
    IPort<1> I[W];
    Portmap _iportmap;
    OPort<1> O;
    Portmap _oportmap = {
        PORT(O),
        };
public:
    void eval()
    {
    }
    LUT<W>()
    {
        for(int i=0; i<W; i++)
        {
            string portname = "I" + to_string(i);
            _iportmap.emplace(portname,&I[i]);
        }
    }
};

class OBUF : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    IPort<1> I;
    Portmap _iportmap = {
        PORT(I),
        };
    OPort<1> O;
    Portmap _oportmap = {
        PORT(O),
        };
public:
    void eval()
    {
    }
};

class OBUFT : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    IPort<1> I;
    IPort<1> T;
    Portmap _iportmap = {
        PORT(I),
        PORT(T),
        };
    OPort<1> O;
    Portmap _oportmap = {
        PORT(O),
        };
public:
    void eval()
    {
    }
};

class VCC : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    Portmap _iportmap = {};
    OPort<1> P;
    Portmap _oportmap = {
        PORT(P),
        };
public:
    void eval()
    {
    }
    VCC()
    {
        bitset<1> val = 1;
        P.set(val,_nlsimu);
    }
};

template<typename T> class GateMethods : public T
{
    void mergeDefaults(Parmap& parmap)
    {
        for(auto pv:T::_defaults)
        {
            auto p = pv.first;
            auto it = parmap.find(p);
            if ( it == parmap.end() )
            {
                auto v = pv.second;
                parmap.emplace(p,v);
            }
        }
    }
    void validateParmap(Parmap& parmap)
    {
        auto expparams = T::_defaults.size() + T::_nodefaults.size();
        auto actualparams = parmap.size();
        for(auto ndp:T::_nodefaults)
        {
            auto it = parmap.find(ndp);
            if ( it == parmap.end() )
            {
                cout << "Mandatory generic parameter not found: " << ndp << " opid:" << Gate::_opid << endl;
                exit(1);
            }
        }
        if ( expparams != actualparams )
        {
            cout << "Generic parameter count mismatch for opid:" << Gate::_opid
                << " expected:" << expparams << " actual:" << actualparams << endl;
            exit(1);
        }
    }
    void processParmap(Parlist& parlist)
    {
        // We don't store the parmap, deal with it in the constructor
        Parmap parmap;
        for(auto pv:parlist)
            parmap.emplace( get<0>(pv), get<1>(pv) );
        mergeDefaults(parmap);
        validateParmap(parmap);
    }
    template<bool isipin> Pin* getPin(string portname, unsigned pinindex)
    {
        Portmap *pm;
        if constexpr (isipin) pm = &(T::_iportmap);
        else pm = &(T::_oportmap);
        auto it = pm->find(portname);
        if ( it == pm->end() )
        {
            cout << "getPin received invalid portname " << portname << " " << Gate::_opid << endl;
            exit(1);
        }
        return it->second->getPin(pinindex);
    }
public:
    void setEventHandlers()
    {
        for(auto ip:T::_iportmap) ip.second->setEventHandlers(this,Gate::_nlsimu);
    }
    // TODO: ideally should have getPin<bool>, but no virtual function templates allowed. Any other way?
    Pin* getIPin(string portname, unsigned pinindex) { return getPin<true>(portname,pinindex); }
    Pin* getOPin(string portname, unsigned pinindex) { return getPin<false>(portname,pinindex); }
    GateMethods<T>(unsigned opid, Parlist& parlist, NLSimulatorBase *nlsimu)
    {
        Gate::_opid = opid;
        Gate::_nlsimu = nlsimu;
        processParmap(parlist);
    }
};

class Net
{
public:
    Net(unsigned sevent, list<unsigned> tevents)
    {
    }
};

typedef tuple<string,unsigned,string,Parlist> t_opi;
typedef tuple<string,unsigned,unsigned,string,unsigned> t_pin;
typedef tuple<string,unsigned,unsigned,unsigned> t_ev;
typedef tuple<string,unsigned,list<unsigned>> t_net;
typedef map<unsigned,Pin*> t_pinmap;
#define CREATE(GATETYP,CLS) { #GATETYP, [this](unsigned opid, Parlist& parlist) { return new GateMethods<CLS>(opid,parlist,_nlsimu); } }
#define CREATE1(GATETYP) CREATE(GATETYP,GATETYP)
class GateFactory
{
    const map< string, function< Gate* (unsigned,Parlist&) > > _creators = {
        CREATE1( CARRY4 ),
        CREATE1( FDCE   ),
        CREATE1( GND    ),
        CREATE1( IBUF   ),
        CREATE1( OBUF   ),
        CREATE1( OBUFT  ),
        CREATE1( VCC    ),
        CREATE( LUT1, LUT<1> ),
        CREATE( LUT2, LUT<2> ),
        CREATE( LUT3, LUT<3> ),
        CREATE( LUT4, LUT<4> ),
        CREATE( LUT5, LUT<5> ),
        CREATE( LUT6, LUT<6> ),
    };
    map< unsigned, Gate* > _gatemap;
    list<Net*> _netlist;
    NLSimulatorBase *_nlsimu;
    Gate* getGate(unsigned index)
    {
        auto it = _gatemap.find(index);
        if ( it == _gatemap.end() )
        {
            cout << "No gate found with opid " << index << endl;
            exit(1);
        }
        return it->second;
    }
    Pin* getPinFromMap(unsigned pinid, t_pinmap& pinmap)
    {
        auto it = pinmap.find(pinid);
        if ( it == pinmap.end() )
        {
            cout << "Invalid pin id sought " << pinid << endl;
            exit(1);
        }
        return it->second;
    }
    void process_net(t_net& nett, t_pinmap& pinmap)
    {
        auto spinid = get<1>(nett);
        auto tpinids = get<2>(nett);
        auto spin = getPinFromMap(spinid,pinmap);
        list<Pin*> tpins;
        for(auto tpinid:tpinids)
            tpins.push_back( getPinFromMap(tpinid,pinmap) );
        for(int i=0; i<Pin::EVENTTYPS; i++)
        {
            auto seid = spin->getEid(i);
            list<unsigned> teids;
            for(auto tpin:tpins)
                teids.push_back( tpin->getEid(i) );
            auto net = new Net(seid,teids);
            _netlist.push_back( net );
        }
    }
    void process_ev(t_ev& evt, t_pinmap& pinmap)
    {
        auto eid = get<1>(evt);
        auto pinid = get<2>(evt);
        auto tv = get<3>(evt);
        auto pin = getPinFromMap(pinid, pinmap);
        pin->setEid(tv,eid);
    }
    template<bool isipin> void process_pin(t_pin& pint, t_pinmap& pinmap)
    {
        auto pinid = get<1>(pint);
        auto opid = get<2>(pint);
        auto portname = get<3>(pint);
        auto pinindex = get<4>(pint);
        auto gate = getGate(opid);
        Pin *pin;
        if constexpr ( isipin )
            pin = gate->getIPin(portname,pinindex);
        else
            pin = gate->getOPin(portname,pinindex);
        pinmap.emplace(pinid,pin);
    }
    void process_opi(t_opi& opit)
    {
        auto opid = get<1>(opit);
        auto it1 = _gatemap.find(opid);
        if ( it1 != _gatemap.end() )
        {
            cout << "Duplicate opid " << opid << endl;
            exit(1);
        }
        auto optyp = get<2>(opit);
        auto it2 = _creators.find(optyp);
        if ( it2 == _creators.end() )
        {
            cout << "Unknown gate type: " << optyp << endl;
            exit(1);
        }
        auto creator = it2->second;
        auto parlist = get<3>(opit);
        auto gate = creator(opid,parlist);
        _gatemap.emplace(opid,gate);
    }
public:
    GateFactory(string netlistir, NLSimulatorBase* nlsimu) : _nlsimu(nlsimu)
    {
        PDb netlistdb;
        t_predspec ps_opi = {"opi",3};
        t_predspec ps_ipin = {"ipin",4};
        t_predspec ps_opin = {"opin",4};
        t_predspec ps_ev = {"ev",3};
        t_predspec ps_net = {"net",2};
        netlistdb.load(netlistir,{ps_opi,ps_ipin,ps_opin,ps_ev,ps_net});

        auto opil = netlistdb.terms2tuples<t_opi>(ps_opi);
        for(auto opit : opil) process_opi(opit);

        t_pinmap pinmap;
        auto ipinl = netlistdb.terms2tuples<t_pin>(ps_ipin);
        for(auto ipint:ipinl) process_pin<true>(ipint,pinmap);

        auto opinl = netlistdb.terms2tuples<t_pin>(ps_opin);
        for(auto opint:opinl) process_pin<false>(opint,pinmap);

        auto evl = netlistdb.terms2tuples<t_ev>(ps_ev);
        for(auto evt : evl) process_ev(evt,pinmap);

        auto netl = netlistdb.terms2tuples<t_net>(ps_net);
        for(auto nett : netl) process_net(nett,pinmap);

        // set event handlers after all pins know their events
        for(auto ig:_gatemap) ig.second->setEventHandlers();
    }
    ~GateFactory()
    {
        for(auto ig:_gatemap) delete ig.second;
        for(auto net:_netlist) delete net;
    }
};

#endif
