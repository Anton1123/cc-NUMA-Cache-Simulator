/*
	Created a structure for the cache line (valid bit, tag and data fields).
	Created a CPU object that will contain the two registers and the cache.
	Created a memLine (memory line) structure that contains the data and the 5 directory fields.
	All of these are included in the Node structure
*/

#include <iostream>
#include <string>
#include <bitset>

using namespace std;

// Cache line object
struct cLine {
	bool valid; //valid bit
  string tag; //tag field (4 bits)
	int data; //data field (32 bits)
};

// CPU object
struct CPU {
	int s1; //s1 register (32 bits)
	int s2; //s2 register (32 bits)
	cLine cache[4]; // 
};

// Object representing a line in memory which has the 32 bit memory and the directory contents
struct memLine {
	int data; //1 word (32 bit) content in main memory
	int dir[5]; //dir[0] is for the state of the mem entry, other 4 are representative of all the 4 nodes in the system.
};

// Each node has 2 CPU's, each with their own cache and a main memory/directory.
class Node {
	private:
		int id;
		int total_access_cost;
		double avg_access_cost;
  public:
		CPU cpu0;
		CPU cpu1;		
    memLine memory[16];

  	Node(int);
		void display();
		void mem_read(bool, string, string, int);
		void mem_write(bool, string, string, int);
};

// Node initialization
Node::Node(int number) {
	id = number;
	total_access_cost = 0;
	avg_access_cost = 0;
	// Init of CPUs
	cpu0.s1 = 0;
	cpu0.s2 = 0;
	cpu1.s1 = 0;
	cpu1.s2 = 0;

	// Init of CPU caches
	for(int i = 0; i < 4; ++i) {
		cpu0.cache[i].valid = 0;
		cpu0.cache[i].tag = "0000";
		cpu0.cache[i].data = 0;
		cpu1.cache[i].valid = 0;
		cpu1.cache[i].tag = "0000";
		cpu1.cache[i].data = 0;
	}

	// Init of memory and directory
	if(number < 0 || number > 3) {
		cout << "Invalid initialization of Node" << endl;
	}			
	else {
		for(int i = 0; i < 16; ++i) {
			memory[i].data = number*16 + i + 5; // initialize mem entry with address + 5
 			for(int j = 0; j < 5; ++j) 
				memory[i].dir[j] = 0;
		}	
	}
}

// Displays the contents of a Node in binary 
void Node::display() {
	cout << "Node" << id << endl;
	cout << "-------------------------------------------\n";
	cout << "Total Access Cost: " << total_access_cost << " Average Access Cost: " << avg_access_cost << endl;
	cout << "***CPU0***\n";
	cout << "S1:       " << bitset<32>(cpu0.s1) << endl;
	cout << "S2:       " << bitset<32>(cpu0.s2) << endl;
	cout << "Cache-0" << endl;
	
	for(int i = 0; i < 4; ++i) 
		cout << i << ": " << cpu0.cache[i].valid << " " << cpu0.cache[i].tag << " " << bitset<32>(cpu0.cache[i].data) << " " << endl;

	cout << "***CPU1***\n";
	cout << "S1:       " << bitset<32>(cpu1.s1) << endl;
	cout << "S2:       " << bitset<32>(cpu1.s2) << endl;
	cout << "Cache-1" << endl;
	
	for(int i = 0; i < 4; ++i) 
		cout << i << ": " << cpu1.cache[i].valid << " " << cpu1.cache[i].tag << " " << bitset<32>(cpu1.cache[i].data) << " " << endl;
	
	cout << "***Memory***\n"; 
	for(int i = 0; i < 16; ++i) {
		cout << i + id*16 << ": " << bitset<32>(memory[i].data) << " ";
		for(int j = 0; j < 5; ++j)
			cout << memory[i].dir[j] << " ";
		cout << endl;
	}
	cout << endl;
}

void Node::mem_read(bool cpu, string rs, string rt, int address) {
	return;
}

void Node::mem_write(bool cpu, string rs, string rt, int address) {
	return;
}

