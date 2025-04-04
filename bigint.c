// author：8891689
#include "bigint.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>


// --- Static Helper Declarations ---
static void trimLeadingZeros(BigInt *num);
static void normalize(BigInt *num); // Combines trimming and sign for zero
static double power(double base, int exponent); // Keep simple power for string conversion helper
static BigIntError addBigIntAbs(const BigInt *a, const BigInt *b, BigInt **result_ptr);
static BigIntError subtractBigIntAbs(const BigInt *larger, const BigInt *smaller, BigInt **result_ptr);
static BigIntError multiplyBy10(BigInt *num); // Multiply in-place by 10 (block-aware)
static BigIntError multiplyByInt(const BigInt *a, int b_int, BigInt **result_ptr); // Simple block multiplication by small int
static BigIntError divideBigIntAbs(const BigInt *a_abs, const BigInt *b_abs, BigInt **q_abs_ptr, BigInt **r_abs_ptr); // Core block-based division

// --- Lifecycle & Setup Implementations ---

// Helper: Ensure capacity
BigIntError ensureCapacity(BigInt *num, size_t min_capacity) {
    assert(num != NULL);
    if (num->capacity >= min_capacity) {
        return BIGINT_SUCCESS;
    }

    size_t new_capacity = (num->capacity == 0) ? min_capacity : num->capacity;
    while (new_capacity < min_capacity) {
        new_capacity *= 2;
    }

    int *new_digits = (int *)realloc(num->digits, new_capacity * sizeof(int));
    if (!new_digits) {
        // Don't change num if realloc fails
        return BIGINT_ALLOCATION_ERROR;
    }

    // Zero out the *newly* allocated part
    if (new_capacity > num->capacity) {
        memset(new_digits + num->capacity, 0, (new_capacity - num->capacity) * sizeof(int));
    }

    num->digits = new_digits;
    num->capacity = new_capacity;
    return BIGINT_SUCCESS;
}


// Create BigInt with initial capacity
BigInt* createBigInt(size_t initial_capacity) {
    BigInt *bigInt = (BigInt*)malloc(sizeof(BigInt));
    if (!bigInt) {
        return NULL;
    }
    if (initial_capacity == 0) initial_capacity = 1; // Minimum capacity for zero
    bigInt->digits = (int*)calloc(initial_capacity, sizeof(int));
    if (!bigInt->digits) {
        free(bigInt);
        return NULL;
    }
    bigInt->length = 1; // Represents zero initially
    bigInt->capacity = initial_capacity;
    bigInt->sign = 1;
    bigInt->ref_count = 1;
    bigInt->base = DEFAULT_BASE;
    bigInt->base_digits = DEFAULT_BASE_DIGITS;
    // bigInt->original_digit_length = 0; // Reset if keeping track
    return bigInt;
}

// Destroy BigInt (handles reference counting from multiplication.h)
void destroyBigInt(BigInt *bigInt) {
    if (bigInt) {
        bigInt->ref_count--;
        if (bigInt->ref_count <= 0) {
            free(bigInt->digits);
            free(bigInt);
        }
    }
}

// Retain BigInt (for reference counting)
void retainBigInt(BigInt *bigInt) {
    if (bigInt) {
        bigInt->ref_count++;
    }
}

// Copy BigInt
BigInt* copyBigInt(const BigInt *src) {
    if (!src) return NULL;

    BigInt *dest = createBigInt(src->length); // Allocate enough capacity
    if (!dest) return NULL;

    // Ensure capacity is sufficient (createBigInt might allocate minimum)
    if (ensureCapacity(dest, src->length) != BIGINT_SUCCESS) {
        destroyBigInt(dest);
        return NULL;
    }

    dest->length = src->length;
    dest->sign = src->sign;
    memcpy(dest->digits, src->digits, dest->length * sizeof(int));
    // dest->ref_count = 1; // Already set by createBigInt
    dest->base = src->base;
    dest->base_digits = src->base_digits;
    // dest->original_digit_length = src->original_digit_length;
    return dest;
}

// Trim leading zero blocks
static void trimLeadingZeros(BigInt *num) {
     if (!num || !num->digits) return;
    while (num->length > 1 && num->digits[num->length - 1] == 0) {
        num->length--;
    }
    // Don't shrink capacity here unless memory is critical
}

// Normalize: trim leading zeros and set sign for zero
static void normalize(BigInt *num) {
    if (!num) return;
    trimLeadingZeros(num);
    if (num->length == 1 && num->digits[0] == 0) {
        num->sign = 1; // Zero is always positive
    }
}

