#ifndef _SECURITY_H
#define _SECURITY_H

// Used for hashing passwords
#define SALT_SIZE       16
#define ITERATION_COUNT 10000
#define HASH_SIZE       16

#define TOKEN_SIZE      16
#define SID_SIZE        25

/**
 * @brief Essentially wastes time so that timing attacks can't be used to crack our passwords
 */
bool safe_compare(const void *a, const void *b, size_t length);

/**
 * @brief Generates an array of TOKEN_SIZE random bytes
 */
uint8_t *generate_session_token(void);

/**
 * @brief Generates a string representation of a array of random bytes of TOKEN_SIZE
 */
char *generate_session_token_str(void);

/**
 * @brief Takes a array of TOKEN_SIZE bytes, converts it and returns a string copy
 */
char *session_token_to_str(uint8_t *session_token);

#endif
