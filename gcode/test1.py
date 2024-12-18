i = RS274Interpreter()

i.g54()
i.g91()

def func():
    i.g54()

i.f(1000)
for jj in range(100):
    func()

i.m02()
