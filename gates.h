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

template <unsigned W> class HDLConst
{
    bitset<W> _bits = 0;
    void str2bits_h(string str)
    {
        int strtpos = 0;
        for(auto rit=str.crbegin(); rit!=str.crend(); rit++, strtpos += 4)
            switch ( *rit )
            {
                case '0': str2bits_b("0000",strtpos); break;
                case '1': str2bits_b("0001",strtpos); break;
                case '2': str2bits_b("0010",strtpos); break;
                case '3': str2bits_b("0011",strtpos); break;
                case '4': str2bits_b("0100",strtpos); break;
                case '5': str2bits_b("0101",strtpos); break;
                case '6': str2bits_b("0110",strtpos); break;
                case '7': str2bits_b("0111",strtpos); break;
                case '8': str2bits_b("1000",strtpos); break;
                case '9': str2bits_b("1001",strtpos); break;
                case 'A': str2bits_b("1010",strtpos); break;
                case 'B': str2bits_b("1011",strtpos); break;
                case 'C': str2bits_b("1100",strtpos); break;
                case 'D': str2bits_b("1101",strtpos); break;
                case 'E': str2bits_b("1110",strtpos); break;
                case 'F': str2bits_b("1111",strtpos); break;
                default:
                    cout << "Invalid character in number string " << str << endl;
                    exit(1);
            }
    }
    void str2bits_b(string str, int strtpos = 0)
    {
        int i=strtpos;
        for(auto rit=str.crbegin(); rit!=str.crend(); rit++, i++)
            if ( *rit == '1' ) _bits[i] = 1;
    }
public:
    bool operator [] (int i) { return _bits[i]; }
    bitset<W>& as_bitset() { return _bits; }
    HDLConst(string str)
    {
        auto qpos = str.find("'");
        auto szstr = str.substr(0,qpos);
        auto sz = stoi(szstr);
        if ( sz > W )
        {
            cout << "Constant too large " << sz << ">" << W << endl;
            exit(1);
        }
        auto numstr = str.substr(qpos+2);
        auto typ = str.substr(qpos+1,1)[0];
        switch(typ)
        {
            case 'h' : str2bits_h(numstr); break;
            case 'b' : str2bits_b(numstr); break;
            default:
                cout << "Invalid number system, expect h or b " << str << endl;
                exit(1);
        }
    }
};

class Pin;
class Gate;
class PortBase
{
public:
    virtual void setEventHandlers()=0;
    virtual Pin* getPin(unsigned index)=0;
    virtual void notify()=0;
    virtual void watch()=0;
    virtual void unwatch()=0;
    virtual Gate* gate()=0;
    virtual void init()=0;
    virtual void eval()=0;
    virtual bool initCompleted()=0;
    virtual EventRouter& router()=0;
    virtual NLSimulatorBase* nlsimu()=0;
};


typedef map<string,PortBase*> Portmap;
class Gate
{
protected:
    bool _initCompleted = false;
    Portmap _iportmap;
    Portmap _oportmap;
    unsigned _opid;
    NLSimulatorBase *_nlsimu;
    virtual void evalOp(PortBase*) {}
    virtual void handleParmap(Parmap&) {}
public:
    template<bool isipin> Pin* getPin(string portname, unsigned pinindex)
    {
        Portmap *pm;
        if constexpr (isipin) pm = &_iportmap;
        else pm = &_oportmap;
        auto it = pm->find(portname);
        if ( it == pm->end() )
        {
            cout << "getPin received invalid portname " << portname << " " << Gate::_opid << endl;
            exit(1);
        }
        return it->second->getPin(pinindex);
    }
    bool initCompleted() { return _initCompleted; }
    NLSimulatorBase* nlsimu() { return _nlsimu; }
    unsigned opid() { return _opid; }
    virtual void setEventHandlers()=0;
    virtual void init() { for(auto ip:_iportmap) ip.second->init(); }
    void eval(PortBase* port)
    {
        evalOp(port);
        _initCompleted = true;
    }
    EventRouter& router() { return _nlsimu->router(); }
    PortBase* getPort(string portname)
    {
        auto it1 = _iportmap.find(portname);
        if ( it1 != _iportmap.end() ) return it1->second;
        auto it2 = _oportmap.find(portname);
        if ( it2 != _oportmap.end() ) return it2->second;
        cout << "Could not find port " << portname << endl;
        exit(1);
    }
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
    PortBase *_port;
    unsigned _eids[EVENTTYPS];
    virtual void postSet(bool)=0;
public:
    virtual void init() {}
    virtual bool isSysInp() { return false; }
    virtual void markDriven() {}
    virtual void setViaEvent(bool val) {}
    virtual void setEventHandlers() {}
    virtual void set(bool)=0;
    EventRouter& router() { return _port->router(); }
    NLSimulatorBase* nlsimu() { return _port->nlsimu(); }
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
    Pin(PortBase *port) : _port(port) {}
    virtual ~Pin() {}
};

