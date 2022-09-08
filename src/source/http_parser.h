#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define HTTP_PARSER_ERRNO_NONE                      (0)
#define HTTP_PARSER_ERRNO_INVALID_PARAMETER         (-1)
#define HTTP_PARSER_ERRNO_WANT_MORE_DATA            (-2)
#define HTTP_PARSER_ERRNO_PARSE_FAILURE             (-3)

typedef struct HttpParser *HttpParserHandle;

HttpParserHandle Hp_create();

int Hp_parse(HttpParserHandle xHttpParserandle, char *pBuf, size_t uLen, size_t *puByteParsed, unsigned int *puStatusCode, const char **ppChunkLoc, size_t *puChunkLen);

void Hp_terminate(HttpParserHandle xHttpParserandle);

#endif /* HTTP_PARSER_H */