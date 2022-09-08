#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "mbedtls/md.h"
#include "mbedtls/sha256.h"

#include "sigv4.h"

/* The buffer length used for doing SHA256 hash check. */
#define SHA256_DIGEST_LENGTH 32

/* The buffer length used for ASCII Hex encoded SHA256 result. */
#define HEX_ENCODED_SHA_256_STRING_SIZE 65

/* The string length of "date" format defined by AWS Signature V4. */
#define DATE_STRING_LEN 8

/* The signature end described by AWS Signature V4. */
#define AWS_SIG_V4_SIGNATURE_END "aws4_request"

#define SIGNED_HEADERS  "host;x-amz-date"

static int prvHexEncodedSha256(const unsigned char *pMsg, size_t uMsgLen, char pHexEncodedHash[HEX_ENCODED_SHA_256_STRING_SIZE])
{
    int res = SIGV4_ERRNO_NONE;
    unsigned char pHashBuf[SHA256_DIGEST_LENGTH] = {0};
    char *p = NULL;

    if (mbedtls_sha256_ret(pMsg, uMsgLen, pHashBuf, 0) != 0)
    {
        res = SIGV4_ERRNO_FAIL_TO_CALCULATE_SHA256;
    }
    else
    {
        p = pHexEncodedHash;
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
        {
            p += snprintf(p, 3, "%02x", pHashBuf[i]);
        }
    }

    return res;
}

static int prvGenScope(SigV4Para_t *pPara, char **ppScope, size_t *puScopeLen)
{
    int res = SIGV4_ERRNO_NONE;
    char *pScope = NULL;
    size_t uScopeLen = 0;

    /* Calculate needed length */
    uScopeLen = snprintf(NULL, 0, "%.*s/%s/%s/%s",
        DATE_STRING_LEN, pPara->pDateIso8601,
        pPara->pRegion,
        pPara->pService,
        AWS_SIG_V4_SIGNATURE_END
    );

    if ((pScope = (char *)malloc(uScopeLen + 1)) == NULL)
    {
        res = SIGV4_ERRNO_OUT_OF_MEMORY;
    }
    else
    {
        snprintf(pScope, uScopeLen + 1, "%.*s/%s/%s/%s",
            DATE_STRING_LEN, pPara->pDateIso8601,
            pPara->pRegion,
            pPara->pService,
            AWS_SIG_V4_SIGNATURE_END
        );
        *ppScope = pScope;
        *puScopeLen = uScopeLen;
    }

    return res;
}

static int prvGenCanonicalReqHexEncHash(SigV4Para_t *pPara, char pHexEncodedHash[HEX_ENCODED_SHA_256_STRING_SIZE])
{
    int res = SIGV4_ERRNO_NONE;
    char *pCanonicalReq = NULL;
    size_t uCanonicalReqLen = 0;
    char pPayloadHexEncodedHash[HEX_ENCODED_SHA_256_STRING_SIZE];
    const char *pPath = (pPara->pPath == NULL) ? "/" : pPara->pPath;
    const char *pQuery = (pPara->pQuery == NULL) ? "" : pPara->pQuery;

    if ((res = prvHexEncodedSha256(pPara->pPayload, pPara->uPayloadLen, pPayloadHexEncodedHash)) != SIGV4_ERRNO_NONE)
    {
        /* Propagate the error code. */
    }
    else
    {
        /* Calculate needed length */
        uCanonicalReqLen = snprintf(NULL, 0, "%s\n%s\n%s\nhost:%s\nx-amz-date:%s\n\n%s\n%s",
            pPara->pHttpMethod,
            pPath,
            pQuery,
            pPara->pHost,
            pPara->pDateIso8601,
            SIGNED_HEADERS,
            pPayloadHexEncodedHash
        );

        if ((pCanonicalReq = (char *)malloc(uCanonicalReqLen + 1)) == NULL)
        {
            res = SIGV4_ERRNO_OUT_OF_MEMORY;
        }
        else
        {
            snprintf(pCanonicalReq, uCanonicalReqLen + 1, "%s\n%s\n%s\nhost:%s\nx-amz-date:%s\n\n%s\n%s",
                pPara->pHttpMethod,
                pPath,
                pQuery,
                pPara->pHost,
                pPara->pDateIso8601,
                SIGNED_HEADERS,
                pPayloadHexEncodedHash
            );

            /* Calculate SHA256 hex of canonical request */
            prvHexEncodedSha256(pCanonicalReq, uCanonicalReqLen, pHexEncodedHash);
        }
    }

    if (pCanonicalReq != NULL)
    {
        free(pCanonicalReq);
    }

    return res;
}