template<unsigned W> class PinState : public Pin
{
protected:
    typename bitset<W>::reference _state;
public:
    void set(bool val)
    {
        if ( not Pin::_port->initCompleted() or _state != val )
        {
            _state = val;
            Pin::_port->notify();
            postSet(val);
        }
    }
    PinState<W>( typename bitset<W>::reference state, PortBase *port )
        : _state(state), Pin(port) {}
};

template<unsigned W> class IPin : public PinState<W>
{
using PinState<W>::PinState;
    bool _isSysInp = true; // Marked false explicitly if driven
    EventHandler *_eventHandlers[Pin::EVENTTYPS];
protected:
    void postSet(bool) { Pin::_port->eval(); }
public:
    // Note: Sole purpose of initializing system input pins is to trigger
    // events such that all combinational outputs are consistent with their
    // inputs. Actual value of initialization doesn't matter.
    void init() { if ( isSysInp() ) PinState<W>::set(0); }
    bool isSysInp() { return _isSysInp; }
    void markDriven() { _isSysInp = false; }
    void setEventHandlers()
    {
        for(int i=0; i<Pin::EVENTTYPS; i++)
        {
            _eventHandlers[i] = new EventHandler( Pin::router(), Pin::_eids[i],
                [this,i](Event,unsigned long) { this->set(i); } );
            _eventHandlers[i]->start();
        }
    }
    void setViaEvent(bool val)
    {
        auto eid = Pin::getEid( val ? 1 : 0 );
        Pin::nlsimu()->sendEvent( eid );
    }
    ~IPin()
    {
        for(int i=0; i<Pin::EVENTTYPS; i++) delete _eventHandlers[i];
    }
};

template<unsigned W> class PassiveIPin : public IPin<W>
{
using IPin<W>::IPin;
protected:
    void postSet(bool) {}
};

template<unsigned W> class OPin : public PinState<W>
{
using PinState<W>::PinState;
protected:
    void postSet(bool val)
    {
        Pin::nlsimu()->sendEventImmediate( Pin::_eids[val] );
    }
};

