#ifndef BIGINT_H
#define BIGINT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <complex.h> // For potential FFT fallback/comparison if needed
#include <limits.h> // For LLONG_MIN/MAX

// --- 数论变换 (NTT) 相关定义 (来自 multiplication.h) ---
#define PI 3.14159265358979323846
// NTT Modulus and Primitive Root (Choose appropriate ones)
// Example using common ones often used with base 10000 blocks:
#define MOD 998244353ULL          // Common NTT modulus (prime)
#define G 3ULL                    // Primitive root modulo MOD
#define INV_G 332748118ULL        // Modular inverse of G mod MOD (precomputed)
// Or use the ones from your original multiplication.h if they were different:
// #define MOD 2281701377ULL
// #define G 3
// #define INV_G 760567126

// Default base for internal representation
#define DEFAULT_BASE 1000       // 10^3
#define DEFAULT_BASE_DIGITS 3  // 3 digits per block

// --- BigInt 结构体 (采用 multiplication.h 的版本) ---
typedef struct BigInt { // Self-referential struct needs tag name
    int *digits;       // Array of digits (blocks), little-endian order
    size_t length;     // Number of blocks used
    int sign;          // 1 for positive/zero, -1 for negative
    int ref_count;     // Reference count for potential sharing (optional, but in multiplication.h)
    int base;          // The base of each digit block (e.g., 10000)
    int base_digits;   // Number of decimal digits per block (e.g., 4)
    // size_t original_digit_length; // Keep if needed, maybe remove for simplicity
    size_t capacity; // Add capacity tracking similar to division.h? Useful for in-place modification. Let's add it.
} BigInt;

// --- 统一的错误码 (基于 multiplication.h，可添加 division 的错误) ---
typedef enum {
    BIGINT_SUCCESS = 0,
    BIGINT_ERROR,              // Generic error
    BIGINT_NULL_POINTER,
    BIGINT_INVALID_INPUT,
    BIGINT_ALLOCATION_ERROR,
    BIGINT_OVERFLOW,           // Arithmetic overflow during calculation
    BIGINT_DIVIDE_BY_ZERO,
    BIGINT_BUFFER_TOO_SMALL   // For string conversion if buffer isn't large enough
} BigIntError;


// --- 函数原型声明 ---

// Lifecycle & Setup
BigInt* createBigInt(size_t initial_capacity); // Modified to accept capacity
BigInt* createBigIntFromLL(long long val);
BigInt* createBigIntFromString(const char *str);
void destroyBigInt(BigInt *bigInt);
void retainBigInt(BigInt *bigInt); // Keep if using reference counting
BigInt* copyBigInt(const BigInt *src);
BigIntError ensureCapacity(BigInt *num, size_t min_capacity); // Helper

// Conversion & Output
char* bigIntToString(const BigInt *num); // multiplication.h version (returns allocated string)
void printBigInt(const BigInt *num);

// Comparison
int compareAbsolute(const BigInt *a, const BigInt *b); // multiplication.h version (compares blocks)
int compareBigInt(const BigInt *a, const BigInt *b);    // Compares with sign
bool isBigIntZero(const BigInt *num);

// Arithmetic Operations
BigIntError addBigInt(const BigInt *a, const BigInt *b, BigInt **result_ptr); // Use result_ptr to return new BigInt
BigIntError subtractBigInt(const BigInt *a, const BigInt *b, BigInt **result_ptr);
BigIntError nttMultiplyBigInt(const BigInt *a, const BigInt *b, BigInt **result_ptr);
BigIntError multiplyBigIntByLL(const BigInt *a, long long b_ll, BigInt **result_ptr);

#define multiplyBigInt nttMultiplyBigInt

// Division Functions (Adapted from division.c)
BigIntError divideBigInt(const BigInt *a, const BigInt *b, BigInt **quotient_ptr, BigInt **remainder_ptr);
char* bigIntToDecimalString(const BigInt *a, const BigInt *b, int precision); // Returns allocated string


// --- Potentially keep FFT/NTT helpers public if needed, or make static in .c ---
unsigned long long mod_pow(unsigned long long a, unsigned long long b, unsigned long long m);
unsigned long long mod_inverse(unsigned long long a, unsigned long long m);
void ntt(unsigned long long *a, int n, int invert);



#endif // BIGINT_H
