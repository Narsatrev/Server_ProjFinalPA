
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

char buffFecha[1000];

void calcularFecha(){
    time_t esteInstante = time(0);
    struct tm tm = *gmtime(&esteInstante);
    strftime(buffFecha, sizeof buffFecha, "%a, %d %b %Y %H:%M:%S %Z", &tm);
}

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

int contSaltos=0;

char residuos[1024];

int banderaUbicacionContentLength=0;
int readLine(int s, char *line, int *result_size) {

    //vaciar los residuos para permitir la entrada de los datos del post
    memset(&residuos[0], 0, sizeof(residuos));

    int acum=0, size;
    char buffer[SIZE];

    int esPost=0;

    // char buffer_

    while( (size=read(s, buffer, SIZE)) > 0) {
        if (size < 0) return -1;
        strncpy(line+acum, buffer, size);
        strncat(residuos,buffer,8);
        // printf("BUFFER: %s\n",buffer);
        // printf("RESIDUOS: %s\n",residuos);

        if(strstr(buffer,"POST")>0){
            esPost=1;
            printf("ES POST DESDE READLINE!!!\n");
                       
        }

        //Sacer el content length.....
        if(!banderaUbicacionContentLength){
            if(strstr(line,"Cache-Control")>0){
                if(strstr(line,"Length")>0){
                    char * aux; 
                    aux=strstr(line,"Content-Length");
                    int posicion_substring=aux-line;;          
                    char *xgh=line+posicion_substring;
                    char *token_pos1;
                    char *token_pos2;
                    token_pos1=strtok(xgh," ");
                    token_pos1=strtok(NULL," ");
                    token_pos2=strtok(token_pos1,"C");
                    int f=0;
                    char buff[50];
                    strcpy(buff,token_pos2);
                    while(f<50){
                        if(buff[f]=='\n'){
                            break;
                        }
                        printf("%c",buff[f]);
                        f++;
                    }
                    printf("TOKEN CHAK: %s\n",buff);                
                }   
                banderaUbicacionContentLength=1; 
            }
        }

        // printf("LINEA: %s\n",line);


        acum += size;
        if(!esPost){
            if(line[acum-1] == '\n' && line[acum-2] == '\r') {
                // printf("SUPONGO QUE ENCONTRE UN SALTO DE LINEA...");
                break;    
            }     
        }else{
            if(line[acum-1] == '\n' && line[acum-2] == '\r' && line[acum-3] == '\n' && line[acum-4] == '\r') {
        //         // int j=0;
        //         // while(j<29){

        //         // }
                // printf("DOBLE SALTO ZI!");        
            }
            // printf("BUFFER: %s\n",buffer);
        //     break;
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
    char buff[8192];

    char query[512];            

    int esPost=0;
    int contadorLineaVaciaParaElPost=0;

    while(1) {
        r = readLine(s, command, &size);
        // printf("COMMAND COMPLETO: %s\n",command);

        command[size-2] = 0;
        size-=2;

        //Guarda toda la informacion de las peticiones en el log ubicado en /var/log/messages
        openlog("Peticiones_al_servidor", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "Header de la peticion: %s\n",command);
        closelog();

        printf("[%s]\n", command);

        char buff_query[512];

        if(strstr(command,"POST")>0){
            esPost=1;
            printf("ES POST!!!\n");
        }

        //Guardar todos los comandos para su manipulacion posterior
        strcat(buff,command);
        strcat(buff,"\n");

        if(!esPost){
            if(command[size-1] == '\n' && command[size-2] == '\r') {
                break;               
            }    
        }
        
    }

    char buff_aux[8192];
    strncpy(buff_aux,buff,8192);

    char *token_header;

    //primer token=>TIPO DE ACCION (GET, POST, ETC...
    token_header = strtok(buff_aux," ");

    char *tipo_metodo = token_header;

    //segundo token=>URI PETICION
    token_header = strtok(NULL," ");

    char *uri = token_header;

    int metodo=0;

    //Si el uri de peticion contiene '?' o es de tipo POST 
    //debemos usar cgi para procesar los datos de la forma

    if(strcmp(tipo_metodo,"POST")==0){
        metodo=2;
    }else{
        if(strstr(uri, "?")>0){
            if(strcmp(tipo_metodo,"GET")==0){
                metodo=1;
            }else{
                    metodo=3;
            }
            printf("METODO %s\n",tipo_metodo);
            printf("MODO CGI: %d\n",metodo);
        }    
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
        calcularFecha();
        sprintf(command, "Date: %s\r\n",buffFecha);
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
            calcularFecha();
            sprintf(command, "Date: %s\r\n",buffFecha);
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

               calcularFecha();
                sprintf(command, "Date: %s\r\n",buffFecha);
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

                //separar las pipes para entender mejor que 
                //esta pasando.. la sintaxis n_pipe[2][2]
                int pipe_salida[2];
                int pipe_entrada[2];
                pipe(pipe_salida);
                pipe(pipe_entrada);

                int i;

                //////SUFFERING IS REAL.....
                char *token_a;
                char *token_b;

                // char buff_aux[8192];
                // strncpy(buff_aux,buff,8192);
                token_a=strtok(buff," ");
                token_a=strtok(NULL," ");
                token_b=strtok(token_a,"?");
                token_b=strtok(NULL,"?");
                char *xyz=token_b;
                char *zyx="QUERY_STRING=";
                //query string final
                char ggg[1024];
                strcat(ggg,zyx);
                strcat(ggg,xyz);

                if(!fork()) {
                    close(pipe_salida[0]);
                    close(pipe_entrada[1]);
                    
                    dup2(pipe_salida[1], 1);
                    dup2(pipe_entrada[0], 0);
                    
            
                    putenv("REQUEST_METHOD=GET");
                    putenv("REDIRECT_STATUS=True");
                    putenv(ggg);
                    putenv("SCRIPT_FILENAME=test.php");

                    if(execlp("php-cgi", "php-cgi",url_completo, 0)<0){
                        openlog("ErrorEXECLP", LOG_PID | LOG_CONS, LOG_USER);
                        syslog(LOG_INFO, "Error: %s\n", strerror(errno));
                        closelog();
                        perror("execlp");
                    }
                }

                close(pipe_salida[1]);
                close(pipe_entrada[0]);

                char c;

                int t=0;
                char buffx[100000];
                while (read(pipe_salida[0], &c, 1) > 0){                    
                    buffx[t]=c;
                    t++;
                }

                char buffer[32];
                int size = 0;

                sprintf(command, "HTTP/1.0 200 OK\r\n");
                writeLine(s, command, strlen(command));

                calcularFecha();
                sprintf(command, "Date: %s\r\n",buffFecha);
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

        atexit(servidorCayo);
    }
    close(sd);
}