/*
 Copyright (c) 2013, University of Lugano
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the copyright holders nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

extern "C"
{
#include <paxos.h>
#include <evpaxos.h>
}
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <event2/event.h>
#include <iostream>
#include <limits>
#include <algorithm>
#include <chrono>
#include <ctime>

using namespace std;

#define MAX_VALUE_SIZE 65536
char value[MAX_VALUE_SIZE];
auto t0 = std::chrono::high_resolution_clock::now();
auto t1 = std::chrono::high_resolution_clock::now();
auto tsub = std::chrono::high_resolution_clock::now();

using namespace std::chrono;

class IncrementalEstimator
{
public:
	double n;
	double mean;
	double m2;
	double min;
	double max;
	IncrementalEstimator()
	{
		clear();
	}

	void addData(double x)
	{
		if (x > max)
		{
			max = x;
		}
		if (x < min)
		{
			min = x;
		}

		n = n + 1.0;
		double delta = x - mean;
		mean = mean + delta / n;
		m2 = m2 + delta * (x - mean);
	}

	double getVariance()
	{
		if (n < 2)
		{
			return 0;
		}
		return m2 / (n - 1.0);
	}

	void clear()
	{
		n = 0;
		mean = 0;
		m2 = 0;
		min = numeric_limits<double>::max();
		max = numeric_limits<double>::min();
	}

	string toString()
	{
		return "m: " + to_string(mean) + "\tv: " + to_string(getVariance()) + "\tcount: " + to_string(n) + "\tmin: "
				+ to_string(min) + "\tmax: " + to_string(max);
	}

};

int size = 4000;
const int tries = 2;
IncrementalEstimator avg;
IncrementalEstimator submit;

struct stats
{
	int delivered;
};

struct client
{
	int value_size;
	int outstanding;
	struct stats stats;
	struct event_base* base;
	struct bufferevent* bev;
	struct event* stats_ev;
	struct timeval stats_interval;
	struct event* sig;
	struct evlearner* learner;
};

static void handle_sigint(int sig, short ev, void* arg)
{
	struct event_base* base = (event_base*)arg;
	printf("Caught signal %d\n", sig);
	event_base_loopexit(base, NULL);
}

static void random_string(char *s, const int len)
{
	int i;
	static const char alphanum[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	for (i = 0; i < len - 1; ++i)
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	s[len - 1] = 0;
}

static void client_submit_value(struct client* c)
{
	if (avg.n >= tries)
	{

		std::cout << "Paxos - Size: " << size << "\tsubmit\t" << submit.toString() << " ns\t";
		std::cout << "deliverd: " << avg.toString() << " ns\t" << endl;


		avg.clear();
		submit.clear();
		size += 200;

		if(size>65536) {
			exit(0);
		}

		for(int i=0; i<size; i++) {
			value[i] = 'a';
		}
		value[size]=0;
	}

	c->value_size = size;

	//random_string(value, c->value_size);

	t0 = std::chrono::high_resolution_clock::now();
	paxos_submit(c->bev, value, c->value_size);
	tsub = std::chrono::high_resolution_clock::now();
}

static void on_deliver(unsigned iid, char* value, size_t size, void* arg)
{
	t1 = std::chrono::high_resolution_clock::now();
	//cout << size << ">" << value << endl;
	avg.addData(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
	submit.addData(std::chrono::duration_cast<std::chrono::nanoseconds>(tsub - t0).count());
	//cout << "." << endl;

	struct client* c = (struct client*)arg;
	c->stats.delivered++;
	client_submit_value(c);
}

static void on_stats(evutil_socket_t fd, short event, void *arg)
{
	struct client* c = (struct client*)arg;
	double mbps = (double)(c->stats.delivered * c->value_size * 8) / (1024 * 1024);
	printf("%d value/sec, %.2f Mbps\n", c->stats.delivered, mbps);
	c->stats.delivered = 0;
	event_add(c->stats_ev, &c->stats_interval);
}

static void on_connect(struct bufferevent* bev, short events, void* arg)
{
	int i;
	struct client* c = (struct client*)arg;
	if (events & BEV_EVENT_CONNECTED)
	{
		printf("Connected to proposer\n");
		for (i = 0; i < c->outstanding; ++i)
			client_submit_value(c);
	}
	else
	{
		printf("%s\n", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
	}
}

static struct bufferevent*
connect_to_proposer(struct client* c, const char* config, int proposer_id)
{
	struct bufferevent* bev;
	struct evpaxos_config* conf = evpaxos_config_read(config);
	struct sockaddr_in addr = evpaxos_proposer_address(conf, proposer_id);
	bev = bufferevent_socket_new(c->base, -1, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, NULL, NULL, on_connect, c);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	bufferevent_socket_connect(bev, (struct sockaddr*)&addr, sizeof(addr));
	return bev;
}

static struct client*
make_client(const char* config, int proposer_id, int outstanding, int value_size)
{
	struct client* c;
	c = (struct client*)malloc(sizeof(struct client));
	c->base = event_base_new();


	memset(&c->stats, 0, sizeof(struct stats));
	c->bev = connect_to_proposer(c, config, proposer_id);
	if (c->bev == NULL)
		exit(1);

	c->value_size = value_size;
	c->outstanding = outstanding;

	//c->stats_interval = (struct timeval) {1, 0};
	//c->stats_ev = evtimer_new(c->base, on_stats, c);
	//event_add(c->stats_ev, &c->stats_interval);

	paxos_config.learner_catch_up = 0;
	c->learner = evlearner_init(config, on_deliver, c, c->base);

	c->sig = evsignal_new(c->base, SIGINT, handle_sigint, c->base);
	evsignal_add(c->sig, NULL);

	return c;
}

static void client_free(struct client* c)
{
	bufferevent_free(c->bev);
	event_free(c->stats_ev);
	event_free(c->sig);
	event_base_free(c->base);
	free(c);
}

static void usage(const char* name)
{
	char* opts = "config [proposer id] [# outstanding values] [value size]";
	printf("Usage: %s %s\n", name, opts);
	exit(1);
}

int main(int argc, char const *argv[])
{
	int proposer_id = 0;
	int outstanding = 1;
	int value_size = 64;

	if (argc < 2 || argc > 5)
		usage(argv[0]);
	if (argc == 3)
		proposer_id = atoi(argv[2]);
	if (argc >= 4)
		outstanding = atoi(argv[3]);
	if (argc == 5)
		value_size = atoi(argv[4]);

	srand(time(NULL));

	struct client* client;
	client = make_client(argv[1], proposer_id, outstanding, value_size);
	event_base_dispatch(client->base);
	client_free(client);

	return 0;
}
