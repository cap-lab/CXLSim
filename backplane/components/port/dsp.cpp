#include "dsp.h"

extern Configurations cfgs;

DSP::DSP(sc_module_name name, int id) : sc_module(name), master("master"), slave("slave"), clock("clock"),
										id(id), name(string(name)),
										f_idx(0), w_stack(0), w_msg(0), r_msg(0)
{
	init();
	SC_THREAD(bw_thread);
	master.register_nb_transport_bw(this, &DSP::nb_transport_bw);
	slave.register_nb_transport_fw(this, &DSP::nb_transport_fw);
}

DSP::~DSP() {
	if (stats) {
		stats->print_stats();
		free(stats);
		stats = NULL;
	}
}

void DSP::init() {
	period = cfgs.get_period(id);
	flit_mode = cfgs.get_flit_mode();
	port_latency = cfgs.get_port_latency();
	link_latency = cfgs.get_link_latency();
	req_num = cfgs.get_packet_size()/cfgs.get_dram_req_size();
	stats = new Statistics();
	stats->set_name(name);

	/* Flit-Packing variables (payload num per flit) */	
	/* 68B/256B WRITE rsp (wo Data) */
	w_msg = (flit_mode == 68) ? 2 : (flit_mode == 256) ? 12 : w_msg;

	/* 256B READ rsp (w Data) */
	r_msg = 3;
}

void DSP::bw_thread() {
	while(1) {
		/* 68B Flit */
		if (flit_mode == 68) {
			/* READ */
			if(!rack_queue.empty()) {
				flit_packing_68(true);
			}

			/* READ (Last Flit) */
			else if (f_idx == 8) {
				flit_packing_68(true);
			}

			/* WRITE */
			if(!wack_queue.empty() && w_stack >= w_msg) {
				flit_packing_68(false);
			}
		}

		/* 256B Flit */
		else {
			/* WRITE */
			if (!wack_queue.empty() && w_stack >= w_msg) {
				flit_packing_256(false);
			}

			/* WRITE (Last Flit) */	
			if (!wack_queue.empty() && ((stats->get_w_flit_num()+1)%(req_num/w_msg+1) == 0 && w_stack >= req_num%w_msg)) {
				flit_packing_256(false);
			}

			/* READ */
			if(!rack_queue.empty()) {
				flit_packing_256(true);
			}
		}
		wait(period, SC_NS);
	}
}

void DSP::flit_packing_68(bool read) {
	tlm_phase phase = BEGIN_RESP;
	sc_time t = SC_ZERO_TIME;

	/* READ */
	if(read) {
		/* Last Flit */
		if (f_idx == 8) {
			f_idx = 0;
			tlm_generic_payload *trans = pending_queue.front();	
			pending_queue.pop_front();

			/* CXL Latency (per flit) */
			wait(port_latency+link_latency, SC_NS);
			stats->increase_r_flit();

			tlm_sync_enum reply = slave->nb_transport_bw(*trans, phase, t);
		}

		else {
			if (f_idx > 0) {
				tlm_generic_payload *trans = pending_queue.front();	
				pending_queue.pop_front();

				/* CXL Latency (per flit) */
				wait(port_latency+link_latency, SC_NS);
				stats->increase_r_flit();

				tlm_sync_enum reply = slave->nb_transport_bw(*trans, phase, t);
			}
			tlm_generic_payload *trans = rack_queue.front();
			rack_queue.pop_front();
			pending_queue.push_back(trans);

			if (f_idx == 0) {
				/* CXL Latency (per flit) */
				wait(port_latency+link_latency, SC_NS);
				stats->increase_r_flit();
			}

			f_idx++;
		}
	}

	/* WRITE */
	else {
		/* CXL Latency (per flit) */
		wait(port_latency+link_latency, SC_NS);
		stats->increase_w_flit();

		for (int i = 0; i < w_msg; i++) {
			tlm_generic_payload *trans = wack_queue.front();
			wack_queue.pop_front();
			tlm_sync_enum reply = slave->nb_transport_bw(*trans, phase, t);
			w_stack--;
			if (wack_queue.empty())
				break;
		}
	}
}

void DSP::flit_packing_256(bool read) {
	tlm_phase phase = BEGIN_RESP;
	sc_time t = SC_ZERO_TIME;

	/* READ */
	if(read) {
		if (f_idx < 2) {
			/* CXL Latency (per flit) */
			wait(port_latency, SC_NS);
			stats->increase_r_flit();

			for (int i = 0; i < r_msg; i++) {
				tlm_generic_payload *trans = rack_queue.front();
				rack_queue.pop_front();
				tlm_sync_enum reply = slave->nb_transport_bw(*trans, phase, t);

				if (rack_queue.empty())
					break;
			}
			f_idx++;
		}

		else {
			/* CXL Latency (per flit) */
			wait(port_latency, SC_NS);
			stats->increase_r_flit();

			for (int i = 0; i < r_msg+1; i++) {
				tlm_generic_payload *trans = rack_queue.front();
				rack_queue.pop_front();
				tlm_sync_enum reply = slave->nb_transport_bw(*trans, phase, t);

				if (rack_queue.empty())
					break;
			}
			f_idx = 0;
		}
	}

	/* WRITE */
	else {
		/* CXL Latency (per flit) */
		wait(port_latency, SC_NS);
		stats->increase_w_flit();

		for (int i = 0; i < w_msg; i++) {
			tlm_generic_payload *trans = wack_queue.front();
			wack_queue.pop_front();
			tlm_sync_enum reply = slave->nb_transport_bw(*trans, phase, t);
			w_stack--;
			if (wack_queue.empty())
				break;
		}
	}
}

tlm_sync_enum DSP::nb_transport_fw(int id, tlm_generic_payload& trans, tlm_phase& phase, sc_time& t) {
	if (phase == END_RESP)
		return TLM_COMPLETED;

	tlm_sync_enum reply = master->nb_transport_fw(trans, phase, t);

	return reply;
}

tlm_sync_enum DSP::nb_transport_bw(int id, tlm_generic_payload& trans, tlm_phase& phase, sc_time& t) {
	/* READ */
	if (trans.get_command() == TLM_READ_COMMAND) {
		rack_queue.push_back(&trans);
	}

	/* WRITE */
	else {
		wack_queue.push_back(&trans);
		w_stack++;
	}

	return TLM_UPDATED;
}