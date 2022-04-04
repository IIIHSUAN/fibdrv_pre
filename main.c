#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*
 * max digit_count & max sizeof(bignum.data) = max(unsigned int)
 * in iterator, max digit_count is max(int) / 8
 */
struct bignum
{
    unsigned char *data;
    unsigned char *string;  // Ex. '5', '6', '0', '8', '\0', '\0'
    unsigned int size, data_size;  // current_size
};

void trim(struct bignum *b)
{
    for(int i = b->size - 1; i >= 0; i--)
        if (b->data[i]) {
            b->data_size = i + 1;
            break;
        }
}

/*
 * in decimal
 */
void add_bignum_string(struct bignum *b, int a)
{
    if (!b->string[0])
        b->string[0] = '0';
    b->string[0] += a;

    for (int i = 0; i < b->size && b->string[i]; i++) {
        if (b->string[i] > '0' + 9) {  // carry
            b->string[i] -= 10;

            if (i < b->size - 1) {
                if (!b->string[i + 1])
                    b->string[i + 1] = '1';
                else
                    b->string[i + 1] += 1;
            }
        }
    }
}

void double_bignum_string(struct bignum *b)
{
    unsigned int carry = 0;
    for (int i = 0; i < b->size && (b->string[i] || carry); i++) {
        if (!b->string[i])
            b->string[i] = 0;
        else
            // double first
            b->string[i] = (b->string[i] - '0') << 1;

        // then add last_carry
        b->string[i] += carry;
        
        carry = (b->string[i] >= 10);// + (b->string[i] >= 20);
        
        b->string[i] = (b->string[i] % 10) + '0';
    }
}

void print_raw(struct bignum b)
{
    for (int i = 0; i < b.data_size; i++)
        printf("%u ", b.data[i]);
    printf("\n");
}

#define print_bignum(x) _print_bignum(x, #x);
void _print_bignum_string(struct bignum b)
{
    for (int i = b.size - 1; i >= 0; i--) {
        if(b.string[i])
            printf("%c", b.string[i]);
    }
    printf("\n");
}
void _print_bignum(struct bignum b, const char *name)
{
    unsigned int dec = 0, shift = 0, base = 1, ind = 1;

    printf("[bignum %s] (max_size: %u, data_size: %u)  \n", name, b.size, b.data_size);

    // find MSB
    int i = b.data_size - 1;
    for (; i >= 0; i--) {
        if (b.data[i])
            break;
        else if (i == 0) {
            printf("  |> %s = 0\n", name);
            return;
        }
    }
    
    int offset = 31 - __builtin_clz(b.data[i]);  // Ex. 00000100 -> 3

    printf("  |> ");
    print_raw(b);

    memset(b.string, 0, b.size);
    for (; i >= 0; i--) {
        for (; offset >= 0; offset--) {
            double_bignum_string(&b);

            if (b.data[i] & (1 << offset))
                add_bignum_string(&b, 1);
        }
        offset = 7;
    }
    printf("  |> %s = ", name);
    _print_bignum_string(b);
}

#define init_bignum(b, _size) \
struct bignum b = {\
.data = malloc(_size * sizeof(unsigned char)),\
.string = malloc(_size * sizeof(unsigned char)),\
.size = _size,\
.data_size = 0\
};

void set_bignum_value(struct bignum *b, unsigned int value)
{
    memset(b->data, 0, b->size);

    memcpy(b->data, &value, sizeof(unsigned int));

    trim(b);
}
void set_bignum_data(struct bignum *b, unsigned char *src, unsigned int size)
{
    b->data_size = size;
    b->size = size * 8;
    memcpy(b->data, src, b->size * sizeof(unsigned char));
}

void assign_bignum(struct bignum *dst, struct bignum *src)
{
    //if(dst->data)
    //    free(dst->data);
    //dst->data = malloc(src->size);

    dst->data_size = src->data_size;
    memcpy(dst->data, src->data, src->data_size);
}

void add_bignum(struct bignum *ans, struct bignum *b1, struct bignum * b2)
{
    // !!!! todo: cache friendly || data -> unsigned long long || asm 

    unsigned char carry = 0;

    //unsigned char tmp, tmp2;

    //unsigned int tmp;

    unsigned char tmp;

    int i = 0;
    int size = (b1->data_size > b2->data_size ? b1->data_size : b2->data_size);
    for (; i < size || (i == size && carry); i++) {
        // method 1: use bitwise op
        //tmp = b1->data[i] ^ b2->data[i];
        //tmp2 = b1->data[i] & b2->data[i];
        //ans->data[i] = tmp + (tmp2 << 1) + carry;
        //carry = !!((((tmp + carry)/*!! implicit cast: uint*/ >> 1) + tmp2) & 0x80);  // if (a+b)/2 has 10000...

        // method 2: add via asm_add directly
        //tmp = b1->data[i] + b2->data[i] + carry;
        //ans->data[i] = tmp;
        //carry = (tmp > 0xFF);

        // method 3: Ex. carry = true if "from MSB has 1 til meet two"
        //                                   a: 11110111010
        //                                   b: 00001000101
        //                               carry: 00000000001
        //               carry = a > ~b
        tmp = (b1->data[i] + carry > (~b2->data[i] & 0xFF)) || (b1->data[i] > (~carry & 0xFF));

        ans->data[i] = b1->data[i] + b2->data[i] + carry;
        carry = tmp;
    }
    ans->data_size = i;
}