// Create BigInt from string (block-based, from multiplication.c)
BigInt* createBigIntFromString(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    int sign = 1;
    size_t start = 0;

    // 处理符号
    if (len > 0 && str[0] == '-') {
        sign = -1;
        start = 1;
    } else if (len > 0 && str[0] == '+') {
        start = 1;
    }

    // 跳过前导零
    size_t original_start = start; // 保存处理符号后的起始位置
    while (start < len && str[start] == '0') {
        start++;
    }

    // 检查剩余字符是否全为零
    int all_zero = 1;
    for (size_t i = start; i < len; i++) {
        if (str[i] != '0') {
            all_zero = 0;
            break;
        }
    }

    if (all_zero) {
        // 存在符号但后面没有字符（如"-"或"+"）
        if (original_start > 0 && start == original_start) {
            return NULL;
        }
        // 全为零，返回有效零对象
        BigInt *zero = createBigInt(1);
        if (zero) zero->sign = 1;
        return zero;
    }

    // 处理无效字符
    for (size_t i = start; i < len; i++) {
        if (!isdigit((unsigned char)str[i])) {
            return NULL;
        }
    }
    // +++ 修复结束 +++

    size_t num_dec_digits = len - start;
    const int base = 1000;
    const int base_digits = 3;
    size_t num_blocks = (num_dec_digits + base_digits - 1) / base_digits;

    BigInt *bigInt = createBigInt(num_blocks);
    if (!bigInt) return NULL;

    bigInt->sign = sign;
    bigInt->base = base;
    bigInt->base_digits = base_digits;
    bigInt->length = num_blocks;

    // 解析字符串到块（小端序）
    size_t current_block = 0;
    int current_block_val = 0;
    int power = 1;

    for (size_t i = 0; i < num_dec_digits; i++) {
        char c = str[len - 1 - i]; 
        if (!isdigit((unsigned char)c)) {
            destroyBigInt(bigInt);
            return NULL;
        }

        current_block_val += (c - '0') * power;
        power *= 10;

        if ((i + 1) % base_digits == 0 || i == num_dec_digits - 1) {
            bigInt->digits[current_block++] = current_block_val;
            current_block_val = 0;
            power = 1;
        }
    }

    normalize(bigInt);
    return bigInt;
}

// Create BigInt from long long
BigInt* createBigIntFromLL(long long val) {
    char buffer[30]; // Sufficient for long long string representation
    snprintf(buffer, sizeof(buffer), "%lld", val);
    return createBigIntFromString(buffer); // Reuse string conversion
}

// --- Conversion & Output Implementations ---

// Helper for bigIntToString
static double power(double base, int exponent) {
     if (exponent == 0) return 1.0;
     if (exponent < 0) return 1.0 / power(base, -exponent);
     double res = 1.0;
     while(exponent > 0) {
         if(exponent % 2 == 1) res *= base;
         base *= base;
         exponent /= 2;
     }
     return res;
}


// Convert BigInt to string (block-based, from multiplication.c, returns allocated string)
char* bigIntToString(const BigInt *num) {
    if (!num) return NULL;
    if (isBigIntZero(num)) {
        char *zero_str = malloc(2);
        if (zero_str) strcpy(zero_str, "0");
        return zero_str;
    }

    // Estimate required size: sign + (length * base_digits) + null terminator
    size_t max_chars = 1 + (num->length * num->base_digits) + 1;
    char *str = (char *)malloc(max_chars);
    if (!str) return NULL;

    char *p = str;
    if (num->sign < 0) {
        *p++ = '-';
    }

    // Buffer to format each block
    char block_buffer[DEFAULT_BASE_DIGITS + 1];
    // Format for blocks (except the most significant one)
    char format[10];
    snprintf(format, sizeof(format), "%%0%dd", num->base_digits);

    // Print the most significant block first (no leading zeros needed)
    sprintf(block_buffer, "%d", num->digits[num->length - 1]);
    strcpy(p, block_buffer);
    p += strlen(block_buffer);

    // Print remaining blocks with leading zeros
    for (ssize_t i = (ssize_t)num->length - 2; i >= 0; i--) {
        sprintf(block_buffer, format, num->digits[i]);
        strcpy(p, block_buffer);
        p += num->base_digits;
    }
    *p = '\0'; // Null terminate

    // Note: Might overallocate slightly, could realloc to exact size if needed
    return str;
}

// Print BigInt
void printBigInt(const BigInt *num) {
    char *str = bigIntToString(num);
    if (str) {
        printf("%s", str);
        free(str);
    } else if (!num) {
         printf("(NULL BigInt)");
    } else {
         printf("(Error printing BigInt)");
    }
}

// --- Comparison Implementations ---

