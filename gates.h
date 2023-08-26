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

class Gate
{
};

class CARRY4 : public Gate
{
public:
    CARRY4(unsigned opid, Parmap& parmap)
    {
    }
};

class FDCE : public Gate
{
public:
    FDCE(unsigned opid, Parmap& parmap)
    {
    }
};

template<bool val> class Const : public Gate
{
public:
    Const(unsigned opid, Parmap& parmap)
    {
    }
};

class IBUF : public Gate
{
public:
    IBUF(unsigned opid, Parmap& parmap)
    {
    }
};

template<int suf> class LUT : public Gate
{
public:
    LUT(unsigned opid, Parmap& parmap)
    {
    }
};

class OBUF : public Gate
{
public:
    OBUF(unsigned opid, Parmap& parmap)
    {
    }
};

class OBUFT : public Gate
{
public:
    OBUFT(unsigned opid, Parmap& parmap)
    {
    }
};

typedef struct {
    const function< Gate* (unsigned,Parmap&) > creator;
    const Parmap defaults;
    const set<string> nodefaults;
    const map<string,unsigned> iports;
    const map<string,unsigned> oports;
} GateInfo;

typedef tuple<string,unsigned,string,list<tuple<string,string>>> t_opi;
#define CREATE(GATETYP) [](unsigned opid, Parmap& parmap) { return new GATETYP(opid,parmap); }
class GateFactory
{
    const map<string,string> _lutdefaults = { { "SOFT_HLUTNM",""}, { "box_type",""} };
    const set<string> _lutnondefaults = { "INIT" };
const map< string, GateInfo  > _gateinfomap = {
    { "CARRY4", { CREATE(CARRY4),
        { {"ADDER_THRESHOLD",""} },
        {},
        { {"CYINIT",1}, {"CI",1}, {"S",4}, {"DI",4} },
        { {"CO",4}, {"O",4} },
        }
    },
    { "FDCE", { CREATE(FDCE),
        { {"IS_C_INVERTED","1'b0"} },
        {"INIT"},
        { {"C",1},{"CE",1},{"CLR",1},{"D",1} },
        { {"Q",1} },
        }
    },
    { "GND", { CREATE(Const<false>),
        {},
        {},
        {},
        { {"G",1} },
        }
    },
    { "IBUF", { CREATE(IBUF),
        {},
        {"CCIO_EN"},
        { {"I",1} },
        { {"O",1} },
        }
    },
    { "LUT1", { CREATE(LUT<1>),
        _lutdefaults,
        _lutnondefaults,
        { {"I0",1} },
        { {"O",1} },
        }
    },
    { "LUT2", { CREATE(LUT<2>),
        _lutdefaults,
        _lutnondefaults,
        { {"I0",1}, {"I1",1} },
        { {"O",1} },
        }
    },
    { "LUT3", { CREATE(LUT<3>),
        _lutdefaults,
        _lutnondefaults,
        { {"I0",1}, {"I1",1}, {"I2",1} },
        { {"O",1} },
        }
    },
    { "LUT4", { CREATE(LUT<4>),
        _lutdefaults,
        _lutnondefaults,
        { {"I0",1}, {"I1",1}, {"I2",1}, {"I3",1} },
        { {"O",1} },
        }
    },
    { "LUT5", { CREATE(LUT<5>),
        _lutdefaults,
        _lutnondefaults,
        { {"I0",1}, {"I1",1}, {"I2",1}, {"I3",1}, {"I4",1} },
        { {"O",1} },
        }
    },
    { "LUT6", { CREATE(LUT<6>),
        _lutdefaults,
        _lutnondefaults,
        { {"I0",1}, {"I1",1}, {"I2",1}, {"I3",1}, {"I4",1}, {"I5",1} },
        { {"O",1} },
        }
    },
    { "OBUF", { CREATE(OBUF),
        {},
        {},
        { {"I",1} },
        { {"O",1} },
        }
    },
    { "OBUFT", { CREATE(OBUFT),
        {},
        {},
        { {"I",1}, {"T",1} },
        { {"O",1} },
        }
    },
    { "VCC", { CREATE(Const<true>),
        {},
        {},
        {},
        { {"P",1} },
        }
    },
};
    map< unsigned, Gate* > _gatemap;
    void mergeDefaults(Parmap& parmap, const Parmap& defaults)
    {
        for(auto pv:defaults)
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
        auto it2 = _gateinfomap.find(optyp);
        if ( it2 == _gateinfomap.end() )
        {
            cout << "Unknown gate type: " << optyp << endl;
            return; // exit(1);
        }
        auto gateinfo = it2->second;
        auto parlist = get<3>(opit);
        Parmap parmap;
        for(auto pv:parlist)
            parmap.emplace( get<0>(pv), get<1>(pv) );
        mergeDefaults(parmap, gateinfo.defaults);
        auto expparams = gateinfo.defaults.size() + gateinfo.nodefaults.size();
        auto actualparams = parmap.size();
        if ( expparams != actualparams )
        {
            cout << "Generic parameter count mismatch for opid:" << opid
                << " expected:" << expparams << " actual:" << actualparams << endl;
            return; // exit(1);
        }
        auto gate = gateinfo.creator(opid,parmap);
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
