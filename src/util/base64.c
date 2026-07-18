/**
 * base64.c - Base64 Encoding/Decoding Implementation
 */

#include "util/base64.h"
#include <stdint.h>
#include <string.h>

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char base64_decode_table[256] = {
    ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,  ['E'] = 4,  ['F'] = 5,
    ['G'] = 6,  ['H'] = 7,  ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11,
    ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15, ['Q'] = 16, ['R'] = 17,
    ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29,
    ['e'] = 30, ['f'] = 31, ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35,
    ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39, ['o'] = 40, ['p'] = 41,
    ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
    ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51, ['0'] = 52, ['1'] = 53,
    ['2'] = 54, ['3'] = 55, ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59,
    ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63, ['='] = 64
};

size_t base64_encode_size(size_t input_len) {
    return ((input_len + 2) / 3) * 4 + 1;  /* +1 for null terminator */
}

size_t base64_decode_size(size_t input_len) {
    return (input_len / 4) * 3 + 3;  /* +3 for padding */
}

int base64_encode(const unsigned char *input, size_t input_len,
                  char *output, size_t output_len) {
    if (!input || !output || output_len < base64_encode_size(input_len)) {
        return -1;
    }

    size_t i, j;
    for (i = 0, j = 0; i < input_len;) {
        uint32_t octet_a = i < input_len ? input[i++] : 0;
        uint32_t octet_b = i < input_len ? input[i++] : 0;
        uint32_t octet_c = i < input_len ? input[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        output[j++] = base64_chars[(triple >> 18) & 0x3F];
        output[j++] = base64_chars[(triple >> 12) & 0x3F];
        output[j++] = base64_chars[(triple >> 6) & 0x3F];
        output[j++] = base64_chars[triple & 0x3F];
    }

    /* Add padding */
    size_t padding = (3 - (input_len % 3)) % 3;
    for (size_t p = 0; p < padding; p++) {
        output[j - 1 - p] = '=';
    }

    output[j] = '\0';
    return j;
}

int base64_decode(const char *input, size_t input_len,
                  unsigned char *output, size_t *output_len) {
    if (!input || !output || !output_len) {
        return -1;
    }

    if (input_len % 4 != 0) {
        return -1;  /* Invalid base64 length */
    }

    size_t padding = 0;
    if (input_len >= 2) {
        if (input[input_len - 1] == '=') padding++;
        if (input[input_len - 2] == '=') padding++;
    }

    size_t decoded_len = (input_len / 4) * 3 - padding;
    if (*output_len < decoded_len) {
        return -1;  /* Output buffer too small */
    }

    size_t i, j;
    for (i = 0, j = 0; i < input_len;) {
        uint32_t a = input[i] == '=' ? 0 : base64_decode_table[(unsigned char)input[i]]; i++;
        uint32_t b = input[i] == '=' ? 0 : base64_decode_table[(unsigned char)input[i]]; i++;
        uint32_t c = input[i] == '=' ? 0 : base64_decode_table[(unsigned char)input[i]]; i++;
        uint32_t d = input[i] == '=' ? 0 : base64_decode_table[(unsigned char)input[i]]; i++;

        uint32_t triple = (a << 18) + (b << 12) + (c << 6) + d;

        if (j < decoded_len) output[j++] = (triple >> 16) & 0xFF;
        if (j < decoded_len) output[j++] = (triple >> 8) & 0xFF;
        if (j < decoded_len) output[j++] = triple & 0xFF;
    }

    *output_len = decoded_len;
    return 0;
}