// Compare absolute values (block-based, from multiplication.c)
int compareAbsolute(const BigInt *a, const BigInt *b) {
    assert(a && b);
    if (a->length > b->length) return 1;
    if (a->length < b->length) return -1;
    // Lengths are equal, compare block by block from most significant
    for (ssize_t i = (ssize_t)a->length - 1; i >= 0; i--) {
        if (a->digits[i] > b->digits[i]) return 1;
        if (a->digits[i] < b->digits[i]) return -1;
    }
    return 0; // Numbers are identical
}

// Compare with sign
int compareBigInt(const BigInt *a, const BigInt *b) {
     assert(a && b);
    if (a->sign > b->sign) return 1;  // a is positive, b is negative
    if (a->sign < b->sign) return -1; // a is negative, b is positive
    // Signs are the same
    int abs_cmp = compareAbsolute(a, b);
    // If positive, result is abs_cmp. If negative, result is -abs_cmp.
    return (a->sign > 0) ? abs_cmp : -abs_cmp;
}

// Check if zero
bool isBigIntZero(const BigInt *num) {
    if (!num) return false;
    // Assumes normalization ensures zero is represented as length 1, digit[0] = 0
    return (num->length == 1 && num->digits[0] == 0);
}

// Helper: Subtract absolute values (|larger| - |smaller|)
// Result is allocated and returned via result_ptr. Assumes |larger| >= |smaller|.
static BigIntError addBigIntAbs(const BigInt *a, const BigInt *b, BigInt **result_ptr) {
    assert(a && b && result_ptr);
    const int base = a->base;
    size_t max_len = (a->length > b->length) ? a->length : b->length;
    BigInt *result = createBigInt(max_len + 1);
    if (!result) return BIGINT_ALLOCATION_ERROR;
    result->base = base;
    result->base_digits = a->base_digits;

    int carry = 0;
    for (size_t i = 0; i < max_len; ++i) {
        int digit_a = (i < a->length) ? a->digits[i] : 0;
        int digit_b = (i < b->length) ? b->digits[i] : 0;
        long long sum = (long long)digit_a + digit_b + carry;
        result->digits[i] = sum % base;
        carry = sum / base;
    }
    if (carry > 0) {
        result->digits[max_len] = carry;
        result->length = max_len + 1;
    } else {
        result->length = max_len;
    }
    normalize(result);
    *result_ptr = result;
    return BIGINT_SUCCESS;
}

static BigIntError subtractBigIntAbs(const BigInt *larger, const BigInt *smaller, BigInt **result_ptr) {
    assert(larger && smaller && result_ptr);
    assert(compareAbsolute(larger, smaller) >= 0);

    size_t max_len = larger->length;
    BigInt *result = createBigInt(max_len);
    if (!result) return BIGINT_ALLOCATION_ERROR;

    // 设置正确的基数参数
    result->base = larger->base;
    result->base_digits = larger->base_digits;

    int borrow = 0;
    // 减法逻辑（subtractBigIntAbs函数中）
    for (size_t i = 0; i < max_len; ++i) {
    int digit_larger = larger->digits[i]; // 小端序处理
    int digit_smaller = (i < smaller->length) ? smaller->digits[i] : 0;
    int diff = digit_larger - digit_smaller - borrow;

        if (diff < 0) {
            diff += result->base; // 使用正确的基数
            borrow = 1;
        } else {
            borrow = 0;
        }
        result->digits[i] = diff;
    }
    result->length = max_len;

    normalize(result);
    *result_ptr = result;
    return BIGINT_SUCCESS;
}


// Add BigInts (handles signs, returns new BigInt via pointer)
BigIntError addBigInt(const BigInt *a, const BigInt *b, BigInt **result_ptr) {
    if (!a || !b || !result_ptr) return BIGINT_NULL_POINTER;

    *result_ptr = NULL; // Ensure output is NULL on entry/error
    BigIntError err;
    BigInt *result = NULL;

    if (a->sign == b->sign) {
        // Same signs: Add absolute values, keep the sign
        err = addBigIntAbs(a, b, &result);
        if (err == BIGINT_SUCCESS) {
            result->sign = a->sign; // Assign the common sign
            normalize(result); // Ensure canonical zero if result is 0
        }
    } else {
        // Different signs: Subtract absolute values. Sign depends on which absolute value is larger.
        int cmp = compareAbsolute(a, b);
        if (cmp >= 0) { // |a| >= |b|
            err = subtractBigIntAbs(a, b, &result);
            if (err == BIGINT_SUCCESS) {
                result->sign = a->sign; // Result sign follows 'a'
                normalize(result);
            }
        } else { // |a| < |b|
            err = subtractBigIntAbs(b, a, &result);
            if (err == BIGINT_SUCCESS) {
                result->sign = b->sign; // Result sign follows 'b'
                normalize(result);
            }
        }
    }

    if (err != BIGINT_SUCCESS) {
        destroyBigInt(result); // Clean up potentially partially created result
        return err;
    }

    *result_ptr = result;
    return BIGINT_SUCCESS;
}

