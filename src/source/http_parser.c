#include <stdlib.h>
#include <string.h>

#include "llhttp.h"
#include "http_parser.h"

typedef enum
{
    LLHTTP_PAUSE_ON_UNKNOWN_REASON = 0,
    LLHTTP_PAUSE_ON_HEADERS_COMPLETE = 1,
    LLHTTP_PAUSE_ON_CHUNK_COMPLETE = 2,
} LlhttpPauseReason_t;

typedef struct
{
    llhttp_settings_t xSettings;

    LlhttpPauseReason_t ePauseReason;
    const char *pChuckLoc;
    size_t uChunkLen;
} llhttp_settings_ex_t;

typedef struct HttpParser
{
    llhttp_t xLlhttp;
    llhttp_settings_ex_t xSettingsEx;
} HttpParser_t;

static int prvOnHeadersCompleteCb(llhttp_t *pLlhttp)
{
    llhttp_settings_ex_t *pxSettingsEx = (llhttp_settings_ex_t *)(pLlhttp->settings);
    pxSettingsEx->ePauseReason = LLHTTP_PAUSE_ON_HEADERS_COMPLETE;
    return HPE_PAUSED;
}

static int prvOnChunkCompleteCb(llhttp_t *pLlhttp)
{
    llhttp_settings_ex_t *pxSettingsEx = (llhttp_settings_ex_t *)(pLlhttp->settings);
    pxSettingsEx->ePauseReason = LLHTTP_PAUSE_ON_CHUNK_COMPLETE;
    return HPE_PAUSED;
}

static int prvOnBodyCb(llhttp_t *pLlhttp, const char *at, size_t length)
{
    llhttp_settings_ex_t *pxSettingsEx = (llhttp_settings_ex_t *)(pLlhttp->settings);
    pxSettingsEx->pChuckLoc = at;
    pxSettingsEx->uChunkLen = length;
    return 0;
}

HttpParserHandle Hp_create()
{
    HttpParser_t *pHttpParser = NULL;
    llhttp_t *pLlhttp = NULL;
    llhttp_settings_t *pSettings = NULL;

    if ((pHttpParser = (HttpParser_t *)malloc(sizeof(HttpParser_t))) != NULL)
    {
        memset(pHttpParser, 0, sizeof(HttpParser_t));

        pSettings = &(pHttpParser->xSettingsEx.xSettings);
        pLlhttp = &(pHttpParser->xLlhttp);

        llhttp_settings_init(pSettings);
        pSettings->on_headers_complete = prvOnHeadersCompleteCb;
        pSettings->on_chunk_complete = prvOnChunkCompleteCb;
        pSettings->on_body = prvOnBodyCb;

        llhttp_init(pLlhttp, HTTP_RESPONSE, pSettings);
    }

    return pHttpParser;
}

int Hp_parse(HttpParserHandle xHttpParserandle, char *pBuf, size_t uLen, size_t *puByteParsed, unsigned int *puStatusCode, const char **ppChunkLoc, size_t *puChunkLen)
{
    int res = HTTP_PARSER_ERRNO_NONE;
    HttpParser_t *pHttpParser = (HttpParser_t *)xHttpParserandle;
    llhttp_settings_ex_t *pxSettingsEx = NULL;
    llhttp_t *pLlhttp = NULL;
    llhttp_t xLlhttpBak = {0};
    enum llhttp_errno xHttpErrno = HPE_OK;
    const char *pPauseLoc = NULL;
    size_t uBytesParsed = 0;
    unsigned int uStatusCode = 0;
    const char *pChunkLoc = NULL;
    size_t uChunkLen = 0;

    if (pHttpParser == NULL || pBuf == NULL || uLen == 0 || puByteParsed == NULL)
    {
        res = HTTP_PARSER_ERRNO_INVALID_PARAMETER;
    }
    else
    {
        pxSettingsEx = &(pHttpParser->xSettingsEx);
        pLlhttp = &(pHttpParser->xLlhttp);

        memcpy(&xLlhttpBak, pLlhttp, sizeof(llhttp_t));
        pHttpParser->xSettingsEx.ePauseReason = LLHTTP_PAUSE_ON_UNKNOWN_REASON;
        xHttpErrno = llhttp_execute(pLlhttp, pBuf, uLen);
        if (xHttpErrno == HPE_OK)
        {
            /* We haven't gotten a complete data block, so we rollback the status. */
            memcpy(pLlhttp, &xLlhttpBak, sizeof(llhttp_t));
            res = HTTP_PARSER_ERRNO_WANT_MORE_DATA;
        }
        else if (xHttpErrno == HPE_PAUSED)
        {
            pPauseLoc = llhttp_get_error_pos(pLlhttp);
            uBytesParsed = (pPauseLoc > pBuf) ? (pPauseLoc - pBuf) : 0;

            if (pHttpParser->xSettingsEx.ePauseReason == LLHTTP_PAUSE_ON_HEADERS_COMPLETE)
            {
                uStatusCode = pLlhttp->status_code;
            }
            else if (pHttpParser->xSettingsEx.ePauseReason == LLHTTP_PAUSE_ON_CHUNK_COMPLETE)
            {
                if (ppChunkLoc != NULL && puChunkLen != NULL)
                {
                    if (pxSettingsEx->pChuckLoc != NULL && pxSettingsEx->uChunkLen > 0)
                    {
                        pChunkLoc = pxSettingsEx->pChuckLoc;
                        uChunkLen = pxSettingsEx->uChunkLen;
                    }
                }
            }

            llhttp_resume(pLlhttp);
        }
        else
        {
            res = HTTP_PARSER_ERRNO_PARSE_FAILURE;
        }

        *puByteParsed = uBytesParsed;
        if (puStatusCode != NULL && *puStatusCode == 0)
        {
            *puStatusCode = uStatusCode;
        }
        if (ppChunkLoc != NULL)
        {
            *ppChunkLoc = pChunkLoc;
        }
        if (puChunkLen != NULL)
        {
            *puChunkLen = uChunkLen;
        }
    }

    return res;
}

void Hp_terminate(HttpParserHandle xHttpParserandle)
{
    HttpParser_t *pHttpParser = (HttpParser_t *)xHttpParserandle;

    if (pHttpParser != NULL)
    {
        free(pHttpParser);
    }
}