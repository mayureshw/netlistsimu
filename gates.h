#ifndef _GATES_H
#define _GATES_H

#include <map>
#include <set>
#include <string>
#include <functional>

using namespace std;

typedef map<string,string> Parmap;

class Gate
{
    const Parmap _defaults;
public:
    Gate(unsigned opid, Parmap& parmap)
    {
    }
};

class LUT : public Gate
{
using Gate::Gate;
};

// TODO: Use templatized LUT instead of inheritance
class LUT1 : public LUT
{
    const Parmap _defaults = {
        { "INIT", "1" },
        };
using LUT::LUT;
};


typedef struct {
    const function< Gate* (unsigned,Parmap&) > creator;
    const Parmap defaults;
    const set<string> nodefaults;
} GateInfo;

#define CREATE(GATENAME) [](unsigned opid, Parmap& parmap) { return new GATENAME(opid,parmap); }
class GateFactory
{
const map< string, GateInfo  > _gatemap = {
    { "LUT1", { CREATE(LUT1),
        {},
        {},
        }
    },
};
public:
    GateFactory(string netlistir)
    {
    }
};

// TODOs
// - Read netlistir and invoke gate creator using opi
// - Implement property set validation (all props specified, no extra prop)
// - Pass property set to gate constructor
// - Define pin class, work out pin bindings in a gate, avoid interpreting names at run time (ok during construction)
// - Pin class will senf an event when its value changes (only when new value is different from the previous)
//   for which the event ids for 0 and 1 will be a property known to the pin at construction time

#endif
