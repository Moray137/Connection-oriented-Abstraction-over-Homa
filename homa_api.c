// SPDX-License-Identifier: BSD-2-Clause

/* This file contains functions that implement the Homa API visible to
 * applications. It is intended to be part of the user-level run-time library.
 */

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#ifndef NDEBUG
#include <stdlib.h>
#endif
#include <sys/ioctl.h>
#include <sys/types.h>

#include "homa.h"

/**
 * homa_reply() - Send a response message for an RPC previously received
 * with a call to recvmsg.
 * @sockfd:       File descriptor for the socket on which to send the message.
 * @message_buf:  First byte of buffer containing the response message.
 * @length:       Number of bytes in the message at @message_buf.
 * @dest_addr:    Address of the RPC's client (returned by recvmsg when
 *                the message was received).
 * @addrlen:      # bytes at *dest_addr.
 * @id:           Unique identifier for the request, as returned by recvmsg
 *                when the request was received.
 *
 * @dest_addr and @id must correspond to a previously-received request
 * for which no reply has yet been sent; if there is no such active request,
 * then this function does nothing.
 *
 * Return:      0 means the response has been accepted for delivery. If an
 *              error occurred, -1 is returned and errno is set appropriately.
 */
ssize_t homa_reply(int sockfd, const void *message_buf, size_t length,
		   const struct sockaddr *dest_addr, uint32_t addrlen,
		   uint64_t id)
{
	struct homa_sendmsg_args args;
	struct msghdr hdr;
	struct iovec vec;
	int result;

	args.id = id;
	args.completion_cookie = 0;

	vec.iov_base = (void *)message_buf;
	vec.iov_len = length;

	hdr.msg_name = (void *)dest_addr;
	hdr.msg_namelen = addrlen;
	hdr.msg_iov = &vec;
	hdr.msg_iovlen = 1;
	hdr.msg_control = &args;
	hdr.msg_controllen = 0;
	result = sendmsg(sockfd, &hdr, 0);
	return result;
}

/**
 * homa_replyv() - Similar to homa_reply, except the response
 * message can be divided among several chunks of memory.
 * @sockfd:     File descriptor for the socket on which to send the message.
 * @iov:        Pointer to array that describes the chunks of the response
 *              message.
 * @iovcnt:     Number of elements in @iov.
 * @dest_addr:  Address of the RPC's client (returned by recvmsg when
 *              the message was received).
 * @addrlen:    # bytes at *dest_addr.
 * @id:         Unique identifier for the request, as returned by recvmsg
 *              when the request was received.
 *
 * @dest_addr and @id must correspond to a previously-received request
 * for which no reply has yet been sent; if there is no such active request,
 * then this function does nothing.
 *
 * Return:      0 means the response has been accepted for delivery. If an
 *              error occurred, -1 is returned and errno is set appropriately.
 */
ssize_t homa_replyv(int sockfd, const struct iovec *iov, int iovcnt,
		    const struct sockaddr *dest_addr, uint32_t addrlen,
		    uint64_t id)
{
	struct homa_sendmsg_args args;
	struct msghdr hdr;
	int result;

	args.id = id;
	args.completion_cookie = 0;

	hdr.msg_name = (void *)dest_addr;
	hdr.msg_namelen = addrlen;
	hdr.msg_iov = (struct iovec *)iov;
	hdr.msg_iovlen = iovcnt;
	hdr.msg_control = &args;
	hdr.msg_controllen = 0;
	result = sendmsg(sockfd, &hdr, 0);
	return result;
}

/**
 * homa_send() - Send a request message to initiate an RPC.
 * @sockfd:            File descriptor for the socket on which to send the
 *                     message.
 * @message_buf:       First byte of buffer containing the request message.
 * @length:            Number of bytes at @message_buf.
 * @dest_addr:         Address of server to which the request should be sent.
 * @addrlen:           # bytes at *dest_addr.
 * @id:                A unique identifier for the request will be returned
 *                     here; this can be used later to find the response for
 *                     this request.
 * @completion_cookie: Value to be returned by recvmsg when RPC completes.
 *
 * Return:      0 means the request has been accepted for delivery. If an
 *              error occurred, -1 is returned and errno is set appropriately.
 */
int homa_send(int sockfd, const void *message_buf, size_t length,
	      const struct sockaddr *dest_addr, uint32_t addrlen,
	      uint64_t *id, uint64_t completion_cookie)
{
	struct homa_sendmsg_args args;
	struct msghdr hdr;
	struct iovec vec;
	int result;

	args.id = 0;
	args.completion_cookie = completion_cookie;

	vec.iov_base = (void *)message_buf;
	vec.iov_len = length;

	hdr.msg_name = (void *)dest_addr;
	hdr.msg_namelen = addrlen;
	hdr.msg_iov = &vec;
	hdr.msg_iovlen = 1;
	hdr.msg_control = &args;
	hdr.msg_controllen = 0;
	result = sendmsg(sockfd, &hdr, 0);
	if (result >= 0 && id)
		*id = args.id;
	return result;
}

