# Yixiao Chen's Team (HUST-PDSL) exercise 2

Captain email: chenyixiao@foxmail.com (backup email chenyixiao@hust.edu.cn)

## Implementation

1. Set max_inline_data for QP.
2. Allocate a page-aligned, shorter-length data buffer for writing.
3. Client sends tx_depth IBV_WR_RDMA_WRITEs to server, then receive one IBV_WR_SEND from server. Loop until client finishs its send.

## Outputs

```
[rdmaworkshop01@helios017 cyx_exercise]$ ./server -s 4096 -m 4096 -r 500 -n 1000
[rdmaworkshop01@helios018 cyx_exercise]$ ./client -s 4096 -m 4096 -r 500 -n 1000 helios017
1       0.0046  GiB/s
2       0.0105  GiB/s
4       0.0208  GiB/s
8       0.0444  GiB/s
16      0.0870  GiB/s
32      0.1778  GiB/s
64      0.3556  GiB/s
128     0.7111  GiB/s
256     1.4222  GiB/s
512     2.7676  GiB/s
1024    5.6264  GiB/s
2048    10.4490 GiB/s
4096    12.0117 GiB/s
8192    12.4310 GiB/s
16384   12.7701 GiB/s
32768   12.8754 GiB/s
65536   12.8881 GiB/s
131072  12.7726 GiB/s
```