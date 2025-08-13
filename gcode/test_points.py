i = RS274Interpreter()

i.g54()
i.f(1000).g90().g00(x=1, y=1, z=1)

ps = Points()
ps.g90().f(100)
ps.push(x=3)
#ps.f(10)
ps.push(y=1000, f=20)
ps.f(100)
ps.push(z=3)
ps.push(x=6,y=6,z=6)
ps.g91()
ps.push(x=6,y=6,z=6)

i.g01_group(ps)

i.m02()