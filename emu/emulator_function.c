#include "emulator_function.h"

uint32_t get_code8(Emulator* emu, int index)
{
    return emu->memory[emu->eip + index];
}

int32_t get_sign_code8(Emulator* emu, int index)
{
    return (int8_t)emu->memory[emu->eip + index];
}

uint32_t get_code32(Emulator* emu, int index)
{
    int i;
    uint32_t ret = 0;

    /* リトルエンディアンでメモリの値を取得する */
    for (i = 0; i < 4; i++) {
        ret |= get_code8(emu, index + i) << (i * 8);
    }

    return ret;
}

int32_t get_sign_code32(Emulator* emu, int index)
{
    return (int32_t)get_code32(emu, index);
}

uint8_t get_register8(Emulator* emu, int index)
{
    if (index < 4) {
        return emu->registers[index] & 0xff;
    } else {
        return (emu->registers[index - 4] >> 8) & 0xff;
    }
}

uint32_t get_register32(Emulator* emu, int index)
{
    return emu->registers[index];
}

void set_register8(Emulator* emu, int index, uint8_t value)
{
    if (index < 4) {
        /* レジスタのLOWの部分を取得 */
        uint32_t r = emu->registers[index] & 0xffffff00;
        emu->registers[index] = r | (uint32_t)value;
    } else {
        /* indexから4を引いてレジスタ番号をずらし、レジスタのHIGHの部分を取得 */
        uint32_t r = emu->registers[index - 4] & 0xffff00ff;
        emu->registers[index - 4] = r | ((int32_t)value << 8);
    }
}

void set_register32(Emulator* emu, int index, uint32_t value)
{
    emu->registers[index] = value;
}

void set_memory8(Emulator* emu, uint32_t address, uint32_t value)
{
    emu->memory[address] = value & 0xFF;
}

void set_memory32(Emulator* emu, uint32_t address, uint32_t value)
{
    int i;

    /* リトルエンディアンでメモリの値を設定する */
    for (i = 0; i < 4; i++) {
        set_memory8(emu, address + i, value >> (i * 8));
    }
}

uint32_t get_memory8(Emulator* emu, uint32_t address)
{
    return emu->memory[address];
}

uint32_t get_memory32(Emulator* emu, uint32_t address)
{
    int i;
    uint32_t ret = 0;

    /* リトルエンディアンでメモリの値を取得する */
    for (i = 0; i < 4; i++) {
        ret |= get_memory8(emu, address + i) << (8 * i);
    }

    return ret;
}

void push32(Emulator* emu, uint32_t value)
{
    int32_t address = get_register32(emu, ESP) - 4;
    set_register32(emu, ESP, address);
    set_memory32(emu, address, value);
}

uint32_t pop32(Emulator* emu)
{
    uint32_t address = get_register32(emu, ESP);
    uint32_t ret = get_memory32(emu, address);
    set_register32(emu, ESP, address + 4);

    return ret;
}

void set_carry(Emulator* emu, int is_carry)
{
    if (is_carry) {
        /* is_carry = True(>0)  => CARRY_FLAG: ON(1)  */
        emu->eflags |= CARRY_FLAG;
    } else {
        /* is_carry = False(=0) => CARRY_FLAG: OFF(0) */
        emu->eflags &= ~CARRY_FLAG;
    }
}

void set_zero(Emulator* emu, int is_zero)
{
    if (is_zero) {
        emu->eflags |= ZERO_FLAG;
    } else {
        emu->eflags &= ~ZERO_FLAG;
    }
}

void set_overflow(Emulator* emu, int is_overflow)
{
    if (is_overflow) {
        emu->eflags |= OVERFLOW_FLAG;
    } else {
        emu->eflags &= ~OVERFLOW_FLAG;
    }
}

void set_sign(Emulator* emu, int is_sign)
{
    if (is_sign) {
        emu->eflags |= SIGN_FLAG;
    } else {
        emu->eflags &= ~SIGN_FLAG;
    }
}

int is_carry(Emulator* emu)
{
    return (emu->eflags & CARRY_FLAG) != 0;
}

int is_zero(Emulator* emu)
{
    return (emu->eflags & ZERO_FLAG) != 0;
}

int is_sign(Emulator* emu)
{
    return (emu->eflags & SIGN_FLAG) != 0;
}

int is_overflow(Emulator* emu)
{
    return (emu->eflags & OVERFLOW_FLAG) != 0;
}

void update_eflags_sub(Emulator* emu, uint32_t v1, uint32_t v2, uint64_t result)
{
    /* 各値の符号を取得 */
    /* v1, v2の32ビット目を取得します */
    /* X_32...X_1 -> 0_31...0_1X_32 */
    int sign1 = v1 >> 31;
    int sign2 = v2 >> 31;
    /* resultの32ビット目を取得します(resultは64ビットなので32ビット目以外は0にします) */
    /* X_64...X_1 -> 0_31...0_1X_32...X_1 -> 0_63...0_1X_64 */
    int signr = (result >> 31) & 1;

    /* 演算結果にcarryがあればCarryフラグ設定 */
    /* 繰り下がり考慮のため */
    set_carry(emu, result >> 32);

    /* 演算結果が0ならばZeroフラグ設定 */
    set_zero(emu, result == 0);

    /* 演算結果に符合があればSignフラグ設定 */
    set_sign(emu, signr);

    /* 演算結果がオーバーフローしていたらOverflowフラグ設定 */
    set_overflow(emu, sign1 != sign2 && sign1 != signr);
}