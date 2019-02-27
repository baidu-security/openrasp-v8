#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct flex_sql_token_result
    {
        int *result;
        int result_len;
    } flex_sql_token_result;

    flex_sql_token_result flex_sql_lexing(const char *input, int len);

#ifdef __cplusplus
}
#endif