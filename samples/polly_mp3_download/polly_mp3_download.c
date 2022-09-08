#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "polly/polly.h"

#define DEFAULT_AWS_ACCESS_KEY          "xxxxxxxxxxxxxxxxxxxx"
#define DEFAULT_AWS_SECRET_KEY          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define DEFAULT_AWS_REGION              "us-east-1"

#define AWS_ACCESS_KEY_ENV_VAR          "AWS_ACCESS_KEY_ID"
#define AWS_SECRET_KEY_ENV_VAR          "AWS_SECRET_ACCESS_KEY"
#define AWS_SESSION_TOKEN_ENV_VAR       "AWS_SESSION_TOKEN"
#define AWS_DEFAULT_REGION_ENV_VAR      "AWS_DEFAULT_REGION"

#define MAX_HOST_NAME_LEN   128
static char pPollyHostName[MAX_HOST_NAME_LEN] = {0};

static const char *prvOptGetAccessKey()
{
    const char *pAwsAccessKey = NULL;

    if ((pAwsAccessKey = getenv(AWS_ACCESS_KEY_ENV_VAR)) == NULL)
    {
        pAwsAccessKey = DEFAULT_AWS_ACCESS_KEY;
    }

    return pAwsAccessKey;
}

static const char *prvOptGetSecretKey()
{
    const char *pAwsSecretKey = NULL;

    if ((pAwsSecretKey = getenv(AWS_SECRET_KEY_ENV_VAR)) == NULL)
    {
        pAwsSecretKey = DEFAULT_AWS_SECRET_KEY;
    }

    return pAwsSecretKey;
}

static const char *prvOptGetRegion()
{
    const char *pAwsRegion = NULL;

    if ((pAwsRegion = getenv(AWS_DEFAULT_REGION_ENV_VAR)) == NULL)
    {
        pAwsRegion = DEFAULT_AWS_REGION;
    }

    return pAwsRegion;
}

static int prvPollySynthesizeSpeechCallback(uint8_t *pData, size_t uLen, void *pUserData)
{
    FILE *fp = (FILE *)pUserData;
    if (fp != NULL)
    {
        if (fwrite(pData, 1, uLen, fp) != uLen)
        {
            printf("%s(): Failed to write data to file\n", __FUNCTION__);
            fclose(fp);
            fp = NULL;
        }
    }
    return 0;
}

static int prvInitPollyServiceParameter(PollyServiceParameter_t *pServPara)
{
    memset(pServPara, 0, sizeof(PollyServiceParameter_t));
    pServPara->pAccessKey = prvOptGetAccessKey();
    pServPara->pSecretKey = prvOptGetSecretKey();
    pServPara->pToken = NULL;
    pServPara->pRegion = prvOptGetRegion();
    pServPara->pService = AWS_POLLY_SERVICE_NAME;
    snprintf(pPollyHostName, MAX_HOST_NAME_LEN, "%s.%s.amazonaws.com", AWS_POLLY_SERVICE_NAME, pServPara->pRegion);
    pServPara->pHost = pPollyHostName;
    pServPara->uSendTimeoutMs = 1000;
    pServPara->uRecvTimeoutMs = 1000;

    return 0;
}

static int prvInitPollySynthesizeSpeechParameter(PollySynthesizeSpeechParameter_t *pPara, const char *pText)
{
    memset(pPara, 0, sizeof(PollySynthesizeSpeechParameter_t));
    pPara->pEngine = NULL;
    pPara->pLanguageCode = NULL;
    pPara->pLexiconNames = NULL;
    pPara->pOutputFormat = "mp3";
    pPara->pSampleRate = NULL;
    pPara->pSpeechMarkTypes = NULL;
    pPara->pText = pText;
    pPara->pTextType = NULL;
    pPara->pVoiceId = "Amy";
    return 0;
}

int main(int argc, char *argv[])
{
    const char *pOutputFilename = NULL;
    const char *pText = NULL;
    PollyServiceParameter_t xServPara = { 0 };
    PollySynthesizeSpeechParameter_t xPara = { 0 };
    PollySynthesizeSpeechOutput_t xOut = { 0 };

    if (argc < 3)
    {
        printf("Usage: %s <OutputFilename.mp3> \"<Text>\"\n", argv[0]);
        return 0;
    }

    pOutputFilename = argv[1];
    pText = argv[2];
    printf("Output filename: %s\nText: %s\n", pOutputFilename, pText);

    prvInitPollyServiceParameter(&xServPara);
    prvInitPollySynthesizeSpeechParameter(&xPara, pText);

    FILE *fp = fopen(pOutputFilename, "wb");
    if (fp == NULL)
    {
        printf("Failed to open file %s\n", pOutputFilename);
        return 0;
    }

    xOut.callback = prvPollySynthesizeSpeechCallback;
    xOut.pUserData = fp;

    printf("Polly synthesize speech begin...\n");
    Polly_synthesizeSpeech(&xServPara, &xPara, &xOut);
    printf("Polly synthesize speech end\n");

    if (fp != NULL)
    {
        fclose(fp);
    }

    return 0;
}