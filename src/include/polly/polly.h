#ifndef POLLY_H
#define POLLY_H

#include <stddef.h>
#include <inttypes.h>

#define POLLY_ERRNO_NONE                            (0)
#define POLLY_ERRNO_INVALID_PARAMETER               (-1)
#define POLLY_ERRNO_OUT_OF_MEMORY                   (-2)
#define POLLY_ERRNO_SIGN_FAILURE                    (-3)
#define POLLY_ERRNO_NET_CONNECT_FAILED              (-4)
#define POLLY_ERRNO_NET_CONFIG_FAILED               (-5)
#define POLLY_ERRNO_NET_SEND_FAILED                 (-6)
#define POLLY_ERRNO_NET_RECV_FAILED                 (-7)
#define POLLY_ERRNO_HTTP_100_CONTINUE_EXPECT_MORE   (-8)
#define POLLY_ERRNO_HTTP_WANT_MORE                  (-9)
#define POLLY_ERRNO_HTTP_PARSE_FAILURE              (-10)
#define POLLY_ERRNO_HTTP_REQ_FAILURE                (-11)

#define AWS_POLLY_SERVICE_NAME                      "polly"

typedef struct
{
    const char *pAccessKey;
    const char *pSecretKey;
    const char *pToken;

    const char *pRegion;
    const char *pService;
    const char *pHost;

    unsigned int uRecvTimeoutMs;
} PollyServiceParameter_t;

typedef struct
{
    const char *pEngine;
    const char *pLanguageCode;
    const char *pLexiconNames;
    const char *pOutputFormat; // Required, json | mp3 | ogg_vorbis | pcm
    const char *pSampleRate;
    const char *pSpeechMarkTypes;
    const char *pText; // Required
    const char *pTextType;
    const char *pVoiceId; // Required
} PollySynthesizeSpeechParameter_t;

typedef struct
{
    int (*onDataCallback)(uint8_t *pData, size_t uLen, void *pUserData);
    void *pUserData;
    unsigned int uStatusCode;
} PollySynthesizeSpeechOutput_t;

int Polly_synthesizeSpeech(PollyServiceParameter_t *pServPara, PollySynthesizeSpeechParameter_t *pPara, PollySynthesizeSpeechOutput_t *pOut);

#endif /* POLLY_H */