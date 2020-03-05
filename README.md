# ParallelismMaster-ZerotoOne
Dedicated in making future come true: large-scale parallelism. 


Laws of the past, like Moore's Law, are losing power. Energy is the key restraint of microprocessors performance improvements. Large-scale parallelism, customized accelerators, data orchestration are coming future. 

## Start from ZERO!!! 
### This is an old-school multithreading problem: [Bank account transferring](https://github.com/WangSiman-Carol/ParallelismMaster-ZerotoOne/tree/master/Old-school-problem). 
Your program maintains an array of clients with their bank accounts and supports an operation: transfer money from one bank account to another.

You need to ensure all accounts are in a consistent state, which means that negative balance is not allowed, and total amount of all accounts should not change.

### [Producer-consumer problem](https://github.com/WangSiman-Carol/ParallelismMaster-ZerotoOne/tree/master/producer-consumer).
Producer produces tasks that include three actions (insert, delete, lookup), while consumer executes tasks. Tasks are stored in a queue. The elements that are processed in each task are stored in a treeset.

There are four scenerios:
- One producer and one consumer
- One producer and n consumers (n > 1)
- One producer, but produces 100 tasks once; n consumers (n > 1)
