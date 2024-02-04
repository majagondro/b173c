import sys

shaders = [f for f in sys.argv[2:]]

f = open(sys.argv[1], "w+")

for shader in shaders:
    f.write(f"\nconst char *{shader.split('/')[-1].replace('.', '_')} =")

    with open(shader, "r") as fs:
        for line in fs.readlines():
            if line.strip() == "":
                continue
            line = line.replace('"', '\\"')
            f.write(f"\n\"{line.strip()}\\n\"")

    f.write(";\n")

f.close()




