#include "rdtp.h"
#include "rdtp_server.h"

extern unsigned char global_send_buf[MAX_BUF_SIZE]; 
extern unsigned char global_recv_buf[MAX_BUF_SIZE];

static pthread_t thread[2];
static pthread_cond_t cond_work = PTHREAD_COND_INITIALIZER; 
static pthread_cond_t cond_done = PTHREAD_COND_INITIALIZER; 
static pthread_mutex_t mutex_work = PTHREAD_MUTEX_INITIALIZER; 
static pthread_mutex_t mutex_done = PTHREAD_MUTEX_INITIALIZER; 

/**
 * State variable
 *  -1  No connection
 *  0   Accept block - waiting SYN
 *  1   Waiting for ACK / DATA
 *  2   ACK / DATA received ... Main phase
 *  3   Start 4WHS
 *  4   Waiting for ACK of FIN
 */
static int state;
static int seq; 

static void sender(struct rdtp_argv *argv) {
    int sockfd = argv -> sockfd;
    struct sockaddr_in *addr = argv -> addr; 

    while (1) {
        int header; 
        pthread_cond_wait(&cond_work, &mutex_work);
        printe("Start work .. \n");
        switch (state) {
            case 0: 
                do {
                    struct timespec timer; 
                    header = set_header(SYN_ACK, 1);
                    printe("[> SYN-ACK] (%d: %d-%d)\n", 
                            header, get_type(header), get_seq(header));
                    sendto( sockfd, &header, sizeof(int), 0, 
                            (struct sockaddr*) addr, sizeof(struct sockaddr_in));
                    perror("[> SYN-ACK] sendto()");
                    state = 1;
                    set_abstimer(timer, 1);
                    printe("[> SYN-ACK] Cond wait ...\n");
                    seq = 1;
                    if (pthread_cond_timedwait(&cond_work, &mutex_work, &timer)) {
                        printe("[> SYN-ACK] Timeout ... \n");
                        continue;
                    }
                    state = 2; 

                    pthread_cond_signal(&cond_done);
                    break;
                } while (1);
                break;
        }
    } 
    pthread_cond_wait(&cond_work, &mutex_work);

    pthread_exit(0);
}

static void receiver(struct rdtp_argv *argv) {
    int sockfd = argv -> sockfd;
    struct sockaddr_in *addr = argv -> addr; 
    
    /* 3WHS */
/*    do {
        int header; 
        char buf[1004];
        int reclen; 
        socklen_t addrlen = sizeof(struct sockaddr_in);
        reclen = recvfrom(sockfd, buf, 1004, 0, 
                          (struct soockaddr*) addr, &addrlen);
        if (reclen < 4)
            printe)"Reclen (%d) less than header ... \n", reclen
    } while (0); 
*/

    while (1) {
        int header;
        char buf[1004]; 
        int reclen;
        socklen_t addrlen = sizeof(struct sockaddr_in);
        printe("recvfrom() ... \n");
        reclen = recvfrom(sockfd, buf, 1004, 0, 
                          (struct sockaddr*) addr, &addrlen);
        printe("recvfrom() return (%d) !\n", reclen);
        if (reclen < 4) {
            printe("Reclen (%d) less than header ... \n", reclen);
            continue;
        }
        header = ((int*) buf)[0];
        if (seq < get_seq(header)) {
            printe( "Wrong SEQ ... Expected: %d | Received: %d\n", 
                    seq, get_seq(header));
        }
        switch (state) {
            case 0: 
                if (get_seq(header) == 0) {
                    if (get_type(header) == SYN) {
                        printe("[< SYN] Correct %d: %d-%d\n", 
                                header, get_type(header), get_seq(header)); 
                        pthread_cond_signal(&cond_work);
                    } else {
                        printe("[X<SYN] WRONG type: %d: %d-%d", 
                                header, get_type(header), get_seq(header)); 
                        continue;
                    }
                } else {
                    printe("[X<???] WRONG seq %d: %d-%d\n", 
                            header, get_type(header), get_seq(header));
                    continue;
                }
                break;
            case 1: 
                if (get_seq(header) == 1) {
                    state = 1;
                    if (get_type(header) == ACK) {
                        printe("[< ACK] Correct %d: %d-%d\n", 
                                header, get_type(header), get_seq(header));
                        state = 2;
                        pthread_cond_signal(&cond_work);
                        break;
                    } else if (get_type(header) == DATA) {
                        printe("[< ACK] Receive DATA instead. \n");
                        state = 2;
                        pthread_cond_signal(&cond_work);
                    } else {
                        printe("[X<ACK] WRONG type %d: %d-%d\n", 
                                header, get_type(header), get_seq(header));
                        continue;
                    }
                } else {
                    printe("[X<???] WRONG seq %d: %d-%d\n", 
                            header, get_type(header), get_seq(header));
                    continue;
                }
            case 2: 
                
                ;
                break;
            default: 
                ;
        }
    }
    
    pthread_exit(0);
}

void rdtp_accept(int socket_fd, struct sockaddr_in *server_addr) {
    struct rdtp_argv *argv; 

    argv = malloc(sizeof(struct rdtp_argv));
    argv -> sockfd = socket_fd; 
    argv -> addr = server_addr; 

    seq = 0;
    state = 0;
    
    pthread_create(&thread[0], NULL, (void* (*) (void *)) sender, argv);
    pthread_create(&thread[1], NULL, (void* (*) (void *)) receiver, argv);
   
    pthread_cond_wait(&cond_done, &mutex_done);

	return;
}

int rdtp_read(int socket_fd, unsigned char *buf, int buf_len) {
    // please extend this function
	return 1;		//return bytes of data read
}