// Helper: Negate a BigInt (in place)
static BigIntError negateBigInt(BigInt *num) {
    if (!num) return BIGINT_NULL_POINTER;
    if (!isBigIntZero(num)) {
        num->sign *= -1;
    }
    return BIGINT_SUCCESS;
}


// Subtract BigInts (returns new BigInt via pointer)
BigIntError subtractBigInt(const BigInt *a, const BigInt *b, BigInt **result_ptr) {
     if (!a || !b || !result_ptr) return BIGINT_NULL_POINTER;

    *result_ptr = NULL;
    BigIntError err;

    // Subtraction a - b is equivalent to addition a + (-b)
    BigInt *neg_b = copyBigInt(b);
    if (!neg_b) return BIGINT_ALLOCATION_ERROR;

    negateBigInt(neg_b); // Flip the sign of the copy

    // Perform the addition: result = a + neg_b
    err = addBigInt(a, neg_b, result_ptr);

    destroyBigInt(neg_b); // Clean up the temporary copy
    return err;
}


// --- NTT Multiplication (from multiplication.c, adapted for result_ptr) ---

// Helper: Modular exponentiation (needed for NTT)
unsigned long long mod_pow(unsigned long long a, unsigned long long b, unsigned long long m) {
    unsigned long long result = 1;
    a %= m;
    while (b > 0) {
        if (b & 1)
            result = (result * a) % m;
        a = (a * a) % m;
        b >>= 1;
    }
    return result;
}

// Helper: Modular inverse (needed for NTT)
unsigned long long mod_inverse(unsigned long long a, unsigned long long m) {
    // Assumes m is prime (like 998244353)
    // Uses Fermat's Little Theorem: a^(m-2) mod m
    return mod_pow(a, m - 2, m);
}

// Helper: Bit reversal permutation (needed for NTT)
static void bit_reverse_ntt(unsigned long long *a, int n) {
     // Standard bit reversal algorithm
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j >= bit; bit >>= 1)
            j -= bit;
        j += bit;
        if (i < j) {
            unsigned long long temp = a[i];
            a[i] = a[j];
            a[j] = temp;
        }
    }
}

// Helper: Number Theoretic Transform (NTT)
void ntt(unsigned long long *a, int n, int invert) {
    assert(a != NULL && n > 0 && (n & (n - 1)) == 0); // n must be power of 2

    bit_reverse_ntt(a, n);

    for (int len = 2; len <= n; len <<= 1) {
        unsigned long long wlen = mod_pow(G, (MOD - 1) / len, MOD);
        if (invert)
            wlen = mod_inverse(wlen, MOD); // Use precomputed INV_G if len=n? No, general case.

        for (int i = 0; i < n; i += len) {
            unsigned long long w = 1;
            for (int j = 0; j < len / 2; ++j) {
                unsigned long long u = a[i + j];
                // Check for potential overflow before modulo? Unlikely with ulonglong for MOD<2^31
                unsigned long long v = (a[i + j + len / 2] * w) % MOD;
                a[i + j] = (u + v) % MOD;
                // Ensure subtraction doesn't underflow before modulo
                a[i + j + len / 2] = (u + MOD - v) % MOD;
                w = (w * wlen) % MOD;
            }
        }
    }

    if (invert) {
        unsigned long long inv_n = mod_inverse(n, MOD);
        for (int i = 0; i < n; ++i)
            a[i] = (a[i] * inv_n) % MOD;
    }
}