void complement_bignum(struct bignum *dst, struct bignum *src)
{
    dst->data_size = src->data_size;

    for (int i = 0; i < src->data_size; i++)
        dst->data[i] = ~src->data[i];
}

void double_bignum(struct bignum *b)
{
    // Ex. 0000 0111 1011 -> 0000 1111 0110 | 104 vs 255

    unsigned int is_carry = 0, is_last_carry = 0;

    int i = 0;
    for (; i < b->data_size || is_carry; i++) {
        is_carry = !!(b->data[i] & 0x80);

        // shift
        b->data[i] = (b->data[i] << 1) + is_last_carry;

        is_last_carry = is_carry;
    }
    b->data_size = i;
}

void multiply_bignum(struct bignum *ans, struct bignum *a, struct bignum *b)
{
    // Ex.   a  *  b
    //     1011 * 0101 -> (1011 << 3) + (1011 << 0)

    init_bignum(tmp, ans->size);
    assign_bignum(&tmp, a);

    set_bignum_value(ans, 0);

    int tmp_double_count = 0, bit_count = 0;
    for (int i = 0; i < b->data_size; i++) {
        if (b->data[i]) {
            for (int offset = 0; offset < 8; offset++) {  // O(n)
                if (b->data[i] & (1 << offset)) {
                    // double a for (offset + i * 8) times
                    bit_count = offset + i * 8;
                    for (int j = tmp_double_count; j < bit_count; j++)  // a << bit_count
                        double_bignum(&tmp);  // O(n)

                    tmp_double_count = bit_count;  // at the foundation of last tmp result

                    add_bignum(ans, ans, &tmp);
                }
            }
        }
    }
}

/*
 * from n's MSB to LSB, if '1' add ans with last_ans, if '0' double ans
 */
void fast_fib(unsigned int n)
{
    init_bignum(_n, n);
    init_bignum(x, n); init_bignum(y, n);
    init_bignum(_x, n); init_bignum(_y, n);
    init_bignum(tmp, n); init_bignum(tmp2, n);

    init_bignum(_one, n);
    set_bignum_value(&_one, 1);

    //////////////////////

    set_bignum_value(&x, 0); set_bignum_value(&y, 1);

    set_bignum_value(&_n, n);
    trim(&_n);

    for (int i = _n.data_size - 1; i >= 0; i--) {
        for (int b = 7; b >= 0; b--) {  // O(n)
            assign_bignum(&tmp, &y);
            double_bignum(&tmp);

            x.data_size = (tmp.data_size > x.data_size ? tmp.data_size : x.data_size);  // complement data_size of max(2y, x)
            complement_bignum(&tmp2, &x);
            add_bignum(&tmp2, &tmp2, &_one);

            add_bignum(&tmp, &tmp, &tmp2);  // 2y - x
            tmp.data_size -= 1;  // subtract overflow

            multiply_bignum(&_x, &tmp, &x);  // _x = x * (2y - x)

            printf("\ntmp = 2y - x\n");
            print_bignum(tmp);

            ///////////////////

            multiply_bignum(&tmp, &x, &x);
            multiply_bignum(&tmp2, &y, &y);

            add_bignum(&y, &tmp, &tmp2);  // y = x^2 * y^2;

            ///////////////////

            assign_bignum(&x, &_x);  // x = _x

            printf("\nx = x * (2y - x), y = x^2 * y^2\n");
            print_bignum(x);
            print_bignum(y);

            if (_n.data[i] & (1 << b)) {
                add_bignum(&tmp, &x, &y);

                assign_bignum(&x, &y);    // x = y
                assign_bignum(&y, &tmp);  // y = x + y

                printf("\nx = y, y = x + y\n");
                print_bignum(x);
                print_bignum(y);
            }
            
        }
    }
    printf("\nfib(%i) = \n", n);
    print_bignum(x);
}

int main()
{
    unsigned int i = 4294967295;

    fast_fib(500);
    return 0;


    init_bignum(a, 200);
    init_bignum(b, 200);
    init_bignum(c, 200);
    init_bignum(d, 200);
    init_bignum(e, 200);

    init_bignum(_one, 200);
    set_bignum_value(&_one, 1);

    set_bignum_value(&a, 144);
    set_bignum_value(&b, 233);


    assign_bignum(&c, &b);
    double_bignum(&c);

    a.data_size = (a.data_size > c.data_size ? a.data_size : c.data_size);
    complement_bignum(&d, &a);
    add_bignum(&d, &d, &_one);
    
    print_bignum(c);
    add_bignum(&c, &c, &d);  // 2y - x


    c.data_size -= 1;  // subtract overflow


    print_bignum(c);

    multiply_bignum(&e, &c, &a);  // _x = x * (2y - x)

    //printf("\n2y - x\n");
    //print_bignum(e);



    return 0;


    //char src[] = {0x99, 0x41, 0xF1, 0xC0, 0x8D, 0x3A, 0x42, 0xD8, 0x2D, 0x01};
    //set_bignum_data(&a, src, sizeof(src));

    return 0;
    
}