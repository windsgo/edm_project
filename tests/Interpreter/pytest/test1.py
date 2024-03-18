i = RS274Interpreter()

i.g54()
i.g04(t=1)
i.g90()
i.g04(t=1)
i.e(5)
i.m00()
i.g04(t=1)
i.f(1000)
for j in range(3):
    i.g04(t=0.5)
    i.g00(x=10, y=10)
    i.g04(t=0.5)
    i.g00(x=0, y=0)
i.m02() # end of program

# following won't generate code

# print(i.get_command_list())
# print(i.get_command_list_jsonstr())

# print(i.__class__.__name__)

# import sys
# d = sys.modules[__name__].__dict__
# print(d["RS274Interpreter"].__name__)
# print(i.__str__())


