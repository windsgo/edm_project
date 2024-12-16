i = RS274Interpreter()

i.g54()
i.f(1000)
i.g91()
for j in range(10):
    i.g00(x=5)
    i.g00(x=-5)

i.m02()