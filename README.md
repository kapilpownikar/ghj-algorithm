To Make:
main.cpp: this file loads the text files and emulates the whole process of GHJ. It also outputs the GHJ result.
Makefile: this file allows you to compile and run a test run of GHJ.
left_rel.txt, right_rel.txt: these two sample text files store all the data records for a left and right relation, which you can use for testing. 

For simplicity, each line in the text file serves as one data record. The data records in the text files are formatted as:
                              
                              key1 data1
                              key2 data2
                              key3 data3
                              ... ...

To build the project and run the executable file, use the Makefile, 
where left_rel.txt and right_rel.txt represent the two text file names that contain all the data records for joining two relations.

                  $ make
                  $ ./GHJ left_rel.txt right_rel.txt

To remove all extraneous files, run
        
                  $ make clean

<img width="544" alt="image" src="https://github.com/kapilpownikar/ghj-algorithm/assets/93685855/0f19a5ed-424e-4cfa-a7a6-1e0634a4fe14">

