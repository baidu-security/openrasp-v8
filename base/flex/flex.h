#include <inttypes.h>
typedef struct flex_token_result {
    uint32_t * result;
    uint32_t result_len;
} flex_token_result;

flex_token_result flex_lexing(const char *input, uint32_t len, const char *tokenizer_type);

#define YY_FATAL_ERROR(msg) throw msg
