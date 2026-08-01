int mathematical_modulo(int a, int b)
{
        // This is a workaround for the fact that the % operator in C++ can
        // return negative values if the first operand is negative.
        return (a % b + b) % b;
}

int modulo_increment(int a, int b) { return mathematical_modulo(a + 1, b); }
int modulo_decrement(int a, int b) { return mathematical_modulo(a - 1, b); }