// NTT Multiplication (returns new BigInt via pointer)
BigIntError nttMultiplyBigInt(const BigInt *a, const BigInt *b, BigInt **result_ptr) {
    if (!a || !b || !result_ptr) return BIGINT_NULL_POINTER;
    if (a->base != b->base || a->base != DEFAULT_BASE) return BIGINT_INVALID_INPUT; // Ensure compatible base

    *result_ptr = NULL;

    // Handle multiplication by zero efficiently
    if (isBigIntZero(a) || isBigIntZero(b)) {
        *result_ptr = createBigInt(1); // Return canonical zero
        return (*result_ptr) ? BIGINT_SUCCESS : BIGINT_ALLOCATION_ERROR;
    }

    // Determine result sign
    int result_sign = a->sign * b->sign;

    // Calculate required NTT size (power of 2 >= combined length)
    size_t n = 1;
    size_t combined_len = a->length + b->length; // Max possible blocks in result before carry
    while (n < combined_len) n <<= 1;

    // Allocate NTT buffers
    unsigned long long *ntt_a = calloc(n, sizeof(unsigned long long));
    unsigned long long *ntt_b = calloc(n, sizeof(unsigned long long));
    if (!ntt_a || !ntt_b) {
        free(ntt_a); free(ntt_b);
        return BIGINT_ALLOCATION_ERROR;
    }

    // Copy digits to NTT buffers
    for (size_t i = 0; i < a->length; i++) ntt_a[i] = a->digits[i];
    for (size_t i = 0; i < b->length; i++) ntt_b[i] = b->digits[i];

    // Perform NTT
    ntt(ntt_a, n, 0); // Forward NTT for a
    ntt(ntt_b, n, 0); // Forward NTT for b

    // Pointwise multiplication in frequency domain
    for (size_t i = 0; i < n; i++) {
        ntt_a[i] = (ntt_a[i] * ntt_b[i]) % MOD;
    }

    // Inverse NTT
    ntt(ntt_a, n, 1); // ntt function handles inverse transform and scaling by 1/n

    // 在 nttMultiplyBigInt 中（乘法部分），在创建 result 后增加基数设置：
    BigInt *result = createBigInt(n + 1); // Allocate potentially n+1 blocks for carries
    if (!result) {
          free(ntt_a); free(ntt_b);
    return BIGINT_ALLOCATION_ERROR;
    }
    result->base = a->base;
    result->base_digits = a->base_digits;


    unsigned long long carry = 0;
    size_t result_len = 0;
    for (size_t i = 0; i < n; ++i) {
         // Add carry from previous block to the current NTT result coefficient
        carry += ntt_a[i];
         // The new digit is the sum modulo the base
        result->digits[i] = (int)(carry % result->base);
         // The new carry is the sum divided by the base
        carry /= result->base;
        result_len++; // Tentatively increase length
    }

    // Handle final carry
    while (carry > 0) {
         if (result_len >= result->capacity) {
              if (ensureCapacity(result, result_len + 1) != BIGINT_SUCCESS) {
                    free(ntt_a); free(ntt_b); destroyBigInt(result);
                    return BIGINT_ALLOCATION_ERROR;
              }
         }
        result->digits[result_len] = (int)(carry % result->base);
        carry /= result->base;
        result_len++;
    }

    free(ntt_a);
    free(ntt_b);

    result->length = result_len;
    result->sign = result_sign;
    normalize(result); // Trim leading zeros and set sign=1 if result is 0

    *result_ptr = result;
    return BIGINT_SUCCESS;
}


// Multiply BigInt by long long (returns new BigInt via pointer)
BigIntError multiplyBigIntByLL(const BigInt *a, long long b_ll, BigInt **result_ptr) {
    if (!a || !result_ptr) return BIGINT_NULL_POINTER;

    *result_ptr = NULL;
    BigInt *b_bi = createBigIntFromLL(b_ll);
    if (!b_bi) return BIGINT_ALLOCATION_ERROR;

    BigIntError err = nttMultiplyBigInt(a, b_bi, result_ptr);

    destroyBigInt(b_bi); // Clean up temporary BigInt for b_ll
    return err;
}

// Helper: Multiply BigInt by a small integer (<= base) - Used in division
// Returns allocated BigInt via result_ptr
static BigIntError multiplyByInt(const BigInt *a, int b_int, BigInt **result_ptr) {
     assert(a && result_ptr);
     assert(b_int >= 0 && b_int < a->base); // Assume small non-negative int

     if (isBigIntZero(a) || b_int == 0) {
         *result_ptr = createBigInt(1);
         return *result_ptr ? BIGINT_SUCCESS : BIGINT_ALLOCATION_ERROR;
     }
     if (b_int == 1) {
         *result_ptr = copyBigInt(a);
         return *result_ptr ? BIGINT_SUCCESS : BIGINT_ALLOCATION_ERROR;
     }


     size_t res_capacity = a->length + 1; // Might need one extra block for carry
     BigInt *result = createBigInt(res_capacity);
     if (!result) return BIGINT_ALLOCATION_ERROR;


     unsigned long long carry = 0;
     size_t i;
     for (i = 0; i < a->length; ++i) {
         unsigned long long product = (unsigned long long)a->digits[i] * b_int + carry;
         result->digits[i] = (int)(product % result->base);
         carry = product / result->base;
     }


     if (carry > 0) {
         result->digits[i] = (int)carry;
         result->length = i + 1;
     } else {
         result->length = i;
     }

     result->sign = a->sign; // Sign matches operand 'a' as b_int is positive
     normalize(result);
     *result_ptr = result;
     return BIGINT_SUCCESS;
}


