# Simple HTTP Server
This is a very simple, single file HTTP server using select() system call to select the available socket FD to R/W.

## TODOs
- Adding full support on HTTP GET request 
    - Returning 404 page if the requested resource was not found.
    - Hosting other documents under the same folder. 
