# nullmap
A very simple driver manual mapper based on my older [voidmap](https://github.com/SamuelTulach/voidmap) and CVE-2023-21768 POC by [chompie](https://twitter.com/chompie1337) and [b33f](https://twitter.com/FuzzySec). Because the underlying IoRing post-exploitation memory r/w primite is not handling many consequent reads and writes very well, I've decided to overwrite CR4 to disable SMEP/SMAP to execute the driver mapped in usermode.

