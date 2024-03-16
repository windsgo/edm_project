i = RS274Interpreter()

i.g54()
i.g91()
i.e(5)
i.f(300)
i.g00(x=5, z=10)
for j in range(5):
    i.g00(x=5)
    i.g00(x=-5)
i.m02() # end of program

# following won't generate code

# print(i.get_command_list())
# print(i.get_command_list_jsonstr())

# print(i.__class__.__name__)

# import sys
# d = sys.modules[__name__].__dict__
# print(d["RS274Interpreter"].__name__)
# print(i.__str__())


