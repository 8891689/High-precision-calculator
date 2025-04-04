# High-precision calculator

High-precision integer calculation library, supports integers of arbitrary length, negative decimals and other operations, and provides high-precision decimal calculation capabilities. Built-in command line calculator, can parse complex mathematical expressions, supports addition, subtraction, multiplication, division, brackets, and decimal operations.

# Key Features

BigInt Library

1. Basic Operations​​: Addition, subtraction, multiplication, division

​​2. High-Performance Multiplication​​: Accelerated using Number Theoretic Transform (NTT) algorithm

​​3. Memory Management​​: Automatic capacity expansion and reference counting

​​4. Base Conversion​​: Bidirectional conversion between decimal strings and big integers

​​5. Error Handling​​: Comprehensive error code system (division by zero, allocation errors, etc.)

# Precision Calculator

​​Decimal Support​​: Automatic decimal alignment and precision control

​​Expression Parsing​​: Supports nested parentheses, negative numbers, and consecutive operations

​​Precision Guarantee​​: Default 100-digit decimal precision (configurable)

​​Interactive Mode​​: User-friendly command-line interface

# ompilation Instructions

Compile with GCC (requires C99 standard)
```
gcc calculator.c bigint.c -o calculator
```

Usage Examples

# Start calculator
```
./calculator
supports + - * /, decimals, negative numbers, parentheses, for example: (123.45 + -67.89) * 10
> (12345678901234567890 + 9876543210987654321) * 2
result: 44444444224444444422


> 3.1415926535 * 2.718281828459045
result: 8.5397342224294829976259075

```

# Adjust for higher precision
```
1. Multiplication of large integers is set to 1000 digits
typedef struct {
    TokenType type;
    char lexeme[1000];
} Token;

2.Division to 100 decimal places

} else { // TOKEN_DIV
            tmp = divBigDecimal(result, right, 100);

Set as required
```
# High-Precision Decimals
```
1.
> 1 / 33333333333333333333333333
result: 0.0000000000000000000000000300000000000000000000000003000000000000000000000000030000000000000000000000
2.
> 0.0000000000017547722846034358457493988015057417367885445385074201097886470485403000920363858392858236 * 0.0000000000017547722846034358457493988015057417367885445385074201097886470485403000920363858392858236
result: 0.00000000000000000000000307922577081236165095042327302547835677592988802455109741719050242946390739726838859889317162963587974758854852750365132525555982510397747160622940604494005207618406569593031696
3.
> 307922577081236165095042327302547835677592988802455109741719050242946390739726838859889317162963587974758854852750365132525555982510397747160622940604494005207618406569593031696 * 307922577081236165095042327302547835677592988802455109741719050242946390739726838859889317162963587974758854852750365132525555982510397747160622940604494005207618406569593031696
result: 94816313476349827609925081213504949829734554316448534074133971824280212608852840943147716042562846304090927251800978515441585360089971159071689621366548202025501913516236596785339899833143992924367775718575760647438808218549624146902419295298689773192009746587681673607152313514146319619998094891456335167670861774098021126510795903298214908640460636416

```

# Technical Details

1. Core Algorithms
NTT-accelerated Multiplication​​: O(n log n) complexity using Fast Number Theoretic Transform

​​Block Storage​​: Uses base-10³ representation for optimal memory-computation balance

​​Dynamic Expansion​​: Exponential growth strategy for automatic storage scaling

2. recision Control

​​Decimal Alignment​​: Automatic scale unification during operations

​​Precision Extension​​: Automatic dividend precision extension in division

​​Trailing Zero Handling​​: Automatic removal of insignificant zeros in string conversion


### ⚙️ Thanks


Thanks for your help : gemini, ChatGPT, deepseek .

### Sponsorship
If this project has been helpful to you, please consider sponsoring. Your support is greatly appreciated. Thank you!
```
BTC: bc1qt3nh2e6gjsfkfacnkglt5uqghzvlrr6jahyj2k
ETH: 0xD6503e5994bF46052338a9286Bc43bC1c3811Fa1
DOGE: DTszb9cPALbG9ESNJMFJt4ECqWGRCgucky
TRX: TAHUmjyzg7B3Nndv264zWYUhQ9HUmX4Xu4
```
### 📜 Disclaimer

This tool is provided for learning and research purposes only. Please use it with an understanding of the relevant risks. The developers are not responsible for financial losses or legal liability -caused by the use of this tool.