template<unsigned W, typename PT> class Port : public PortBase
{
    Gate *_gate;
    string _name;
    bool _watch = false;
protected:
    PT *_pins[W];
    bitset<W> _state;
public:
    bool initCompleted() { return _gate->initCompleted(); }
    NLSimulatorBase* nlsimu() { return _gate->nlsimu(); }
    EventRouter& router() { return _gate->router(); }
    void init() { for(auto p:_pins) p->init(); }
    void eval() { _gate->eval(this); }
    Gate* gate() { return _gate; }
    void watch() { _watch = true; }
    void unwatch() { _watch = false; }
    bitset<W>& state() { return _state; }
    bool operator [] ( int i ) { return _state[i]; }
    void notify()
    {
        if ( _watch )
            cout << "watch:" << _gate->opid() << ":" << _name << ":" << _state << endl;
    }
    void set(bitset<W>& val)
    {
        for(int i=0; i<W; i++) _pins[i]->set(val[i]);
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
    void setEventHandlers()
    {
        for(int i=0; i<W; i++) _pins[i]->setEventHandlers();
    }
    ~Port()
    {
        for(int i=0; i<W; i++) delete _pins[i];
    }
    Port<W,PT>(Gate *gate, string name, Portmap& portmap) : _gate(gate), _name(name)
    {
        portmap.emplace(name,this);
        for(int i=0; i<W; i++) _pins[i] = new PT( _state[i], this );
    }
};

#define IPort(Name,W) Port<W,IPin<W>> Name {this, #Name,_iportmap}
#define PassiveIPort(Name,W) Port<W,PassiveIPin<W>> Name {this, #Name,_iportmap}
#define OPort(Name,W) Port<W,OPin<W>> Name {this, #Name,_oportmap}

#define DEFPARM static inline const Parmap _defaults =
#define NODEFPARM static inline const set<string> _nodefaults =
#define PORT(PORTNAME) {#PORTNAME,&PORTNAME}
class CARRY4 : public Gate
{
protected:
    DEFPARM   { {"ADDER_THRESHOLD",""} };
    NODEFPARM {};
    IPort(CYINIT, 1);
    IPort(CI,     1);
    IPort(S,      4);
    IPort(DI,     4);
    OPort(CO, 4);
    OPort(O,  4);
public:
// See https://github.com/awersatos/AD/blob/master/Library/HDL%20Simulation/Xilinx%20ISE%2012.1%20VHDL%20Libraries/unisim/src/primitive/CARRY4.vhd
    void evalOp(PortBase*)
    {
        bool ci_or_cyinit = CI[0] or CYINIT[0];

        bitset<4> CO_out;
        CO_out[0] = S[0] ? ci_or_cyinit : DI[0];
        for(int i=1;i<4;i++)  CO_out[i] = S[i] ? CO_out[i-1] : DI[i];
        CO.set(CO_out);

        bitset<4> O_out;
        O_out[0] = S[0] ^ ci_or_cyinit;
        for(int i=1;i<4;i++)  O_out[i] = S[i] ^ CO_out[i-1];
        O.set(O_out);
    }
};

class FDCE : public Gate
{
protected:
    DEFPARM   { {"IS_C_INVERTED","1'b0"} };
    NODEFPARM {"INIT"};
    IPort(C,   1);
    IPort(CLR, 1);
    PassiveIPort(CE, 1);
    PassiveIPort(D,  1);
    OPort(Q,1);
public:
    void evalOp(PortBase *port)
    {
        if ( CLR[0] )
        {
            bitset<1> val = 0;
            Q.set(val);
        }
        else if ( port == &C and CE[0] and C[0] )
            Q.set(D.state());
    }
};

class GND : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    OPort(G,1);
public:
    void init()
    {
        bitset<1> val = 0;
        G.set(val);
    }
};

class IBUF : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {"CCIO_EN"};
    IPort(I,1);
    OPort(O,1);
public:
    void evalOp(PortBase*) { O.set(I.state()); }
};

#define LUTSZ 1<<W
template<unsigned W> class LUT : public Gate
{
    bitset<LUTSZ> _o;
protected:
    DEFPARM   { { "SOFT_HLUTNM",""}, { "box_type",""} };
    NODEFPARM { "INIT" };
    Port<1,IPin<1>> *I[W];
    OPort(O,1);
    void handleParmap(Parmap& parmap)
    {
        auto init = parmap["INIT"];
        HDLConst<LUTSZ> initconst {init};
        _o = initconst.as_bitset();
    }
public:
    void evalOp(PortBase*)
    {
        int ival = 0;
        for(int i=0, mask=1; i<W; i++, mask<<=1)
            if ( (*I[i])[0] ) ival+=mask;
        bitset<1> o = { _o[ival] };
        O.set(o);
    }
    LUT<W>()
    {
        for(int i=0; i<W; i++)
        {
            string portname = "I" + to_string(i);
            I[i] = new Port<1,IPin<1>>( this, portname, _iportmap );
        }
    }
    ~LUT<W>() { for(int i=0; i<W; i++) delete I[i]; }
};

