#ifndef _GATES_H
#define _GATES_H

#include <map>
#include <tuple>
#include <set>
#include <string>
#include <functional>

using namespace std;
#include "xsb2cpp.h"

typedef map<string,string> Parmap;
typedef list<tuple<string,string>> Parlist;

class Pin
{
};

class IPin : public Pin
{
};

class OPin : public Pin
{
};

class PortBase
{
};

template<unsigned W> class Port : public PortBase
{
    Pin *_pins[W];
};

template<unsigned W> class IPort : public Port<W>
{
};

template<unsigned W> class OPort : public Port<W>
{
};

class Gate
{
protected:
    unsigned _opid;
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
};

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
public:
    GateMethods<T>(unsigned opid, Parlist& parlist)
    {
        Gate::_opid = opid;
        processParmap(parlist);
    }
};

typedef tuple<string,unsigned,string,Parlist> t_opi;
#define CREATE(GATETYP,CLS) { #GATETYP, [](unsigned opid, Parlist& parlist) { return new GateMethods<CLS>(opid,parlist); } }
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
    GateFactory(string netlistir)
    {
        t_predspec ps_opi = {"opi",3};
        PDb netlistdb;
        netlistdb.load(netlistir,{ps_opi});
        auto opil = netlistdb.terms2tuples<t_opi>(ps_opi);
        for(auto opit : opil) process_opi(opit);
    }
    ~GateFactory()
    {
        for(auto ig:_gatemap) delete ig.second;
    }
};

// TODOs
// - Define pin class, work out pin bindings in a gate, avoid interpreting names at run time (ok during construction)
// - Pin class will senf an event when its value changes (only when new value is different from the previous)
//   for which the event ids for 0 and 1 will be a property known to the pin at construction time

#endif
