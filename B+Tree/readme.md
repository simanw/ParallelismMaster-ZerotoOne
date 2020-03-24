# Thread-safe B+ Tree

This multi-threaded implementation of B+ Tree utilizes two concurrent APIs: lookup and insertion. 
**********************
### Two concurrency solutions (implemented):
- **Read-Write Lock (fine-grained locks)**
    Based on Crabbing protocol.

    | | Write | Read |
    --|-------|-------
    | **Write**| No | No |
    | **Read** | No | Yes |

    Multiple readers are allowed to acquire Read-lock, while only one writer is allowed to acquire a Write-lock at a time.


- **Version counter**


### Other possible solutions:
- **Read/Write split**
    
    Two B+ Trees: Read-tree & Write-tree

**********
#### Chats that help understand source codes
![](https://github.com/WangSiman-Carol/ParallelismMaster-ZerotoOne/blob/master/B%2BTree/pictures/1.PNG)
![](https://github.com/WangSiman-Carol/ParallelismMaster-ZerotoOne/blob/master/B%2BTree/pictures/2.PNG)
![](https://github.com/WangSiman-Carol/ParallelismMaster-ZerotoOne/blob/master/B%2BTree/pictures/3.PNG)
![](https://github.com/WangSiman-Carol/ParallelismMaster-ZerotoOne/blob/master/B%2BTree/pictures/4.PNG)
![](https://github.com/WangSiman-Carol/ParallelismMaster-ZerotoOne/blob/master/B%2BTree/pictures/5.PNG)
