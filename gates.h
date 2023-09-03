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

class Gate
{
protected:
    unsigned _opid;
    NLSimulatorBase *_nlsimu;
    virtual void handleParmap(Parmap&) {}
public:
    virtual Pin* getIPin(string portname, unsigned pinindex)=0;
    virtual Pin* getOPin(string portname, unsigned pinindex)=0;
    virtual void setEventHandlers()=0;
    virtual void evalWithId(unsigned) {}
    virtual void eval() {}
    virtual void init() {}
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
    virtual void set(bool val, NLSimulatorBase *nlsimu) {}
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
        eval(gate);
    }
protected:
    virtual void eval(Gate* gate) { gate->eval(); }
public:
    void setEventHandlers(Gate *gate, NLSimulatorBase *nlsimu)
    {
        _eventHandlers[0] = new EventHandler( nlsimu->router(), Pin::_eids[0],
            [this,gate](Event,unsigned long) { this->handle<0>(gate); } );
        _eventHandlers[1] = new EventHandler( nlsimu->router(), Pin::_eids[1],
            [this,gate](Event,unsigned long) { this->handle<1>(gate); } );
        for(int i=0; i<Pin::EVENTTYPS; i++) _eventHandlers[i]->start();
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
    void eval(Gate*) {}
};

template<unsigned W, unsigned ID> class WithIdIPin : public IPin<W>
{
using IPin<W>::IPin;
protected:
    void eval(Gate* gate) { gate->evalWithId(ID); }
};

typedef function<void(bool,NLSimulatorBase*)> t_setterfn;
template<unsigned W> class OPin : public PinState<W>
{
using PinState<W>::PinState;
    // For first ever 'set' call we must send the event, later only on state change
    // Instead of checking a flag every time, we use a function pointer
    // Does this really pay off or a flag would be cheaper? TODO: Experiment
    void _do_set(bool val, NLSimulatorBase *nlsimu)
    {
        PinState<W>::_state = val;
        nlsimu->sendEvent( Pin::_eids[val] );
    }
    t_setterfn
        _set_init = [this](bool val, NLSimulatorBase *nlsimu)
        {
            _do_set(val,nlsimu);
            _setter = _set_postinit;
        },
        _set_postinit = [this](bool val, NLSimulatorBase *nlsimu)
        {
            if ( PinState<W>::_state != val ) _do_set(val,nlsimu);
        },
        _setter = _set_init;
public:
    void set(bool val, NLSimulatorBase *nlsimu) { this->_setter(val,nlsimu); }
};

class PortBase
{
public:
    virtual void setEventHandlers(Gate *, NLSimulatorBase *) {}
    virtual Pin* getPin(unsigned index)=0;
};

template<unsigned W, typename PT> class Port : public PortBase
{
protected:
    PT *_pins[W];
    bitset<W> _state;
public:
    bitset<W>& state() { return _state; }
    bool operator [] ( int i ) { return _state[i]; }
    void set(bitset<W>& val, NLSimulatorBase *nlsimu)
    {
        for(int i=0; i<W; i++) _pins[i]->set(val[i], nlsimu);
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
    Port<W,PT>()
    {
        for(int i=0; i<W; i++) _pins[i] = new PT( _state[i] );
    }
};

#define IPort(W) Port<W,IPin<W>>
#define PassiveIPort(W) Port<W,PassiveIPin<W>>
#define WithIdIPort(W,Id) Port<W,WithIdIPin<W,Id>>
#define OPort(W) Port<W,OPin<W>>

typedef map<string,PortBase*> Portmap;
#define DEFPARM static inline const Parmap _defaults =
#define NODEFPARM static inline const set<string> _nodefaults =
#define PORT(PORTNAME) {#PORTNAME,&PORTNAME}
class CARRY4 : public Gate
{
protected:
    DEFPARM   { {"ADDER_THRESHOLD",""} };
    NODEFPARM {};
    IPort(1) CYINIT;
    IPort(1) CI;
    IPort(4) S;
    IPort(4) DI;
    Portmap _iportmap = {
        PORT(CYINIT),
        PORT(CI),
        PORT(S),
        PORT(DI),
        };
    OPort(4) CO;
    OPort(4) O;
    Portmap _oportmap = {
        PORT(CO),
        PORT(O),
        };
public:
// See https://github.com/awersatos/AD/blob/master/Library/HDL%20Simulation/Xilinx%20ISE%2012.1%20VHDL%20Libraries/unisim/src/primitive/CARRY4.vhd
    void eval()
    {
        bool ci_or_cyinit = CI[0] or CYINIT[0];

        bitset<4> CO_out;
        CO_out[0] = S[0] ? ci_or_cyinit : DI[0];
        for(int i=1;i<4;i++)  CO_out[i] = S[i] ? CO_out[i-1] : DI[i];
        CO.set(CO_out,_nlsimu);

        bitset<4> O_out;
        O_out[0] = S[0] ^ ci_or_cyinit;
        for(int i=1;i<4;i++)  O_out[i] = S[i] ^ CO_out[i-1];
        O.set(O_out,_nlsimu);
    }
};

class FDCE : public Gate
{
    typedef enum { ID_C, ID_CLR } t_PortId;
protected:
    DEFPARM   { {"IS_C_INVERTED","1'b0"} };
    NODEFPARM {"INIT"};
    WithIdIPort(1,ID_C) C;
    WithIdIPort(1,ID_CLR) CLR;
    PassiveIPort(1) CE;
    PassiveIPort(1) D;
    Portmap _iportmap = {
        PORT(C),
        PORT(CE),
        PORT(CLR),
        PORT(D),
        };
    OPort(1) Q;
    Portmap _oportmap = {
        PORT(Q),
        };
public:
    void evalWithId(t_PortId portid)
    {
        if ( CLR[0] )
        {
            bitset<1> val = 0;
            Q.set(val,_nlsimu);
        }
        else if ( portid == ID_C and CE[0] and C[0] )
            Q.set(D.state(),_nlsimu);
    }
};

class GND : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    Portmap _iportmap = {};
    OPort(1) G;
    Portmap _oportmap = {
        PORT(G),
        };
public:
    void init()
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
    IPort(1) I;
    Portmap _iportmap = {
        PORT(I),
        };
    OPort(1) O;
    Portmap _oportmap = {
        PORT(O),
        };
public:
    void eval() { O.set(I.state(),_nlsimu); }
};

