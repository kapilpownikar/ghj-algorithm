#include "Join.hpp"

#include <vector>

using namespace std;

/*
 * Input: Disk, Memory, Disk page ids for left relation, Disk page ids for right relation
 * Output: Vector of Buckets of size (MEM_SIZE_IN_PAGE - 1) after partition
 */
vector<Bucket> partition(Disk* disk, Mem* mem, pair<uint, uint> left_rel,
                         pair<uint, uint> right_rel) {

	// TODO: implement partition phase
	vector<Bucket> partitions(MEM_SIZE_IN_PAGE - 1, Bucket(disk));
	uint input_page_id = MEM_SIZE_IN_PAGE - 1; // input buffer is the 16th page in pages[]; is a mem_page_id
	
	// partition relation R (left relation)
	for(uint page_id = left_rel.first; page_id < left_rel.second; ++page_id){

		mem->loadFromDisk(disk, page_id, input_page_id); // loads the page from disk into input buffer
		Page* input_page = mem->mem_page(input_page_id); // referencing the input page

		for(uint record_id = 0; record_id < input_page->size(); ++record_id){

			Record curr_record = input_page->get_record(record_id);
			uint hash_record = curr_record.partition_hash() % (MEM_SIZE_IN_PAGE - 1);

			// add record into one of the input buffers in memory
			if(mem->mem_page(hash_record)->full()){
				uint disk_page_id = mem->flushToDisk(disk, hash_record);
				partitions[hash_record].add_left_rel_page(disk_page_id);
			} // flushes the designated buffer to disk if full

			mem->mem_page(hash_record)->loadRecord(curr_record); // loads record into page
		}
		
		mem->mem_page(input_page_id)->reset(); // reset mem buffer with the input page so it can take the next input
	}

	// flush all pages in memory to disk before starting the right relation
	for(uint mem_page_id = 0; mem_page_id < MEM_SIZE_IN_PAGE; ++mem_page_id){
		if(!mem->mem_page(mem_page_id)->empty()){
			uint disk_page_id = mem->flushToDisk(disk, mem_page_id);
			partitions[mem_page_id].add_left_rel_page(disk_page_id);
		}
	}

	// partition relation S (right relation)
	for(uint page_id = right_rel.first; page_id < right_rel.second; ++page_id){

		mem->loadFromDisk(disk, page_id, input_page_id);
		Page* input_page = mem->mem_page(input_page_id); 

		for(uint record_id = 0; record_id < input_page->size(); ++record_id){

			Record curr_record = input_page->get_record(record_id);
			uint hash_record = curr_record.partition_hash() % (MEM_SIZE_IN_PAGE - 1);

			// we put this record into one of the input buffers in memory
			if(mem->mem_page(hash_record)->full()){
				uint disk_page_id = mem->flushToDisk(disk, hash_record);
				partitions[hash_record].add_right_rel_page(disk_page_id);
			} // first flushes the designated buffer to disk if it is full

			mem->mem_page(hash_record)->loadRecord(curr_record); // loads record into page
		}

		mem->mem_page(input_page_id)->reset();
	}

	// flush all pages in memory to disk after right relation
	for(uint mem_page_id = 0; mem_page_id < MEM_SIZE_IN_PAGE; ++mem_page_id){
		if(!mem->mem_page(mem_page_id)->empty()){
			uint disk_page_id = mem->flushToDisk(disk, mem_page_id);
			partitions[mem_page_id].add_right_rel_page(disk_page_id);
		}
	}

	// Note: MEM_SIZE_IN_PAGE or MEM_SIZE_IN_PAGE - 1 can be used for the cleanup loop 
	// since the R & S loops ensure the input page is reset for each loop
	return partitions; 
}

/*
 * Input: Disk, Memory, Vector of Buckets after partition
 * Output: Vector of disk page ids for join result
 */
