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

class Gate
{
protected:
    unsigned _opid;
};

#define DEFPARM static inline const Parmap _defaults =
#define NODEFPARM static inline const set<string> _nodefaults =
class CARRY4 : public Gate
{
protected:
    DEFPARM   { {"ADDER_THRESHOLD",""} };
    NODEFPARM {};
    //{ {"CYINIT",1}, {"CI",1}, {"S",4}, {"DI",4} },
    //{ {"CO",4}, {"O",4} },
};

class FDCE : public Gate
{
protected:
    DEFPARM   { {"IS_C_INVERTED","1'b0"} };
    NODEFPARM {"INIT"};
    //{ {"C",1},{"CE",1},{"CLR",1},{"D",1} },
    //{ {"Q",1} },
};

class GND : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    //{},
    //{ {"G",1} },
};

class IBUF : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {"CCIO_EN"};
    //{ {"I",1} },
    //{ {"O",1} },
};

template<int suf> class LUT : public Gate
{
protected:
    DEFPARM   { { "SOFT_HLUTNM",""}, { "box_type",""} };
    NODEFPARM { "INIT" };
    //{ {"I0",1}, {"I1",1}, {"I2",1}, {"I3",1}, {"I4",1}, {"I5",1} },
    //{ {"O",1} },
};

class OBUF : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    //{ {"I",1} },
    //{ {"O",1} },
};

class OBUFT : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    //{ {"I",1}, {"T",1} },
    //{ {"O",1} },
};

class VCC : public Gate
{
protected:
    DEFPARM   {};
    NODEFPARM {};
    //{},
    //{ {"P",1} },
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
