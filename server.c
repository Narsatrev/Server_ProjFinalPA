    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/stat.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <string.h>
    #include <signal.h>
    #include <time.h>
    #include <arpa/inet.h>
    #include <stdlib.h>
    #include "listaMimeTypes.h"
    #include <syslog.h>
    #include <errno.h>
    #include <pthread.h>
    #include <unistd.h>

    #define PORT 80
    #define SIZE 8
    #define MSGSIZE 1024
    #define READ 0
    #define WRITE 0

int sdo;

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
        if(acum >= total_size){
            break;  
        } 

        size = ((total_size-acum)>=SIZE)?SIZE:(total_size-acum)%SIZE;
        strncpy(buffer, line+acum, size);
    }
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

    char *tipo_metodo = token_header;

        //segundo token=>URI PETICION
    token_header = strtok(NULL," ");

    char *uri = token_header;

    int metodo=0;

    //Si el uri de peticion contiene '?' debemos usar cgi para
    //procesar los datos de la forma

    if(strstr(uri, "?")>0){
        if(strcmp(tipo_metodo,"GET")==0){
            metodo=1;
        }else{
            if(strcmp(tipo_metodo,"POST")==0){
                metodo=2;
            }else{  
                metodo=3;
            }
        }
        printf("METODO %s\n",tipo_metodo);
        printf("MODO CGI: %d\n",metodo);
    }    

    char nombre_archivo_uri[500];
    if(strncmp(token_header,"/",strlen(token_header))==0){
        strncpy(nombre_archivo_uri, "/index.html", 12);
        nombre_archivo_uri[12]='\0';
    }else{
        strncpy(nombre_archivo_uri, token_header, strlen(token_header)+1);
        nombre_archivo_uri[strlen(token_header)]='\0';
    }

    printf("ARCHIVO URI: %s LEN: %lu\n",nombre_archivo_uri,strlen(nombre_archivo_uri));
    printf("para el 403: %s\n",nombre_archivo_uri);
        //ERROR 403    
    if(!(strstr(nombre_archivo_uri,"."))){

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

        char *token_extension;
            //primer token=>path del archivo        
        char nombre_archivo_uri_copia[512];

        strncpy(nombre_archivo_uri_copia,nombre_archivo_uri,512);

        token_extension = strtok(nombre_archivo_uri,".");
            //segundo token=>extension del archivo
        token_extension = strtok(NULL,".");

        char* tipoMime;

        if(strstr(token_extension,"php")>0) {
            strncpy(token_extension,"php",strlen(token_extension));
            tipoMime="text/html";
        }else{
            tipoMime=recuperarMimeType(token_extension);    
        }        

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

        char *path_ejecutable= url_completo;
        char *query;            

        da=fopen(url_completo, "r");

        if(da == NULL){
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
            printf("METODO: %d\n",metodo);
            if(metodo==0){
                //Si no hay datos que requieran procesamiento, solo regresa un archivo estatico
               printf("SI EXISTE EL ARCHIVO YAY!!!\n");

               fseek(da, 0L, SEEK_END);
               tamano = ftell(da);
               fseek(da, 0L, SEEK_SET);

               sprintf(command, "HTTP/1.0 200 OK\r\n");
               writeLine(s, command, strlen(command));

               sprintf(command, "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n");
               writeLine(s, command, strlen(command));

               sprintf(command, "Content-Type: %s\r\n",tipoMime);
               writeLine(s, command, strlen(command));

               sprintf(command, "Content-Length: %d\r\n",tamano);
               writeLine(s, command, strlen(command));

               sprintf(command, "\r\n");
               writeLine(s, command, strlen(command));

               char file[tamano];
               int suma=0;
               size=fread(file,1,tamano,da);
               printf("ARCHIVO: %d\n",size);

               while((size=write(s,&file[suma],MSGSIZE))>0){                
                    suma+=size;
                    if(suma>=tamano){
                        break;
                    }
                }
            }else{
            // int message_fd[2][2];

                int cgi_output[2];
                int cgi_input[2];
                pipe(cgi_output);
                pipe(cgi_input);

                int i;

                // pipe(message_fd[READ]);
                // pipe(message_fd[WRITE]);

                if(!fork()) {
                    // close(message_fd[READ][READ]);
                    // close(message_fd[WRITE][WRITE]);

                    // dup2(message_fd[READ][WRITE], 1);
                    // dup2(message_fd[WRITE][READ], 0);
                    dup2(cgi_output[1], 1);
                    dup2(cgi_input[0], 0);

                    close(cgi_output[0]);
                    close(cgi_input[1]);

                    putenv("REQUEST_METHOD=GET");
                    putenv("REDIRECT_STATUS=True");
                    putenv("QUERY_STRING=hola=a&mundo=b");
                    putenv("SCRIPT_FILENAME=test.php");

                    if(execlp("php-cgi", "php-cgi",url_completo, 0)<0){
                        perror("execlp");
                    }
                }
                // close(message_fd[READ][WRITE]);
                // close(message_fd[WRITE][READ]);

                close(cgi_output[1]);
                close(cgi_input[0]);

                char c;
                // int t2=0;

                // char *string_p=(char *)malloc(t2+1);   

                // while (read(cgi_output[0], &c, 1) > 0){

                //     printf("%c",c);
                //     char x2[2]={c,'\0'};
                //     t2++;
                //     string_p=(char *)realloc(string_p,t2);
                //     strcat(string_p, x2);

                // }

                // printf("SIZE:::::: %d\n",t2);
                // printf("BUFFER:::: %s",string_p);
                // printf("STRLEN:::: %lu",strlen(string_p));

                int t=0;
                char buffx[100000];
                while (read(cgi_output[0], &c, 1) > 0){                    
                    buffx[t]=c;
                    t++;
                }

                char buffer[32];
                int size = 0;

                sprintf(command, "HTTP/1.0 200 OK\r\n");
                writeLine(s, command, strlen(command));

                sprintf(command, "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n");
                writeLine(s, command, strlen(command));

                sprintf(command, "Content-Type: text/html\r\n");
                writeLine(s, command, strlen(command));

                sprintf(command, "Content-Length: %d\r\n",t-50);
                writeLine(s, command, strlen(command));

                sprintf(command, "\r\n");
                writeLine(s, command, strlen(command));

                int aux=50;                
                while(aux<t){      
                    write(s,&buffx[aux],1);
                    aux++;
                }

                
                printf("SIZE----> %d\n",t);
            }
    }    
    fclose(da);
}
return 0;    
}

