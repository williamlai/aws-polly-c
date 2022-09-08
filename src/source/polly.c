#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>

#include "polly/polly.h"

#include "http_parser.h"
#include "sigv4.h"
#include "netio.h"

#define DEFAULT_HTTP_RECV_BUFSIZE   2048

static int prvGenDateTimeiso8601(char pDateISO8601[DATE_TIME_ISO_8601_FORMAT_STRING_SIZE])
{
    int res = POLLY_ERRNO_NONE;
    time_t xTimeUtcNow = {0};

    xTimeUtcNow = time(NULL);
    strftime(pDateISO8601, DATE_TIME_ISO_8601_FORMAT_STRING_SIZE, "%Y%m%dT%H%M%SZ", gmtime(&xTimeUtcNow));

    return res;
}

static int prvGenSynthesizeSpeechHttpPayload(PollySynthesizeSpeechParameter_t *pPara, char **ppPayload, size_t *puPayloadLen)
{
    int res = POLLY_ERRNO_NONE;
    char *pPayload = NULL;
    size_t uPayloadLen = 0;

    uPayloadLen = snprintf(NULL, 0, "{\"OutputFormat\": \"%s\",\"VoiceId\": \"%s\", \"Text\": \"%s\"}",
        pPara->pOutputFormat,
        pPara->pVoiceId,
        pPara->pText
    );

    if ((pPayload = (char *)malloc(uPayloadLen + 1)) == NULL)
    {
        res = POLLY_ERRNO_OUT_OF_MEMORY;
    }
    else
    {
        snprintf(pPayload, uPayloadLen + 1, "{\"OutputFormat\": \"%s\",\"VoiceId\": \"%s\", \"Text\": \"%s\"}",
            pPara->pOutputFormat,
            pPara->pVoiceId,
            pPara->pText
        );
        *ppPayload = pPayload;
        *puPayloadLen = uPayloadLen;
    }

    return res;
}

static int prvSynthesizeSpeechRecv(NetIoHandle xNetIo, PollySynthesizeSpeechOutput_t *pOut)
{
    int res = POLLY_ERRNO_NONE;
    int resHttpParser = HTTP_PARSER_ERRNO_NONE;

    char *pRecvBuf = NULL;
    size_t uRecvBufSize = DEFAULT_HTTP_RECV_BUFSIZE;
    size_t uBytesReceived = 0;
    size_t uBytesTotalReceived = 0;
    HttpParserHandle xHttpParser = NULL;
    unsigned int uHttpStatusCode = 0;
    size_t uBytesParsed = 0;
    const char *pChunkLoc = NULL;
    size_t uChunkLen = 0;
    char *pTemp = NULL;

    if ((pRecvBuf = (char *)malloc(uRecvBufSize)) == NULL)
    {
        res = POLLY_ERRNO_OUT_OF_MEMORY;
    }
    else if ((xHttpParser = Hp_create()) == NULL)
    {
        res = POLLY_ERRNO_OUT_OF_MEMORY;
    }
    else
    {
        do
        {            
            if (uBytesTotalReceived == uRecvBufSize)
            {
                if ((pTemp = (char *)realloc(pRecvBuf, uRecvBufSize*2)) == NULL)
                {
                    res = POLLY_ERRNO_OUT_OF_MEMORY;
                    break;
                }
                else
                {
                    pRecvBuf = pTemp;
                    uRecvBufSize *= 2;
                }
            }

            if ((res = NetIo_recv(xNetIo, (unsigned char *)(pRecvBuf + uBytesTotalReceived), uRecvBufSize - uBytesTotalReceived, &uBytesReceived)) != NETIO_ERRNO_NONE)
            {
                res = POLLY_ERRNO_NET_RECV_FAILED;
                break;
            }
            else if (uBytesReceived == 0)
            {
                res = POLLY_ERRNO_NET_RECV_FAILED;
                break;
            }
            else
            {
                uBytesTotalReceived += uBytesReceived;
                resHttpParser = Hp_parse(xHttpParser, pRecvBuf, uBytesTotalReceived, &uBytesParsed, &uHttpStatusCode, &pChunkLoc, &uChunkLen);
                if (resHttpParser == HTTP_PARSER_ERRNO_NONE || resHttpParser == HTTP_PARSER_ERRNO_WANT_MORE_DATA)
                {
                    if (pOut->callback != NULL && pChunkLoc != NULL && uChunkLen > 0)
                    {
                        pOut->callback((uint8_t *)pChunkLoc, uChunkLen, pOut->pUserData);
                    }

                    /* Move the parsed data forward */
                    uBytesTotalReceived -= uBytesParsed;
                    memmove(pRecvBuf, pRecvBuf + uBytesParsed, uBytesTotalReceived);
                    res = POLLY_ERRNO_HTTP_WANT_MORE;
                }
                else
                {
                    res = POLLY_ERRNO_HTTP_PARSE_FAILURE;
                    break;
                }
            }
        } while (res != POLLY_ERRNO_NONE);
    }

    Hp_terminate(xHttpParser);
    if (pRecvBuf != NULL)
    {
        free(pRecvBuf);
    }

    return res;
}

