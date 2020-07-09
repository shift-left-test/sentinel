struct test {
    int i;
    int j;
};

struct test add(int a, int b)
{
    struct test t;
    t.i = a;
    t.j = b;

    if (a != b) {
        t.i = a + b;
    }

    return t;
}

int sub(int a, int b)
{
    a++;
    int d = b;
    if (a > b) {
        d += a;
    }
    return a - b;
}

int shifttest(int a)
{
    return a >> 3;
}

int bitwise(int a, int b)
{
    if (a || b) {
        return a & b;
    }
    return a | b;
}

bool negate(bool a)
{
    return !a;
}