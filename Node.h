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
		int mem_read(Node&, Node&, Node&, Node&, bool, string, string, int);
		int mem_write(Node&, Node&, Node&, Node&, bool, string, string, int);
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
		cout << "Invalid initialization of Node (Valid options are: 0,1,2,3 only)\n";
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

// Performs search on both the caches in a Node using index and tag
// Returns the data if tags match. If not, displays error message and returns -1.
int searchNode(Node node, int index, int tag) {
	if(node.cpu0.cache[index].tag == tag) return node.cpu0.cache[index].data;
	else if(node.cpu1.cache[index].tag == tag) return node.cpu1.cache[index].data;
	else {
		cout << "Data not found in cache of dirty node\n";
		return -1;
	}
}

// cc-NUMA mem-read protocol 
// will pass in all the nodes as arguments so I can access and update their memory/directory contents
// returns access cost
// 2 sections of basically the same code seperated by which CPU is making a read request
// have to pass in the current nodeID to know which node is making a read/write 
int Node::mem_read(Node &node0, Node &node1, Node &node2, Node &node3, bool cpu, string rs, string rt, int address) {

	int index = address % 4;
	int tag = address / 4;
	
	//start of cpu0
	if(cpu == 0) {
		if(cpu0.cache[index].valid == 1 && cpu0.cache[index].tag == tag) { // data found in local cache of CPU0
			if(rt == "10001") { // $s1 register load
				cpu0.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache 
			}
			else if(rt == "10010") { // $s2 register load
				cpu0.s2 = cpu0.cache[index].data; // update contents of $s2 register with the data from cache 
			}
			else {
				cout << "Invalid rt (destination) value. Must be either `10001` for $s1 register or `10010` for $s2 register.\n"; 
				return -1;
			}
			return 1; // return access cost of 1 (local cache hit)
		}

		else if(cpu1.cache[index].valid == 1 && cpu1.cache[index].tag == tag) { // data found in cache of the other CPU local to the node
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
				return -1;
			}
			return 30; // return access cost of 30 (cache hit in the other CPU)
		}		

		// if not found in either of local caches, search the home memory directory
		// will find the home directory by dividing address (word) by 16
		int homeNodeID = address / 16;
		int memSlot = address % 16; // calculate the memory slot of where the word address points to (0-indexed at each individual node)
		switch(homeNodeID) {
			case 0:	if(node0.memory[memSlot].dir[0] == 0 || node0.memory[memSlot].dir[0] == 1) {// directory indicates "uncached" or "shared" (0 or 1)
								node0.memory[memSlot].dir[0] = 1; // set to shared (stays same if already shared)
								node0.memory[memSlot].dir[1] = 1; // set the corresponding directory slot of node2 to indicate it has this data
								// bring up the data into the local cache
								cpu0.cache[index].data = node0.memory[memSlot].data;
								cpu0.cache[index].tag = tag;
								cpu0.cache[index].valid = 1;
								if(rt == "10001") cpu0.s1 = node0.memory[memSlot].data; // update contents of $s1 register with the data from memory
								else if(rt == "10010") cpu0.s2 = node0.memory[memSlot].data; // update contents of $s2 register with the data from memory
								return 100;
							}
							else { // directory indicates "dirty"
								for(int i = 1; i < 5; ++i) { 
									if(node0.memory[memSlot].dir[i] == 1) { // search for the dirty node
										switch(i) { // search all the caches in the dirty node
											case 1: cpu0.cache[index].data = searchNode(node0, index, tag);
															break;
											case 2: cpu0.cache[index].data = searchNode(node1, index, tag);
															break;
											case 3: cpu0.cache[index].data = searchNode(node2, index, tag);
															break;
											case 4: cpu0.cache[index].data = searchNode(node3, index, tag);
															break;
										}
									}
								} 
								cpu0.cache[index].tag = tag;
								cpu0.cache[index].valid = 1;

								// update home directory
								node0.memory[memSlot].data = cpu0.cache[index].data; // copy the cached data into the memory to overwrite the "dirty" data
								node0.memory[memSlot].dir[0] = 1; // indicate "shared" now instead of "dirty"
								node0.memory[memSlot].dir[id+1] = 1; // indicate that the current node now has this data as well.

								// load data into the appropriate register
								if(rt == "10001") cpu0.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache
								else if(rt == "10010") cpu0.s2 = cpu0.cache[index].data; // or update contents of $s2 register with the data from cache
								return 135;
							}
			case 1:	if(node1.memory[memSlot].dir[0] == 0 || node1.memory[memSlot].dir[0] == 1) {// directory indicates "uncached" or "shared" (0 or 1)
								node1.memory[memSlot].dir[0] = 1; // set to shared (stays same if already shared)
								node1.memory[memSlot].dir[2] = 1; // set the corresponding directory slot of node2 to indicate it has this data
								// bring up the data into the local cache
								cpu0.cache[index].data = node1.memory[memSlot].data;
								cpu0.cache[index].tag = tag;
								cpu0.cache[index].valid = 1;
					
								if(rt == "10001") cpu0.s1 = node1.memory[memSlot].data; // update contents of $s1 register with the data from memory
								else if(rt == "10010") cpu0.s2 = node1.memory[memSlot].data; // update contents of $s2 register with the data from memory			
								return 100;
							}
							else { // directory indicates "dirty"
								for(int i = 1; i < 5; ++i) { 
									if(node1.memory[memSlot].dir[i] == 1) { // search for the dirty node
										switch(i) { // search all the caches in the dirty node
											case 1: cpu0.cache[index].data = searchNode(node0, index, tag);
															break;
											case 2: cpu0.cache[index].data = searchNode(node1, index, tag);
															break;
											case 3: cpu0.cache[index].data = searchNode(node2, index, tag);
															break;
											case 4: cpu0.cache[index].data = searchNode(node3, index, tag);
															break;
										}
									}
								} 
								cpu0.cache[index].tag = tag;
								cpu0.cache[index].valid = 1;

								// update home directory
								node1.memory[memSlot].data = cpu0.cache[index].data; // copy the cached data into the memory to overwrite the "dirty" data
								node1.memory[memSlot].dir[0] = 1; // indicate "shared" now instead of "dirty"
								node1.memory[memSlot].dir[id+1] = 1; // indicate that the current node now has this data as well.

								// load data into the appropriate register
								if(rt == "10001") cpu0.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache
								else if(rt == "10010") cpu0.s2 = cpu0.cache[index].data; // or update contents of $s2 register with the data from cache
								return 135;
							}
			case 2:	if(node2.memory[memSlot].dir[0] == 0 || node2.memory[memSlot].dir[0] == 1) {// directory indicates "uncached" or "shared" (0 or 1)
								node2.memory[memSlot].dir[0] = 1; // set to shared (stays same if already shared)
								node2.memory[memSlot].dir[3] = 1; // set the corresponding directory slot of node2 to indicate it has this data
								// bring up the data into the local cache
								cpu0.cache[index].data = node2.memory[memSlot].data;
								cpu0.cache[index].tag = tag;
								cpu0.cache[index].valid = 1;
						
								if(rt == "10001") cpu0.s1 = node2.memory[memSlot].data; // update contents of $s1 register with the data from memory
								else if(rt == "10010") cpu0.s2 = node2.memory[memSlot].data; // update contents of $s2 register with the data from memory					
								return 100;
							}
							else { // directory indicates "dirty"
								for(int i = 1; i < 5; ++i) { 
									if(node2.memory[memSlot].dir[i] == 1) { // search for the dirty node
										switch(i) { // search all the caches in the dirty node
											case 1: cpu0.cache[index].data = searchNode(node0, index, tag);
															break;
											case 2: cpu0.cache[index].data = searchNode(node1, index, tag);
															break;
											case 3: cpu0.cache[index].data = searchNode(node2, index, tag);
															break;
											case 4: cpu0.cache[index].data = searchNode(node3, index, tag);
															break;
										}
									}
								} 
								cpu0.cache[index].tag = tag;
								cpu0.cache[index].valid = 1;

								// update home directory
								node2.memory[memSlot].data = cpu0.cache[index].data; // copy the cached data into the memory to overwrite the "dirty" data
								node2.memory[memSlot].dir[0] = 1; // indicate "shared" now instead of "dirty"
								node2.memory[memSlot].dir[id+1] = 1; // indicate that the current node now has this data as well.

								// load data into the appropriate register
								if(rt == "10001") cpu0.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache
								else if(rt == "10010") cpu0.s2 = cpu0.cache[index].data; // or update contents of $s2 register with the data from cache
								return 135;
							}
			case 3:	if(node3.memory[memSlot].dir[0] == 0 || node3.memory[memSlot].dir[0] == 1) {// directory indicates "uncached" or "shared" (0 or 1)
								node3.memory[memSlot].dir[0] = 1; // set to shared (stays same if already shared)
								node3.memory[memSlot].dir[4] = 1; // set the corresponding directory slot of node3 to indicate it has this data
								// bring up the data into the local cache
								cpu0.cache[index].data = node3.memory[memSlot].data;
								cpu0.cache[index].tag = tag;
								cpu0.cache[index].valid = 1;
						
								if(rt == "10001") cpu0.s1 = node3.memory[memSlot].data; // update contents of $s1 register with the data from memory
								else if(rt == "10010") cpu0.s2 = node3.memory[memSlot].data; // update contents of $s2 register with the data from memory				
								return 100;
							}
							else { // directory indicates "dirty"
								for(int i = 1; i < 5; ++i) { 
									if(node3.memory[memSlot].dir[i] == 1) { // search for the dirty node
										switch(i) { // search all the caches in the dirty node
											case 1: cpu0.cache[index].data = searchNode(node0, index, tag);
															break;
											case 2: cpu0.cache[index].data = searchNode(node1, index, tag);
															break;
											case 3: cpu0.cache[index].data = searchNode(node2, index, tag);
															break;
											case 4: cpu0.cache[index].data = searchNode(node3, index, tag);
															break;
										}
									}
								} 
								cpu0.cache[index].tag = tag;
								cpu0.cache[index].valid = 1;

								// update home directory
								node3.memory[memSlot].data = cpu0.cache[index].data; // copy the cached data into the memory to overwrite the "dirty" data
								node3.memory[memSlot].dir[0] = 1; // indicate "shared" now instead of "dirty"
								node3.memory[memSlot].dir[id+1] = 1; // indicate that the current node now has this data as well.

								// load data into the appropriate register
								if(rt == "10001") cpu0.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache
								else if(rt == "10010") cpu0.s2 = cpu0.cache[index].data; // or update contents of $s2 register with the data from cache
								return 135;
							}
		}

	} // end of cpu0

	// start of cpu1
	else if(cpu == 1) {
		if(cpu1.cache[index].valid == 1 && cpu1.cache[index].tag == tag) { // data found in local cache of CPU1
			if(rt == "10001") { // $s1 register load
				cpu1.s1 = cpu1.cache[index].data; // update contents of $s1 register with the data from cache 
			}
			else if(rt == "10010") { // $s2 register load
				cpu1.s2 = cpu1.cache[index].data; // update contents of $s2 register with the data from cache 
			}
			else {
				cout << "Invalid rt (destination) value. Must be either `10001` for $s1 register or `10010` for $s2 register.\n"; 
			}
			return 1; // return access cost of 1 (local cache hit)
		}		

		else if(cpu0.cache[index].valid == 1 && cpu0.cache[index].tag == tag) { // data found in cache of the other CPU local to the node
			// load contents into cache of CPU 1
			cpu1.cache[index].data = cpu0.cache[index].data;
			cpu1.cache[index].tag = tag;
			cpu1.cache[index].valid = 1;
			if(rt == "10001") { // $s1 register load
				cpu1.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache 
			}
			else if(rt == "10010") { // $s2 register load
				cpu1.s2 = cpu0.cache[index].data; // update contents of $s2 register with the data from cache 
			}
			else {
				cout << "Invalid rt (destination) value. Must be either `10001` for $s1 register or `10010` for $s2 register.\n"; 
			}
			return 30; // return access cost of 30 (cache hit in the other CPU)
		}

		// if not found in either of local caches, search the home memory directory
		// will find the home directory by dividing address (word) by 16
		int homeNodeID = address / 16;
		int memSlot = address % 16; // calculate the memory slot of where the word address points to (0-indexed at each individual node)
		switch(homeNodeID) {
			case 0:	if(node0.memory[memSlot].dir[0] == 0 || node0.memory[memSlot].dir[0] == 1) {// directory indicates "uncached" or "shared" (0 or 1)
								node0.memory[memSlot].dir[0] = 1; // set to shared (stays same if already shared)
								// bring up the data into the local cache
								cpu1.cache[index].data = node0.memory[memSlot].data;
								cpu1.cache[index].tag = tag;
								cpu1.cache[index].valid = 1;
						
								if(rt == "10001") cpu1.s1 = node0.memory[memSlot].data; // update contents of $s1 register with the data from memory
								else if(rt == "10010") cpu1.s2 = node0.memory[memSlot].data; // update contents of $s2 register with the data from memory 					
								return 100;
							}
							else { // directory indicates "dirty"
								for(int i = 1; i < 5; ++i) { 
									if(node0.memory[memSlot].dir[i] == 1) { // search for the dirty node
										switch(i) { // search all the caches in the dirty node
											case 1: cpu1.cache[index].data = searchNode(node0, index, tag);
															break;
											case 2: cpu1.cache[index].data = searchNode(node1, index, tag);
															break;
											case 3: cpu1.cache[index].data = searchNode(node2, index, tag);
															break;
											case 4: cpu1.cache[index].data = searchNode(node3, index, tag);
															break;
										}
									}
								} 
								cpu1.cache[index].tag = tag;
								cpu1.cache[index].valid = 1;

								// update home directory
								node0.memory[memSlot].data = cpu1.cache[index].data; // copy the cached data into the memory to overwrite the "dirty" data
								node0.memory[memSlot].dir[0] = 1; // indicate "shared" now instead of "dirty"
								node0.memory[memSlot].dir[id+1] = 1; // indicate that the current node now has this data as well.

								// load data into the appropriate register
								if(rt == "10001") cpu1.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache
								else if(rt == "10010") cpu1.s2 = cpu0.cache[index].data; // or update contents of $s2 register with the data from cache
								return 135;
							}
			case 1:	if(node1.memory[memSlot].dir[0] == 0 || node1.memory[memSlot].dir[0] == 1) {// directory indicates "uncached" or "shared" (0 or 1)
								node1.memory[memSlot].dir[0] = 1; // set to shared (stays same if already shared)
								// bring up the data into the local cache
								cpu1.cache[index].data = node1.memory[memSlot].data;
								cpu1.cache[index].tag = tag;
								cpu1.cache[index].valid = 1;
						
								if(rt == "10001") cpu1.s1 = node1.memory[memSlot].data; // update contents of $s1 register with the data from memory
								else if(rt == "10010") cpu1.s2 = node1.memory[memSlot].data; // update contents of $s2 register with the data from memory				
								return 100;
							}
							else { // directory indicates "dirty"
								for(int i = 1; i < 5; ++i) { 
									if(node1.memory[memSlot].dir[i] == 1) { // search for the dirty node
										switch(i) { // search all the caches in the dirty node
											case 1: cpu1.cache[index].data = searchNode(node0, index, tag);
															break;
											case 2: cpu1.cache[index].data = searchNode(node1, index, tag);
															break;
											case 3: cpu1.cache[index].data = searchNode(node2, index, tag);
															break;
											case 4: cpu1.cache[index].data = searchNode(node3, index, tag);
															break;
										}
									}
								} 
								cpu1.cache[index].tag = tag;
								cpu1.cache[index].valid = 1;

								// update home directory
								node1.memory[memSlot].data = cpu1.cache[index].data; // copy the cached data into the memory to overwrite the "dirty" data
								node1.memory[memSlot].dir[0] = 1; // indicate "shared" now instead of "dirty"
								node1.memory[memSlot].dir[id+1] = 1; // indicate that the current node now has this data as well.

								// load data into the appropriate register
								if(rt == "10001") cpu1.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache
								else if(rt == "10010") cpu1.s2 = cpu0.cache[index].data; // or update contents of $s2 register with the data from cache
								return 135;
							}
			case 2:	if(node2.memory[memSlot].dir[0] == 0 || node2.memory[memSlot].dir[0] == 1) {// directory indicates "uncached" or "shared" (0 or 1)
								node2.memory[memSlot].dir[0] = 1; // set to shared (stays same if already shared)
								// bring up the data into the local cache
								cpu1.cache[index].data = node2.memory[memSlot].data;
								cpu1.cache[index].tag = tag;
								cpu1.cache[index].valid = 1;
						
								if(rt == "10001") cpu1.s1 = node2.memory[memSlot].data; // update contents of $s1 register with the data from memory
								else if(rt == "10010") cpu1.s2 = node2.memory[memSlot].data; // update contents of $s2 register with the data from memory			
								return 100;
							}
							else { // directory indicates "dirty"
								for(int i = 1; i < 5; ++i) { 
									if(node2.memory[memSlot].dir[i] == 1) { // search for the dirty node
										switch(i) { // search all the caches in the dirty node
											case 1: cpu1.cache[index].data = searchNode(node0, index, tag);
															break;
											case 2: cpu1.cache[index].data = searchNode(node1, index, tag);
															break;
											case 3: cpu1.cache[index].data = searchNode(node2, index, tag);
															break;
											case 4: cpu1.cache[index].data = searchNode(node3, index, tag);
															break;
										}
									}
								} 
								cpu1.cache[index].tag = tag;
								cpu1.cache[index].valid = 1;

								// update home directory
								node2.memory[memSlot].data = cpu1.cache[index].data; // copy the cached data into the memory to overwrite the "dirty" data
								node2.memory[memSlot].dir[0] = 1; // indicate "shared" now instead of "dirty"
								node2.memory[memSlot].dir[id+1] = 1; // indicate that the current node now has this data as well.

								// load data into the appropriate register
								if(rt == "10001") cpu1.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache
								else if(rt == "10010") cpu1.s2 = cpu0.cache[index].data; // or update contents of $s2 register with the data from cache
								return 135;
							}
			case 3:	if(node3.memory[memSlot].dir[0] == 0 || node3.memory[memSlot].dir[0] == 1) {// directory indicates "uncached" or "shared" (0 or 1)
								node3.memory[memSlot].dir[0] = 1; // set mem directory to shared (stays same if already shared)
								// bring up the data into the local cache
								cpu1.cache[index].data = node3.memory[memSlot].data;
								cpu1.cache[index].tag = tag;
								cpu1.cache[index].valid = 1;
						
								if(rt == "10001") cpu1.s1 = node3.memory[memSlot].data; // update contents of $s1 register with the data from memory
								else if(rt == "10010") cpu1.s2 = node3.memory[memSlot].data; // update contents of $s2 register with the data from memory 					
								return 100;
							}
							else { // directory indicates "dirty"
								for(int i = 1; i < 5; ++i) { 
									if(node3.memory[memSlot].dir[i] == 1) { // search for the dirty node
										switch(i) { // search all the caches in the dirty node (maybe I need to check if they are valid or not still)
											case 1: cpu1.cache[index].data = searchNode(node0, index, tag);
															break;
											case 2: cpu1.cache[index].data = searchNode(node1, index, tag);
															break;
											case 3: cpu1.cache[index].data = searchNode(node2, index, tag);
															break;
											case 4: cpu1.cache[index].data = searchNode(node3, index, tag);
															break;
										}
									}
								} 
								cpu1.cache[index].tag = tag;
								cpu1.cache[index].valid = 1;

								// update home directory
								node3.memory[memSlot].data = cpu1.cache[index].data; // copy the cached data into the memory to overwrite the "dirty" data
								node3.memory[memSlot].dir[0] = 1; // indicate "shared" now instead of "dirty"
								node3.memory[memSlot].dir[id+1] = 1; // indicate that the current node now has this data as well.

								// load data into the appropriate register
								if(rt == "10001") cpu1.s1 = cpu0.cache[index].data; // update contents of $s1 register with the data from cache
								else if(rt == "10010") cpu1.s2 = cpu0.cache[index].data; // or update contents of $s2 register with the data from cache
								return 135;
							}
		}
	} // end of cpu1

	else cout << "Invalid cpu value on a read request (Must be 0 or 1).\n";
	return -1;
}

