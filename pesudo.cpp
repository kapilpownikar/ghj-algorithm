vector<Bucket> partition(Disk* disk, Mem* mem, pair<uint, uint> left_rel,
                         pair<uint, uint> right_rel) {
	// TODO: implement partition phase
	vector<Bucket> partitions(MEM_SIZE_IN_PAGE - 1, Bucket(disk));

    1. We created a vector of buckets of size MEM_SIZE_IN_PAGE - 1.
    Each bucket gets its own buffer page in memory - what does it mean?
    

    return parititons;
}