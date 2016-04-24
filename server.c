#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdlib.h>


#define PORT 80
#define SIZE 8
#define MSGSIZE 1024

int readLine(int s, char *line, int *result_size) {
    int acum=0, size;
    char buffer[SIZE];

    while( (size=read(s, buffer, SIZE)) > 0) {
        if (size < 0) return -1;
        strncpy(line+acum, buffer, size);
        acum += size;
        if(line[acum-1] == '\n' && line[acum-2] == '\r') {
            break;
        } 
    }

    *result_size = acum;

    return 0;
}

int writeLine(int s, char *line, int total_size) {
    int acum = 0, size;
    char buffer[SIZE];

    if(total_size > SIZE) {
        strncpy(buffer, line, SIZE);
        size = SIZE;
    } else  {
        strncpy(buffer, line, total_size);
        size = total_size;
    }

    while( (size=write(s, buffer, size)) > 0) {
        if(size<0) return size;
        acum += size;
        if (acum >= total_size) break;

        size = ((total_size-acum)>=SIZE)?SIZE:(total_size-acum)%SIZE;
        strncpy(buffer, line+acum, size);
    }

    return 0;
}

int serve(int s) {
    char command[MSGSIZE];
    int size, r, nlc = 0;
    char *url_archivo;
    char *archivo_peticion;        
    char buff[2048];
    while(1) {
        r = readLine(s, command, &size);
        command[size-2] = 0;
        size-=2;
        printf("[%s]\n", command);

        //Guardar todos los comandos para su manipulacion posterior
        strcat(buff,command);
        strcat(buff,"\n");

        if(command[size-1] == '\n' && command[size-2] == '\r') {
            break;
        }
    }

    char buff_aux[2048];
    strncpy(buff_aux,buff,2048);

    char *token_header;
    //primer token=>GET
    token_header = strtok(buff_aux," ");
    //segundo token=>URI PETICION
    token_header = strtok(NULL," ");
    
    char nombre_archivo_uri[500];

    printf("TOKEN HEADER: %s LEN: %lu\n",token_header,strlen(token_header));

    // if(strncmp(token_header,"/",strlen(token_header))==0){
    //     strncpy(nombre_archivo_uri, "index.html", 11);
    //     nombre_archivo_uri[10]='\0';
    // }else{
    //     strncpy(nombre_archivo_uri, token_header, strlen(token_header)+1);
    //     nombre_archivo_uri[strlen(token_header)]='\0';
    // }


    // printf("ARCHIVO URI: %s\n", nombre_archivo_uri);


    sleep(1);

    sprintf(command, "HTTP/1.0 200 OK\r\n");
    writeLine(s, command, strlen(command));

    sprintf(command, "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n");
    writeLine(s, command, strlen(command));


    sprintf(command, "Content-Type: text/html\r\n");
    writeLine(s, command, strlen(command));

    //test recuperar y calcular tamano de archivo

    FILE *da;
    int tamano;
    url_archivo="/home/ec2-user/var/www/html/index.html";
    // printf("URL: %s URI: %s\n", url_archivo, archivo_peticion);
    // sprintf(url_archivo,"%s%s",url_archivo, archivo_peticion);
    // printf("URL: %s URI: %s\n", url_archivo, archivo_peticion);

    da=fopen(url_archivo, "r");

    fseek(da, 0L, SEEK_END);
    tamano = ftell(da);
    fseek(da, 0L, SEEK_SET);

    char *archivo = malloc(tamano+1);
    fread(archivo, tamano, 1, da);
    fclose(da);

    printf("%s\n", archivo);
    printf("Tam archivo: %d\n", tamano);
    
    sprintf(command, "Content-Length: %d\r\n",tamano);
    writeLine(s, command, strlen(command));

    sprintf(command, "\r\n%s",archivo);
    writeLine(s, command, strlen(command));

    free(archivo);
    //termina prueba
}