#define LUTSZ 1<<W
template<unsigned W> class LUT : public Gate
{
    bitset<LUTSZ> _o;
protected:
    DEFPARM   { { "SOFT_HLUTNM",""}, { "box_type",""} };
    NODEFPARM { "INIT" };
    IPort(1) I[W];
    Portmap _iportmap;
    OPort(1) O;
    Portmap _oportmap = {
        PORT(O),
        };
    void handleParmap(Parmap& parmap)
    {
        auto init = parmap["INIT"];
        HDLConst<LUTSZ> initconst {init};
        _o = initconst.as_bitset();
    }
public:
    void eval()
    {
        int ival = 0;
        for(int i=0, mask=1; i<W; i++, mask<<=1)
            if ( I[i][0] ) ival+=mask;
        bitset<1> o = { _o[ival] };
        O.set(o,_nlsimu);
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
    IPort(1) I;
    Portmap _iportmap = {
        PORT(I),
        };
    OPort(1) O;
    Portmap _oportmap = {
        PORT(O),
        };
public:
    void eval() { O.set(I.state(),_nlsimu); }
};

class OBUFT : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    IPort(1) I;
    IPort(1) T;
    Portmap _iportmap = {
        PORT(I),
        PORT(T),
        };
    OPort(1) O;
    Portmap _oportmap = {
        PORT(O),
        };
public:
    void eval()
    {
        if (T[0]) O.set(I.state(),_nlsimu);
    }
};

class VCC : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    Portmap _iportmap = {};
    OPort(1) P;
    Portmap _oportmap = {
        PORT(P),
        };
public:
    void init()
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
        T::handleParmap(parmap);
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
    list<unsigned> _tevents;
    EventHandler *_eventHandler;
    void relay(NLSimulatorBase *nlsimu) { for(auto te:_tevents) nlsimu->sendEvent(te); }
public:
    Net(unsigned sevent, list<unsigned> tevents, NLSimulatorBase *nlsimu) : _tevents(tevents)
    {
        _eventHandler = new EventHandler( nlsimu->router(), sevent,
            [this,nlsimu](Event,unsigned long) { this->relay(nlsimu); } );
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
    set< unsigned > _inpinset;
    list<Net*> _netlist;
    NLSimulatorBase *_nlsimu;
    t_pinmap _pinmap;
    bool isSysInpPin(unsigned pinid)
    {
        auto it = _inpinset.find(pinid);
        return it != _inpinset.end();
    }
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
    Pin* getPinFromMap(unsigned pinid)
    {
        auto it = _pinmap.find(pinid);
        if ( it == _pinmap.end() )
        {
            cout << "Invalid pin id sought " << pinid << endl;
            exit(1);
        }
        return it->second;
    }
    void process_net(t_net& nett)
    {
        auto spinid = get<1>(nett);
        auto tpinids = get<2>(nett);
        auto spin = getPinFromMap(spinid);
        list<Pin*> tpins;
        for(auto tpinid:tpinids)
        {
            tpins.push_back( getPinFromMap(tpinid) );
            _inpinset.erase( tpinid );
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
    void process_ev(t_ev& evt)
    {
        auto eid = get<1>(evt);
        auto pinid = get<2>(evt);
        auto tv = get<3>(evt);
        auto pin = getPinFromMap(pinid);
        pin->setEid(tv,eid);
    }
    template<bool isipin> void process_pin(t_pin& pint)
    {
        auto pinid = get<1>(pint);
        auto opid = get<2>(pint);
        auto portname = get<3>(pint);
        auto pinindex = get<4>(pint);
        auto gate = getGate(opid);
        Pin *pin;
        if constexpr ( isipin )
        {
            pin = gate->getIPin(portname,pinindex);
            _inpinset.insert(pinindex); // Note: later removed if found driven
        }
        else
            pin = gate->getOPin(portname,pinindex);
        _pinmap.emplace(pinid,pin);
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
    void setPin(unsigned pinid, bool tv)
    {
        if ( not isSysInpPin(pinid) )
        {
            cout << "setPin invoked on non system input pin " << pinid << endl;
            exit(1);
        }
        auto pin = getPinFromMap(pinid);
        auto eid = pin->getEid( tv ? 1 : 0 );
        _nlsimu->sendEvent( eid );
    }
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

        auto ipinl = netlistdb.terms2tuples<t_pin>(ps_ipin);
        for(auto ipint:ipinl) process_pin<true>(ipint);

        auto opinl = netlistdb.terms2tuples<t_pin>(ps_opin);
        for(auto opint:opinl) process_pin<false>(opint);

        auto evl = netlistdb.terms2tuples<t_ev>(ps_ev);
        for(auto evt : evl) process_ev(evt);

        auto netl = netlistdb.terms2tuples<t_net>(ps_net);
        for(auto nett : netl) process_net(nett);

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
