i = RS274Interpreter()

i.g55()
i.f(1000)
i.g91()

i.f(1000).g90().g00(x=1, y=2, z=3)

ps = Points()
ps.g90()
ps.push(x=3)
ps.g91()
ps.push(y=0)
ps.g91()
ps.push(b=-1)

i.g01_group(ps)

i.m02()