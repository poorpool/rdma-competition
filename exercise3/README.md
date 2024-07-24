# Yixiao Chen's Team (HUST-PDSL) exercise 2

Captain email: chenyixiao@foxmail.com (backup email chenyixiao@hust.edu.cn)

## Implementation

We first create the necessary structures such as `ucp_context` in the main function, then exchange information using MPI_Send/Recv, create the UCP endpoint, and put data. We use `ucp_put_nbx` as the representative RMA operation.

Considering that the task requires us to measure the time taken to transfer data from one machine to another, we use `ucp_put_nbx` to send messages of various sizes from the client. After sending 1000 messages for each size, we use ucp_ep_flush_nbx to flush them.

Once all messages are sent, we use `ucp_tag_send_nbx` to notify the server that it can stop to call `ucp_worker_progress` infinitely. Then, we calculate the latency of each `ucp_put_nbx` operation.

In practice, we measure the average latency of sending 1000 messages. To strictly test the latency of each individual message, simply place the `blocking_ep_flush` check inside the ITERS loop.

## Results
