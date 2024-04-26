#include "csapp.h"

int main(void) {
    char *buf, *p, *method;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1=0, n2=0;

    // Extract the two arguments
    if ((buf = getenv("QUERY_STRING")) != NULL) {
    p = strchr(buf, '&');
    if (p != NULL) {
        *p = '\0';
        if (strchr(buf, '=') == NULL) {
            strcpy(arg1, buf);
            strcpy(arg2, p+1);
        } else {
            char *eq = strchr(buf, '=');  
            *eq = '\0'; 
            strcpy(arg1, eq+1);
            eq = strchr(p+1, '=');
            *eq = '\0';
            strcpy(arg2, eq+1);
        }
        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }
    }
    method = getenv("METHOD");


    // Make the response body
    sprintf(content, "QUERY_STRING=%s<p>", buf);
    sprintf(content, "Welcome to add.com: \r\n<p>");
    sprintf(content, "%sTHE Internet addition portal. \r\n<p>", content);
    if (p != NULL) {
        sprintf(content, "%sTHE answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
    } else {
        sprintf(content, "%s Insert Two Number \r\n<p>", content);
        sprintf(content, "%s <form action=\"adder\"><input type=\"text\" placeholder=\"num1\" name=\"num1\"><input type=\"text\" placeholder=\"num2\" name=\"num2\"><button type=\"submit\">add!</button></form>\r\n", content);
    }
    sprintf(content, "%sThanks for visiting!\r\n", content);

    // Generate the HTTP response
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    if(strcasecmp(method, "GET")) exit(0);
    printf("%s", content);
    fflush(stdout);

    exit(0);
}
