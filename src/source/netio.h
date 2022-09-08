#ifndef NETIO_H
#define NETIO_H

#include <stdbool.h>

#define NETIO_ERRNO_NONE                            (0)
#define NETIO_ERRNO_INVALID_PARAMETER               (-1)
#define NETIO_ERRNO_OUT_OF_MEMORY                   (-2)
#define NETIO_ERRNO_SEND_MORE_THAN_REMAINING_DATA   (-3)
#define NETIO_ERRNO_RECV_MORE_THAN_AVAILABLE_SPACE  (-4)
#define NETIO_ERRNO_UNABLE_TO_SET_SEND_TIMEOUT      (-5)
#define NETIO_ERRNO_UNABLE_TO_PARSE_CERT            (-5)
#define NETIO_ERRNO_NET_SOCKET_FAILED               (-6)
#define NETIO_ERRNO_NET_UNKNOWN_HOST                (-7)
#define NETIO_ERRNO_NET_CONNECT_FAILED              (-8)
#define NETIO_ERRNO_SSL_HANDSHAKE_ERROR             (-9)
#define NETIO_ERRNO_SSL_WRITE_ERROR                 (-10)
#define NETIO_ERRNO_SSL_READ_ERROR                  (-11)

typedef struct NetIo *NetIoHandle;

/**
 * @brief Create a network I/O handle
 *
 * @return The network I/O handle
 */
NetIoHandle NetIo_create(void);

/**
 * @brief Terminate a network I/O handle
 *
 * @param[in] xNetIoHandle The network I/O handle
 */
void NetIo_terminate(NetIoHandle xNetIoHandle);

/**
 * @brief Connect to a host with port
 *
 * @param[in] xNetIoHandle The network I/O handle
 * @param[in] pcHost The hostname
 * @param[in] pcPort The port
 * @return 0 on success, non-zero value otherwise
 */
int NetIo_connect(NetIoHandle xNetIoHandle, const char *pcHost, const char *pcPort);

/**
 * @brief Connect to a host with port and X509 certificates
 *
 * @param[in] xNetIoHandle The network I/O handle
 * @param[in] pcHost The hostname
 * @param[in] pcPort The port
 * @param[in] pcRootCA The X509 root CA
 * @param[in] pcCert The X509 client certificate
 * @param[in] pcPrivKey The x509 client private key
 * @return 0 on success, non-zero value otherwise
 */
int NetIo_connectWithX509(NetIoHandle xNetIoHandle, const char *pcHost, const char *pcPort, const char *pcRootCA, const char *pcCert, const char *pcPrivKey);

/**
 * @breif Disconnect from a host
 *
 * @param[in] xNetIoHandle The network I/O handle
 */
void NetIo_disconnect(NetIoHandle xNetIoHandle);

/**
 * @brief Send data
 *
 * @param[in] xNetIoHandle The network I/O handle
 * @param[in] pBuffer The data buffer
 * @param[in] uBytesToSend The length of data
 * @return 0 on success, non-zero value otherwise
 */
int NetIo_send(NetIoHandle xNetIoHandle, const unsigned char *pBuffer, size_t uBytesToSend);

/**
 * @brief Receive data
 *
 * @param[in] xNetIoHandle The network I/O handle
 * @param[in,out] pBuffer The data buffer
 * @param[in] uBufferSize The size of the data buffer
 * @param[out] puBytesReceived The actual bytes received
 * @return 0 on success, non-zero value otherwise
 */
int NetIo_recv(NetIoHandle xNetIoHandle, unsigned char *pBuffer, size_t uBufferSize, size_t *puBytesReceived);

/**
 * @brief Check if any data available
 *
 * @param xNetIoHandle The network I/O handle
 * @return true if data available, false otherwise
 */
bool NetIo_isDataAvailable(NetIoHandle xNetIoHandle);

/**
 * @brief Configure receive timeout.
 *
 * @param xNetIoHandle The network I/O handle
 * @param uRecvTimeoutMs Receive timeout in milliseconds
 * @return 0 on success, non-zero value otherwise
 */
int NetIo_setRecvTimeout(NetIoHandle xNetIoHandle, unsigned int uRecvTimeoutMs);

/**
 * @brief Configure send timeout.
 *
 * @param xNetIoHandle The network I/O handle
 * @param uSendTimeoutMs Send timeout in milliseconds
 * @return 0 on success, non-zero value otherwise
 */
int NetIo_setSendTimeout(NetIoHandle xNetIoHandle, unsigned int uSendTimeoutMs);

#endif /* NETIO_H */