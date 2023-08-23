# module to convert Python data structures to Prolog terms
# Python data structures need to be PTerms

class PTerm:
    def __str__(self): return self.functor + str(self.argterm)
    def __init__(self,functor,args):
        self.functor = functor
        self.argterm = PTuple(args)

class PList(PTerm):
    def __str__(self): return '[' + ','.join(
        str(e) for e in self.elems
        ) + ']'
    def __init__(self,elems): self.elems = elems

class PTuple(PTerm):
    def __str__(self): return '(' + ','.join(
        str(a) for a in self.targs
        ) + ')'
    def __init__(self,targs): self.targs = targs

# Note: Use PAtom for strings that need quoting in Prolog, else native items
# are ok
class PAtom(PTerm):
    def __str__(self): return ("'"+self.atom.replace("'","''")+"'") if isinstance(
        self.atom,str ) else str(self.atom)
    def __init__(self,atom): self.atom = atom

