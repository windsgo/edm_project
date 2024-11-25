i = RS274Interpreter()

i.g54()
i.g91()

def func():
    i.g00(x=10, m05=True)
    i.g00(x=-10, m05=True)

i.f(1000)
for jj in range(100):
    func()

i.m02()
