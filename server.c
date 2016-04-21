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

    while(1) {
        r = readLine(s, command, &size);
        command[size-2] = 0;
        size-=2;

        ////////        
        char *token;        
        if(strstr(command, "GET") != NULL){
            token = strtok(command," ");
            token = strtok(NULL," ");
            printf("TOKEN: %s\n",token);
        }
        ////////
        
        if(command[size-1] == '\n' && command[size-2] == '\r') {
            break;
        }
    }

    // printf("COMMAND:: %s\n",command);

    sleep(1);

    sprintf(command, "HTTP/1.0 200 OK\r\n");
    writeLine(s, command, strlen(command));

    //PRUEBA TIEMPO

    //fecha y hora actual
    time_t tiempo = time(NULL);
    //estructura de tiempo que contiene los componentes de la fecha desglosados
    //metodo localtime que extrae componentes de tiempo
    struct tm *tiempo_s = localtime(&tiempo);
    char fechaHora[64];
    //strftime regresa un string con los datos de la estructura tm
    //con un formato especificado, en este caso %c-> representacion de fecha Y hora
    strftime(fechaHora, sizeof(fechaHora), "%c", tiempo_s);
    printf("%s\n", fechaHora);

    //PRUEBA TIEMPO->   FALTA DARLE FORMATO A LA FECHA

    sprintf(command, "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n");
    writeLine(s, command, strlen(command));


    sprintf(command, "Content-Type: text/html\r\n");
    writeLine(s, command, strlen(command));

    //test recuperar y calcular tamano de archivo

    FILE *da;
    int tamano;
    da=fopen("/home/ec2-user/var/www/html/index.html", "r");

    fseek(da, 0L, SEEK_END);
    tamano = ftell(da);
    fseek(da, 0L, SEEK_SET);

    char *archivo = malloc(tamano+1);
    fread(archivo, tamano, 1, da);
    fclose(da);

    printf("%s\n", archivo);
    
    sprintf(command, "Content-Length: %d\r\n", tamano);
    writeLine(s, command, strlen(command));

    sprintf(command, "\r\n%s", archivo);
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

        if(!fork()) {
            close(sd);
            printf("Conectado desde %s\n", inet_ntoa(pin.sin_addr));
            printf("Puerto %d\n", ntohs(pin.sin_port));
            serve(sdo);
            close(sdo);
            exit(0);
        }
    }

    close(sd);

    sleep(1);

}

