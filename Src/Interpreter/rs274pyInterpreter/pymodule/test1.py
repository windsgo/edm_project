from rs274 import *
import traceback
from inspect import stack

i = RS274Interpreter()

try:
    a = 0/0
except Exception as e:
    print (e.__traceback__)
    print (e.__traceback__.tb_frame.f_globals["__file__"])
    print (e.__traceback__.tb_lineno)
    
    print (traceback.extract_tb(e.__traceback__)[0])
    exit()

i.g54()
# i.g04(t='a')
i.g90().g04(t=1)
i.f(1000)
for x in range(2):
    i.g00(x=1, y=0, z=0).g04(t=1)
    i.g04(t=1).g04(t=1)

i.e(1).g04(t=1)
i.g01(z=1).g04(t=1)
i.m02().g04(t=1)

# print(i.get_command_list())
print(i.get_command_list_jsonstr())

# print(i.__class__.__name__)

# import sys
# d = sys.modules[__name__].__dict__
# print(d["RS274Interpreter"].__name__)
# print(i.__str__())


