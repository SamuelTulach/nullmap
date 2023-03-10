# nullmap
A very simple driver manual mapper based on my older [voidmap](https://github.com/SamuelTulach/voidmap) and CVE-2023-21768 POC by [chompie](https://twitter.com/chompie1337) and [b33f](https://twitter.com/FuzzySec). Because the underlying IoRing post-exploitation memory r/w primitive is not handling many consequent reads and writes very well, I've decided to overwrite CR4 to disable SMEP/SMAP to execute the driver mapped in usermode. Tested on Windows 11 22H2 (22621.525). 

Possible problems:
- Manual mapped driver will be in a pool allocated by ExAllocatePool. If you want to use this for anything more serious you should consider finding a better way of memory allocation so it can't be dumped so easily.
- There is no easy way to read the original cr4 value which means that I had to hardcode the value that was there on my system. While it should be the same for most modern CPUs, you should still double-check that the value is correct.
- I've hard-coded offset to NtGdiGetEmbUFI since there is no easy way to sigscan it, which means that you will have to update this offset for your specific Windows build

Video:
[![video](https://img.youtube.com/vi/qdAZ8mTsTrc/0.jpg)](https://www.youtube.com/watch?v=qdAZ8mTsTrc)