int main() {
    int sd, sdo, addrlen, size;
    struct sockaddr_in sin, pin;

    // 1. Crear el socket
    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(PORT);

    // 2. Asociar el socket a un IP/puerto
    bind(sd, (struct sockaddr *) &sin, sizeof(sin));
    // 3. Configurar el backlog
    listen(sd, 5);

    addrlen = sizeof(pin);
    
    // 4. aceptar conexión
    while(1){

        sdo = accept(sd, (struct sockaddr *)  &pin, &addrlen);
        if (sdo == -1) {
            perror("accept");
            exit(0);
        }

        // if(!fork()) {
            close(sd);
            printf("Conectado desde %s\n", inet_ntoa(pin.sin_addr));
            printf("Puerto %d\n", ntohs(pin.sin_port));
            serve(sdo);
            close(sdo);
            exit(0);
        // }
    }

    close(sd);

    sleep(1);

}

// #include <stdio.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netdb.h>
// #include <string.h>
// #include <arpa/inet.h>
// #include <stdlib.h>


// #define PORT 80
// #define SIZE 8
// #define MSGSIZE 1024

// int readLine(int s, char *line, int *result_size) {
//     int acum=0, size;
//     char buffer[SIZE];

//     while( (size=read(s, buffer, SIZE)) > 0) {
//         if (size < 0) return -1;
//         strncpy(line+acum, buffer, size);
//         acum += size;
//         if(line[acum-1] == '\n' && line[acum-2] == '\r') {
//             break;
//         } 
//     }

//     *result_size = acum;

//     return 0;
// }

// int writeLine(int s, char *line, int total_size) {
//     int acum = 0, size;
//     char buffer[SIZE];

//     if(total_size > SIZE) {
//         strncpy(buffer, line, SIZE);
//         size = SIZE;
//     } else  {
//         strncpy(buffer, line, total_size);
//         size = total_size;
//     }

//     while( (size=write(s, buffer, size)) > 0) {
//         if(size<0) return size;
//         acum += size;
//         if (acum >= total_size) break;

//         size = ((total_size-acum)>=SIZE)?SIZE:(total_size-acum)%SIZE;
//         strncpy(buffer, line+acum, size);
//     }

//     return 0;
// }

// int serve(int s) {
//     char command[MSGSIZE];
//     int size, r, nlc = 0;

    // while(1) {
    //     r = readLine(s, command, &size);
    //     command[size-2] = 0;
    //     size-=2;
    //     printf("[%s]\n", command);
    //     if(command[size-1] == '\n' && command[size-2] == '\r') {
    //         break;
    //     }
    // }
//     sleep(1);

//     sprintf(command, "HTTP/1.0 200 OK\r\n");
//     writeLine(s, command, strlen(command));

//     sprintf(command, "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n");
//     writeLine(s, command, strlen(command));

//     sprintf(command, "Content-Type: text/plain\r\n");
//     writeLine(s, command, strlen(command));

//     sprintf(command, "Content-Length: 11\r\n");
//     writeLine(s, command, strlen(command));

//     sprintf(command, "\r\nHola mundo!");
//     writeLine(s, command, strlen(command));
// }

// int main() {
//     int sd, sdo, addrlen, size;
//     struct sockaddr_in sin, pin;

//     // 1. Crear el socket
//     sd = socket(AF_INET, SOCK_STREAM, 0);

//     memset(&sin, 0, sizeof(sin));
//     sin.sin_family = AF_INET;
//     sin.sin_addr.s_addr = INADDR_ANY;
//     sin.sin_port = htons(PORT);

//     // 2. Asociar el socket a un IP/puerto
//     bind(sd, (struct sockaddr *) &sin, sizeof(sin));
//     // 3. Configurar el backlog
//     listen(sd, 5);

//     addrlen = sizeof(pin);
//     // 4. aceptar conexión
//     while( (sdo = accept(sd, (struct sockaddr *)  &pin, &addrlen)) > 0) {
//         if(!fork()) {
//             printf("Conectado desde %s\n", inet_ntoa(pin.sin_addr));
//             printf("Puerto %d\n", ntohs(pin.sin_port));

//             serve(sdo);

//             close(sdo);
//             exit(0);
//         }
//     }
//     close(sd);

//     sleep(1);

// }


