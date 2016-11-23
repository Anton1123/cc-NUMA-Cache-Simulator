/*
	Simulation of cc-NUMA architecture (DASH machine) with directory-
	based cache coherence control (using write-invalidate protocol).

	Created a structure for the cache line (valid bit, tag and data fields).
	Created a CPU object that will contain the two registers and the cache.

*/

#include <iostream>
#include <fstream>
#include <string>

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
  public:
		CPU cpu0;
		CPU cpu1;
		
    memLine memory[16];
  	Node(int);
		void display();
	
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

// Displays the contents of a Node in binary ******** Need to convert from int to binary.
void Node::display() {
	cout << "Node" << id << endl;
	cout << "---------------------------------\n";
	cout << "***CPU0***\n";
	cout << "S1: " << cpu0.s1 << endl;
	cout << "S2: " << cpu0.s2 << endl;
	cout << "Cache-0" << endl;
	
	for(int i = 0; i < 4; ++i) 
		cout << i << ": " << cpu0.cache[i].valid << " " << cpu0.cache[i].tag << " " << cpu0.cache[i].data << " " << endl;

	cout << "***CPU1***\n";
	cout << "S1: " << cpu1.s1 << endl;
	cout << "S2: " << cpu1.s2 << endl;
	cout << "Cache-1" << endl;
	
	for(int i = 0; i < 4; ++i) 
		cout << i << ": " << cpu1.cache[i].valid << " " << cpu1.cache[i].tag << " " << cpu1.cache[i].data << " " << endl;
	
	cout << "***Memory***\n"; 
	for(int i = 0; i < 16; ++i) {
		cout << i + id*16 << ": " << memory[i].data << " ";
		for(int j = 0; j < 5; ++j)
			cout << memory[i].dir[j] << " ";
		cout << endl;
	}
	cout << endl;
}

// Main program
int main(int argc, char *argv[]) {
	ifstream stream(argv[1]);
	string line;

	while (getline(stream, line)) {
		cout << line << endl;
	}

	Node node0(0);
	node0.display();
	Node node1(1);
	node1.display();
	Node node2(2);
	node2.display();
	Node node3(3);
	node3.display();

	return 0;
}