class OBUF : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    IPort(I,1);
    OPort(O,1);
public:
    void evalOp(PortBase*) { O.set(I.state()); }
};

class OBUFT : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    IPort(I,1);
    IPort(T,1);
    OPort(O,1);
public:
    void evalOp(PortBase*)
    {
        if (T[0]) O.set(I.state());
    }
};

class VCC : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    OPort(P,1);
public:
    void init()
    {
        bitset<1> val = 1;
        P.set(val);
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
        T::handleParmap(parmap);
    }
public:
    void setEventHandlers()
    {
        for(auto ip:T::_iportmap) ip.second->setEventHandlers();
    }
    GateMethods<T>(unsigned opid, Parlist& parlist, NLSimulatorBase *nlsimu)
    {
        Gate::_opid = opid;
        Gate::_nlsimu = nlsimu;
        processParmap(parlist);
    }
};

class Net
{
    list<unsigned> _tevents;
    EventHandler *_eventHandler;
    NLSimulatorBase* _nlsimu;
    void relay() { for(auto te:_tevents) _nlsimu->sendEvent(te); }
public:
    Net(unsigned sevent, list<unsigned> tevents, NLSimulatorBase *nlsimu) : _tevents(tevents), _nlsimu(nlsimu)
    {
        _eventHandler = new EventHandler( _nlsimu->router(), sevent,
            [this](Event,unsigned long) { this->relay(); } );
        _eventHandler->start();
    }
    ~Net() { delete _eventHandler; }
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
        auto spin = getPinFromMap(spinid, pinmap);
        list<Pin*> tpins;
        for(auto tpinid:tpinids)
        {
            auto tpin = getPinFromMap( tpinid, pinmap );
            tpins.push_back( tpin );
            tpin->markDriven();
        }
        for(int i=0; i<Pin::EVENTTYPS; i++)
        {
            auto seid = spin->getEid(i);
            list<unsigned> teids;
            for(auto tpin:tpins)
                teids.push_back( tpin->getEid(i) );
            auto net = new Net(seid,teids,_nlsimu);
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
        auto pin = gate->getPin<isipin>(portname,pinindex);
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
    // TODO: To set limited pins of a port suitable api variant can be added
    template<unsigned W> void setPort(unsigned opid, string portname, bitset<W> val)
    {
        auto gate = getGate(opid);
        for( int i=0; i<W; i++ )
        {
            auto pin = gate->getPin<true>(portname, i);
            if ( not pin->isSysInp() )
            {
                cout << "set invoked on non system input pin " << opid <<  " " << portname << " " << i << endl;
                exit(1);
            }
            pin->setViaEvent( val[i] );
        }
    }
    void watch(unsigned opid, string portname) { getGate(opid)->getPort(portname)->watch(); }
    void unwatch(unsigned opid, string portname) { getGate(opid)->getPort(portname)->unwatch(); }
    // Note: init must be called after construction of factory
    // since it does sendEvent, which needs to happen from a different thread
    void init()
    {
        for(auto ig:_gatemap) ig.second->init();
    }
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
        for(auto ipint:ipinl) process_pin<true>(ipint, pinmap);

        auto opinl = netlistdb.terms2tuples<t_pin>(ps_opin);
        for(auto opint:opinl) process_pin<false>(opint, pinmap);

        auto evl = netlistdb.terms2tuples<t_ev>(ps_ev);
        for(auto evt : evl) process_ev(evt, pinmap);

        auto netl = netlistdb.terms2tuples<t_net>(ps_net);
        for(auto nett : netl) process_net(nett, pinmap);

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
