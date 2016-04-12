Proyecto final: servidor HTTP

Tim Berners Lee es conocido por haber creado el HTML y el protocolo HTTP para
intercambiar documentos electrónicos. Seguramente has usado ambos desde hace 
mucho tiempo. El proyecto final será un servidor de HTTP, es decir, un web server
 al que podrás apuntar con tu navegador favorito.

Caracteristicas:
* Basado en HTTP 1.0.
* Implementa los métodos HEAD, GET, POST y algunos encabezados.
* Genera Content-Type y Content-Length correcto para archivos html, css, js, gif,
 jpg, png, .ico, .txt (como mínimo).
* Por default corre en modo multihilos.
* Responde con los mensajes correctos cuando el recurso pedido se encuentra o no.
* En un módulo dinámica activado por switch de línea de comandos implementa CGI 
mediante GET y POST para ejecutar archivos .php (o bien el lenguaje de tu elección).
* Es un demonio.
* Existen un razonable números de commits en el historial del proyecto que evidencían
 el proceso de desarrollo.
¿Cómo se entrega?

***De manera Individual*** - se hará una demo el 03/05/16 haciendo apuntar cualquier
 navegador a la dirección donde estará instalado tu servidor (en AWS).

Evaluación según la siguiente tabla:
50 - Entrega solo .html.
+10 - Entrega páginas completas (con imágenes).
+5 - usa syslog para loggear todo request/error.
+15 - Multithread, sin memory leaks.
+10 - Ejecuta  CGI mediante GET.
+10 - Ejecuta CGI mediante POST.
+20 - recibe archivos usando <input type="file"> (y se pueden consultar con un 
directorio virtual).
+20 - conexiones persistentes y respuestas parciales de HTTP/1.1.
+20 - No es multithreaded ni multiproceso, usa select para atender eventos asíncronos.
+30 - Auto scaling y load balancing en AWS, pruebas con JMeter.
+30 - Implementar Simian Army.
 
Nota: Para ejecutar los .php mediante exec es necesario definir las variables de 
entorno REQUEST_METHOD=GET, REDIRECT_STATUS, QUERY_STRING=..., SCRIPT_FILENAME=... 
cuando la petición es GET, o bien REQUEST_METHOD=POST, REDIRECT_STATUS, SCRIPT_FILENAME=...,
CONTENT_LENGTH=n, CONTENT_TYPE=application/x-www-form-urlencoded.

INFO ADICIONAL:
*Sobre MIME: https://en.wikipedia.org/wiki/Media_type
*HTTP made really easy: http://www.jmarshall.com/easy/http/
*Documentación de uriparser: http://uriparser.sourceforge.net/doc/html/index.html
*Hypertext Transfer Protocol -- HTTP/1.0: https://www.w3.org/Protocols/HTTP/1.0/spec.html
*How to Build a simple HTTP server in C: http://stackoverflow.com/questions/176409/build-a-simple-http-server-in-c
*How the web works: HTTP and CGI explained: http://www.garshol.priv.no/download/text/http-tut.html
*Getting Started with CGI Programming in C: http://www.cs.tut.fi/~jkorpela/forms/cgic.html
*http://www.w3.org/Protocols/rfc2616/rfc2616.html:https://www.w3.org/Protocols/rfc2616/rfc2616.html