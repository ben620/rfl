N = 100
MAX_COL = 32

for ii in range(1, N + 1):
    print(f"MAKE_PTR_ANY({ii}", end='')
    for jj in range(ii):
        if (jj + 1) % MAX_COL == 0:
            print("\\");
        print(f",_{jj}", end='')
    print(");")