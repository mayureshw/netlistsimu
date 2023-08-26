import sys
import spydrnet as sdn
from spydrnet.flatten import flatten
from pterms import *
from itertools import groupby

class EdifIR:
    def instid(self,instname): return self.instids.setdefault(instname,len(self.instids))
    def pinid(self,iid,portname,pinindex): return self.pinids.setdefault((iid,portname,pinindex),len(self.pinids))
    def eventid(self,pin,val): return self.eventids.setdefault((pin,val),len(self.eventids))
    def instname(self,inst): return inst.data['EDIF.identifier']
    def portname(self,port): return port.name if port.is_scalar else port.name.partition('[')[0]
    def processEvent(self,pinid,tv):
        eid = self.eventid(pinid,tv)
        term = PTerm( 'ev', [ eid, pinid, tv ] )
        print(str(term)+'.')
    def processPin(self,direction,iid,pname,pinindex):
        functor = direction + 'pin'
        pinid = self.pinid(iid,pname,pinindex)
        term = PTerm(functor,[pinid,iid,PAtom(pname),pinindex])
        print(str(term)+'.')
        for tv in [0,1]: self.processEvent(pinid,tv)
    def processPort(self,iid,port):
        if port.direction not in {sdn.IN,sdn.OUT}:
            print('Unhandled port direction',port.direction)
            sys.exit(1)
        direction = 'i' if port.direction is sdn.IN else 'o'
        pname = self.portname(port)
        for pinindex in range(len(port.pins)): self.processPin(direction,iid,pname,pinindex)
    def processInst(self,inst):
        iname = self.instname(inst)
        iid = self.instid(iname)
        typ = PAtom(inst.reference.name)
        generics = PList(
            PTuple([ PAtom(str(propd[k])) for k in ['identifier','value'] ])
            for propd in inst.data.get('EDIF.properties',[])
            )
        term = PTerm('opi',[iid, typ, generics])
        print(str(term)+'.')
        for port in inst.get_ports(): self.processPort(iid,port)
    def wire2driver(self,w):
        drivers = w.get_driver()
        if len(drivers) > 1:
            print('>1 drivers for wire',w,drivercnt)
            sys.exit(1)
        return drivers[0] if len(drivers) == 1 else None
    def pin2port(self,p):
        ports = list( p.get_ports() )
        if len(ports) != 1:
            print('len(ports)!=1 for pin',p)
            sys.exit(1)
        return ports[0]
    def pin2inst(self,p):
        instances = list( p.get_instances() )
        if len(instances) != 1:
            print('len(instances)!=1 for pin',p,len(instances))
            sys.exit(1)
        return instances[0]
    def pininst2id(self,p):
        port = self.pin2port(p)
        pname = self.portname(port)
        instance = self.pin2inst(p)
        iid = self.instid(self.instname(instance))
        return self.pinid(iid,pname,p.index())
    def processWire(self,w):
        driver = self.wire2driver(w)
        if driver == None: return
        if not isinstance(driver,sdn.OuterPin): return
        driverid = self.pininst2id( driver )
        receiverids = [ self.pininst2id(p)
            for p in w.pins if p != driver and isinstance(p,sdn.OuterPin) ]
        if len(receiverids) > 0:
            term = PTerm('net',[driverid,PList(receiverids)])
            print(str(term)+'.')
    def print2prolog(self):
        for inst in  self.netlist.get_instances(): self.processInst(inst)
        for w in self.netlist.get_wires(): self.processWire(w)
    def __init__(self, filename):
        self.netlist = sdn.parse(filename)
        flatten(self.netlist)
        self.instids = {}
        self.pinids = {}
        self.eventids = {}
