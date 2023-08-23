#!/usr/bin/env python3

import sys
from edifir import EdifIR


if len(sys.argv) != 2:
    print('Usage:',sys.argv[0],'<ediffile>')
    sys.exit()
edifir = EdifIR(sys.argv[1])
edifir.print2prolog()
