i = RS274Interpreter()

i.g55()
i.f(1000)
i.g91()

ps = Points()
ps.g90()
ps.push(x=1, a=2, c=1.1)
ps.push(x=2)
ps.push(c=-20)
ps.g91()
ps.push(z=3)

i.g01_group(ps)

i.m02()