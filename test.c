//  gcc test.c bigint.c -o test
// author： 8891689
#include "bigint.h" // 包含你的 BigInt 库头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h> // For LLONG_MAX/MIN testing

// --- 辅助函数：打印测试结果 ---
void print_test_header(const char* test_name) {
    printf("\n--- 开始测试: %s ---\n", test_name);
}

void print_test_footer(const char* test_name) {
    printf("--- 结束测试: %s ---\n", test_name);
}

// 辅助函数：比较结果并打印
void check_result(const char* operation, const BigInt* result, const char* expected_str) {
    char* result_str = bigIntToString(result);
    printf("操作: %s\n", operation);
    printf("  预期结果: %s\n", expected_str);
    printf("  实际结果: %s\n", result_str ? result_str : "(NULL 或转换错误)");
    if (result_str && strcmp(result_str, expected_str) == 0) {
        printf("  状态: 通过\n");
    } else {
        printf("  状态: !!! 失败 !!!\n");
    }
    free(result_str);
}

void check_division_result(const char* operation, const BigInt* quotient, const BigInt* remainder, const char* expected_q_str, const char* expected_r_str) {
    char* q_str = bigIntToString(quotient);
    char* r_str = bigIntToString(remainder);
    printf("操作: %s\n", operation);
    printf("  预期商: %s, 预期余数: %s\n", expected_q_str, expected_r_str);
    printf("  实际商: %s, 实际余数: %s\n", q_str ? q_str : "(NULL)", r_str ? r_str : "(NULL)");

    bool q_pass = (q_str && strcmp(q_str, expected_q_str) == 0);
    bool r_pass = (r_str && strcmp(r_str, expected_r_str) == 0);

    if (q_pass && r_pass) {
        printf("  状态: 通过\n");
    } else {
        printf("  状态: !!! 失败 !!! (商: %s, 余数: %s)\n", q_pass ? "通过" : "失败", r_pass ? "通过" : "失败");
    }

    free(q_str);
    free(r_str);
}

void check_comparison_result(const char* comparison, int result, int expected) {
    printf("比较: %s\n", comparison);
    printf("  预期结果: %d\n", expected);
    printf("  实际结果: %d\n", result);
    if (result == expected) {
        printf("  状态: 通过\n");
    } else {
        printf("  状态: !!! 失败 !!!\n");
    }
}

void check_bool_result(const char* check, bool result, bool expected) {
    printf("检查: %s\n", check);
    printf("  预期结果: %s\n", expected ? "true" : "false");
    printf("  实际结果: %s\n", result ? "true" : "false");
    if (result == expected) {
        printf("  状态: 通过\n");
    } else {
        printf("  状态: !!! 失败 !!!\n");
    }
}


void check_decimal_string_result(const char* operation, const char* result, const char* expected) {
    printf("操作: %s\n", operation);
    printf("  预期结果: %s\n", expected);
    printf("  实际结果: %s\n", result ? result : "(NULL)");
     if (result && strcmp(result, expected) == 0) {
        printf("  状态: 通过\n");
    } else {
        printf("  状态: !!! 失败 !!!\n");
    }
}

