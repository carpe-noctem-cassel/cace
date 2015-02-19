/*
 * Copyright (c) 2013-2014, University of Lugano
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holders nor the names of it
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "paxos.h"
#include "message.h"
#include "paxos_types_pack.h"
#include <string.h>

struct timeval tTest;
static int
bufferevent_pack_data(void* data, const char* buf, size_t len)
{
	struct bufferevent* bev = (struct bufferevent*)data;
	int fd = bufferevent_getfd(bev);

	struct timeval tBegin;
	gettimeofday(&tBegin, NULL);

	send(fd, buf, len, 0);
	//bufferevent_write(bev, buf, len);

	struct timeval t2;
	gettimeofday(&t2, NULL);

	double elapsedTime = (t2.tv_sec - tBegin.tv_sec) * 1000000.0;      // sec to us
    elapsedTime += (t2.tv_usec - tBegin.tv_usec);   // us to us
    if(elapsedTime > 1000.0) {
    	printf("Transmission took too long: \t\t\t\t%f\n", elapsedTime);
    	fflush(stdout);
    }

//	double elapsedTime = (tBegin.tv_sec) * 1000000.0;      // sec to us
//	elapsedTime += (tBegin.tv_usec);   // us to us
//	printf("Proposer after end : %d\t%f\n", elapsedTime);


	/*struct timeval t2;
	gettimeofday(&t2, NULL);
	double elapsedTime;
	elapsedTime = (t2.tv_sec - tTest.tv_sec) * 1000000.0;      // sec to us
    elapsedTime += (t2.tv_usec - tTest.tv_usec);   // us to us
    	double elapsedTime2;
	elapsedTime2 = (t2.tv_sec) * 1000000.0;      // sec to us
    elapsedTime2 += (t2.tv_usec);   // us to us
	printf("\t%f\t%f\n", elapsedTime2, elapsedTime);
	fflush(stdout);
*/
	return 0;
}

void
send_paxos_message(struct bufferevent* bev, paxos_message* msg)
{
	msgpack_packer* packer;
	packer = msgpack_packer_new(bev, bufferevent_pack_data);
	msgpack_pack_paxos_message(packer, msg);
	msgpack_packer_free(packer);
}

void
send_paxos_prepare(struct bufferevent* bev, paxos_prepare* p)
{
	paxos_message msg = {
		.type = PAXOS_PREPARE,
		.u.prepare = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send prepare for iid %d ballot %d", p->iid, p->ballot);
}

void
send_paxos_promise(struct bufferevent* bev, paxos_promise* p)
{
	paxos_message msg = {
		.type = PAXOS_PROMISE,
		.u.promise = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send promise for iid %d ballot %d", p->iid, p->ballot);
}

void 
send_paxos_accept(struct bufferevent* bev, paxos_accept* p)
{
	paxos_message msg = {
		.type = PAXOS_ACCEPT,
		.u.accept = *p };
	send_paxos_message(bev, &msg);

	struct timeval tBegin;
		gettimeofday(&tBegin, NULL);
	double elapsedTime = (tBegin.tv_sec) * 1000000.0;      // sec to us
	elapsedTime += (tBegin.tv_usec);   // us to us
	printf("Proposer end: %d\t%f\n",(int)p->value.paxos_value_val[0], elapsedTime);
	paxos_log_debug("Send accept for iid %d ballot %d", p->iid, p->ballot);
}

void
send_paxos_accepted(struct bufferevent* bev, paxos_accepted* p)
{	
	paxos_message msg = {
		.type = PAXOS_ACCEPTED,
		.u.accepted = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send accepted for inst %d ballot %d", p->iid, p->ballot);
}

void
send_paxos_preempted(struct bufferevent* bev, paxos_preempted* p)
{
	paxos_message msg = {
		.type = PAXOS_PREEMPTED,
		.u.preempted = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send preempted for inst %d ballot %d", p->iid, p->ballot);
}

void
send_paxos_repeat(struct bufferevent* bev, paxos_repeat* p)
{
	paxos_message msg = {
		.type = PAXOS_REPEAT,
		.u.repeat = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send repeat for inst %d-%d", p->from, p->to);
}

void
send_paxos_trim(struct bufferevent* bev, paxos_trim* t)
{
	paxos_message msg = {
		.type = PAXOS_TRIM,
		.u.trim = *t };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send trim for inst %d", t->iid);
}

void
paxos_submit(struct bufferevent* bev, char* data, int size)
{

	gettimeofday(&tTest, NULL);
	double elapsedTime;
	elapsedTime = (tTest.tv_sec) * 1000000.0;      // sec to us
    elapsedTime += (tTest.tv_usec);   // us to us

	paxos_message msg = {
		.type = PAXOS_CLIENT_VALUE,
		.u.client_value.value.paxos_value_len = size,
		.u.client_value.value.paxos_value_val = data };

	//printf("+\t%f\n", elapsedTime);
	//fflush(stdout);
	send_paxos_message(bev, &msg);
	//printf("+ ");
}

int
recv_paxos_message(struct evbuffer* in, paxos_message* out)
{
	int rv = 0;
	char* buffer;
	size_t size, offset = 0;
	msgpack_unpacked msg;
	
	size = evbuffer_get_length(in);
	if (size == 0) {
		printf("no more messages\n");
		//if(rv==0) printf("X------------------------------ should never happen\n");
		return rv;
	}
	
	msgpack_unpacked_init(&msg);
	buffer = (char*)evbuffer_pullup(in, size);	
	if (msgpack_unpack_next(&msg, buffer, size, &offset)) {
		msgpack_unpack_paxos_message(&msg.data, out);
		evbuffer_drain(in, offset);
		rv = 1;
	}
	msgpack_unpacked_destroy(&msg);

	printf("%d\n", rv);
	//if(rv==0)printf("O------------------------------ should never happen\n");
	return rv;
}
