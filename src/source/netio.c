/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* Third party headers */
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/net.h"
#include "mbedtls/net_sockets.h"

#include "netio.h"

#define DEFAULT_CONNECTION_TIMEOUT_MS       (10 * 1000)

typedef struct NetIo
{
    /* Basic ssl connection parameters */
    mbedtls_net_context xFd;
    mbedtls_ssl_context xSsl;
    mbedtls_ssl_config xConf;
    mbedtls_ctr_drbg_context xCtrDrbg;
    mbedtls_entropy_context xEntropy;

    /* Variables for IoT credential provider. It's optional feature so we declare them as pointers. */
    mbedtls_x509_crt *pRootCA;
    mbedtls_x509_crt *pCert;
    mbedtls_pk_context *pPrivKey;

    /* Options */
    uint32_t uRecvTimeoutMs;
} NetIo_t;

static int prvCreateX509Cert(NetIo_t *pxNet)
{
    int res = NETIO_ERRNO_NONE;

    if (pxNet == NULL)
    {
        res = NETIO_ERRNO_INVALID_PARAMETER;
    }
    else if ((pxNet->pRootCA = (mbedtls_x509_crt *)malloc(sizeof(mbedtls_x509_crt))) == NULL ||
        (pxNet->pCert = (mbedtls_x509_crt *)malloc(sizeof(mbedtls_x509_crt))) == NULL ||
        (pxNet->pPrivKey = (mbedtls_pk_context *)malloc(sizeof(mbedtls_pk_context))) == NULL)
    {
        res = NETIO_ERRNO_OUT_OF_MEMORY;
    }
    else
    {
        mbedtls_x509_crt_init(pxNet->pRootCA);
        mbedtls_x509_crt_init(pxNet->pCert);
        mbedtls_pk_init(pxNet->pPrivKey);
    }

    return res;
}

