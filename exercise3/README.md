# Yixiao Chen's Team (HUST-PDSL) exercise 2

Captain email: chenyixiao@foxmail.com (backup email chenyixiao@hust.edu.cn)

## Implementation

We first create the necessary structures such as `ucp_context` in the main function, then exchange information using MPI_Send/Recv, create the UCP endpoint, and put data. We use `ucp_put_nbx` as the representative RMA operation.

Considering that the task requires us to measure the time taken to transfer data from one machine to another, we use `ucp_put_nbx` to send messages of various sizes from the client. After sending 1000 messages for each size, we use ucp_ep_flush_nbx to flush them.

Once all messages are sent, we use `ucp_tag_send_nbx` to notify the server that it can stop to call `ucp_worker_progress` infinitely. Then, we calculate the latency of each `ucp_put_nbx` operation.

In practice, we measure the average latency of sending 1000 messages. To strictly test the latency of each individual message, simply place the `blocking_ep_flush` check inside the ITERS loop.

## Results

```
module load gcc hpcx
mpirun -np 2 --host helios019,helios020 -mca pml ucx -mca btl ^vader,tcp,openib,uct -x UCX_TLS=rc pingpong
```

Mean latency of 1000 overlapping messages:

```
[rdmaworkshop01@helios019 cyx_exercise]$ mpirun -np 2 --host helios019,helios020 -mca pml ucx -mca btl ^vader,tcp,openib,uct -x UCX_TLS=rc pingpong
8       0.11    microseconds
16      0.10    microseconds
32      0.15    microseconds
64      0.14    microseconds
128     0.16    microseconds
256     0.14    microseconds
512     0.15    microseconds
1024    0.22    microseconds
2048    0.26    microseconds
4096    0.34    microseconds
8192    0.65    microseconds
16384   1.29    microseconds
32768   2.56    microseconds
65536   5.14    microseconds
131072  10.35   microseconds
262144  20.65   microseconds
524288  41.26   microseconds
1048576 82.50   microseconds
2097152 165.04  microseconds
4194304 330.01  microseconds
8388608 659.83  microseconds
```

Mean latency of 1000 non-overlapping messages:

```
[rdmaworkshop01@helios019 cyx_exercise]$ mpirun -np 2 --host helios019,helios020 -mca pml ucx -mca btl ^vader,tcp,openib,uct -x UCX_TLS=rc pingpong
8       1.42    microseconds
16      1.43    microseconds
32      1.46    microseconds
64      1.46    microseconds
128     1.50    microseconds
256     1.64    microseconds
512     1.71    microseconds
1024    1.82    microseconds
2048    2.01    microseconds
4096    2.65    microseconds
8192    3.37    microseconds
16384   4.69    microseconds
32768   6.02    microseconds
65536   8.54    microseconds
131072  13.39   microseconds
262144  23.09   microseconds
524288  42.47   microseconds
1048576 85.00   microseconds
2097152 166.66  microseconds
4194304 333.83  microseconds
8388608 661.82  microseconds
```