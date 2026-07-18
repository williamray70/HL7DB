/**
 * base64.h - Base64 Encoding/Decoding Utility
 *
 * Used for encoding binary encrypted data for storage in text files.
 */

#ifndef HL7DB_BASE64_H
#define HL7DB_BASE64_H

#include <stddef.h>

/**
 * Calculate the size needed for base64 encoding
 *
 * Args:
 *   input_len: Length of binary input data
 *
 * Returns:
 *   Number of bytes needed for base64 output (including null terminator)
 */
size_t base64_encode_size(size_t input_len);

/**
 * Calculate the maximum size needed for base64 decoding
 *
 * Args:
 *   input_len: Length of base64 input string
 *
 * Returns:
 *   Maximum number of bytes needed for binary output
 */
size_t base64_decode_size(size_t input_len);

/**
 * Encode binary data to base64 string
 *
 * Args:
 *   input: Binary data to encode
 *   input_len: Length of input data
 *   output: Output buffer for base64 string
 *   output_len: Size of output buffer (must be >= base64_encode_size(input_len))
 *
 * Returns:
 *   Number of bytes written to output (not including null terminator), or -1 on error
 */
int base64_encode(const unsigned char *input, size_t input_len,
                  char *output, size_t output_len);

/**
 * Decode base64 string to binary data
 *
 * Args:
 *   input: Base64 string to decode
 *   input_len: Length of input string
 *   output: Output buffer for binary data
 *   output_len: Size of output buffer (in), bytes written (out)
 *
 * Returns:
 *   0 on success, -1 on error
 */
int base64_decode(const char *input, size_t input_len,
                  unsigned char *output, size_t *output_len);

#endif /* HL7DB_BASE64_H */