static int prvGenSignature(SigV4Para_t *pPara, char *pScope, char pHexEncodedHash[HEX_ENCODED_SHA_256_STRING_SIZE])
{
    int res = SIGV4_ERRNO_NONE;
    char *pSignature = NULL;
    size_t uSignatureLen = 0;
    char pCanonicalReqHexEncodedSha256[HEX_ENCODED_SHA_256_STRING_SIZE];
    char pHmac[HEX_ENCODED_SHA_256_STRING_SIZE];
    const mbedtls_md_info_t *pxMdInfo = NULL;
    size_t uHmacSize = 0;

    if ((res = prvGenCanonicalReqHexEncHash(pPara, pCanonicalReqHexEncodedSha256)) != SIGV4_ERRNO_NONE)
    {
        /* Propagate the error code */
    }
    else
    {
        /* Calculate needed length */
        uSignatureLen = snprintf(NULL, 0, "AWS4-HMAC-SHA256\n%s\n%s\n%s",
            pPara->pDateIso8601,
            pScope,
            pCanonicalReqHexEncodedSha256
        );

        if ((pSignature = (char *)malloc(uSignatureLen + 1)) == NULL)
        {
            res = SIGV4_ERRNO_OUT_OF_MEMORY;
        }
        else
        {
            snprintf(pSignature, uSignatureLen + 1, "AWS4-HMAC-SHA256\n%s\n%s\n%s",
                pPara->pDateIso8601,
                pScope,
                pCanonicalReqHexEncodedSha256
            );

            pxMdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
            uHmacSize = mbedtls_md_get_size(pxMdInfo);
            snprintf(pHmac, HEX_ENCODED_SHA_256_STRING_SIZE, "AWS4%s", pPara->pSecretKey);
            mbedtls_md_hmac(pxMdInfo, (const unsigned char *)pHmac, strlen(pHmac), (const unsigned char *)pPara->pDateIso8601, DATE_STRING_LEN, (unsigned char *)pHmac);
            mbedtls_md_hmac(pxMdInfo, (const unsigned char *)pHmac, uHmacSize, (const unsigned char *)pPara->pRegion, strlen(pPara->pRegion), (unsigned char *)pHmac);
            mbedtls_md_hmac(pxMdInfo, (const unsigned char *)pHmac, uHmacSize, (const unsigned char *)pPara->pService, strlen(pPara->pService), (unsigned char *)pHmac);
            mbedtls_md_hmac(pxMdInfo, (const unsigned char *)pHmac, uHmacSize, (const unsigned char *)AWS_SIG_V4_SIGNATURE_END, sizeof(AWS_SIG_V4_SIGNATURE_END) - 1, (unsigned char *)pHmac);
            mbedtls_md_hmac(pxMdInfo, (const unsigned char *)pHmac, uHmacSize, (const unsigned char *)pSignature, uSignatureLen, (unsigned char *)pHmac);

            char *p = pHexEncodedHash;
            for (size_t i = 0; i<uHmacSize; i++)
            {
                p += sprintf(p, "%02x", pHmac[i] & 0xFF);
            }
        }
    }

    if (pSignature != NULL)
    {
        free(pSignature);
    }

    return res;
}

int SigV4_Sign(SigV4Para_t *pPara, char **ppAuth, size_t *puAuthLen)
{
    int res = SIGV4_ERRNO_NONE;
    char *pScope = NULL;
    size_t uScopeLen = 0;
    char pSigHexEncodedHash[HEX_ENCODED_SHA_256_STRING_SIZE];
    char *pAuth = NULL;
    size_t uAuthLen = 0;

    if ((res = prvGenScope(pPara, &pScope, &uScopeLen)) != SIGV4_ERRNO_NONE)
    {
        /* Propagate the error code */
    }
    else if ((res = prvGenSignature(pPara, pScope, pSigHexEncodedHash)) != SIGV4_ERRNO_NONE)
    {
        /* Propagate the error code */
    }
    else
    {
        /* Calculated needed length */
        uAuthLen = snprintf(NULL, 0, "AWS4-HMAC-SHA256 Credential=%s/%s, SignedHeaders=%s, Signature=%s",
            pPara->pAccessKey,
            pScope,
            SIGNED_HEADERS,
            pSigHexEncodedHash
        );

        if ((pAuth = (char *)malloc(uAuthLen + 1)) == NULL)
        {
            res = SIGV4_ERRNO_OUT_OF_MEMORY;
        }
        else
        {
            snprintf(pAuth, uAuthLen + 1, "AWS4-HMAC-SHA256 Credential=%s/%s, SignedHeaders=%s, Signature=%s",
                pPara->pAccessKey,
                pScope,
                SIGNED_HEADERS,
                pSigHexEncodedHash
            );
            *ppAuth = pAuth;
            *puAuthLen = uAuthLen;
        }
    }

    if (pScope != NULL)
    {
        free(pScope);
    }

    return res;
}