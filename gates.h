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

template<int suf> class LUT : public Gate
{
    const Parmap _defaults = {
        { "INIT", "1" },
        };
public:
    LUT(unsigned opid, Parmap& parmap)
    {
    }
};

typedef struct {
    const function< Gate* (unsigned,Parmap&) > creator;
    const Parmap defaults;
    const set<string> nodefaults;
} GateInfo;

typedef tuple<string,unsigned,string,list<tuple<string,string>>> t_opi;
#define CREATE(GATETYP) [](unsigned opid, Parmap& parmap) { return new GATETYP(opid,parmap); }
class GateFactory
{
const map< string, GateInfo  > _gateinfomap = {
    { "LUT1", { CREATE(LUT<1>),
        {},
        {},
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
