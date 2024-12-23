i = RS274Interpreter()

i.g55()
i.f(1000)
i.g91()

# i.f(1000).g90().g00(x=0, y=0, z=0)

# Touch Once
i.f(100).g91().g00(a=-100, touch=True)
i.g04(1)
i.f(500).g91().g00(a=1, m05=True)
i.g04(1)

# Touch Twice
i.f(50).g91().g00(a=-5, touch=True)
i.g04(1)
# set zero
i.coord_set_a_zero()
i.g04(1)

# up
i.f(500).g91().g00(a=1, m05=True)
i.g04(1)

# drill
ps = Points()
ps.g90()
ps.push(a=-0.2) # drill

# mill
ps.g91()
ps.push(x=2)
ps.push(y=2)
ps.push(x=2, y=2)
for num in range(1000):
    ps.push(x=0.001)

i.g01_group(ps)

i.m02()