// Helper: Multiply BigInt by 10 (block-aware, in-place)
static BigIntError multiplyBy10(BigInt *num) {
    if (!num) return BIGINT_NULL_POINTER;
    if (isBigIntZero(num)) return BIGINT_SUCCESS;


    unsigned long long carry = 0;
    BigIntError err = BIGINT_SUCCESS;


    // Iterate through existing blocks
    for (size_t i = 0; i < num->length; ++i) {
        unsigned long long product = (unsigned long long)num->digits[i] * 10 + carry;
        num->digits[i] = (int)(product % num->base);
        carry = product / num->base;
    }


    // If there's a carry left, add a new block
    if (carry > 0) {
        err = ensureCapacity(num, num->length + 1);
        if (err != BIGINT_SUCCESS) return err;
        num->digits[num->length] = (int)carry;
        num->length++;
    }


    // No need to normalize usually, unless initial value was 0 (handled)
    return BIGINT_SUCCESS;
}

/*
 * Multiply a BigInt by its base (left-shift blocks).
 * e.g., [1,2,3] with base 1000 -> [0,1,2,3]
 */
static BigIntError multiplyByBase(BigInt *num) {
    if (!num) return BIGINT_NULL_POINTER;

    BigIntError err = ensureCapacity(num, num->length + 1);
    if (err != BIGINT_SUCCESS) return err;

    memmove(num->digits + 1, num->digits, num->length * sizeof(int));
    num->digits[0] = 0;  // New least significant block
    num->length++;
    normalize(num);      // Remove leading zero blocks if needed (unlikely for division)
    return BIGINT_SUCCESS;
}

// --- Division Implementation (Adapted for Blocks) ---

/**
 * Core long division logic for absolute values (block-based).
 * Calculates |quotient| = |a| / |b| and |remainder| = |a| % |b|.
 * Assumes |a| >= 0, |b| > 0.
 * Returns results via pointers. Pointers will hold newly allocated BigInts.
 */
static BigIntError divideBigIntAbs(const BigInt *a_abs, const BigInt *b_abs, BigInt **q_abs_ptr, BigInt **r_abs_ptr) {
    assert(a_abs && b_abs && q_abs_ptr && r_abs_ptr);
    assert(a_abs->sign >= 0 && b_abs->sign > 0);
    assert(!isBigIntZero(b_abs));

    *q_abs_ptr = NULL;
    *r_abs_ptr = NULL;
    BigIntError err = BIGINT_SUCCESS;

    // Handle case where |a| < |b|
    if (compareAbsolute(a_abs, b_abs) < 0) {
        *q_abs_ptr = createBigInt(1);
        *r_abs_ptr = copyBigInt(a_abs);
        if (!*q_abs_ptr || !*r_abs_ptr) {
            destroyBigInt(*q_abs_ptr);
            destroyBigInt(*r_abs_ptr);
            *q_abs_ptr = NULL;
            *r_abs_ptr = NULL;
            return BIGINT_ALLOCATION_ERROR;
        }
        return BIGINT_SUCCESS;
    }

    // Initialize working variables
    BigInt *q = createBigInt(a_abs->length);
    BigInt *r = createBigInt(b_abs->length);
    BigInt *current_dividend_part = createBigInt(b_abs->length + 1);
    BigInt *temp_product = NULL;
    BigInt *temp_sub_result = NULL;

    if (!q || !r || !current_dividend_part) {
        destroyBigInt(q);
        destroyBigInt(r);
        destroyBigInt(current_dividend_part);
        return BIGINT_ALLOCATION_ERROR;
    }
    q->length = 0;

    // 修改后的长除法核心逻辑
    for (ssize_t i = (ssize_t)a_abs->length - 1; i >= 0; --i) {
    // 1. 将当前块加入余数部分
    if (current_dividend_part->length > 0 || current_dividend_part->digits[0] != 0) {
        multiplyByBase(current_dividend_part); // 左移一个块
    }
    current_dividend_part->digits[0] = a_abs->digits[i];
    normalize(current_dividend_part);

    // 2. 精确试商逻辑
    int quotient_block = 0;
    if (compareAbsolute(current_dividend_part, b_abs) >= 0) {
        // 计算高位估计值
        int high_a = current_dividend_part->digits[current_dividend_part->length - 1];
        int high_b = b_abs->digits[b_abs->length - 1];
        if (current_dividend_part->length > b_abs->length) {
            high_a = high_a * current_dividend_part->base;
            if (current_dividend_part->length >= 2) {
                high_a += current_dividend_part->digits[current_dividend_part->length - 2];
            }
        }
        
        // 更精确的商估计
        quotient_block = high_a / (high_b + 1);
        quotient_block = (quotient_block > current_dividend_part->base - 1) ? 
                        current_dividend_part->base - 1 : quotient_block;

        // 二分法精确调整
        int low = 0, high = current_dividend_part->base - 1;
        while (low <= high) {
            int mid = (low + high) / 2;
            BigInt *product = NULL;
            multiplyByInt(b_abs, mid, &product);
            if (compareAbsolute(product, current_dividend_part) <= 0) {
                quotient_block = mid;
                low = mid + 1;
            } else {
                high = mid - 1;
            }
            destroyBigInt(product);
        }
    }

    // 3. 更新余数
    // 在 divideBigIntAbs 中，更新当前余数部分时修改为：
    if (quotient_block > 0) {
    BigInt *product = NULL;
    multiplyByInt(b_abs, quotient_block, &product);
    BigInt *new_part = NULL;
    err = subtractBigIntAbs(current_dividend_part, product, &new_part);
    destroyBigInt(current_dividend_part);
    current_dividend_part = new_part;
    destroyBigInt(product);
    }


    // 4. 存储商块
    q->digits[q->length++] = quotient_block;
    }

    


    // Reverse the order of blocks in quotient q (since we built it LSB first by prepending)
    // Reverse quotient blocks
    for (size_t i = 0; i < q->length / 2; ++i) {
        int temp = q->digits[i];
        q->digits[i] = q->digits[q->length - 1 - i];
        q->digits[q->length - 1 - i] = temp;
    }
    normalize(q);

    // Prepare remainder
    destroyBigInt(r);
    r = copyBigInt(current_dividend_part);
    if (!r) {
        err = BIGINT_ALLOCATION_ERROR;
        goto div_abs_cleanup;
    }
    normalize(r);

    // Transfer ownership
    *q_abs_ptr = q;
    *r_abs_ptr = r;
    q = NULL;
    r = NULL;

div_abs_cleanup:
    // Cleanup resources
    destroyBigInt(q);
    destroyBigInt(r);
    destroyBigInt(current_dividend_part);
    destroyBigInt(temp_product);
    destroyBigInt(temp_sub_result);

    // Error handling
    if (err != BIGINT_SUCCESS) {
        destroyBigInt(*q_abs_ptr);
        destroyBigInt(*r_abs_ptr);
        *q_abs_ptr = NULL;
        *r_abs_ptr = NULL;
    }

    return err;
}