vector<uint> probe(Disk* disk, Mem* mem, vector<Bucket>& partitions) {
	// TODO: implement probe phase
	vector<uint> disk_pages; // placeholder

    // Initialize input and output buffer pages
    uint input_buffer_idx = MEM_SIZE_IN_PAGE - 2; 
    uint output_buffer_idx = MEM_SIZE_IN_PAGE - 1;
    mem->mem_page(input_buffer_idx)->reset();
    mem->mem_page(output_buffer_idx)->reset();

	// loop through each partition
	for(uint partition_id = 0; partition_id < partitions.size(); ++partition_id){

		vector<uint> left_rel = partitions[partition_id].get_left_rel();
		vector<uint> right_rel = partitions[partition_id].get_right_rel();

		// Determine the smaller relation for each partition
		// If true, left rel is smaller in partition
		bool smaller_rel = (partitions[partition_id].num_left_rel_record < partitions[partition_id].num_right_rel_record);
		if(smaller_rel){
		
			// Use left partition for inputs
			for(uint left_rel_id = 0; left_rel_id < left_rel.size(); ++left_rel_id){
				// load each page into input bucket
				mem->loadFromDisk(disk, left_rel[left_rel_id], input_buffer_idx);
				Page* input_page = mem->mem_page(input_buffer_idx);
				
				// loops through records in this page and hashes into correct bucket
				for(uint record_id = 0; record_id < input_page->size(); ++record_id){
					Record curr_record = input_page->get_record(record_id);
					uint hash_record = curr_record.probe_hash() % (MEM_SIZE_IN_PAGE - 2); // ensures record hashed to bucket idx 0-13
					mem->mem_page(hash_record)->loadRecord(curr_record); 
					// NOTE: since the smaller relation is guarenteed to fit in memory, we do not need to recursively partition
				}
				mem->mem_page(input_buffer_idx)->reset(); // resets input buffer
			}
			
			// loop through right relation
			for(uint right_rel_id = 0; right_rel_id < right_rel.size(); ++right_rel_id){

				mem->loadFromDisk(disk, right_rel[right_rel_id], input_buffer_idx);
				Page* match_page = mem->mem_page(input_buffer_idx); // page to match

				for(uint record_id = 0; record_id < match_page->size(); ++record_id){
					Record match_record = match_page->get_record(record_id);
					uint hash_record = match_record.probe_hash() % (MEM_SIZE_IN_PAGE - 2);

					for(uint iter = 0; iter < mem->mem_page(hash_record)->size(); ++iter){

						Record curr_record = mem->mem_page(hash_record)->get_record(iter);
						if(match_record == curr_record){

							// check if the output buffer is full, if full flush to disk and add disk page id
							if(mem->mem_page(output_buffer_idx)->full()){
								uint disk_page_id = mem->flushToDisk(disk, output_buffer_idx);
								disk_pages.push_back(disk_page_id);
							}
							mem->mem_page(output_buffer_idx)->loadRecord(match_record);
							mem->mem_page(output_buffer_idx)->loadRecord(curr_record);
							if(mem->mem_page(output_buffer_idx)->full()){
								uint disk_page_id = mem->flushToDisk(disk, output_buffer_idx);
								disk_pages.push_back(disk_page_id);
							}
						}
					// check if the output buffer is full, if full flush to disk and add disk page id
						if(mem->mem_page(output_buffer_idx)->full()){
							uint disk_page_id = mem->flushToDisk(disk, output_buffer_idx);
							disk_pages.push_back(disk_page_id);
						}
					}

					if(mem->mem_page(output_buffer_idx)->full()){
					uint disk_page_id = mem->flushToDisk(disk, output_buffer_idx);
					disk_pages.push_back(disk_page_id);
					}
				}
				mem->mem_page(input_buffer_idx)->reset();	
			}
			// if(!mem->mem_page(output_buffer_idx)->empty()){
			// uint disk_page_id = mem->flushToDisk(disk, output_buffer_idx);
			// disk_pages.push_back(disk_page_id);
			// }
			mem->mem_page(input_buffer_idx)->reset();
		}
		else{
			// Use right partitions for input
			for(uint right_rel_id = 0; right_rel_id < right_rel.size(); ++right_rel_id){
				// load each page into input bucket
				mem->loadFromDisk(disk, right_rel[right_rel_id], input_buffer_idx);
				Page* input_page = mem->mem_page(input_buffer_idx);
				
				// loops through records in this page and hashes into correct bucket
				for(uint record_id = 0; record_id < input_page->size(); ++record_id){
					Record curr_record = input_page->get_record(record_id);
					uint hash_record = curr_record.probe_hash() % (MEM_SIZE_IN_PAGE - 2); // ensures record hashed to bucket idx 0-13
					mem->mem_page(hash_record)->loadRecord(curr_record); 
					// NOTE: since the smaller relation is guarenteed to fit in memory, we do not need to recursively partition
				}
				mem->mem_page(input_buffer_idx)->reset(); // resets input buffer
			}
			
			// loop through left relation
			for(uint left_rel_id = 0; left_rel_id < left_rel.size(); ++left_rel_id){

				mem->loadFromDisk(disk, left_rel[left_rel_id], input_buffer_idx);
				Page* match_page = mem->mem_page(input_buffer_idx); // page to match

				for(uint record_id = 0; record_id < match_page->size(); ++record_id){
					Record match_record = match_page->get_record(record_id);
					uint hash_record = match_record.probe_hash() % (MEM_SIZE_IN_PAGE - 2);

					for(uint iter = 0; iter < mem->mem_page(hash_record)->size(); ++iter){

						Record curr_record = mem->mem_page(hash_record)->get_record(iter);
						if(match_record == curr_record){

							// check if the output buffer is full, if full flush to disk and add disk page id
							if(mem->mem_page(output_buffer_idx)->full()){
								uint disk_page_id = mem->flushToDisk(disk, output_buffer_idx);
								disk_pages.push_back(disk_page_id);
							}
							mem->mem_page(output_buffer_idx)->loadRecord(match_record);
							mem->mem_page(output_buffer_idx)->loadRecord(curr_record);
							if(mem->mem_page(output_buffer_idx)->full()){
								uint disk_page_id = mem->flushToDisk(disk, output_buffer_idx);
								disk_pages.push_back(disk_page_id);
							}
						}
					}
				}
				mem->mem_page(input_buffer_idx)->reset();
			}
			// if(!mem->mem_page(output_buffer_idx)->empty()){
			// uint disk_page_id = mem->flushToDisk(disk, output_buffer_idx);
			// disk_pages.push_back(disk_page_id);
			// }
			mem->mem_page(input_buffer_idx)->reset();	
		}

		// reset all buffers in memory after each partition run
		for(uint mem_page_id = 0; mem_page_id < MEM_SIZE_IN_PAGE - 1; ++mem_page_id){
			mem->mem_page(mem_page_id)->reset();
		}
	}

	if(!mem->mem_page(output_buffer_idx)->empty()){
		uint disk_page_id = mem->flushToDisk(disk, output_buffer_idx);
		disk_pages.push_back(disk_page_id);
	}
    
	return disk_pages;
}
