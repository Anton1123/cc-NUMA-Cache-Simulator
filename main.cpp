/*
	Main.cpp	

	Simulation of cc-NUMA architecture (DASH machine) with directory-
	based cache coherence control (using write-invalidate protocol).

*/

#include <fstream>
#include <stdlib.h>
#include "Node.h"

bool getCPUID(char);
int getNodeID(string);
int binary2word(string);

int main(int argc, char *argv[]) {

	int total_access_cost = 0;
	double avg_access_cost = 0;
	int num_of_accesses = 0;

	Node node0(0);
	Node node1(1);
	Node node2(2);
	Node node3(3);

	ifstream stream(argv[1]);
	string line;
	int nodeID;
	bool cpuID;
	string opcode;
	string rs;
	string rt;
	int offset;

	while (getline(stream, line)) {
		nodeID = getNodeID(line.substr(0,2));
		cpuID = getCPUID(line[2]);
		opcode = line.substr(5,6);
		rs = line.substr(11,5);
		rt = line.substr(16,5);
		offset = binary2word(line.substr(21,16));
		cout << nodeID << " " << cpuID << " " << opcode << " " << rs << " " << rt << " " << offset <<endl;
	
		// update home

		switch(nodeID) {
			case 0: if(opcode == "100011") total_access_cost += node0.mem_read(node1, node2, node3, cpuID, rs, rt, offset);
							else total_access_cost += node0.mem_write(node1, node2, node3, cpuID, rs, rt, offset);
							break;

			case 1: if(opcode == "100011") total_access_cost += node1.mem_read(node0, node2, node3, cpuID, rs, rt, offset);
							else total_access_cost += node1.mem_write(node0, node2, node3, cpuID, rs, rt, offset);
							break;

			case 2: if(opcode == "100011") total_access_cost += node2.mem_read(node0, node1, node3, cpuID, rs, rt, offset);
							else total_access_cost += node2.mem_write(node0, node1, node3, cpuID, rs, rt, offset);
							break;

			case 3: if(opcode == "100011") total_access_cost += node3.mem_read(node0, node1, node2, cpuID, rs, rt, offset);
							else total_access_cost += node3.mem_write(node0, node1, node2, cpuID, rs, rt, offset);
							break;
		}

  node0.display();
	node1.display();
	node2.display();
	node3.display();
	cin.get();
	}
	return 0;
}

// convert char to CPU ID w/ '0' corresponding to CPU-0
bool getCPUID(char ch) {
	if(ch == '0') return 0;
	else return 1;
}

// convert from String to corresponding node ID.
int getNodeID(string str) {
	if(str == "00") return 0;
	else if(str == "01") return 1;
	else if(str == "10") return 2;
	else return 3;
}

// converts the binary offset string to word address
int binary2word(string str) {
	int address = strtol(str.c_str(), NULL, 2);
	address /= 4;
	return address;
}
