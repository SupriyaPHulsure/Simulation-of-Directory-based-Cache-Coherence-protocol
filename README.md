# Simulation-of-Directory-based-Cache-Coherence-protocol

In computer architecture, cache coherence is the uniformity of shared resource data that ends up stored in multiple local caches. When clients in a system maintain caches of a common memory resource, problems may arise with incoherent data, which is particularly the case with CPUs in a multiprocessing system.

In a shared memory multiprocessor system with a separate cache memory for each processor, it is possible to have many copies of shared data: one copy in the main memory and one in the local cache of each processor that requested it. When one of the copies of data is changed, the other copies must reflect that change. Cache coherence is the discipline which ensures that the changes in the values of shared operands(data) are propagated throughout the system in a timely fashion


In a directory-based coherence system, the data being shared is placed in a common directory that maintains the coherence between caches. The directory acts as a filter through which the processor must ask permission to load an entry from the primary memory to its cache. When an entry is changed, the directory either updates or invalidates the other caches with that entry.

Directory-based cache consistency protocols have the potential to allow shared-memory
multiprocessors to scale to a large number of processors

### Getting Started
This project is implemented in C language. 


### Prerequisites

You need C compiler to excute this project.

 
### Running the tests
```
make clean

make
```
```
./Cache_Coherence TraceFileName
```
If debug version:
```
./Cache_Coherence TraceFileName -d
```
