import sys
import spydrnet as sdn
from spydrnet.flatten import flatten
from pterms import *
from itertools import groupby

class EdifIR:
    def instid(self,instname): return self.instids.setdefault(instname,len(self.instids))
    def pinid(self,iid,portname): return self.pinids.setdefault((iid,portname),len(self.pinids))
    def eventid(self,pin,val): return self.eventids.setdefault((pin,val),len(self.eventids))
    def instname(self,inst): return inst.data['EDIF.identifier']
    def printopi(self):
        for inst in  self.netlist.get_instances():
            iname = self.instname(inst)
            iid = self.instid(iname)
            typ = PAtom(inst.reference.name)
            generics = PList(
                PTuple([ PAtom(str(propd[k])) for k in ['identifier','value'] ])
                for propd in inst.data.get('EDIF.properties',[])
                )
            term = PTerm('opi',[iid, typ, generics])
            print(str(term)+'.')
    def printevents(self,pin):
        for tv in [0,1]:
            term = PTerm('eid',[pin,tv,self.eventid(pin,tv)])
            print(str(term)+'.')
    def printpins(self):
        for w in self.netlist.get_wires():
            driverpins = w.get_driver()
            if len(driverpins) > 1:
                print('wire has >1 drivers',w)
                sys.exit(1)
            if len(driverpins) == 0 and len(w.pins) > 0:
                print('>0 pins but 0 drivers for wire',w)
                sys.exit(1)
            if len(driverpins) == 0: continue
            driver = driverpins[0]
            receiverids = []
            for p in w.pins:
                ports = list( p.get_ports() )
                if len(ports) != 1:
                    print('len(ports)!=1 for pin',p)
                    sys.exit(1)
                port = ports[0]
                instances = list( p.get_instances() )
                if len(instances) != 1:
                    print('len(instances)!=1 for pin',p,len(instances))
                    sys.exit(1)
                instance = instances[0]
                iid = self.instid(self.instname(instance))
                pid = self.pinid(iid,port.name)
                if p == driver:
                    functor = 'opin'
                    driverid = pid
                else:
                    functor = 'ipin'
                    receiverids += [pid]
                term = PTerm(functor,[pid,iid,PAtom(port.name)])
                print(str(term)+'.')
                self.printevents(pid)
            term = PTerm('net',[driverid,PList(receiverids)])
            print(str(term)+'.')
    def print2prolog(self):
        self.printopi()
        self.printpins()
    def __init__(self, filename):
        self.netlist = sdn.parse(filename)
        flatten(self.netlist)
        self.instids = {}
        self.pinids = {}
        self.eventids = {}
