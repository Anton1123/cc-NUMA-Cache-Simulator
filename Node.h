/*
	Node.h

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
  int tag; //tag field (4 bits)
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

	public:
		CPU cpu0;
		CPU cpu1;		
    memLine memory[16];

  	Node(int);
		void display();
		int mem_read(Node&, Node&, Node&, bool, string, string, int);
		int mem_write(Node&, Node&, Node&, bool, string, string, int);
};

// Node initialization
Node::Node(int number) {
	id = number;

	// Init of CPUs
	cpu0.s1 = 0;
	cpu0.s2 = 0;
	cpu1.s1 = 0;
	cpu1.s2 = 0;

	// Init of CPU caches
	for(int i = 0; i < 4; ++i) {
		cpu0.cache[i].valid = 0;
		cpu0.cache[i].tag = 0;
		cpu0.cache[i].data = 0;
		cpu1.cache[i].valid = 0;
		cpu1.cache[i].tag = 0;
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

// cc-NUMA mem-read protocol 
// will pass in the other nodes as arguments so I can access and update their memory/directory contents
// returns access cost
int Node::mem_read(Node &nodeA, Node &nodeB, Node &nodeC, bool cpu, string rs, string rt, int address) {

	int index = address % 4;
	int tag = address / 4;

	if(cpu == 0) {
		// data found in local cache of CPU0
		if(cpu0.cache[index].valid == 1 && cpu0.cache[index].tag == tag) {
			if(rt == "10001") { // $s1 register load
				cpu0.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache 
			}
			else if(rt == "10010") { // $s2 register load
				cpu0.s2 = cpu0.cache[index].data; // update contents of $s2 register with the data from cache 
			}
			else {
				cout << "Invalid rt (destination) value. Must be either `10001` for $s1 register or `10010` for $s2 register.\n"; 
			}
			// return access cost of 1 (local cache hit)
			return 1;
		}

		// data found in cache of the other CPU local to the node
		else if(cpu1.cache[index].valid == 1 && cpu1.cache[index].tag == tag) { 
			// load contents into cache of CPU 0
			cpu0.cache[index].data = cpu1.cache[index].data;
			cpu0.cache[index].tag = tag;
			cpu0.cache[index].valid = 1;
			if(rt == "10001") { // $s1 register load
				cpu0.s1 = cpu1.cache[index].data; // update contents of $s1 register (of CPU0) with the data from the CPU1 cache 
			}
			else if(rt == "10010") { // $s2 register load
				cpu0.s2 = cpu1.cache[address % 4].data; // update contents of $s2 register (of CPU0) with the data from the CPU1 cache 
			}
			else {
				cout << "Invalid rt (destination) value. Must be either `10001` for $s1 register or `10010` for $s2 register.\n"; 
			}
			// return access cost of 30 (cache hit in the other CPU)
			return 30;
		}		

		// if not found in either of local caches, search the home memory directory

	}

	else if(cpu == 1) {
		// data found in local cache of CPU1
		if(cpu1.cache[address % 4].valid == 1 && cpu1.cache[address % 4].tag == address / 4) { 
			if(rt == "10001") { // $s1 register load
				cpu1.s1 = cpu1.cache[address % 4].data; // update contents of $s1 register with the data from cache 
			}
			else if(rt == "10010") { // $s2 register load
				cpu1.s2 = cpu1.cache[address % 4].data; // update contents of $s2 register with the data from cache 
			}
			else {
				cout << "Invalid rt (destination) value. Must be either `10001` for $s1 register or `10010` for $s2 register.\n"; 
			}
			// return access cost of 1 (local cache hit)
			return 1;
		}		

		// data found in cache of the other CPU local to the node
		else if(cpu0.cache[address % 4].valid == 1 && cpu0.cache[address % 4].tag == address / 4) { 
			// load contents into cache of CPU 1
			cpu1.cache[index].data = cpu0.cache[index].data;
			cpu1.cache[index].tag = tag;
			cpu1.cache[index].valid = 1;
			if(rt == "10001") { // $s1 register load
				cpu1.s1 = cpu0.cache[address % 4].data; // update contents of $s1 register with the data from cache 
			}
			else if(rt == "10010") { // $s2 register load
				cpu1.s2 = cpu0.cache[address % 4].data; // update contents of $s2 register with the data from cache 
			}
			else {
				cout << "Invalid rt (destination) value. Must be either `10001` for $s1 register or `10010` for $s2 register.\n"; 
			}
			// return access cost of 30 (cache hit in the other CPU)
			return 30;
		}

		// if not found in either of local caches, search the home memory directory

	}

	else {
		cout << "Invalid cpu value (Must be 0 or 1).\n";
		return -1;
	}

	return -1;
}

// cc-NUMA mem-write protocol 
// will pass in the other nodes as arguments so I can access and update their memory/directory contents
// returns access cost
int Node::mem_write(Node &nodeA, Node &nodeB, Node &nodeC, bool cpu, string rs, string rt, int address) {

	return -1;
}

