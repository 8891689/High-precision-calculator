// gcc calculator.c bigint.c -o calculator
#define _GNU_SOURCE  // 使 getline 可用
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bigint.h"  // 包含 BigInt 库相关声明

// 定义 BigDecimal 结构：存储大整数和小数位数
typedef struct {
    BigInt *value;  // 大整数表示（去掉小数点）
    int scale;      // 小数点右侧位数
} BigDecimal;

// 工具函数：计算10的幂（以 BigInt 形式返回），例如 10^n
BigInt* bigintPow10(int n) {
    BigInt *result = createBigIntFromLL(1);
    for (int i = 0; i < n; i++) {
        BigInt *temp = NULL;
        if (multiplyBigIntByLL(result, 10, &temp) != BIGINT_SUCCESS) {
            destroyBigInt(result);
            return NULL;
        }
        destroyBigInt(result);
        result = temp;
    }
    return result;
}

// 解析字符串为 BigDecimal  
// 输入格式：可含可不含小数点，允许正负号，例如 "-123.456"
BigDecimal parseBigDecimal(const char *s) {
    BigDecimal dec;
    dec.value = NULL;
    dec.scale = 0;
    if (!s) return dec;

    // 去除空白字符
    while (isspace((unsigned char)*s)) s++;

    int len = strlen(s);
    char *numStr = (char*)malloc(len+1);
    if (!numStr) return dec;
    int j = 0;
    int scale = 0;
    int seenDot = 0;
    for (int i = 0; i < len; i++) {
        if (s[i] == '.') {
            seenDot = 1;
            continue;
        }
        if (isdigit((unsigned char)s[i]) || ((s[i]=='-' || s[i]=='+') && i==0)) {
            numStr[j++] = s[i];
            if (seenDot && isdigit((unsigned char)s[i])) {
                scale++;
            }
        } else if (isspace((unsigned char)s[i])) {
            continue;
        } else {
            // 非法字符，返回空 BigDecimal
            free(numStr);
            return dec;
        }
    }
    numStr[j] = '\0';
    dec.scale = scale;
    // 创建 BigInt（字符串中已经不含小数点）
    dec.value = createBigIntFromString(numStr);
    free(numStr);
    return dec;
}

// 将 BigDecimal 转换为字符串（带小数点）
// 如果整数部分位数不足，会自动补 0
char* bigDecimalToString(const BigDecimal *dec) {
    if (!dec || !dec->value) return NULL;
    char *numStr = bigIntToString(dec->value);
    if (!numStr) return NULL;
    int len = strlen(numStr);
    int scale = dec->scale;
    int sign = 0;
    if (numStr[0]=='-' || numStr[0]=='+') {
        sign = 1;
    }
    // 若 scale==0，则直接返回
    if (scale == 0) return numStr;
    
    // 如果整数部分位数不足，则需要在前面补零
    int intPartLen = len - sign;
    int totalLen = (intPartLen > scale ? intPartLen : scale) + 1 /*小数点*/ + sign;
    char *result = (char*)malloc(totalLen + 2); // 多加一位防止额外
    if (!result) { free(numStr); return NULL; }
    
    int pos = 0;
    if (sign) {
        result[pos++] = numStr[0];
    }
    // 如果数字长度小于等于 scale，则整数部分为0，需要补0
    if (intPartLen <= scale) {
        result[pos++] = '0';
        result[pos++] = '.';
        // 补齐前导0
        for (int i = 0; i < scale - intPartLen; i++) {
            result[pos++] = '0';
        }
        // 复制剩余数字
        for (int i = sign; i < len; i++) {
            result[pos++] = numStr[i];
        }
    } else {
        int intDigits = intPartLen - scale;
        // 复制整数部分
        for (int i = sign; i < sign + intDigits; i++) {
            result[pos++] = numStr[i];
        }
        result[pos++] = '.';
        // 复制小数部分
        for (int i = sign + intDigits; i < len; i++) {
            result[pos++] = numStr[i];
        }
    }
    result[pos] = '\0';
    free(numStr);
    return result;
}

