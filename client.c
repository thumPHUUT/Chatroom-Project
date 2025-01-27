#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{
    struct sockaddr_in address;
    int sock_fd;
    char buf[1024];
    int buf_len = sizeof(buf);
    //set stdout to unbuffered for debugging
    setbuf(stdout, NULL);
    printf("meow");
    fflush(stdout);


    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        close(sock_fd);
        return 1;
    }
    printf("socket created successfully.\n");
    fflush(stdout);




    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1 aka localhost
    address.sin_port = htons(61738); //see server.c file for my comment on this
    // ^ to avoid conflicts with others, change the port number
    // to something else. Reflect that change in server.c


    if (0 >  connect(sock_fd, (struct sockaddr *)&address, sizeof(address))) {
        perror("connect");
        if (errno == ECONNREFUSED || errno == ETIMEDOUT || errno == EHOSTUNREACH){
                printf("connect specific err codes, see conditional");
        }


        close(sock_fd);
        return 1;
    }
    printf("connected to server succesffully.\n");
    // make stdin nonblocking:
    if(-1 == fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK)) { //0 refers to stdin
        perror("fcntl stdin nonblock");
        close(sock_fd);
        return 1;
    }


    // make the socket nonblocking:
    if(-1 == fcntl(sock_fd, F_SETFL, O_NONBLOCK)){
        perror("fcntl socket nonblock");
        close(sock_fd);
        return 1;
    }


    //opening server and std as file!
    FILE *server = fdopen(sock_fd, "r+");
    FILE* stdin_file = fdopen(STDIN_FILENO, "r+");




    if(NULL == server){
        perror("fdopen server");
        fclose(server);
        return 1;
    }else if(NULL == stdin_file){
        //if using fgets, use stdin_file instead of stdin
        perror("fdopen stdin");
        fclose(stdin_file);
        return 1;
    }


    while (1) {
        // - Complete the below polling loop:
        //      - try to read from stdin, and forward across the socket
        //      - try to read from the socket, and forward to stdout
        //check stdin first


        ssize_t bytes_read = read(STDIN_FILENO, buf, buf_len-1); //add room for null terminator
        if(0 < bytes_read){
                //add null terminator
                buf[bytes_read] = '\0';
                fprintf(server,"%s", buf);
                fflush(server);
                //could add err message for fflush, fprintf
        } else if(0 > bytes_read && errno != EAGAIN && errno != EWOULDBLOCK){
                perror("read");
                break;


        }


        //check socket
        //
        ssize_t bytes_recieve = recv(sock_fd, buf, buf_len-1, 0);
        if(0 < bytes_recieve){
                buf[bytes_recieve] = '\0';
                printf("%s", buf);


        } else if(0 > bytes_recieve && errno != EAGAIN && errno != EWOULDBLOCK){
                perror("recv");
                break;


        }


        /*
        if(NULL != fgets(buf, buf_len, stdin_file)){
                if(EOF == fputs(buf, server)){
                        perror("fputs");
                        break; //need to close files, hence break not return
                }
        }else if (ferror(server) && errno != EAGAIN && errno != EWOULDBLOCK) { //checks if blocked error (for debugging)
                perror("fgets stdin");
                break;
        }
        */


        //check fgets for EOF from server
        if(NULL != fgets(buf, buf_len, server)){
                if(EOF == fputs(buf, stdout) ){ //indicates disconnection
                                  printf("server disconnected\n");
                        break;
                }
                fflush(stdout); //ensures immediate printing
        } else if( ferror(server) && errno != EAGAIN && errno != EWOULDBLOCK){
                perror("fgets server");
                break;
        }


        usleep(10 * 1000); // wait 10ms before checking again
    }
    fclose(stdin_file);
    fclose(server);
    return 0;
}