// Public Division function (handles signs)
BigIntError divideBigInt(const BigInt *a, const BigInt *b, BigInt **quotient_ptr, BigInt **remainder_ptr) {
    if (!a || !b) return BIGINT_NULL_POINTER;
    if (isBigIntZero(b)) return BIGINT_DIVIDE_BY_ZERO;


    // Ensure output pointers are initialized
    if (quotient_ptr) *quotient_ptr = NULL;
    if (remainder_ptr) *remainder_ptr = NULL;


    // Handle simple case: dividend is zero
    if (isBigIntZero(a)) {
        if (quotient_ptr) {
            *quotient_ptr = createBigInt(1); // Zero
            if (!*quotient_ptr) return BIGINT_ALLOCATION_ERROR;
        }
        if (remainder_ptr) {
            *remainder_ptr = createBigInt(1); // Zero
             if (!*remainder_ptr) { destroyBigInt(*quotient_ptr); if(quotient_ptr) *quotient_ptr=NULL; return BIGINT_ALLOCATION_ERROR; }
        }
        return BIGINT_SUCCESS;
    }


    // Determine signs
    int q_sign = (a->sign == b->sign) ? 1 : -1;
    int r_sign = a->sign; // Remainder sign usually matches dividend


    BigInt *a_abs = copyBigInt(a);
    BigInt *b_abs = copyBigInt(b);
    BigInt *q_abs = NULL;
    BigInt *r_abs = NULL;
    BigIntError err = BIGINT_SUCCESS;


    if (!a_abs || !b_abs) {
        err = BIGINT_ALLOCATION_ERROR;
        goto main_div_cleanup;
    }
    a_abs->sign = 1;
    b_abs->sign = 1;


    // Perform division on absolute values
    err = divideBigIntAbs(a_abs, b_abs, &q_abs, &r_abs);
    if (err != BIGINT_SUCCESS) {
        goto main_div_cleanup;
    }


    // Assign results and apply signs if pointers are provided
    if (quotient_ptr) {
        *quotient_ptr = q_abs; // Transfer ownership
        q_abs = NULL; // Avoid double free
        if (!isBigIntZero(*quotient_ptr)) {
            (*quotient_ptr)->sign = q_sign;
        }
    }


    if (remainder_ptr) {
        *remainder_ptr = r_abs; // Transfer ownership
        r_abs = NULL; // Avoid double free
        if (!isBigIntZero(*remainder_ptr)) {
             // Adjust remainder according to definition a = q*b + r, where 0 <= |r| < |b|
             // The sign convention can vary. Often, r has the sign of a.
            (*remainder_ptr)->sign = r_sign;


            // Some definitions require 0 <= r < |b|. If r is negative, adjust:
            // Example: -10 / 3 -> q = -4, r = 2 (instead of q=-3, r=-1)
            // if ((*remainder_ptr)->sign < 0) {
            //     BigInt *b_abs_copy = copyBigInt(b_abs); // Need |b| again
            //     BigInt *temp_q = NULL, *temp_r = NULL;
            //     addBigInt(*remainder_ptr, b_abs_copy, &temp_r); // r = r + |b|
            //     subtractBigInt(*quotient_ptr, createBigIntFromLL(1), &temp_q); // q = q - 1
            //     destroyBigInt(*remainder_ptr);
            //     destroyBigInt(*quotient_ptr);
            //     *remainder_ptr = temp_r;
            //     *quotient_ptr = temp_q;
            //     destroyBigInt(b_abs_copy);
            // }
        }
    }


main_div_cleanup:
    destroyBigInt(a_abs);
    destroyBigInt(b_abs);
    destroyBigInt(q_abs); // Destroy if not transferred
    destroyBigInt(r_abs); // Destroy if not transferred


     // If error occurred, ensure output pointers are NULL
     if (err != BIGINT_SUCCESS) {
          if (quotient_ptr && *quotient_ptr) { destroyBigInt(*quotient_ptr); *quotient_ptr = NULL; }
          if (remainder_ptr && *remainder_ptr) { destroyBigInt(*remainder_ptr); *remainder_ptr = NULL; }
     }


    return err;
}