// 对齐两个 BigDecimal 的 scale，使得两者 scale 相同
// 该函数会将 d 的值扩大 10^(delta)（内部修改 d）
void alignScale(BigDecimal *d, int newScale) {
    if (!d || !d->value) return;
    int delta = newScale - d->scale;
    if (delta <= 0) return;
    BigInt *factor = bigintPow10(delta);
    if (!factor) return;
    BigInt *newVal = NULL;
    if (nttMultiplyBigInt(d->value, factor, &newVal) != BIGINT_SUCCESS) {
        destroyBigInt(factor);
        return;
    }
    destroyBigInt(d->value);
    d->value = newVal;
    d->scale = newScale;
    destroyBigInt(factor);
}

// BigDecimal 加法
BigDecimal addBigDecimal(BigDecimal a, BigDecimal b) {
    BigDecimal result;
    result.value = NULL;
    result.scale = 0;
    // 对齐 scale
    int newScale = (a.scale > b.scale ? a.scale : b.scale);
    alignScale(&a, newScale);
    alignScale(&b, newScale);
    if (addBigInt(a.value, b.value, &result.value) != BIGINT_SUCCESS) {
        result.value = NULL;
        return result;
    }
    result.scale = newScale;
    return result;
}

// BigDecimal 减法
BigDecimal subBigDecimal(BigDecimal a, BigDecimal b) {
    BigDecimal result;
    result.value = NULL;
    result.scale = 0;
    int newScale = (a.scale > b.scale ? a.scale : b.scale);
    alignScale(&a, newScale);
    alignScale(&b, newScale);
    if (subtractBigInt(a.value, b.value, &result.value) != BIGINT_SUCCESS) {
        result.value = NULL;
        return result;
    }
    result.scale = newScale;
    return result;
}

// BigDecimal 乘法：结果 scale = a.scale + b.scale
BigDecimal mulBigDecimal(BigDecimal a, BigDecimal b) {
    BigDecimal result;
    result.value = NULL;
    result.scale = a.scale + b.scale;
    if (multiplyBigInt(a.value, b.value, &result.value) != BIGINT_SUCCESS) {
        result.value = NULL;
    }
    return result;
}

// BigDecimal 除法，精度由 precision 给出，结果 scale = precision
// 计算公式：result = (a.value * 10^(precision + b.scale - a.scale)) / b.value
BigDecimal divBigDecimal(BigDecimal a, BigDecimal b, int precision) {
    BigDecimal result;
    result.value = NULL;
    result.scale = precision;
    result.value = NULL;
    if (!b.value || isBigIntZero(b.value)) {
        // 除数为零，返回空
        return result;
    }
    // 计算放大倍数 = 10^(precision + b.scale - a.scale)
    int delta = precision + b.scale - a.scale;
    BigInt *factor = bigintPow10(delta);
    if (!factor) return result;
    BigInt *numerator = NULL;
    if (nttMultiplyBigInt(a.value, factor, &numerator) != BIGINT_SUCCESS) {
        destroyBigInt(factor);
        return result;
    }
    destroyBigInt(factor);
    // 进行整数除法 numerator / b.value
    BigInt *quotient = NULL;
    BigInt *remainder = NULL;
    if (divideBigInt(numerator, b.value, &quotient, &remainder) != BIGINT_SUCCESS) {
        destroyBigInt(numerator);
        destroyBigInt(remainder);
        return result;
    }
    destroyBigInt(numerator);
    destroyBigInt(remainder);
    result.value = quotient;
    return result;
}

// ------- 以下为表达式解析部分 -------

// 定义记号类型
typedef enum {
    TOKEN_NUM,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_END,
    TOKEN_INVALID
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[1000];
} Token;

const char *input_str;
int pos = 0;
Token current_token;

