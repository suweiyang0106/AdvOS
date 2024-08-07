# AdvOS
Two projects are here:
1. Pipe improvement(in Ringbuffer)
2. Network driver improvement(in xv6-labs-2022)  

**Pipe improvement**  
  
![image](https://github.com/suweiyang0106/AdvOS/blob/main/pipecopyinout.png)  
Pipe architecture is shown above, and the differences between modifications are below.  
Original: byte by byte bandwidth + copy 1 byte at once  
Improved: chunk by chunk + copy 4 bytes at once  
Improved2: map physical address from user to kernel directly + write/read 4 bytes at once.  
**Evaluation** (transmit 10MB data from user to kernel):  
using uptime() to evaluate:  
Original/Improved/Improved2(ticks): **337**/**66**/**1**  
Bandwidth:  
Original/Improved/Improved2(MB/tick): **0.03**/**0.15**/**14.49**  
  
**Network driver improvement**  
  
![image](https://github.com/suweiyang0106/AdvOS/blob/main/networkdriverarch.png)  
Network transmit architecture is shown above. User needs to allocate memory and copy the message into kernel address,  
which causes a burden due to increased messages. Hence, there is an improvement space.  
Original: write message chunk by chunk to kernel  
Improved: map physical address from user to kernel directly  
**Evaluation** (single process test transmit 660 bytes):  
Original/Improved(ticks): **19**/**8**   (shown below)  
**Note: Multiprocess test took more time to transmit. I am not sure if that is from increased user activities.**  
Original  
![image](https://github.com/suweiyang0106/AdvOS/blob/main/DriverOriginal.png)  
Improved  
![image](https://github.com/suweiyang0106/AdvOS/blob/main/Driverimproved.png)  