// Decimal String Division (Adapted for Blocks)
// Returns a newly allocated string, caller must free.
char* bigIntToDecimalString(const BigInt *a, const BigInt *b, int precision) {
    if (!a || !b || precision < 0) return NULL;
    if (isBigIntZero(b)) return NULL;

    BigInt *quotient_int = NULL;
    BigInt *remainder_int = NULL;
    BigInt *current_remainder = NULL;
    BigInt *temp_digit_bi = NULL;
    BigInt *b_abs = NULL;
    char *integer_part_str = NULL;
    char *final_str = NULL;
    BigIntError err = BIGINT_SUCCESS;

    // 整数部分除法
    if ((err = divideBigInt(a, b, &quotient_int, &remainder_int)) != BIGINT_SUCCESS)
        goto dec_str_cleanup;

    // 转换整数部分
    if (!(integer_part_str = bigIntToString(quotient_int)))
        goto dec_str_cleanup;

    // 分配最终字符串空间（精确计算长度）
    const size_t int_len = strlen(integer_part_str);
    final_str = malloc(int_len + 1 + precision + 1); // 整数+点+精度+null
    if (!final_str) goto dec_str_cleanup;
    snprintf(final_str, int_len + 1, "%s", integer_part_str);

    // 处理小数部分
    if (precision > 0) {
        char *p = final_str + int_len;
        *p++ = '.';

        if (!(current_remainder = copyBigInt(remainder_int)) || 
            !(b_abs = copyBigInt(b))) {
            err = BIGINT_ALLOCATION_ERROR;
            goto dec_str_cleanup;
        }
        current_remainder->sign = 1;
        b_abs->sign = 1;

        // 修改后的小数部分处理
for (int k = 0; k < precision; ++k) {
    if (isBigIntZero(current_remainder)) {
        *p++ = '0';
        continue;
    }

    // 乘以10并计算
    multiplyBy10(current_remainder);
    BigInt *next_rem = NULL;
    err = divideBigInt(current_remainder, b_abs, &temp_digit_bi, &next_rem);
    if (err != BIGINT_SUCCESS) break;

    // 确保digit在0-9范围内
    int digit = 0;
    if (temp_digit_bi && !isBigIntZero(temp_digit_bi)) {
        digit = temp_digit_bi->digits[0] % 10;
        digit = (digit < 0) ? digit + 10 : digit;
    }

    // 处理负余数
    if (next_rem && next_rem->sign < 0) {
        BigInt *temp = NULL;
        addBigInt(next_rem, b_abs, &temp);
        if (temp) {
            destroyBigInt(next_rem);
            next_rem = temp;
            digit++; // 调整digit
            if (digit >= 10) {
                digit -= 10;
                // 需要处理进位，这里简化处理
            }
        }
    }

    *p++ = '0' + digit;
    destroyBigInt(current_remainder);
    current_remainder = next_rem;
}

        *p = '\0'; // 正确终止字符串
    }

dec_str_cleanup:
    // 统一资源释放
    destroyBigInt(quotient_int);
    destroyBigInt(remainder_int);
    destroyBigInt(current_remainder);
    destroyBigInt(temp_digit_bi);
    destroyBigInt(b_abs);
    free(integer_part_str);

    // 错误处理
    if (err != BIGINT_SUCCESS) {
        free(final_str);
        final_str = NULL;
    }
    return final_str;
}