void nextToken() {
    int i = 0;
    while (input_str[pos] && isspace((unsigned char)input_str[pos])) pos++;
    if (!input_str[pos]) {
        current_token.type = TOKEN_END;
        return;
    }
    char c = input_str[pos];
    if (isdigit(c) || c == '.' || ((c=='-' || c=='+') && isdigit(input_str[pos+1]))) {
        int start = pos;
        if (c=='-' || c=='+') pos++;
        while (input_str[pos] && (isdigit(input_str[pos]) || input_str[pos]=='.'))
            pos++;
        int len = pos - start;
        if (len >= sizeof(current_token.lexeme)) len = sizeof(current_token.lexeme)-1;
        strncpy(current_token.lexeme, input_str+start, len);
        current_token.lexeme[len] = '\0';
        current_token.type = TOKEN_NUM;
        return;
    }
    pos++;
    switch(c) {
        case '+': current_token.type = TOKEN_PLUS; break;
        case '-': current_token.type = TOKEN_MINUS; break;
        case '*': current_token.type = TOKEN_MUL; break;
        case '/': current_token.type = TOKEN_DIV; break;
        case '(': current_token.type = TOKEN_LPAREN; break;
        case ')': current_token.type = TOKEN_RPAREN; break;
        default: current_token.type = TOKEN_INVALID; break;
    }
    current_token.lexeme[0] = c;
    current_token.lexeme[1] = '\0';
}

// 前向声明
BigDecimal parseExpression();

BigDecimal parseFactor() {
    BigDecimal result;
    result.value = NULL;
    result.scale = 0;
    if (current_token.type == TOKEN_NUM) {
        result = parseBigDecimal(current_token.lexeme);
        nextToken();
        return result;
    } else if (current_token.type == TOKEN_MINUS) {
        nextToken();
        result = parseFactor();
        if (result.value) {
            BigDecimal zero;
            zero.value = createBigIntFromLL(0);
            zero.scale = 0;
            BigDecimal tmp = subBigDecimal(zero, result);
            destroyBigInt(zero.value);
            destroyBigInt(result.value);
            return tmp;
        }
    } else if (current_token.type == TOKEN_PLUS) {
        nextToken();
        return parseFactor();
    } else if (current_token.type == TOKEN_LPAREN) {
        nextToken();
        result = parseExpression();
        if (current_token.type != TOKEN_RPAREN) {
            printf("Error: missing )\n");
            destroyBigInt(result.value);
            result.value = NULL;
            return result;
        }
        nextToken();
        return result;
    }
    printf("Error: invalid token in factor\n");
    return result;
}

BigDecimal parseTerm() {
    BigDecimal result = parseFactor();
    while (current_token.type == TOKEN_MUL || current_token.type == TOKEN_DIV) {
        TokenType op = current_token.type;
        nextToken();
        BigDecimal right = parseFactor();
        BigDecimal tmp;
        if (op == TOKEN_MUL) {
            tmp = mulBigDecimal(result, right);
        } else { // TOKEN_DIV
            tmp = divBigDecimal(result, right, 100);
        }
        destroyBigInt(result.value);
        destroyBigInt(right.value);
        result = tmp;
    }
    return result;
}

BigDecimal parseExpression() {
    BigDecimal result = parseTerm();
    while (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS) {
        TokenType op = current_token.type;
        nextToken();
        BigDecimal right = parseTerm();
        BigDecimal tmp;
        if (op == TOKEN_PLUS) {
            tmp = addBigDecimal(result, right);
        } else {
            tmp = subBigDecimal(result, right);
        }
        destroyBigInt(result.value);
        destroyBigInt(right.value);
        result = tmp;
    }
    return result;
}

int main() {
    char *line = NULL;
    size_t linecap = 0;
    printf("supports + - * /, decimals, negative numbers, parentheses, for example: (123.45 + -67.89) * 10\n");
    while (1) {
        printf("> ");
        if (getline(&line, &linecap, stdin) <= 0)
            break;
        // 如果输入行仅为换行符，则跳过
        if (line[0] == '\n')
            continue;
        input_str = line;
        pos = 0;
        nextToken();
        BigDecimal result = parseExpression();
        if (!result.value) {
            printf("Calculation error\n");
        } else {
            char *resStr = bigDecimalToString(&result);
            if (resStr) {
                printf("result: %s\n", resStr);
                free(resStr);
            } else {
                printf("Result conversion error\n");
            }
            destroyBigInt(result.value);
        }
    }
    free(line);
    return 0;
}