static int prvInitConfig(NetIo_t *pxNet, const char *pcRootCA, const char *pcCert, const char *pcPrivKey)
{
    int res = NETIO_ERRNO_NONE;
    int retVal = 0;

    if (pxNet == NULL)
    {
        res = NETIO_ERRNO_INVALID_PARAMETER;
    }
    else
    {
        mbedtls_ssl_set_bio(&(pxNet->xSsl), &(pxNet->xFd), mbedtls_net_send, NULL, mbedtls_net_recv_timeout);

        if ((retVal = mbedtls_ssl_config_defaults(&(pxNet->xConf), MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
        {
            res = NETIO_ERRNO_OUT_OF_MEMORY;
        }
        else
        {
            mbedtls_ssl_conf_rng(&(pxNet->xConf), mbedtls_ctr_drbg_random, &(pxNet->xCtrDrbg));
            mbedtls_ssl_conf_read_timeout(&(pxNet->xConf), pxNet->uRecvTimeoutMs);

            if (pcRootCA != NULL && pcCert != NULL && pcPrivKey != NULL)
            {
                if ((retVal = mbedtls_x509_crt_parse(pxNet->pRootCA, (void *)pcRootCA, strlen(pcRootCA) + 1)) != 0 ||
                    (retVal = mbedtls_x509_crt_parse(pxNet->pCert, (void *)pcCert, strlen(pcCert) + 1)) != 0 ||
                    (retVal = mbedtls_pk_parse_key(pxNet->pPrivKey, (void *)pcPrivKey, strlen(pcPrivKey) + 1, NULL, 0)) != 0)
                {
                    res = NETIO_ERRNO_UNABLE_TO_PARSE_CERT;
                }
                else
                {
                    mbedtls_ssl_conf_authmode(&(pxNet->xConf), MBEDTLS_SSL_VERIFY_REQUIRED);
                    mbedtls_ssl_conf_ca_chain(&(pxNet->xConf), pxNet->pRootCA, NULL);

                    if ((retVal = mbedtls_ssl_conf_own_cert(&(pxNet->xConf), pxNet->pCert, pxNet->pPrivKey)) != 0)
                    {
                        res = NETIO_ERRNO_OUT_OF_MEMORY;
                    }
                }
            }
            else
            {
                mbedtls_ssl_conf_authmode(&(pxNet->xConf), MBEDTLS_SSL_VERIFY_NONE);
            }
        }
    }

    if (res == NETIO_ERRNO_NONE)
    {
        if ((retVal = mbedtls_ssl_setup(&(pxNet->xSsl), &(pxNet->xConf))) != 0)
        {
            res = NETIO_ERRNO_OUT_OF_MEMORY;
        }
    }

    return res;
}

static int prvConnect(NetIo_t *pxNet, const char *pcHost, const char *pcPort, const char *pcRootCA, const char *pcCert, const char *pcPrivKey)
{
    int res = NETIO_ERRNO_NONE;
    int retVal = 0;

    if (pxNet == NULL || pcHost == NULL || pcPort == NULL)
    {
        res = NETIO_ERRNO_INVALID_PARAMETER;
    }
    else if ((pcRootCA != NULL && pcCert != NULL && pcPrivKey != NULL) && (res = prvCreateX509Cert(pxNet)) != NETIO_ERRNO_NONE)
    {
        /* Propagate the res error */
    }
    else if ((retVal = mbedtls_net_connect(&(pxNet->xFd), pcHost, pcPort, MBEDTLS_NET_PROTO_TCP)) != 0)
    {
        switch (retVal)
        {
            case MBEDTLS_ERR_NET_SOCKET_FAILED:
                res = NETIO_ERRNO_NET_SOCKET_FAILED;
                break;
            case MBEDTLS_ERR_NET_UNKNOWN_HOST:
                res = NETIO_ERRNO_NET_UNKNOWN_HOST;
                break;
            case MBEDTLS_ERR_NET_CONNECT_FAILED:
            default:
                res = NETIO_ERRNO_NET_CONNECT_FAILED;
                break;
        }
    }
    else if ((res = prvInitConfig(pxNet, pcRootCA, pcCert, pcPrivKey)) != NETIO_ERRNO_NONE)
    {
        /* Propagate the res error */
    }
    else if ((retVal = mbedtls_ssl_handshake(&(pxNet->xSsl))) != 0)
    {
        res = NETIO_ERRNO_SSL_HANDSHAKE_ERROR;
    }
    else
    {
        /* nop */
    }

    return res;
}

NetIoHandle NetIo_create(void)
{
    NetIo_t *pxNet = NULL;

    if ((pxNet = (NetIo_t *)malloc(sizeof(NetIo_t))) != NULL)
    {
        memset(pxNet, 0, sizeof(NetIo_t));

        mbedtls_net_init(&(pxNet->xFd));
        mbedtls_ssl_init(&(pxNet->xSsl));
        mbedtls_ssl_config_init(&(pxNet->xConf));
        mbedtls_ctr_drbg_init(&(pxNet->xCtrDrbg));
        mbedtls_entropy_init(&(pxNet->xEntropy));

        pxNet->uRecvTimeoutMs = DEFAULT_CONNECTION_TIMEOUT_MS;

        if (mbedtls_ctr_drbg_seed(&(pxNet->xCtrDrbg), mbedtls_entropy_func, &(pxNet->xEntropy), NULL, 0) != 0)
        {
            NetIo_terminate(pxNet);
            pxNet = NULL;
        }
    }

    return pxNet;
}

void NetIo_terminate(NetIoHandle xNetIoHandle)
{
    NetIo_t *pxNet = (NetIo_t *)xNetIoHandle;

    if (pxNet != NULL)
    {
        mbedtls_ctr_drbg_free(&(pxNet->xCtrDrbg));
        mbedtls_entropy_free(&(pxNet->xEntropy));
        mbedtls_net_free(&(pxNet->xFd));
        mbedtls_ssl_free(&(pxNet->xSsl));
        mbedtls_ssl_config_free(&(pxNet->xConf));

        if (pxNet->pRootCA != NULL)
        {
            mbedtls_x509_crt_free(pxNet->pRootCA);
            free(pxNet->pRootCA);
            pxNet->pRootCA = NULL;
        }

        if (pxNet->pCert != NULL)
        {
            mbedtls_x509_crt_free(pxNet->pCert);
            free(pxNet->pCert);
            pxNet->pCert = NULL;
        }

        if (pxNet->pPrivKey != NULL)
        {
            mbedtls_pk_free(pxNet->pPrivKey);
            free(pxNet->pPrivKey);
            pxNet->pPrivKey = NULL;
        }
        free(pxNet);
    }
}

int NetIo_connect(NetIoHandle xNetIoHandle, const char *pcHost, const char *pcPort)
{
    return prvConnect(xNetIoHandle, pcHost, pcPort, NULL, NULL, NULL);
}

int NetIo_connectWithX509(NetIoHandle xNetIoHandle, const char *pcHost, const char *pcPort, const char *pcRootCA, const char *pcCert, const char *pcPrivKey)
{
    return prvConnect(xNetIoHandle, pcHost, pcPort, pcRootCA, pcCert, pcPrivKey);
}

void NetIo_disconnect(NetIoHandle xNetIoHandle)
{
    NetIo_t *pxNet = (NetIo_t *)xNetIoHandle;

    if (pxNet != NULL)
    {
        mbedtls_ssl_close_notify(&(pxNet->xSsl));
    }
}

int NetIo_send(NetIoHandle xNetIoHandle, const unsigned char *pBuffer, size_t uBytesToSend)
{
    int n = 0;
    int res = NETIO_ERRNO_NONE;
    NetIo_t *pxNet = (NetIo_t *)xNetIoHandle;
    size_t uBytesRemaining = uBytesToSend;
    char *pIndex = (char *)pBuffer;

    if (pxNet == NULL || pBuffer == NULL)
    {
        res = NETIO_ERRNO_INVALID_PARAMETER;
    }
    else
    {
        do
        {
            n = mbedtls_ssl_write(&(pxNet->xSsl), (const unsigned char *)pIndex, uBytesRemaining);
            if (n < 0)
            {
                res = NETIO_ERRNO_SSL_WRITE_ERROR;
                break;
            }
            else if (n > uBytesRemaining)
            {
                res = NETIO_ERRNO_SEND_MORE_THAN_REMAINING_DATA;
                break;
            }
            uBytesRemaining -= n;
            pIndex += n;
        } while (uBytesRemaining > 0);
    }

    return res;
}

int NetIo_recv(NetIoHandle xNetIoHandle, unsigned char *pBuffer, size_t uBufferSize, size_t *puBytesReceived)
{
    int n;
    int res = NETIO_ERRNO_NONE;
    NetIo_t *pxNet = (NetIo_t *)xNetIoHandle;

    if (pxNet == NULL || pBuffer == NULL || puBytesReceived == NULL)
    {
        res = NETIO_ERRNO_INVALID_PARAMETER;
    }
    else
    {
        n = mbedtls_ssl_read(&(pxNet->xSsl), pBuffer, uBufferSize);
        if (n < 0)
        {
            res = NETIO_ERRNO_SSL_READ_ERROR;
        }
        else if (n > uBufferSize)
        {
            res = NETIO_ERRNO_RECV_MORE_THAN_AVAILABLE_SPACE;
        }
        else
        {
            *puBytesReceived = n;
        }
    }

    return res;
}

int NetIo_setRecvTimeout(NetIoHandle xNetIoHandle, unsigned int uRecvTimeoutMs)
{
    int res = NETIO_ERRNO_NONE;
    NetIo_t *pxNet = (NetIo_t *)xNetIoHandle;

    if (pxNet == NULL)
    {
        res = NETIO_ERRNO_INVALID_PARAMETER;
    }
    else
    {
        pxNet->uRecvTimeoutMs = (uint32_t)uRecvTimeoutMs;
        mbedtls_ssl_conf_read_timeout(&(pxNet->xConf), pxNet->uRecvTimeoutMs);
    }

    return res;
}