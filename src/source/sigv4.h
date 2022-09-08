#ifndef SIGV4_H
#define SIGV4_H

#include <stddef.h>

#define SIGV4_ERRNO_NONE                        (0)
#define SIGV4_ERRNO_INVALID_PARAMETER           (-1)
#define SIGV4_ERRNO_OUT_OF_MEMORY               (-2)
#define SIGV4_ERRNO_FAIL_TO_CALCULATE_SHA256    (-3)

/* The string length of "date + time" format of ISO 8601 required by AWS Signature V4. */
#define DATE_TIME_ISO_8601_FORMAT_STRING_SIZE           ( 17 )

typedef struct
{
    const char *pAccessKey;
    const char *pSecretKey;

    const char *pRegion;
    const char *pService;
    const char *pDateIso8601;

    const char *pHttpMethod;
    const char *pPath;
    const char *pQuery;
    const char *pHost;
    const char *pPayload;
    size_t uPayloadLen;
} SigV4Para_t;

int SigV4_Sign(SigV4Para_t *pPara, char **ppAuth, size_t *puAuthLen);

#endif /* SIGV4_H */