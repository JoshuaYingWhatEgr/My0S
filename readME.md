##  MyOS  #########

### MBR
BIOS执行完成后跳转到内存0x7c00处执行MBR主引导程序

### 保护模式
* 1.准备GTD
* 2.打开地址线A20
* 3.将cr0的PE位值为1进入保护模式

### 获取物理内存容量
在实模式下通过0x15子功能0xE820获取内存信息然后由应用程序挑选处合适的信息处理

### 虚拟内存,启动内存分页机制

* 内存分页

    1.准备页目录表和页表
    
    2.将页表地址写入控制寄存器cr3
    
    3.寄存器cr0的PG值为1
    
### 第一个内核文件kernel

### 中断