int Polly_synthesizeSpeech(PollyServiceParameter_t *pServPara, PollySynthesizeSpeechParameter_t *pPara, PollySynthesizeSpeechOutput_t *pOut)
{
    int res = POLLY_ERRNO_NONE;
    char *pPayload = NULL;
    size_t uPayloadLen = 0;
    SigV4Para_t xSigV4Para = { 0 };
    char *pAuth = NULL;
    size_t uAuthLen = 0;
    
    char pDateISO8601[DATE_TIME_ISO_8601_FORMAT_STRING_SIZE];
    char *pHttpReq = NULL;
    size_t uHttpReqLen = 0;
    char *p = NULL;

    NetIoHandle xNetIo = NULL;

    if ((res = prvGenDateTimeiso8601(pDateISO8601)) != POLLY_ERRNO_NONE)
    {
        /* Propagate the error code */
    }
    else if ((res = prvGenSynthesizeSpeechHttpPayload(pPara, &pPayload, &uPayloadLen)) != POLLY_ERRNO_NONE)
    {
        /* Propagate the error code */
    }
    else
    {
        xSigV4Para.pAccessKey = pServPara->pAccessKey;
        xSigV4Para.pAccessKey = pServPara->pAccessKey;
        xSigV4Para.pSecretKey = pServPara->pSecretKey;
        xSigV4Para.pRegion = pServPara->pRegion;
        xSigV4Para.pService = pServPara->pService;
        xSigV4Para.pDateIso8601 = pDateISO8601;
        xSigV4Para.pHttpMethod = "POST";
        xSigV4Para.pPath = "/v1/speech";
        xSigV4Para.pQuery = NULL;
        xSigV4Para.pHost = pServPara->pHost;
        xSigV4Para.pPayload = pPayload;
        xSigV4Para.uPayloadLen = uPayloadLen;

        if ((res = SigV4_Sign(&xSigV4Para, &pAuth, &uAuthLen)) != SIGV4_ERRNO_NONE)
        {
            res = POLLY_ERRNO_SIGN_FAILURE;
        }
        else
        {
            /* Calculate needed length */
            uHttpReqLen = 0;
            uHttpReqLen += snprintf(NULL, 0, "POST /v1/speech HTTP/1.1\r\n");
            uHttpReqLen += snprintf(NULL, 0, "host: %s\r\n", pServPara->pHost);
            uHttpReqLen += snprintf(NULL, 0, "content-type: application/json\r\n");
            uHttpReqLen += snprintf(NULL, 0, "content-length: %zu\r\n", uPayloadLen);
            uHttpReqLen += snprintf(NULL, 0, "authorization: %s\r\n", pAuth);
            uHttpReqLen += snprintf(NULL, 0, "x-amz-Date: %s\r\n", pDateISO8601);
            uHttpReqLen += snprintf(NULL, 0, "\r\n");
            uHttpReqLen += snprintf(NULL, 0, "%.*s", (int)uPayloadLen, pPayload);

            if ((pHttpReq = (char *)malloc(uHttpReqLen + 1)) == NULL)
            {
                res = POLLY_ERRNO_OUT_OF_MEMORY;
            }
            else
            {
                p = pHttpReq;
                p += sprintf(p, "POST /v1/speech HTTP/1.1\r\n");
                p += sprintf(p, "host: %s\r\n", pServPara->pHost);
                p += sprintf(p, "content-type: application/json\r\n");
                p += sprintf(p, "content-length: %zu\r\n", uPayloadLen);
                p += sprintf(p, "authorization: %s\r\n", pAuth);
                p += sprintf(p, "x-amz-Date: %s\r\n", pDateISO8601);
                p += sprintf(p, "\r\n");
                p += sprintf(p, "%.*s", (int)uPayloadLen, pPayload);

                /* Free resource early */
                free(pPayload);
                pPayload = NULL;
                free(pAuth);
                pAuth = NULL;

                if ((xNetIo = NetIo_create()) == NULL)
                {
                    res = POLLY_ERRNO_OUT_OF_MEMORY;
                }
                else if (NetIo_connect(xNetIo, pServPara->pHost, "443") != NETIO_ERRNO_NONE)
                {
                    res = POLLY_ERRNO_NET_CONNECT_FAILED;
                }
                else if (NetIo_setSendTimeout(xNetIo, pServPara->uSendTimeoutMs) != NETIO_ERRNO_NONE ||
                        NetIo_setRecvTimeout(xNetIo, pServPara->uRecvTimeoutMs) != NETIO_ERRNO_NONE)
                {
                    res = POLLY_ERRNO_NET_CONFIG_FAILED;
                }
                else if (NetIo_send(xNetIo, pHttpReq, uHttpReqLen) != NETIO_ERRNO_NONE)
                {
                    res = POLLY_ERRNO_NET_SEND_FAILED;
                }
                else
                {
                    /* Free resource early */
                    free(pHttpReq);
                    pHttpReq = NULL;

                    res = prvSynthesizeSpeechRecv(xNetIo, pOut);
                }
            }
        }
    }

    NetIo_terminate(xNetIo);
    if (pHttpReq != NULL)
    {
        free(pHttpReq);
    }
    if (pAuth != NULL)
    {
        free(pAuth);
    }
    if (pPayload != NULL)
    {
        free(pPayload);
    }

    return res;
}