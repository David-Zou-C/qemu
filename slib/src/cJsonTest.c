//
// Created by 25231 on 2023/8/8.
//
#include <stdio.h>
#include "stdint.h"

typedef union {
    uint32_t reg;
    struct {
        uint16_t a;
        uint16_t b;
    } reg_ab;
} REG;

struct S {
    REG *reg;
};

int main() {
    uint32_t a = 0x12345678;
    struct S s;

    s.reg = (REG *)&a;

    s.reg->reg_ab.a = 0x10;

    printf("a: %04x \n", a);
    printf("a: %04x \n", s.reg->reg_ab.a);
    printf("b: %04x \n", s.reg->reg_ab.b);
    printf("ab: %08x \n", s.reg->reg);

    return 0;
}