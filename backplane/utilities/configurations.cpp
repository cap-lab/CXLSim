#include "configurations.h"

Configurations::Configurations() {};

void Configurations::init_dram()
{
	cout << "--------INIT DRAM--------\n";
    stringstream filePath;
    filePath << ROOT_PATH << "/backplane/configs/memory.json";

    ifstream ifs(filePath.str());
    if(!ifs.is_open()) {
    	cout << filePath.str() << " file doesn't exist\n";
    	exit(-1);
    }

    string rawJson;

    ifs.seekg(0, ios::end);
    rawJson.reserve(ifs.tellg());
    ifs.seekg(0, ios::beg);

    rawJson.assign(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());

    JSONCPP_STRING err;
    Json::Value root;
    Json::CharReaderBuilder builder;
    const unique_ptr<Json::CharReader> reader(builder.newCharReader());
    reader->parse(rawJson.c_str(), rawJson.c_str() + rawJson.length(), &root, &err);
	
	dram_num = root["dram_num"].asUInt();

	for (int i = 0; i < dram_num; i++) {
		cout << "[DRAM-" << i << "]\n";

		DRAMInfo draminfo;
		const Json::Value dram = root["dram" + to_string(i)];
	    draminfo.type = dram["config"].asString();

	    stringstream dram_size(dram["size"].asString());
	    dram_size >> hex >> draminfo.size;
    	cout << "DRAM size : " << draminfo.size << '\n';

    	dram_req_size = root["req_size"].asUInt();
		cout << "DRAM Request size : " << dram_req_size << " (B)\n";

	    draminfo.freq = dram["freq"].asDouble();
    	cout << "DRAM Frequency : " << draminfo.freq << " (Ghz)\n";
	    
		link_latency = ceil(flit_size / link_bandwidth);
    	cout << "Link Latency : " << link_latency << " (ns)\n";
		dram_map[i] = draminfo;
	}
}

void Configurations::init_configurations()
{
    stringstream filePath;
    filePath << ROOT_PATH << "/backplane/configs/system.json";

    ifstream ifs(filePath.str());
    if(!ifs.is_open()) {
		cout << "The \"system.json\" file doesn't exist\n";
		exit(-1);
    }

    string rawJson;
    ifs.seekg(0, ios::end);
    rawJson.reserve(ifs.tellg());
    ifs.seekg(0, ios::beg);
	
	rawJson.assign(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());

    JSONCPP_STRING err;
    Json::Value root;
    Json::CharReaderBuilder builder;
    const unique_ptr<Json::CharReader> reader(builder.newCharReader());
    reader->parse(rawJson.c_str(), rawJson.c_str() + rawJson.length(), &root, &err);


	cout << "--------INIT SYSTEM--------\n";
	host_num = root["host_num"].asUInt();
	flit_mode = root["flit_mode"].asUInt();
	packet_size = root["packet_size"].asUInt();
    link_efficiency = root["link_efficiency"].asDouble();
    raw_bandwidth = root["PCIe_raw_bandwidth"].asDouble();
    port_latency = root["cxl_port_latency"].asUInt();
    dev_ic_latency = root["cxl_dev_ic_latency"].asUInt();
    link_bandwidth = link_efficiency * raw_bandwidth;
	
	flit_size = (flit_mode == 68) ? 64 : (flit_mode == 256) ? 240 : flit_size;

	cout << "HOST Number: " << host_num << '\n';
	cout << "PACKET Size: " << packet_size << " (B)\n";
	cout << "FLIT Mode: " << flit_mode << "\n";
	cout << "FLIT Size: " << flit_size << "\n";
	cout << "Link Efficiency : " << link_efficiency << '\n';
	cout << "Raw Bandwidth : " << raw_bandwidth << " (B/ns)\n";
	cout << "Link Bandwidth : " << link_bandwidth << " (B/ns)\n";
	cout << "CXL Port Latency : " << port_latency << " (ns)\n";
	cout << "CXL Device Interconnector Latency : " << dev_ic_latency << " (ns)\n";

	cout << "--------HOST INFO--------\n";
    for (int i = 0; i<host_num; i++){
		cout << "[CPU-"<< i << "]\n";
		CPUInfo cpuinfo;
		const Json::Value cpu = root["cpu" + to_string(i)];
		cpuinfo.simul_freq = cpu["freq"].asDouble();
		cpuinfo.cpu_latency = cpu["cpu_latency"].asUInt();
    	cpuinfo.period = 1/cpuinfo.simul_freq;
    	cout << "CPU Frequency : " << cpuinfo.simul_freq << " (Ghz)\n";
		cout << "Clock Period : " << cpuinfo.period << " (ns)\n";
		cout << "Core+LLC Latency : " << cpuinfo.cpu_latency << " (ns)\n";
		cpu_map[i] = cpuinfo;
    }

	init_dram();

	cout << "------------------------\n";
}

string Configurations::get_dram_config(int id) {
	DRAMInfo draminfo = dram_map[id];
	return draminfo.type;
}

uint64_t Configurations::get_dram_size(int id) {
	DRAMInfo draminfo = dram_map[id];
	return draminfo.size;
}

double Configurations::get_dram_freq(int id) {
	DRAMInfo draminfo = dram_map[id];
	return draminfo.freq;
}

double Configurations::get_freq(int id) {
    CPUInfo cpuinfo = cpu_map[id];
    return cpuinfo.simul_freq;
}

double Configurations::get_period(int id) {
    CPUInfo cpuinfo = cpu_map[id];
    return cpuinfo.period;
}

uint32_t Configurations::get_cpu_latency(int id) {
    CPUInfo cpuinfo = cpu_map[id];
    return cpuinfo.cpu_latency;
}
