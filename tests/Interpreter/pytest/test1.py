i = RS274Interpreter()

i.g54()
i.g90()
i.f(1000)
i.g00(x=5)
i.m02() # end of program

# following won't generate code

i.g54()
i.g04(t=1)
i.g90().g04(t=1)
i.f(1000)
for x in range(2):
    i.g00(x=1, y=0, z=0).g04(t=1)
    i.g04(t=1).g04(t=1)

i.e(1).g04(t=1)
i.g01(z=1).g04(t=1)
i.m02().g04(t=1)

# print(i.get_command_list())
# print(i.get_command_list_jsonstr())

# print(i.__class__.__name__)

# import sys
# d = sys.modules[__name__].__dict__
# print(d["RS274Interpreter"].__name__)
# print(i.__str__())