/**
 * homa_sendv() - Same as homa_send, except that the request message can
 * be divided among multiple disjoint chunks of memory.
 * @sockfd:            File descriptor for the socket on which to send the
 *                     message.
 * @iov:               Pointer to array that describes the chunks of the request
 *                     message.
 * @iovcnt:            Number of elements in @iov.
 * @dest_addr:         Address of server to which the request should be sent.
 * @addrlen:           # bytes at *dest_addr.
 * @id:                A unique identifier for the request will be returned
 *                     here; this can be used later to find the response for
 *                     this request.
 * @completion_cookie: Value to be returned by recvmsg when RPC completes.
 *
 * Return:      0 means the request has been accepted for delivery. If an
 *              error occurred, -1 is returned and errno is set appropriately.
 */
int homa_sendv(int sockfd, const struct iovec *iov, int iovcnt,
	       const struct sockaddr *dest_addr, uint32_t addrlen,
	       uint64_t *id, uint64_t completion_cookie)
{
	struct homa_sendmsg_args args;
	struct msghdr hdr;
	int result;

	args.id = 0;
	args.completion_cookie = completion_cookie;

	hdr.msg_name = (void *)dest_addr;
	hdr.msg_namelen = addrlen;
	hdr.msg_iov = (struct iovec *)iov;
	hdr.msg_iovlen = iovcnt;
	hdr.msg_control = &args;
	hdr.msg_controllen = 0;
	result = sendmsg(sockfd, &hdr, 0);
	if (result >= 0 && id)
		*id = args.id;
	return result;
}

/**
 * homa_abort() - Terminate the execution of an RPC.
 * @sockfd:     File descriptor for the socket associated with the RPC.
 * @id:         Unique identifier for a client RPC to abort (return value
 *              from previous call to homa_send). 0 means abort all client
 *              RPCs on this socket.
 * @error:      0 means that the aborted RPCs should be destroyed
 *              immediately (they will never be returned by recvmsg).
 *              Nonzero means that the RPCs should be moved to the
 *              completed state; recvmsg will return an error for these
 *              RPCs, with @error as the errno value.
 *
 * Return:      If an error occurred, -1 is returned and errno is set
 *              appropriately. Otherwise zero is returned.
 */
int homa_abort(int sockfd, uint64_t id, int error)
{
	struct homa_abort_args args = {id, error};

	return ioctl(sockfd, HOMAIOCABORT, &args);
}

/**
 * homa_reply_connected() - Send a response message from a connected homa socket
 * for an RPC previously received with a call to recvmsg.
 * @sockfd:       File descriptor for the socket on which to send the message.
 * @message_buf:  First byte of buffer containing the response message.
 * @length:       Number of bytes in the message at @message_buf.
 * @id:           Unique identifier for the request, as returned by recvmsg
 *                when the request was received.
 *
 * @dest_addr and @id are not needed, as this method only operates on peeledoff()
 * homa sockets
 *
 * Return:      0 means the response has been accepted for delivery. If an
 *              error occurred, -1 is returned and errno is set appropriately.
 */
ssize_t homa_reply_connected(int sockfd, const void *message_buf, size_t length,
		   uint64_t id)
{
	struct homa_sendmsg_args args;
	struct msghdr hdr;
	struct iovec vec;
	int result;

	args.id = id;
	args.completion_cookie = 0;

	vec.iov_base = (void *)message_buf;
	vec.iov_len = length;

	hdr.msg_name = NULL;
	hdr.msg_namelen = 0;
	hdr.msg_iov = &vec;
	hdr.msg_iovlen = 1;
	hdr.msg_control = &args;
	hdr.msg_controllen = 0;
	result = sendmsg(sockfd, &hdr, 0);
	return result;
}

/**
 * connect()ed-UDP-style Homa send, to make Homa compatible with UDP-style coding
 * uint64_t *id and int completion_cookie is deleted for now
 * const struct sockaddr *dest_addr and size_t addrlen are also deleted,
 * but they will be handled by sendmsg() in kernel
 * flags are not used in Homa
 */
int homa_send_connected(int sockfd, const void *message_buf, size_t length, int flags)
{
	struct homa_sendmsg_args args;
	struct msghdr hdr;
	struct iovec vec;
	int result;

	args.id = 0;
	args.completion_cookie = 0;

	vec.iov_base = (void *)message_buf;
	vec.iov_len = length;

	hdr.msg_name = NULL;
	hdr.msg_namelen = 0;
	hdr.msg_iov = &vec;
	hdr.msg_iovlen = 1;
	hdr.msg_control = &args;
	hdr.msg_controllen = 0;
	result = sendmsg(sockfd, &hdr, 0);
	return result;
}

/**
 * SCTP style peeloff(), used for peeling off a socket to handle
 * communication with a specific remote host.
 * peeloff() is not the same as listen(). It allows the main socket
 * to process data while the new socket is being set up.
 * @sockfd: the main socket to be peeled off
 * @client_addr: retrieved from syscall recvmsg(), specifying remote host
 * @addrlen: length of the address, used for validating the addr
 *
 * @return: new sockfd if sucess, negative if error was encountered
 */
int homa_peeloff(int sockfd, struct sockaddr *client_addr, uint32_t addrlen)
{
  	return getsockopt(sockfd, IPPROTO_HOMA, SO_HOMA_PEELOFF, (void *)client_addr, &addrlen);
}
