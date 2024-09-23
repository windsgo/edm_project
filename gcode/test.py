i = RS274Interpreter()

i.g54()
i.g90()
i.f(2000)

def func():
    i.g00(x=20, y=20, m05=True)
    i.g00(x=0, y=0, m05=True)

for ii in range(10):
    func()

i.m02()
