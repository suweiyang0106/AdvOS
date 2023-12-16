# AdvOS
Two projects are here:
1. Pipe improvement 2. Network driver improvement  

![image](https://github.com/suweiyang0106/AdvOS/blob/main/pipecopyinout.png)  

**Pipe improvement**  

Pipe architecture is shown above, and the differences between modifications are below.  
Original: byte by byte bandwidth + copy 1 byte at once  
Improved: chunk by chunk + copy 4 bytes at once  
Improved2: map physical address from user to kernel directly + write/read 4 bytes at once.  
**Evaluation** (transmit 10MB data from user to kernel):  
using uptime() to evaluate:  
Original/Improved/Improved2(ticks): **337**/**66**/**1**  
Bandwidth:  
Original/Improved/Improved2(MB/tick): **0.03**/**0.15**/**14.49**  

![image](https://github.com/suweiyang0106/AdvOS/blob/main/networkdriverarch.png)  

**Network driver improvement**  
  

