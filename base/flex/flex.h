typedef struct flex_token_result {
    int32_t * result;
    int32_t result_len;
} flex_token_result;

flex_token_result flex_lexing(const char *input, int32_t len, const char *tokenizer_type);

#define YY_FATAL_ERROR(msg) throw msg
