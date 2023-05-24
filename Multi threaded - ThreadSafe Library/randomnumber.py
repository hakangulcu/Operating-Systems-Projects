import random
f = open("input.txt", "a")
for i in range(10):
    a = pow(2, 20)
    rand = random.randint(0, a)
    f.write(str(rand))
    f.write("\n")