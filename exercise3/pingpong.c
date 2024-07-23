#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucp/api/ucp.h>
#include <ucs/type/status.h>

#define BUFFER_SIZE (10LL * 1024 * 1024) // ucp_put_nbx/ucp_get_nbx max size
#define ITERS (1000)

int mpi_rank;
int mpi_size;

ucp_context_h ucp_context;
ucp_worker_h ucp_worker;
ucp_address_t *address;         // my address
size_t address_length;          // my address_length
ucp_address_t *remote_address;  // remote address
size_t remote_address_length;   // remote address_length
char *my_buffer;                // my data buffer
uint64_t remote_buffer;         // remote data buffer
void *rkey_buffer;              // my rkey buffer
size_t rkey_buffer_size;        // my rkey_buffer_size
void *remote_rkey_buffer;       // remote rkey buffer
size_t remote_rkey_buffer_size; // remote rkey_buffer_size

void send_callback(void *request, ucs_status_t status, void *user_data) {
  ucp_request_free(request);
}

int client_function() {
  printf("client_function!\n");
  // TODO
  ucs_status_t status;

  // send client address
  uint64_t tmp_buf = (uint64_t)my_buffer;
  MPI_Send(&address_length, 1, MPI_UNSIGNED_LONG, 1, 0, MPI_COMM_WORLD);
  MPI_Send(address, address_length, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
  MPI_Send(&rkey_buffer_size, 1, MPI_UNSIGNED_LONG, 1, 0, MPI_COMM_WORLD);
  MPI_Send(rkey_buffer, rkey_buffer_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
  MPI_Send(&tmp_buf, 1, MPI_UNSIGNED_LONG, 1, 0, MPI_COMM_WORLD);
  printf("client send finished\n");
  // get server address
  MPI_Recv(&remote_address_length, 1, MPI_UNSIGNED_LONG, 1, 0, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);
  remote_address = (ucp_address_t *)malloc(remote_address_length);
  MPI_Recv(remote_address, remote_address_length, MPI_BYTE, 1, 0,
           MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(&remote_rkey_buffer_size, 1, MPI_UNSIGNED_LONG, 1, 0, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);
  remote_rkey_buffer = malloc(remote_rkey_buffer_size);
  MPI_Recv(remote_rkey_buffer, remote_rkey_buffer_size, MPI_BYTE, 1, 0,
           MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(&remote_buffer, 1, MPI_UNSIGNED_LONG, 1, 0, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);
  printf("client recv finished\n");

  // create ep
  ucp_ep_h ep;
  ucp_ep_params_t ep_params;
  memset(&ep_params, 0, sizeof(ep_params));
  ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
  ep_params.address = remote_address;
  status = ucp_ep_create(ucp_worker, &ep_params, &ep);
  if (status != UCS_OK) {
    fprintf(stderr, "ucp_ep_create failed\n");
    return 1;
  }
  printf("created ep on client!\n");

  // unpack remote rkey
  ucp_rkey_h remote_rkey;
  status = ucp_ep_rkey_unpack(ep, remote_rkey_buffer, &remote_rkey);
  if (status != UCS_OK) {
    fprintf(stderr, "ucp_ep_rkey_unpack failed\n");
    return 1;
  }

  // Send data to server
  ucp_request_param_t request_param;
  memset(&request_param, 0, sizeof(request_param));

  ucs_status_ptr_t status_ptr;
  for (size_t size = 8; size <= BUFFER_SIZE; size *= 2) {
    double start_time = MPI_Wtime();
    for (int i = 0; i < ITERS; i++) {
      status_ptr = ucp_put_nbx(ep, my_buffer, size, remote_buffer, remote_rkey,
                               &request_param);
      if (UCS_PTR_STATUS(status_ptr) == UCS_INPROGRESS) {
        ucp_request_free(status_ptr); //  releases the non-blocking request back
                                      //  to the library and continue handling
      } else if (UCS_PTR_IS_ERR(status_ptr)) {
        fprintf(stderr, "ucp_put_nbx failed\n");
        return 1;
      }
    }
    status_ptr = ucp_ep_flush_nbx(ep, &request_param);
    if (status_ptr == NULL) {
      ; // UCS_OK
    } else if (UCS_PTR_IS_ERR(status_ptr)) {
      return UCS_PTR_STATUS(status_ptr);
    } else {
      ucs_status_t sta;
      do {
        ucp_worker_progress(ucp_worker);
        sta = ucp_request_check_status(status_ptr);
      } while (sta == UCS_INPROGRESS);
      ucp_request_free(status_ptr);
      return status;
    }
    double end_time = MPI_Wtime();
    printf("%zu\t%.4f\tmicroseconds\n", size, (end_time - start_time) / ITERS);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  // Cleanup
  ucp_rkey_destroy(remote_rkey);
  free(remote_address);
  free(remote_rkey_buffer);
  return 0;
}

int server_function() {
  printf("server_function!\n");
  ucs_status_t status;

  // get client address
  MPI_Recv(&remote_address_length, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);
  remote_address = (ucp_address_t *)malloc(remote_address_length);
  MPI_Recv(remote_address, remote_address_length, MPI_BYTE, 0, 0,
           MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(&remote_rkey_buffer_size, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);
  remote_rkey_buffer = malloc(remote_rkey_buffer_size);
  MPI_Recv(remote_rkey_buffer, remote_rkey_buffer_size, MPI_BYTE, 0, 0,
           MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(&remote_buffer, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);
  printf("server recv finished\n");
  // send server address
  uint64_t tmp_buf = (uint64_t)my_buffer;
  MPI_Send(&address_length, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);
  MPI_Send(address, address_length, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
  MPI_Send(&rkey_buffer_size, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);
  MPI_Send(rkey_buffer, rkey_buffer_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
  MPI_Send(&tmp_buf, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);
  printf("server send finished\n");
  // create ep
  ucp_ep_h ep;
  ucp_ep_params_t ep_params;
  memset(&ep_params, 0, sizeof(ep_params));
  ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
  ep_params.address = remote_address;
  status = ucp_ep_create(ucp_worker, &ep_params, &ep);
  if (status != UCS_OK) {
    fprintf(stderr, "ucp_ep_create failed\n");
    return 1;
  }
  printf("created ep on server!\n");

  // TODO(cyx): some operations here...

  MPI_Barrier(MPI_COMM_WORLD);

  // Cleanup
  ucp_ep_destroy(ep);
  free(remote_address);
  free(remote_rkey_buffer);
  return 0;
}

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
  if (mpi_size != 2) {
    fprintf(stderr, "mpi_size should be 2! current %d\n", mpi_size);
    return 1;
  }

  ucs_status_t status;

  // init ucp_context

  ucp_params_t ucp_params;
  memset(&ucp_params, 0, sizeof(ucp_params));
  ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
  ucp_params.features = UCP_FEATURE_RMA; // exercise 3 only need RMA

  ucp_config_t *config;
  status = ucp_config_read(NULL, NULL, &config);
  if (status != UCS_OK) {
    fprintf(stderr, "ucp_config_read failed\n");
    return 1;
  }

  status = ucp_init(&ucp_params, config, &ucp_context);
  if (status != UCS_OK) {
    fprintf(stderr, "ucp_init failed\n");
    return 1;
  }
  ucp_config_release(config);

  // create worker
  ucp_worker_params_t worker_params;
  memset(&worker_params, 0, sizeof(worker_params));
  worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
  worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
  status = ucp_worker_create(ucp_context, &worker_params, &ucp_worker);
  if (status != UCS_OK) {
    fprintf(stderr, "ucp_worker_create failed\n");
    return 1;
  }

  // get address for later exchange
  // when not in mpi, we can try ucp_listener_t
  ucp_worker_get_address(ucp_worker, &address, &address_length);

  // allocate buffer and register
  my_buffer = (char *)malloc(BUFFER_SIZE);
  ucp_mem_map_params_t mem_map_params;
  memset(&mem_map_params, 0, sizeof(mem_map_params));
  mem_map_params.field_mask =
      UCP_MEM_MAP_PARAM_FIELD_ADDRESS | UCP_MEM_MAP_PARAM_FIELD_LENGTH;
  mem_map_params.address = my_buffer;
  mem_map_params.length = BUFFER_SIZE;
  ucp_mem_h memh;
  status = ucp_mem_map(ucp_context, &mem_map_params, &memh);
  if (status != UCS_OK) {
    fprintf(stderr, "ucp_mem_map failed\n");
    return 1;
  }

  // pack registered memory for exchange
  status = ucp_rkey_pack(ucp_context, memh, &rkey_buffer, &rkey_buffer_size);
  if (status != UCS_OK) {
    fprintf(stderr, "ucp_rkey_pack failed\n");
    return 1;
  }

  if (mpi_rank == 0) { // client
    if (client_function() != 0) {
      fprintf(stderr, "client_function failed\n");
      return 1;
    }
  } else { // server
    if (server_function() != 0) {
      fprintf(stderr, "server_function failed\n");
      return 1;
    }
  }

  // clean
  ucp_rkey_buffer_release(rkey_buffer);
  ucp_worker_release_address(ucp_worker, address);
  ucp_mem_unmap(ucp_context, memh);
  free(my_buffer);

  ucp_worker_destroy(ucp_worker);
  ucp_cleanup(ucp_context);

  MPI_Finalize();
  return 0;
}