int main() {
    int sd, addrlen, size;
    struct sockaddr_in sin, pin;


        // 1. Crear el socket
    sd = socket(AF_INET, SOCK_STREAM, 0);
    int habilitar = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &habilitar, sizeof(int)) < 0){        
        perror("setsockopt");
    }

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

    pid_t pid;    

        //FINALMENTE.... crudo pero funcional
        //al padre no le va a importar que suceda con el hijo mientras
        //este termine, por lo tanto los hijos nunca se transformaran en zombies
    signal(SIGCHLD, SIG_IGN);

    while(1){

        sdo = accept(sd, (struct sockaddr *)  &pin, &addrlen);
        if (sdo == -1) {
                //En coso de que suceda algo raro en el socket y el cliente
                //no pueda conectarse, ingresar el error al log
            openlog("ErrorAceptarConexion", LOG_PID | LOG_CONS, LOG_USER);
            syslog(LOG_INFO, "Error: %s\n", strerror(errno));
            closelog();
            perror("accept");
            exit(1);
        }

        if(!fork()){
            printf("Conectado desde %s\n", inet_ntoa(pin.sin_addr));
            printf("Puerto %d\n", ntohs(pin.sin_port));
            serve(sdo);
            close(sdo);
            exit(0);
        }

            // if ((pid = fork()) == -1){
            //     close(sdo);
            //     continue;
            // }else if(pid > 0){
            //     close(sdo);
            //     wait(0);
            // }else if(pid == 0){
            //     printf("Conectado desde %s\n", inet_ntoa(pin.sin_addr));
            //     printf("Puerto %d\n", ntohs(pin.sin_port));
            //     serve(sdo);
            //     close(sdo);
            //     exit(0);
            // }

            //Multiproceso sin zombies (intento 2: exitoso, pero hace cosas raras con los el orden de los 404,403 y 200....)

            // pid_t id_proc;
            // if (!(id_proc = fork())) {
            //     if (!fork()){
            //      //El nieto hijo ejecuta su proceso
            //         printf("Conectado desde %s\n", inet_ntoa(pin.sin_addr));
            //         printf("Puerto %d\n", ntohs(pin.sin_port));
            //         serve(sdo);
            //         close(sdo);
            //     }else{
            //       //El nieto proceso hijo termina
            //       exit(0);
            //     }
            // } else {
            //     //El proceso original espera a que el primer hijo termine, lo cual 
            //     //es inmediatamente despues del segundo fork
            //     waitpid(id_proc);
            // }                   

            //Multiproceso sin zombies (intento 1: parcialmente exitoso)
            //cambiar para aumentar la capcidad de enkolamyento

            // pid_t id_proc;
            // if ( (id_proc = fork()) < 0 ) {
            //     openlog("ErrorCreacionNuevoProcesoCliente", LOG_PID | LOG_CONS, LOG_USER);
            //     syslog(LOG_INFO, "Error: %s\n", strerror(errno));
            //     closelog();
            //     perror("fork");
            //     return;
            // }
            // if (id_proc == 0){
            //     printf("Conectado desde %s\n", inet_ntoa(pin.sin_addr));
            //     printf("Puerto %d\n", ntohs(pin.sin_port));
            //     serve(sdo);
            //     close(sdo);
            //     exit(0);
            // }else{
            //     waitpid(id_proc);
            // } 

            //Multithread (intento 1: fallido parcialmente, termina al regresar un 404)

            // pthread_t hiloCliente;    
            // if (pthread_create(&hiloCliente , NULL, serve, sdo) != 0){
            //     openlog("ErrorCreacionNuevoThreadClinete", LOG_PID | LOG_CONS, LOG_USER);
            //     syslog(LOG_INFO, "Error: %s\n", strerror(errno));
            //     closelog();
            //     perror("pthread_create");
            // }

        atexit(servidorCayo);
    }
    close(sd);
}