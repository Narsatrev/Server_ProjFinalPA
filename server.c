#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "listaMimeTypes.h"
#include <syslog.h>
#include <errno.h>
#include <pthread.h>

#define PORT 80
#define SIZE 8
#define MSGSIZE 1024

void servidorCayo(){
    openlog("ServidorMurio", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "El servidor fue apagado o termino su proceso de forma inesperada...\n");
    closelog();
}

char *recuperarMimeType(char *extension){
    int i,j,x;
    char buffTipoMime[100];

    for(i=0;i<83;i++){
        if(strncmp(mapaMimeTypes[i],extension,strlen(extension))==0){
            strncpy(buffTipoMime,mapaMimeTypes[i+1],strlen(mapaMimeTypes[i+1]));
            break;
        }
    }    
    return buffTipoMime;
}

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

    while( (size = write(s, buffer, size)) > 0) {
        if(size<0){
            return size;
        } 
        acum += size;
        if (acum >= total_size){
            break;  
        } 

        size = ((total_size-acum)>=SIZE)?SIZE:(total_size-acum)%SIZE;
        strncpy(buffer, line+acum, size);
    }
    printf("size: %d\n",size);
    printf("Acabe! Exitosamente!\n");
    return 0;
}

int serve(int s) {
    char command[MSGSIZE];
    int size, r, nlc = 0;
    char *archivo_peticion;        
    char buff[2048];
    while(1) {
        r = readLine(s, command, &size);
        command[size-2] = 0;
        size-=2;

        //Guarda toda la informacion de las peticiones en el log ubicado en /var/log/messages
        openlog("Peticiones_al_servidor", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "Header de la peticion: %s\n",command);
        closelog();

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

    //primer token=>TIPO DE ACCION (GET, POST, ETC...
    token_header = strtok(buff_aux," ");
    //segundo token=>URI PETICION
    token_header = strtok(NULL," ");
    
    char nombre_archivo_uri[500];

    printf("ARCHIVO URI: %s LEN: %lu\n",token_header,strlen(token_header));

    //ERROR 403
    printf("para el 403: %s",token_header);
    if(strstr(token_header,".")<0 && strstr(token_header,"/")!=0){

        int tamano=0;
        printf("intentando acceder a un directorio restringido omg4!");
        //Guardar en el log del sistema cada vez que alguien intento accesar a un directorio exista o no
        openlog("IntentoDeAccesoRestringidoDirectorios", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "Alguien intento acceder al recurso: %s\n", nombre_archivo_uri);
        closelog();

        FILE *error=fopen("/home/ec2-user/var/www/html/errores/error403.html", "r");

        fseek(error, 0L, SEEK_END);
        tamano = ftell(error);
        fseek(error, 0L, SEEK_SET);

        char *archivo = malloc(tamano+1);
        fread(archivo, tamano, 1, error);
        fclose(error);

        //Mandar una respuesta con header 404, archivo no encontrado
        sprintf(command, "HTTP/1.0 403 ACCESS FORBIDDEN\r\n");
        writeLine(s, command, strlen(command));
        sprintf(command, "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n");
        writeLine(s, command, strlen(command));
        sprintf(command, "Content-Type: text/html\r\n");
        writeLine(s, command, strlen(command));
        sprintf(command, "Content-Length: %d\r\n",tamano);
        writeLine(s, command, strlen(command));
        sprintf(command, "\r\n%s",archivo);
        writeLine(s, command, strlen(command));

        printf("No existe tal archivo!!\n");
        free(archivo);
    }else{

        if(strncmp(token_header,"/",strlen(token_header))==0){
            strncpy(nombre_archivo_uri, "/index.html", 12);
            nombre_archivo_uri[12]='\0';
        }else{
            strncpy(nombre_archivo_uri, token_header, strlen(token_header)+1);
            nombre_archivo_uri[strlen(token_header)]='\0';
        }

        char *token_extension;
        //primer token=>path del archivo
        token_extension = strtok(nombre_archivo_uri,".");
        //segundo token=>extension del archivo
        token_extension = strtok(NULL,".");


        char* tipoMime=recuperarMimeType(token_extension);

        printf("EXTENSION ARCHIVO: %s\n", token_extension);
        printf("TIPO MIME: %s\n", tipoMime);    

        //test recuperar y calcular tamano de archivo
        FILE *da;
        int tamano;

        char *url_archivo="/home/ec2-user/var/www/html";

        char url_completo[1024];
        strcat(url_completo,url_archivo);
        strcat(url_completo,nombre_archivo_uri);
        strcat(url_completo,".");
        strcat(url_completo,token_extension);
        
        printf("URL COMPLETA: %s\n",url_completo);

        da=fopen(url_completo, "r");

        if(da==NULL){
            
            //Guardar en el log del sistema cada vez que alguien intento accesar a un archivo que no existe
            openlog("ErrorArchivoNoEncontrado", LOG_PID | LOG_CONS, LOG_USER);
            syslog(LOG_INFO, "Error: El archivo %s no fue encontrado!\n", url_completo);
            closelog();

            FILE *error=fopen("/home/ec2-user/var/www/html/errores/error404.html", "r");

            fseek(error, 0L, SEEK_END);
            tamano = ftell(error);
            fseek(error, 0L, SEEK_SET);

            char *archivo = malloc(tamano+1);
            fread(archivo, tamano, 1, error);
            fclose(error);

            //Mandar una respuesta con header 404, archivo no encontrado
            sprintf(command, "HTTP/1.0 404 NOT FOUND\r\n");
            writeLine(s, command, strlen(command));
            sprintf(command, "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n");
            writeLine(s, command, strlen(command));
            sprintf(command, "Content-Type: text/html\r\n");
            writeLine(s, command, strlen(command));
            sprintf(command, "Content-Length: %d\r\n",tamano);
            writeLine(s, command, strlen(command));
            sprintf(command, "\r\n%s",archivo);
            writeLine(s, command, strlen(command));

            printf("No existe tal archivo!!\n");
            free(archivo);
        }else{
            printf("SI EXISTE EL ARCHIVO YAY!!!\n");

            fseek(da, 0L, SEEK_END);
            tamano = ftell(da);
            fseek(da, 0L, SEEK_SET);

            char *archivo = malloc(tamano+1);
            fread(archivo, tamano, 1, da);

            sleep(1);

            char buff_archivo[1024];

            sprintf(command, "HTTP/1.0 200 OK\r\n");
            writeLine(s, command, strlen(command));
            sprintf(command, "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n");
            writeLine(s, command, strlen(command));
            sprintf(command, "Content-Type: %s\r\n",tipoMime);
            writeLine(s, command, strlen(command));

            printf("%s\n", archivo);
            printf("Tam archivo: %d\n", tamano);    

            sprintf(command, "Content-Length: %d\r\n",tamano);
            writeLine(s, command, strlen(command));
            sprintf(command, "\r\n");
            writeLine(s, command, strlen(command));

    /////////////////////////////////////////////////////////// Intento 1        
            // fgets(buff_archivo, 1024, da);
            // printf("BUFF:%s\n",buff_archivo);
            // while (!feof(da)){
            //     sprintf(command, "%s",buff_archivo);
            //     writeLine(s, command, strlen(command));
            //     fgets(buff_archivo, 1024, da);
            //     printf("BUFF:%s\n",buff_archivo);
            // }
    /////////////////////////////////////////////////////////// Intento 2
            // int current_char = 0;
            // do{            
            //     current_char = fgetc(da);
            //     printf("%c",current_char);
            //     write(s, &current_char, 1);
            // }
            // while(current_char != EOF);
    ///////////////////////////////////////////////////////////
            // while(1){
            //     unsigned char buff[256]={0};
            //     int nread = fread(buff,1,256,da);
            //     printf("Bytes read %d \n", nread);        
            //     if(nread > 0)
            //     {
            //         printf("Sending \n");
            //         write(s, buff, nread);
            //     }        
            //     if (nread < 256)
            //     {
            //         if (feof(da))
            //             printf("End of file\n");
            //         if (ferror(da))
            //             printf("Error reading\n");
            //         break;
            //     }
            // }
    ///////////////////////////////////////////////////////////        

            sprintf(command, "\r\n%s",archivo);
            writeLine(s, command, strlen(command));

            free(archivo);
        }    
        fclose(da);
    }
    return 0;    
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
    int status;

    while(1){

        sdo = accept(sd, (struct sockaddr *)  &pin, &addrlen);
        if (sdo == -1) {
            openlog("ErrorAceptarConexion", LOG_PID | LOG_CONS, LOG_USER);
            syslog(LOG_INFO, "Error: %s\n", strerror(errno));
            closelog();
            perror("accept");
            exit(0);
        }

        //Multiproceso sin zombies
        pid_t pid;
        if ( (pid = fork()) < 0 ) {
            openlog("ErrorCreacionNuevoProcesoCliente", LOG_PID | LOG_CONS, LOG_USER);
            syslog(LOG_INFO, "Error: %s\n", strerror(errno));
            closelog();
            perror("fork");
            return;
        }
        if (pid == 0){
            printf("Conectado desde %s\n", inet_ntoa(pin.sin_addr));
            printf("Puerto %d\n", ntohs(pin.sin_port));
            serve(sdo);
            close(sdo);
            exit(0);
        }else{
            waitpid(pid, &status, 0);
        }

        //Multithread
        // pthread_t hiloCliente;

        // if (pthread_create(&hiloCliente , NULL, serve, sdo) != 0){
        //     openlog("ErrorCreacionNuevoThreadClinete", LOG_PID | LOG_CONS, LOG_USER);
        //     syslog(LOG_INFO, "Error: %s\n", strerror(errno));
        //     closelog();
        //     perror("pthread_create");
        // }
        // pthread_join(hiloCliente, NULL);

        atexit(servidorCayo);
    }
    close(sd);
}