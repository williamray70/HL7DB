/**
 * encryption.h - HIPAA-Compliant Encryption at Rest
 *
 * Implements AES-256-GCM encryption for PHI data storage
 * Compliance: 45 CFR §164.312(a)(2)(iv) - Encryption and Decryption
 */

#ifndef HL7DB_ENCRYPTION_H
#define HL7DB_ENCRYPTION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ====================================================================
 * Encryption Configuration
 * ==================================================================== */

#define ENCRYPTION_KEY_SIZE 32      /* 256 bits for AES-256 */
#define ENCRYPTION_IV_SIZE 12       /* 96 bits for GCM */
#define ENCRYPTION_TAG_SIZE 16      /* 128 bits authentication tag */
#define ENCRYPTION_SALT_SIZE 16     /* Salt for key derivation */

/**
 * Encryption configuration
 */
typedef struct {
    bool enabled;                   /* Enable encryption? */
    char key_file[256];             /* Path to master key file */
    char *master_key;               /* Master key (loaded from file or HSM) */
    bool use_hsm;                   /* Use Hardware Security Module? */
    char hsm_config[256];           /* HSM configuration path */
} encryption_config_t;

/**
 * Encrypted data envelope
 */
typedef struct {
    uint8_t iv[ENCRYPTION_IV_SIZE];         /* Initialization vector */
    uint8_t tag[ENCRYPTION_TAG_SIZE];       /* Authentication tag */
    uint8_t *ciphertext;                    /* Encrypted data */
    size_t ciphertext_len;                  /* Length of ciphertext */
    uint8_t salt[ENCRYPTION_SALT_SIZE];     /* Salt for key derivation */
} encrypted_data_t;

/* ====================================================================
 * Key Management
 * ==================================================================== */

/**
 * Initialize encryption system
 *
 * Args:
 *   config: Encryption configuration
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int encryption_init(const encryption_config_t *config);

/**
 * Cleanup encryption system
 */
void encryption_cleanup(void);

/**
 * Generate a new master encryption key
 *
 * Creates a cryptographically secure random key and saves it to file.
 * The key file should be protected with filesystem permissions (0400).
 *
 * Args:
 *   key_file_path: Path where key will be saved
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int encryption_generate_master_key(const char *key_file_path);

/**
 * Load master key from file
 *
 * Args:
 *   key_file_path: Path to key file
 *   key_out: Buffer to store key (must be ENCRYPTION_KEY_SIZE bytes)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int encryption_load_master_key(const char *key_file_path, unsigned char *key_out);

/**
 * Derive encryption key from master key using PBKDF2
 *
 * Args:
 *   master_key: Master key
 *   master_key_len: Length of master key
 *   salt: Salt for derivation
 *   salt_len: Length of salt
 *   derived_key: Output buffer (ENCRYPTION_KEY_SIZE bytes)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int encryption_derive_key(const unsigned char *master_key,
                          size_t master_key_len,
                          const unsigned char *salt,
                          size_t salt_len,
                          unsigned char *derived_key);

/* ====================================================================
 * Encryption/Decryption Operations
 * ==================================================================== */

/**
 * Encrypt data using AES-256-GCM
 *
 * Args:
 *   plaintext: Data to encrypt
 *   plaintext_len: Length of plaintext
 *   aad: Additional authenticated data (optional, can be NULL)
 *   aad_len: Length of AAD
 *   encrypted: Output encrypted data structure
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int encryption_encrypt(const unsigned char *plaintext,
                       size_t plaintext_len,
                       const unsigned char *aad,
                       size_t aad_len,
                       encrypted_data_t *encrypted);

/**
 * Decrypt data using AES-256-GCM
 *
 * Args:
 *   encrypted: Encrypted data structure
 *   aad: Additional authenticated data (must match encryption)
 *   aad_len: Length of AAD
 *   plaintext: Output buffer for decrypted data
 *   plaintext_len: Size of output buffer (in), bytes written (out)
 *
 * Returns:
 *   0 on success, -1 on failure (includes authentication failure)
 */
int encryption_decrypt(const encrypted_data_t *encrypted,
                       const unsigned char *aad,
                       size_t aad_len,
                       unsigned char *plaintext,
                       size_t *plaintext_len);

/**
 * Free encrypted data structure
 */
void encryption_free_data(encrypted_data_t *encrypted);

/* ====================================================================
 * Convenience Functions for HL7 Messages
 * ==================================================================== */

/**
 * Encrypt HL7 message for storage
 *
 * Args:
 *   message: Raw HL7 message string
 *   message_id: Message ID (used as AAD for authentication)
 *   encrypted: Output encrypted data
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int encryption_encrypt_message(const char *message,
                                uint64_t message_id,
                                encrypted_data_t *encrypted);

/**
 * Decrypt HL7 message from storage
 *
 * Args:
 *   encrypted: Encrypted data
 *   message_id: Message ID (must match encryption)
 *   message_out: Output buffer for decrypted message
 *   message_len: Size of output buffer (in), bytes written (out)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int encryption_decrypt_message(const encrypted_data_t *encrypted,
                                uint64_t message_id,
                                char *message_out,
                                size_t *message_len);

/* ====================================================================
 * Serialization for Storage
 * ==================================================================== */

/**
 * Serialize encrypted data to binary format
 *
 * Format: [IV(12)][TAG(16)][SALT(16)][CIPHERTEXT_LEN(4)][CIPHERTEXT]
 *
 * Args:
 *   encrypted: Encrypted data
 *   buffer: Output buffer
 *   buffer_len: Size of buffer (in), bytes written (out)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int encryption_serialize(const encrypted_data_t *encrypted,
                         unsigned char *buffer,
                         size_t *buffer_len);

/**
 * Deserialize encrypted data from binary format
 *
 * Args:
 *   buffer: Input buffer
 *   buffer_len: Length of buffer
 *   encrypted: Output encrypted data structure
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int encryption_deserialize(const unsigned char *buffer,
                           size_t buffer_len,
                           encrypted_data_t *encrypted);

/**
 * Check if encryption is enabled
 */
bool encryption_is_enabled(void);

#endif /* HL7DB_ENCRYPTION_H */