// --- 主测试函数 ---
int main() {
    printf("=======================================\n");
    printf("        BigInt 库综合测试程序\n");
    printf("=======================================\n");

    BigInt *a = NULL, *b = NULL, *c = NULL, *d = NULL;
    BigInt *sum = NULL, *diff = NULL, *prod = NULL, *quot = NULL, *rem = NULL;
    BigInt *copy_a = NULL;
    char* str_res = NULL;
    BigIntError err;

    // --- 1. 创建与销毁测试 ---
    print_test_header("创建与销毁");
    a = createBigIntFromString("12345678901234567890"); // 大正数
    b = createBigIntFromString("-98765432109876543210"); // 大负数
    c = createBigIntFromLL(12345);                     // 来自 long long 正数
    d = createBigIntFromLL(-67890);                    // 来自 long long 负数
    BigInt *zero = createBigIntFromLL(0);              // 零
    BigInt *one = createBigIntFromLL(1);               // 一
    BigInt *neg_one = createBigIntFromLL(-1);          // 负一

    printf("创建 a = "); printBigInt(a); printf("\n");
    printf("创建 b = "); printBigInt(b); printf("\n");
    printf("创建 c = "); printBigInt(c); printf("\n");
    printf("创建 d = "); printBigInt(d); printf("\n");
    printf("创建 zero = "); printBigInt(zero); printf("\n");
    printf("创建 one = "); printBigInt(one); printf("\n");
    printf("创建 neg_one = "); printBigInt(neg_one); printf("\n");

    // 测试无效字符串创建
    BigInt *invalid = createBigIntFromString("12a34");
    if (invalid == NULL) {
        printf("从无效字符串 \"12a34\" 创建: 预期 NULL，实际 NULL -> 通过\n");
    } else {
        printf("从无效字符串 \"12a34\" 创建: 预期 NULL，实际非 NULL -> !!! 失败 !!!\n");
        destroyBigInt(invalid);
    }
     invalid = createBigIntFromString("-");
     if (invalid == NULL) {
         printf("从无效字符串 \"-\" 创建: 预期 NULL，实际 NULL -> 通过\n");
     } else {
        printf("从无效字符串 \"-\" 创建: 预期 NULL，实际非 NULL -> !!! 失败 !!!\n");
        destroyBigInt(invalid);
     }

    print_test_footer("创建与销毁");

    // --- 2. 复制测试 ---
    print_test_header("复制");
    copy_a = copyBigInt(a);
    printf("创建 a 的副本 copy_a = "); printBigInt(copy_a); printf("\n");
    // 验证副本独立性 (假设有修改操作，这里用比较代替)
    if (compareBigInt(a, copy_a) == 0) {
         printf("比较 a 和 copy_a: 预期相等 (0)，实际 %d -> 通过\n", compareBigInt(a, copy_a));
    } else {
         printf("比较 a 和 copy_a: 预期相等 (0)，实际 %d -> !!! 失败 !!!\n", compareBigInt(a, copy_a));
    }
    destroyBigInt(copy_a); // 销毁副本
    print_test_footer("复制");

    // --- 3. 比较测试 ---
    print_test_header("比较");
    check_comparison_result("a > b", compareBigInt(a, b), 1);           // 正 > 负
    check_comparison_result("b < a", compareBigInt(b, a), -1);          // 负 < 正
    check_comparison_result("a == a", compareBigInt(a, a), 0);           // 相等
    check_comparison_result("c > d", compareBigInt(c, d), 1);           // 小正 > 小负
    check_comparison_result("a > zero", compareBigInt(a, zero), 1);       // 正 > 0
    check_comparison_result("b < zero", compareBigInt(b, zero), -1);      // 负 < 0
    check_comparison_result("zero == zero", compareBigInt(zero, zero), 0); // 0 == 0
    check_comparison_result("one > neg_one", compareBigInt(one, neg_one), 1);
    check_comparison_result("neg_one < one", compareBigInt(neg_one, one), -1);

    check_bool_result("isBigIntZero(zero)", isBigIntZero(zero), true);
    check_bool_result("isBigIntZero(a)", isBigIntZero(a), false);
    check_bool_result("isBigIntZero(neg_one)", isBigIntZero(neg_one), false);
    print_test_footer("比较");

    // --- 4. 加法测试 ---
    print_test_header("加法");
    // 正 + 正
    err = addBigInt(a, c, &sum); assert(err == BIGINT_SUCCESS);
    check_result("a + c", sum, "12345678901234580235");
    destroyBigInt(sum); sum = NULL;
    // 负 + 负
    err = addBigInt(b, d, &sum); assert(err == BIGINT_SUCCESS);
    check_result("b + d", sum, "-98765432109876611100");
    destroyBigInt(sum); sum = NULL;
    // 正 + 负 (|正| > |负|)
    err = addBigInt(a, d, &sum); assert(err == BIGINT_SUCCESS);
    check_result("a + d", sum, "12345678901234500000");
    destroyBigInt(sum); sum = NULL;
    // 正 + 负 (|正| < |负|)
    err = addBigInt(c, b, &sum); assert(err == BIGINT_SUCCESS);
    check_result("c + b", sum, "-98765432109876530865");
    destroyBigInt(sum); sum = NULL;
    // 正 + 负 (互为相反数)
    BigInt* neg_a = NULL;
    err = multiplyBigIntByLL(a, -1, &neg_a); assert(err == BIGINT_SUCCESS && neg_a != NULL);
    err = addBigInt(a, neg_a, &sum); assert(err == BIGINT_SUCCESS);
    check_result("a + (-a)", sum, "0");
    destroyBigInt(sum); sum = NULL;
    destroyBigInt(neg_a); neg_a = NULL;
    // a + 0
    err = addBigInt(a, zero, &sum); assert(err == BIGINT_SUCCESS);
    check_result("a + 0", sum, "12345678901234567890");
    destroyBigInt(sum); sum = NULL;
    // 0 + b
    err = addBigInt(zero, b, &sum); assert(err == BIGINT_SUCCESS);
    check_result("0 + b", sum, "-98765432109876543210");
    destroyBigInt(sum); sum = NULL;
    print_test_footer("加法");


    // --- 5. 减法测试 ---
    print_test_header("减法");
    // 正 - 正 (a - c)
    err = subtractBigInt(a, c, &diff); assert(err == BIGINT_SUCCESS);
    check_result("a - c", diff, "12345678901234555545");
    destroyBigInt(diff); diff = NULL;
    // 正 - 正 (c - a)
    err = subtractBigInt(c, a, &diff); assert(err == BIGINT_SUCCESS);
    check_result("c - a", diff, "-12345678901234555545");
    destroyBigInt(diff); diff = NULL;
    // 负 - 负 (b - d)
    err = subtractBigInt(b, d, &diff); assert(err == BIGINT_SUCCESS);
    check_result("b - d", diff, "-98765432109876475320");
    destroyBigInt(diff); diff = NULL;
    // 负 - 负 (d - b)
    err = subtractBigInt(d, b, &diff); assert(err == BIGINT_SUCCESS);
    check_result("d - b", diff, "98765432109876475320");
    destroyBigInt(diff); diff = NULL;
    // 正 - 负 (a - b)
    err = subtractBigInt(a, b, &diff); assert(err == BIGINT_SUCCESS);
    check_result("a - b", diff, "111111111011111111100"); // a + |b|
    destroyBigInt(diff); diff = NULL;
    // 负 - 正 (b - a)
    err = subtractBigInt(b, a, &diff); assert(err == BIGINT_SUCCESS);
    check_result("b - a", diff, "-111111111011111111100"); // -( |b| + |a|)
    destroyBigInt(diff); diff = NULL;
    // a - a
    err = subtractBigInt(a, a, &diff); assert(err == BIGINT_SUCCESS);
    check_result("a - a", diff, "0");
    destroyBigInt(diff); diff = NULL;
    // a - 0
    err = subtractBigInt(a, zero, &diff); assert(err == BIGINT_SUCCESS);
    check_result("a - 0", diff, "12345678901234567890");
    destroyBigInt(diff); diff = NULL;
    // 0 - a
    err = subtractBigInt(zero, a, &diff); assert(err == BIGINT_SUCCESS);
    check_result("0 - a", diff, "-12345678901234567890");
    destroyBigInt(diff); diff = NULL;
    print_test_footer("减法");

    // --- 6. 乘法测试 (NTT) ---
    print_test_header("乘法 (NTT)");
    // 正 * 正
    BigInt* c_small = createBigIntFromLL(123);
    BigInt* d_small = createBigIntFromLL(456);
    err = multiplyBigInt(c_small, d_small, &prod); // multiplyBigInt is nttMultiplyBigInt
    assert(err == BIGINT_SUCCESS);
    check_result("123 * 456", prod, "56088");
    destroyBigInt(prod); prod = NULL;
    // 负 * 负
    BigInt* neg_c = createBigIntFromLL(-123);
    BigInt* neg_d = createBigIntFromLL(-456);
    err = multiplyBigInt(neg_c, neg_d, &prod);
    assert(err == BIGINT_SUCCESS);
    check_result("-123 * -456", prod, "56088");
    destroyBigInt(prod); prod = NULL;
    // 正 * 负
    err = multiplyBigInt(c_small, neg_d, &prod);
    assert(err == BIGINT_SUCCESS);
    check_result("123 * -456", prod, "-56088");
    destroyBigInt(prod); prod = NULL;
    // 负 * 正 (结果同上)
    err = multiplyBigInt(neg_c, d_small, &prod);
    assert(err == BIGINT_SUCCESS);
    check_result("-123 * 456", prod, "-56088");
    destroyBigInt(prod); prod = NULL;
    // 乘以 0
    err = multiplyBigInt(a, zero, &prod);
    assert(err == BIGINT_SUCCESS);
    check_result("a * 0", prod, "0");
    destroyBigInt(prod); prod = NULL;
    err = multiplyBigInt(zero, b, &prod);
    assert(err == BIGINT_SUCCESS);
    check_result("0 * b", prod, "0");
    destroyBigInt(prod); prod = NULL;
    // 乘以 1
    err = multiplyBigInt(a, one, &prod);
    assert(err == BIGINT_SUCCESS);
    check_result("a * 1", prod, "12345678901234567890");
    destroyBigInt(prod); prod = NULL;
    // 乘以 -1
    err = multiplyBigInt(a, neg_one, &prod);
    assert(err == BIGINT_SUCCESS);
    check_result("a * -1", prod, "-12345678901234567890");
    destroyBigInt(prod); prod = NULL;

    // 大数乘法 (需要较长时间，结果会很长)
    printf("计算大数乘法: a * b ...\n");
    err = multiplyBigInt(a, b, &prod);
    assert(err == BIGINT_SUCCESS);
    // 注意：这个结果非常大，请确认你的 NTT 和进位处理能处理这么大的数
    check_result("a * b", prod, "-1219326311370217952237463801111263526900");
    destroyBigInt(prod); prod = NULL;

    // 测试 multiplyBigIntByLL
    err = multiplyBigIntByLL(c_small, -10, &prod);
    assert(err == BIGINT_SUCCESS);
    check_result("123 * -10 (LL)", prod, "-1230");
    destroyBigInt(prod); prod = NULL;

    err = multiplyBigIntByLL(b, 0, &prod);
    assert(err == BIGINT_SUCCESS);
    check_result("b * 0 (LL)", prod, "0");
    destroyBigInt(prod); prod = NULL;

    destroyBigInt(c_small);
    destroyBigInt(d_small);
    destroyBigInt(neg_c);
    destroyBigInt(neg_d);
    print_test_footer("乘法 (NTT)");


        // --- 7. 除法测试 ---
    print_test_header("除法 (商和余数)");
    BigInt *n100 = createBigIntFromLL(100);
    BigInt *n10 = createBigIntFromLL(10);
    BigInt *n3 = createBigIntFromLL(3);
    BigInt *n_10 = createBigIntFromLL(-10);
    BigInt *n_3 = createBigIntFromLL(-3);
    BigInt *n12345 = createBigIntFromString("12345");
    BigInt *n567 = createBigIntFromString("567");

    // 正 / 正 (100 / 10)
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(n100, n10, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("100 / 10", quot, rem, "10", "0");
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;
    // 正 / 正 (100 / 3)
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(n100, n3, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("100 / 3", quot, rem, "33", "1"); // 余数符号与被除数一致
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;
     // 正 / 正 (3 / 10)
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(n3, n10, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("3 / 10", quot, rem, "0", "3");
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;
    // 大数 / 小数 (12345 / 567)
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(n12345, n567, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("12345 / 567", quot, rem, "21", "438"); // 12345 = 567 * 21 + 438
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;

    // 负 / 正 (-100 / 3)
    BigInt *n_100 = createBigIntFromLL(-100);
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(n_100, n3, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("-100 / 3", quot, rem, "-33", "-1"); // q=-33, r=-1 => -33*3 + (-1) = -99 - 1 = -100
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;
    destroyBigInt(n_100); n_100 = NULL;
    // 正 / 负 (100 / -3)
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(n100, n_3, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("100 / -3", quot, rem, "-33", "1"); // q=-33, r=1 => -33*(-3) + 1 = 99 + 1 = 100
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;
    // 负 / 负 (-100 / -3)
    n_100 = createBigIntFromLL(-100);
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(n_100, n_3, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("-100 / -3", quot, rem, "33", "-1"); // q=33, r=-1 => 33*(-3) + (-1) = -99 - 1 = -100
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;
    destroyBigInt(n_100); n_100 = NULL;

    // 除以 1
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(a, one, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("a / 1", quot, rem, "12345678901234567890", "0");
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;
    // 除以 -1
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(a, neg_one, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("a / -1", quot, rem, "-12345678901234567890", "0");
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;
    // 0 / a
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(zero, a, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("0 / a", quot, rem, "0", "0");
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;
    // a / a
    // 修正下面这一行：将 "," 改为 "
    err = divideBigInt(a, a, &quot, &rem); // 原错误参数 ", &rem"
    check_division_result("a / a", quot, rem, "1", "0");
    destroyBigInt(quot); destroyBigInt(rem); quot = NULL; rem = NULL;

    // 测试除以 0 (预期错误)
    // 修正下面这一行：将 "," 改为 " (即使预期错误，函数调用语法也要正确)
    err = divideBigInt(n100, zero, &quot, &rem); // 原错误参数 ", &rem"
    if (err == BIGINT_DIVIDE_BY_ZERO) {
        printf("测试 100 / 0: 预期 BIGINT_DIVIDE_BY_ZERO，实际 %d -> 通过\n", err);
    } else {
        printf("测试 100 / 0: 预期 BIGINT_DIVIDE_BY_ZERO，实际 %d -> !!! 失败 !!!\n", err);
        destroyBigInt(quot); // 如果库没有正确处理，可能需要销毁
        destroyBigInt(rem);
    }
    quot = NULL; rem = NULL; // 确保指针设为 NULL

    // ... (后面的代码不变) ...
    destroyBigInt(n100);
    destroyBigInt(n10);
    destroyBigInt(n3);
    destroyBigInt(n_10);
    destroyBigInt(n_3);
    destroyBigInt(n12345);
    destroyBigInt(n567);
    print_test_footer("除法 (商和余数)");

    // --- 8. 小数精度字符串转换测试 ---
    print_test_header("小数精度字符串转换");
    n100 = createBigIntFromLL(100);
    n3 = createBigIntFromLL(3);
    BigInt *n7 = createBigIntFromLL(7);
    n_3 = createBigIntFromLL(-3);
    n_100 = createBigIntFromLL(-100);

    // 100 / 3, precision 5
    str_res = bigIntToDecimalString(n100, n3, 5);
    check_decimal_string_result("100 / 3 (prec 5)", str_res, "33.33333");
    free(str_res); str_res = NULL;
    // 100 / 7, precision 10
    str_res = bigIntToDecimalString(n100, n7, 10);
    check_decimal_string_result("100 / 7 (prec 10)", str_res, "14.2857142857"); // 100 / 7 ≈ 14.285714...
    free(str_res); str_res = NULL;
    // 1 / 3, precision 8
    str_res = bigIntToDecimalString(one, n3, 8);
    check_decimal_string_result("1 / 3 (prec 8)", str_res, "0.33333333");
    free(str_res); str_res = NULL;
    // -100 / 3, precision 5
    str_res = bigIntToDecimalString(n_100, n3, 5);
    check_decimal_string_result("-100 / 3 (prec 5)", str_res, "-33.33333"); // 取决于实现，可能需要 -34.xxxx 或 -33.xxxx
                                                                          // 这个实现倾向于商向0截断，所以 -33.33333 可能性大
    free(str_res); str_res = NULL;
    // 100 / -3, precision 5
    str_res = bigIntToDecimalString(n100, n_3, 5);
    check_decimal_string_result("100 / -3 (prec 5)", str_res, "-33.33333");
    free(str_res); str_res = NULL;
    // 12 / 4, precision 5
    BigInt *n12 = createBigIntFromLL(12);
    BigInt *n4 = createBigIntFromLL(4);
    str_res = bigIntToDecimalString(n12, n4, 5);
    check_decimal_string_result("12 / 4 (prec 5)", str_res, "3.00000");
    free(str_res); str_res = NULL;
    // 10 / 3, precision 0
    n10 = createBigIntFromLL(10);
    str_res = bigIntToDecimalString(n10, n3, 0);
    check_decimal_string_result("10 / 3 (prec 0)", str_res, "3"); // 无小数部分
    free(str_res); str_res = NULL;

    destroyBigInt(n100);
    destroyBigInt(n3);
    destroyBigInt(n7);
    destroyBigInt(n_3);
    destroyBigInt(n_100);
    destroyBigInt(n12);
    destroyBigInt(n4);
    destroyBigInt(n10);
    print_test_footer("小数精度字符串转换");


    // --- 清理所有剩余资源 ---
    printf("\n--- 开始清理所有 BigInt 对象 ---\n");
    destroyBigInt(a);
    destroyBigInt(b);
    destroyBigInt(c);
    destroyBigInt(d);
    destroyBigInt(zero);
    destroyBigInt(one);
    destroyBigInt(neg_one);
    // 确保中间结果也被清理（虽然上面的测试已清理，以防万一）
    destroyBigInt(sum);
    destroyBigInt(diff);
    destroyBigInt(prod);
    destroyBigInt(quot);
    destroyBigInt(rem);
    printf("--- 清理完成 ---\n");

    printf("\n=======================================\n");
    printf("        BigInt 库测试结束\n");
    printf("=======================================\n");

    return 0;
}