// cc-NUMA mem-write protocol 
// will pass in all the nodes as arguments so I can access and update their memory/directory contents
// returns access cost
int Node::mem_write(Node &node0, Node &node1, Node &node2, Node &node3, bool cpu, string rs, string rt, int address) {

	int index = address % 4;
	int tag = address / 4;
	int homeNodeID = address / 16; // I understand it's same value as tag. But useful for readability
	int memSlot = address % 16; // calculate the memory slot of where the word address points to (0-indexed at each individual node)

	if(cpu == 0) {
		if(cpu0.cache[index].tag == tag && cpu0.cache[index].valid == 1) { // data found in local cache (Write-Back policy)
			switch(homeNodeID) { // invalidate the home node and all shared cache copies

				case(0): if(node0.memory[memSlot].dir[0] == 1) { // check if shared memory block
									for(int i = 1; i < 5; ++i) {
										if(node0.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node0.memory[memSlot].dir[i] = 0; // indicate that the node does not contain the up-to-date data anymore if it was shared before
										}
									}	
								 } 
								 node0.memory[memSlot].dir[0] = 2; // update home directory to dirty
								 node0.memory[memSlot].dir[id+1] = 1; // update which node has the dirty information in the directory
								 break;

				case(1): if(node1.memory[memSlot].dir[0] == 1) { // check if shared memory block
									for(int i = 1; i < 5; ++i) {
										if(node1.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node1.memory[memSlot].dir[i] = 0; // indicate that the node does not contain the up-to-date data anymore if it was shared before
										}
									}	
								 } 
								 node1.memory[memSlot].dir[0] = 2; // update home directory to dirty
								 node1.memory[memSlot].dir[id+1] = 1; // update which node has the dirty information in the directory
								 break;

				case(2): if(node2.memory[memSlot].dir[0] == 1) { // check if shared memory block
									for(int i = 1; i < 5; ++i) {
										if(node2.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node2.memory[memSlot].dir[i] = 0; // indicate that the node does not contain the up-to-date data anymore if it was shared before
										}
									}	
								 } 
								 node2.memory[memSlot].dir[0] = 2; // update home directory to dirty
								 node2.memory[memSlot].dir[id+1] = 1; // update which node has the dirty information in the directory
								 break;

				case(3):  if(node3.memory[memSlot].dir[0] == 1) { // check if shared memory block
									for(int i = 1; i < 5; ++i) {
										if(node3.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node0.memory[memSlot].dir[i] = 0; // indicate in the directory that the node does not contain the up-to-date data anymore if it was shared before
										}
									}	
								 } 
								 node3.memory[memSlot].dir[0] = 2; // update home directory to dirty
								 node3.memory[memSlot].dir[id+1] = 1; // update which node has the dirty information in the directory
								 break;
			}
			
			if(rt == "10001") cpu0.cache[index].data = cpu0.s1; // update cache with data from $s1 register
			else if(rt == "10010") cpu0.cache[index].data = cpu0.s2; // or update cache with data from $s2 register
			cpu0.cache[index].valid = 1; // update valid bit and data here because it can be invalidated in the previous switch cases
			return 1; // consumes 1 clock cycle
		}

		else { // data not found in local cache (No-write-allocate policy: only update memory)

			switch(homeNodeID) { // choose the correct node to modify
				case(0): if(node0.memory[memSlot].dir[0] == 1 || node0.memory[memSlot].dir[0] == 2) { // if shared (or dirty) memory block, need to invalidate all the caches that are being shared with
									for(int i = 1; i < 5; ++i) {
										if(node0.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies (Could probably take out this code and create a function out of it to reduce # of lines)
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node0.memory[memSlot].dir[i] = 0; // indicate in the directory that the node does not contain the up-to-date data anymore if it was shared before
										}
									}
									node0.memory[memSlot].dir[0] = 1; // mark as shared (if shared, still stay shared, and if dirty, becomes shared as intended)	
								 } 
								 if(rt == "10001") node0.memory[memSlot].data = cpu0.s1; // update memory with data from $s1 register
								 else if(rt == "10010") node0.memory[memSlot].data = cpu0.s2; // or update memory with data from $s2 register	
								 break;

				case(1): if(node1.memory[memSlot].dir[0] == 1 || node1.memory[memSlot].dir[0] == 2) { // check if shared memory block
									for(int i = 1; i < 5; ++i) {
										if(node1.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node1.memory[memSlot].dir[i] = 0; // indicate in the directory that the node does not contain the up-to-date data anymore if it was shared before
										}
									}
								 	node1.memory[memSlot].dir[0] = 1; // mark as shared (if shared, still stay shared, and if dirty, becomes shared as intended)	
								 } 
								 if(rt == "10001") node1.memory[memSlot].data = cpu0.s1; // update memory with data from $s1 register
								 else if(rt == "10010") node1.memory[memSlot].data = cpu0.s2; // or update memory with data from $s2 register	
								 break;

				case(2): if(node2.memory[memSlot].dir[0] == 1 || node2.memory[memSlot].dir[0] == 2) { // check if shared (or dirty) memory block
									for(int i = 1; i < 5; ++i) {
										if(node2.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node2.memory[memSlot].dir[i] = 0; // indicate in the directory that the node does not contain the up-to-date data anymore if it was shared before
										}
									}
								 	node2.memory[memSlot].dir[0] = 1; // mark as shared (if shared, still stay shared, and if dirty, becomes shared as intended)	
								 } 
								 if(rt == "10001") node2.memory[memSlot].data = cpu0.s1; // update memory with data from $s1 register
								 else if(rt == "10010") node2.memory[memSlot].data = cpu0.s2; // or update memory with data from $s2 register	
								 break;

				case(3): if(node3.memory[memSlot].dir[0] == 1 || node3.memory[memSlot].dir[0] == 2) { // check if shared (or dirty) memory block
									for(int i = 1; i < 5; ++i) {
										if(node3.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node3.memory[memSlot].dir[i] = 0; // indicate in the directory that the node does not contain the up-to-date data anymore if it was shared before
										}
									}	
									node3.memory[memSlot].dir[0] = 1; // mark as shared (if shared, still stay shared, and if dirty, becomes shared as intended)
								 } 
								 if(rt == "10001") node3.memory[memSlot].data = cpu0.s1; // update memory with data from $s1 register
								 else if(rt == "10010") node3.memory[memSlot].data = cpu0.s2; // or update memory with data from $s2 register	
								 break;
			}
			return 100; // update of memory data consumes 100 clock cycles
		}
	} // end cpu0

	else if(cpu == 1) {
		if(cpu1.cache[index].tag == tag && cpu1.cache[index].valid == 1) { // data found in local cache (Write-Back policy)
			switch(homeNodeID) { // invalidate the home node and all shared cache copies

				case(0): if(node0.memory[memSlot].dir[0] == 1) { // check if shared memory block
									for(int i = 1; i < 5; ++i) {
										if(node0.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node0.memory[memSlot].dir[i] = 0; // indicate that the node does not contain the up-to-date data anymore if it was shared before
										}
									}	
								 } 
								 node0.memory[memSlot].dir[0] = 2; // update home directory to dirty
								 node0.memory[memSlot].dir[id+1] = 1; // update which node has the dirty information in the directory
								 break;

				case(1): if(node1.memory[memSlot].dir[0] == 1) { // check if shared memory block
									for(int i = 1; i < 5; ++i) {
										if(node1.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node1.memory[memSlot].dir[i] = 0; // indicate that the node does not contain the up-to-date data anymore if it was shared before
										}
									}	
								 } 
								 node1.memory[memSlot].dir[0] = 2; // update home directory to dirty
								 node1.memory[memSlot].dir[id+1] = 1; // update which node has the dirty information in the directory
								 break;

				case(2): if(node2.memory[memSlot].dir[0] == 1) { // check if shared memory block
									for(int i = 1; i < 5; ++i) {
										if(node2.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node2.memory[memSlot].dir[i] = 0; // indicate that the node does not contain the up-to-date data anymore if it was shared before
										}
									}	
								 } 
								 node2.memory[memSlot].dir[0] = 2; // update home directory to dirty
								 node2.memory[memSlot].dir[id+1] = 1; // update which node has the dirty information in the directory
								 break;

				case(3):  if(node3.memory[memSlot].dir[0] == 1) { // check if shared memory block
									for(int i = 1; i < 5; ++i) {
										if(node3.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node0.memory[memSlot].dir[i] = 0; // indicate in the directory that the node does not contain the up-to-date data anymore if it was shared before
										}
									}	
								 } 
								 node3.memory[memSlot].dir[0] = 2; // update home directory to dirty
								 node3.memory[memSlot].dir[id+1] = 1; // update which node has the dirty information in the directory
								 break;
			}
			
			if(rt == "10001") cpu1.cache[index].data = cpu1.s1; // update cache with data from $s1 register
			else if(rt == "10010") cpu1.cache[index].data = cpu1.s2; // or update cache with data from $s2 register
			cpu1.cache[index].valid = 1; // update valid bit and data here because it can be invalidated in the previous switch cases
			return 1; // consumes 1 clock cycle
		}

		else { // data not found in local cache (No-write-allocate policy: only update memory)

			switch(homeNodeID) { // choose the correct node to modify
				case(0): if(node0.memory[memSlot].dir[0] == 1 || node0.memory[memSlot].dir[0] == 2) { // if shared (or dirty) memory block, need to invalidate all the caches that are being shared with
									for(int i = 1; i < 5; ++i) {
										if(node0.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies (Could probably take out this code and create a function out of it to reduce # of lines)
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node0.memory[memSlot].dir[i] = 0; // indicate in the directory that the node does not contain the up-to-date data anymore if it was shared before
										}
									}
									node0.memory[memSlot].dir[0] = 1; // mark as shared (if shared, still stay shared, and if dirty, becomes shared as intended)	
								 } 
								 if(rt == "10001") node0.memory[memSlot].data = cpu1.s1; // update memory with data from $s1 register
								 else if(rt == "10010") node0.memory[memSlot].data = cpu1.s2; // or update memory with data from $s2 register	
								 break;

				case(1): if(node1.memory[memSlot].dir[0] == 1 || node1.memory[memSlot].dir[0] == 2) { // check if shared memory block
									for(int i = 1; i < 5; ++i) {
										if(node1.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node1.memory[memSlot].dir[i] = 0; // indicate in the directory that the node does not contain the up-to-date data anymore if it was shared before
										}
									}
								 	node1.memory[memSlot].dir[0] = 1; // mark as shared (if shared, still stay shared, and if dirty, becomes shared as intended)	
								 } 
								 if(rt == "10001") node1.memory[memSlot].data = cpu1.s1; // update memory with data from $s1 register
								 else if(rt == "10010") node1.memory[memSlot].data = cpu1.s2; // or update memory with data from $s2 register	
								 break;

				case(2): if(node2.memory[memSlot].dir[0] == 1 || node2.memory[memSlot].dir[0] == 2) { // check if shared (or dirty) memory block
									for(int i = 1; i < 5; ++i) {
										if(node2.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node2.memory[memSlot].dir[i] = 0; // indicate in the directory that the node does not contain the up-to-date data anymore if it was shared before
										}
									}
								 	node2.memory[memSlot].dir[0] = 1; // mark as shared (if shared, still stay shared, and if dirty, becomes shared as intended)	
								 } 
								 if(rt == "10001") node2.memory[memSlot].data = cpu1.s1; // update memory with data from $s1 register
								 else if(rt == "10010") node2.memory[memSlot].data = cpu1.s2; // or update memory with data from $s2 register	
								 break;

				case(3): if(node3.memory[memSlot].dir[0] == 1 || node3.memory[memSlot].dir[0] == 2) { // check if shared (or dirty) memory block
									for(int i = 1; i < 5; ++i) {
										if(node3.memory[memSlot].dir[i] == 1) { // invalidated all shared cache copies
											switch(i) {
												case 1: if(node0.cpu0.cache[index].tag == tag) node0.cpu0.cache[index].valid = 0;
																if(node0.cpu1.cache[index].tag == tag) node0.cpu1.cache[index].valid = 0;
												case 2: if(node1.cpu0.cache[index].tag == tag) node1.cpu0.cache[index].valid = 0;
																if(node1.cpu1.cache[index].tag == tag) node1.cpu1.cache[index].valid = 0;
												case 3: if(node2.cpu0.cache[index].tag == tag) node2.cpu0.cache[index].valid = 0;
																if(node2.cpu1.cache[index].tag == tag) node2.cpu1.cache[index].valid = 0;
												case 4: if(node3.cpu0.cache[index].tag == tag) node3.cpu0.cache[index].valid = 0;
																if(node3.cpu1.cache[index].tag == tag) node3.cpu1.cache[index].valid = 0;
											}
											node3.memory[memSlot].dir[i] = 0; // indicate in the directory that the node does not contain the up-to-date data anymore if it was shared before
										}
									}	
									node3.memory[memSlot].dir[0] = 1; // mark as shared (if shared, still stay shared, and if dirty, becomes shared as intended)
								 } 
								 if(rt == "10001") node3.memory[memSlot].data = cpu1.s1; // update memory with data from $s1 register
								 else if(rt == "10010") node3.memory[memSlot].data = cpu1.s2; // or update memory with data from $s2 register	
								 break;
			}
			return 100; // update of memory data consumes 100 clock cycles
		}
	}

	else cout << "Invalid cpu value on a read request (Must be 0 or 1).\n";
	return -1;
}






