# cc-NUMA-Cache-Simulator

Simulation of cc-NUMA architecture (DASH machine) with directory-
based cache coherence control (using write-invalidate protocol).
	
Hardware description:
- System consists of 4 MIPS-based SMP nodes, each of which consists of: 2 scalar processors with a local cache each, 1 memory block, 1 directory.
- Cache is direct-mapped and uses Write-Back when write hit and no-write-allocate when write miss; Cache size (data only) is 4 words and a cache line (and memory line) size is 1 word (32 bits).
- Memory is globally addressed and the total memory size in the system is 64 words (16 words in each node).
- Physical address is 6 bits (2 bits for index, 4 bits for tag).
- Each directory also consists of 16 entries, one for each line (1 word) in the node memory.

Input/Output:
- Input is given as machine code in binary (stored as a text file in the local directory) which consists of MIPS 32bit instructions (for purpses of simulation we only consider load and store instructions).
- After executing each instruction, the simulator will display each node's cache/memory/directory contents (in binary) and the total and average accessing costs.
- Accessing costs are as follows:
	-- accessing processor's cache - 1 clock
  -- accessing other cache in the local node - 30 clocks
  -- accessing home memory/directory - 100 clocks
  -- accessing cache in the remote node - 135 clocks

Initialization:
Initially, all caches are empty and their valid bits are 0's (invalid);
Local registers ($s1, $s2) in each processors are filled with 0's;
Memory contents are filed with its address number plus 5. (e.g. mem[address] <- address + 5)

cc-NUMA protocol used in Dash:
Memory Read
1. Search local cache (local to each processor);
	- Use MOD function for computing the cache address, and check valid bit and the tag.
	- If found, access it (load it into register; it consumes 1 clock cycle)
	- Else, go to step2.
2. Search another cache in the local node;
  - Do the same as in step1.
	- If found, access it (load it into the cache & register; it consumes 30 clock cycles).
  - Else, go to step3.
3. Search the home node's memory directory;
	- If directory indicates "uncaches" or "shared" (means home memory has most recent or clean data), access it (load it into the local cache and register; it consumes 100 clock cycles)
  - else "dirty", go to step 4.
4. Search all caches in the dirty node (use MOD function for the cache address);
	- this consumes 135 clock cycles.

Memory Write:
1. Search the local cache (local to each processor), i.e.,  compute the cache address, check valid bit and tag;
	- If found, get the exclusive right to access it first (using hoe directory, invalidate all the cache copies if shared), and then update it with the content of the register (it consumes 1 clock cycle; we ignore all other overheads).
  - Home directory is updated with "dirty".
  - Else, go to step2.
2. Update the home node memory with the content off the refister (it consumes 100 clock cyles; we ignore all other overheads).
  - If directory indicates:
     "uncached" -> again, "uncached".
     "shared" -> invalidate all shared copies: keep "shared".
     "dirty" -> invalidate the dirty cache copy, "shared" now.
