#ifndef SIM_STATISTICS_H
#define SIM_STATISTICS_H

#include <iostream>
#include <cstdint>
#include <string>
#include <map>

using namespace std;

typedef struct LATENCY {
    unsigned total_latency;
} Latency;

typedef struct STAT {
    unsigned num_read_request;
    unsigned num_read_flit;
    unsigned num_read_packet;
    unsigned total_read_latency;
    unsigned total_read_size;
    unsigned num_write_request;
    unsigned num_write_flit;
    unsigned num_write_packet;
    unsigned total_write_latency;
    unsigned total_write_size;
} Stats;

class Statistics
{
public:
    Statistics();
    ~Statistics();

    void print_stats();
    void print_mem_stats();
    void set_name(std::string n) {name = n;}
    void increase_r_request() { stats.num_read_request++; }
    void increase_r_flit() { stats.num_read_flit++; }
    void increase_r_packet() { stats.num_read_packet++; }
    void increase_w_request() { stats.num_write_request++; }
    void increase_w_flit() { stats.num_write_flit++; }
    void increase_w_packet() { stats.num_write_packet++; }
    void update_total_read_size(uint32_t size) { stats.total_read_size += size; }
    void update_total_write_size(uint32_t size) { stats.total_write_size += size; }
    void update_read_latency(uint32_t latency) { stats.total_read_latency += latency; }
    void update_write_latency(uint32_t latency) { stats.total_write_latency += latency; }
	uint32_t get_r_flit_num() { return stats.num_read_flit; }
	uint32_t get_w_flit_num() { return stats.num_write_flit; }

	Stats stats;

private:
	string name;
};

#endif
