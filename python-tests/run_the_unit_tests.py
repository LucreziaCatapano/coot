
import sys
import unittest
import coot
import coot_utils
import coot_testing_utils

import os
sys.path.append(".")
# more general!?! append python-test dir, i.e. directory this file resides in
import inspect
this_file=inspect.getsourcefile(lambda:0)
test_dir=os.path.split(this_file)[0]
sys.path.append(test_dir)
# sys.path.append("../../coot/python-tests")

from TestPdbMtzFunctions    import *
from TestShelxFunctions     import *
from TestLigandFunctions    import *
from TestRNAGhostsFunctions import *
from TestSSMFunctions       import *
from TestNCSFunctions       import *
from TestUtilsFunctions     import *
from TestInternalFunctions  import *

# class to write output of unittest into a 'memory file' (unittest_output)
# as well as to sys.stdout
class StreamIO:
        
    def __init__(self, src=sys.stderr, dst=sys.stdout):
        import io
        global unittest_output
        unittest_output = io.StringIO()
        self.src = src
        self.dst = dst
        self.extra = unittest_output

    def write(self, msg):
        #self.src.write(msg)
        self.extra.write(msg)
        self.dst.write(msg)

    def flush(self):
        self.src.flush()
        self.dst.flush()
        self.extra.flush()
        pass


suite = unittest.TestSuite()
test_list = [TestPdbMtzFunctions, TestShelxFunctions, TestLigandFunctions, TestRNAGhostsFunctions,
             TestSSMFunctions, TestNCSFunctions, TestUtilsFunctions, TestInternalFunctions]

# test_list = [TestRNAGhostsFunctions]

for test in test_list:
    suite.addTests(unittest.TestLoader().loadTestsFromTestCase(test))

log = StreamIO(sys.stderr, sys.stdout)

result = unittest.TextTestRunner(stream=log, verbosity=2).run(suite)

# Shouldnt we exit!?
# cheating?! We only exit Coot if we are not in graphics mode
if (coot.use_graphics_interface_state() == 0):
    if (result.wasSuccessful()):
        coot.coot_real_exit(0)
    else:
        coot.coot_real_exit(1)
