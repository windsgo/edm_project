ii = RS274Interpreter()

ii.g54()
ii.g04(t=1)
ii.g90().g04(t=1)
ii.f(1000)
for x in range(2):
    ii.g00(x=1, y=0, z=0).g04(t=1)
    ii.g04(t=1).g04(t=1)

ii.e(1).g04(t=1)
ii.g01(z=1).g04(t=1)
ii.m02().g04(t=1)

# print(ii.get_command_list())
# print(ii.get_command_list_jsonstr())

# print(ii.__class__.__name__)

# import sys
# d = sys.modules[__name__].__dict__
# print(d["RS274Interpreter"].__name__)
# print(ii.__str__())


