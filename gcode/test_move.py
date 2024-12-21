i = RS274Interpreter()

i.g54()
i.f(1000)
i.g91()
for j in range(10):
    i.g00(y=5)
    i.g00(y=-5)

i.m02()