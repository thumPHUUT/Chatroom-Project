#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#define MAX_CLIENTS 4
FILE *CLIENTS[MAX_CLIENTS] = {0};
// ^ Each connected client is represented as a FILE*
// If that client is not connected, its FILE* will be NULL


/** Sends the message `buf` to all connected clients except
 * the one at `sender_index`.
 * - Use `fprintf()` to send the message to a client
 * - Use `fflush()` after printing to the client to force sending out the message.
 * - If fprintf or fflush fail, close the client and reset that slot's FILE*
 *   to NULL.
 * */
void redistribute_message(int sender_index, char *buf){
        for(int i = 0; i< MAX_CLIENTS; i++){
                if(i == sender_index || NULL == CLIENTS[i]) continue; //doesnt send message to the sender


                if(fprintf(CLIENTS[i], "%s\n", buf) < 0){//returns negative value when fails
                        perror("fprintf in redistribute_message");
                        fclose(CLIENTS[i]);
                        CLIENTS[i] = NULL;
                }else if(fflush(CLIENTS[i]) < 0){ //reaches here if fprintf succeeds
                        perror("fflush in redistribute_message");
                        fclose(CLIENTS[i]);
                        CLIENTS[i] = NULL;
                }
        }
}


/** Tries to read a message from the specified client.
 * Since the client has been made nonblocking, fgets will fail (by returning NULL)
 * if there is no data to read.
 * To distinguish this failure from a real one, check the value of `errno`.
 * The values EAGAIN or EWOULDBLOCK mean that there was simply no data to read.
 * If any other kind of error occurred, close the client and reset its FILE* to NULL.
 * Return a boolean indicating whether a message was actually read from the client.
 */
int poll_message(char *buf, size_t len, int client_index){
        //reads SINGLE message


        //add smt to check if client disconnected purposefully using feof(CLIENTS[client_index] )


        //excecutes and then checks
        if(NULL == fgets(buf, len, CLIENTS[client_index]) ){
                //either message empty or error, set boolean false


                if( (errno != EAGAIN) && (errno != EWOULDBLOCK)  ){
                        //true error (message isnt just empty)
                        perror("fgets in poll_message");
                        fclose(CLIENTS[client_index]);
                        CLIENTS[client_index] = NULL;
                }


        }else return 1; //message successfully transmitted to buf
        return 0;
}


/** Tries to accept a new client connection.
 * Since the server socket is nonblocking, `accept` will signal failure by returning -1 in case
 * there are no pending connections.
 * Again, distinguish this case by checking the value of `errno` for the values EAGAIN and
 * EWOULDBLOCK.
 * - If a new connection is accepted, set it nonblocking by using `fcntl()`.
 * - Use `fdopen()` to convert the client file descriptor to a FILE*
 * - Try to find a spot in the CLIENTS array for the new connection.
 *      - If there is none, send a brief message to the new client saying so before closing the
 *        connection.
 * - If `accept` fails for a different reason, call `exit` to stop the server.
 */
void try_add_client(int server_fd){
        //idea, place all fdopen and unblock handling here instead of main
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len );




        //check if failed, while also assigning client_fd
        if(-1 == client_fd){ //as seen in class
                //need to check against EAGAIN and EWOULDBLOCK
                if ( errno != EAGAIN && errno != EWOULDBLOCK){
                        //passes if there is an accept error that is not block error (when no text to read)
                        perror("accept in try_add_client");
                        exit(1);
                }else {
                        //printf("no pending connections"); //im pre sure this would send a message every 100ms. this would be bad
                        return;
                }
        }


        //printf("client_fd assigned to potential connection");
        //set unblocking by using fcntl
        if (-1 == fcntl(client_fd, F_SETFL, O_NONBLOCK)){
                perror("fcntl in try_add_client");
                close(client_fd);
                return;
        }


        //use fdopen
        FILE *client = fdopen(client_fd, "r+");
        if (NULL == client) {
                perror("fdopen in try_add_client");
                close(client_fd);
                return;
        }


        //see if availiable spot for clients
        for (int i = 0; i< MAX_CLIENTS; i++){
                if(CLIENTS[i] == NULL){
                        //check failure status of print and fflush if necessary


                        CLIENTS[i] = client;
                        fprintf(client, "server successfully joined\n");
                        fflush(client); //force immediate message recieptt
                        printf("client %d added.\n", i);
                        return;                                                                                                                                                                                                                                             }                                                                                                                                                                     
}
        //only exits loop if all indexes are full
        fprintf(client, "Unable to connect, server full. Closing connection\n");
        printf("Rejected a client attempting to join full chatroom\n");
        fflush(client);
        fclose(client);//close client
}






int main_loop(int server_fd)
{
    char buf[1024];
    int run_condition = 1;


    while (1) {
        // check each client to see if there are messages to redistribute
        if(run_condition){
                printf("server running on 127.0.0.1 : 61738\n");
                run_condition = 0;
        }


        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (NULL == CLIENTS[i]) continue;
            if (!poll_message(buf, sizeof(buf), i)) continue;
            printf("Received message from client %i: %s", i, buf);
            redistribute_message(i, buf);
        }


        // see if there's a new client to add
        try_add_client(server_fd);


        usleep(10 * 1000); // wait 10ms before checking again
    }
    //need a return statement added, else undefined behaviour in main with the status variable(?)
   // return 0; //no error, return 0
}


int main(int argc, char* argv[])
{
    struct sockaddr_in address;
    int server_fd;


    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }


    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1 aka localhost
    // ^ this is a kind of security: the server will only listen
    // for incoming connections that come from 127.0.0.1 i.e. the same
    // computer that the server process is running on.
    address.sin_port = htons(61738); //remyyyy boyz. yeah baby. 1738. ay, im like hey whats up hello


    if (-1 == bind(server_fd, (struct sockaddr *)&address, sizeof(address))) {
        perror("bind");
        close(server_fd);
        return 1;
    }


    if (-1 == listen(server_fd, 5)) {
        perror("listen");
        close(server_fd);
        return 1;
    }
    printf("listening succesful\n");




    // set the server file descriptor to nonblocking mode
    // so that `accept` returns immediately if there are no pending
    // incoming connections
    if (-1 == fcntl(server_fd, F_SETFL, O_NONBLOCK)) {
        perror("fcntl server_fd NONBLOCK");
        close(server_fd);
        return 1;
    }


    // idk where main loop is called. why does setting its return value do anything. does it call the function?
    int status = main_loop(server_fd); //server closed using ctrl+c
    close(server_fd);
    return status;
}