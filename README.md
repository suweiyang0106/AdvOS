![image](https://github.com/suweiyang0106/AdvOS/assets/20301839/efc8c797-c74f-433d-884a-b7d0128df345)# AdvOS
Two projects are here:
1. Pipe improvement 2. Network driver improvement
**Pipe improvement**
![image](https://github.com/suweiyang0106/AdvOS/blob/main/pipecopyinout.png)
Pipe architecture is shown above, and the differences between modifications are below.
Original: byte by byte bandwidth + copy 1 byte at once
Improved: chunk by chunk + copy 4 bytes at once
Improved2: map physical address from user to kernel directly + write/read 4 bytes at once.
**Evaluation** (transmit 10MB data from user to kernel):
uptime()
Original: **337 ticks**
Improved: 66 ticks
Improved2: **1 ticks**
Bandwidth:
Original: **0.03MB/tick**
Improved:0.15MB/tick
Improved2: 1**4.49MB/tick**

