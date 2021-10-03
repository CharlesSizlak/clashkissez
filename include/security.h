#ifndef _SECURITY_H
#define _SECURITY_H

// Used for hashing passwords
#define SALT_SIZE       16
#define ITERATION_COUNT 10000
#define HASH_SIZE       16

#define TOKEN_SIZE      16
#define SID_SIZE        25

bool safe_compare(const void *a, const void *b, size_t length);
uint8_t *generate_session_token(void);
char *generate_session_token_str(void);
char *session_token_to_str(uint8_t *session_token);

#endif
