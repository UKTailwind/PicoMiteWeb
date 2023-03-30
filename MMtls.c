/***********************************************************************************************************************
PicoMite MMBasic

custom.c

<COPYRIGHT HOLDERS>  Geoff Graham, Peter Mather
Copyright (c) 2021, <COPYRIGHT HOLDERS> All rights reserved. 
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: 
1.	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2.	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.
3.	The name MMBasic be used when referring to the interpreter in any documentation and promotional material and the original copyright message be displayed 
    on the console at startup (additional copyright messages may be added).
4.	All advertising materials mentioning features or use of this software must display the following acknowledgement: This product includes software developed 
    by the <copyright holder>.
5.	Neither the name of the <copyright holder> nor the names of its contributors may be used to endorse or promote products derived from this software 
    without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDERS> AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDERS> BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

************************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
/*#define TLS_CLIENT_HTTP_REQUEST  "GET /random_joke HTTP/1.1\r\n" \
                                 "Host: official-joke-api.appspot.com \r\n" \
                                 "Connection: close\r\n" \
                                 "\r\n"*/
#define TLS_CLIENT_TIMEOUT_SECS  60
#define DEBUG_printf 

typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;
    bool complete;
    int port;
    ip_addr_t remote_addr;
    char *TLS_CLIENT_SERVER;
    uint8_t *buffer;
    int BUF_SIZE;
    int buffer_len;
} TLS_CLIENT_T;

TLS_CLIENT_T *TLS_CLIENT;
static struct altcp_tls_config *tls_config = NULL;

static err_t tls_client_close(void *arg) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    err_t err = ERR_OK;

    state->complete = true;
    if (state->pcb != NULL) {
        altcp_arg(state->pcb, NULL);
        altcp_poll(state->pcb, NULL, 0);
        altcp_recv(state->pcb, NULL);
        altcp_err(state->pcb, NULL);
        err = altcp_close(state->pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            altcp_abort(state->pcb);
            err = ERR_ABRT;
        }
        state->pcb = NULL;
    }
//    altcp_tls_free_config(tls_config);
    return err;
}

static err_t tls_client_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    if (err != ERR_OK) {
        DEBUG_printf("connect failed %d\n", err);
        return tls_client_close(state);
    }
    DEBUG_printf("connected to server\n");
/*    printf("connected to server, sending request\n");
    err = altcp_write(state->pcb, TLS_CLIENT_HTTP_REQUEST, strlen(TLS_CLIENT_HTTP_REQUEST), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("error writing data, err=%d", err);
        return tls_client_close(state);
    }*/
    state->complete=1;
    return ERR_OK;
}

static err_t tls_client_poll(void *arg, struct altcp_pcb *pcb) {
    DEBUG_printf("timed out");
    return tls_client_close(arg);
}

static void tls_client_err(void *arg, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    DEBUG_printf("tls_client_err %d\n", err);
    state->pcb = NULL; /* pcb freed by lwip when _err function is called */
}

static err_t tls_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    if (!p) {
        DEBUG_printf("connection closed\n");
        return tls_client_close(state);
    }

    if (p->tot_len > 0) {
        /* For simplicity this examples creates a buffer on stack the size of the data pending here, 
           and copies all the data to it in one go.
           Do be aware that the amount of data can potentially be a bit large (TLS record size can be 16 KB),
           so you may want to use a smaller fixed size buffer and copy the data to it using a loop, if memory is a concern */
        DEBUG_printf("recv %d err %d\r\n", p->tot_len, err);
        // Receive the buffer
        const uint16_t buffer_left = state->BUF_SIZE - state->buffer_len;
        state->buffer_len += pbuf_copy_partial(p, state->buffer + state->buffer_len,
                                               p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        uint64_t *x=(uint64_t *)state->buffer;
        x--;
        *x=state->buffer_len;
        altcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);

    return ERR_OK;
}

static void tls_client_connect_to_server_ip(const ip_addr_t *ipaddr, TLS_CLIENT_T *state)
{
    err_t err;
    u16_t port = state->port;

    DEBUG_printf("connecting to server IP %s port %d\n", ipaddr_ntoa(ipaddr), port);
    err = altcp_connect(state->pcb, ipaddr, port, tls_client_connected);
    if (err != ERR_OK)
    {
        DEBUG_printf(stderr, "error initiating connect, err=%d\n", err);
        tls_client_close(state);
    }
}

static void tls_client_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg)
{
    if (ipaddr)
    {
        DEBUG_printf("DNS resolving complete\n");
        tls_client_connect_to_server_ip(ipaddr, (TLS_CLIENT_T *) arg);
    }
    else
    {
        DEBUG_printf("error resolving hostname %s\n", hostname);
        tls_client_close(arg);
    }
}


static bool tls_client_open(const char *hostname, void *arg) {
    err_t err;
    ip_addr_t server_ip;
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;

    state->pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    if (!state->pcb) {
        printf("failed to create pcb\n");
        return false;
    }

    altcp_arg(state->pcb, state);
    altcp_poll(state->pcb, tls_client_poll, TLS_CLIENT_TIMEOUT_SECS * 2);
    altcp_recv(state->pcb, tls_client_recv);
    altcp_err(state->pcb, tls_client_err);

    /* Set SNI */
    mbedtls_ssl_set_hostname(altcp_tls_context(state->pcb), hostname);

    DEBUG_printf("resolving %s\n", hostname);

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();

    err = dns_gethostbyname(hostname, &server_ip, tls_client_dns_found, state);
    if (err == ERR_OK)
    {
        /* host is in DNS cache */
        tls_client_connect_to_server_ip(&server_ip, state);
    }
    else if (err != ERR_INPROGRESS)
    {
        DEBUG_printf("error initiating DNS resolving, err=%d\n", err);
        tls_client_close(state->pcb);
    }

    cyw43_arch_lwip_end();

    return err == ERR_OK || err == ERR_INPROGRESS;
}

// Perform initialisation
static TLS_CLIENT_T* tls_client_init(void) {
    TLS_CLIENT_T *state = calloc(1, sizeof(TLS_CLIENT_T));
    if (!state) {
        printf("failed to allocate state\n");
        return NULL;
    }
    state->TLS_CLIENT_SERVER = calloc(1,STRINGSIZE);
    return state;
}

/*void run_tls_client_test(void) {
    // No CA certificate checking 
    tls_config = altcp_tls_create_config_client(NULL, 0);

    TLS_CLIENT_T *state = tls_client_init();
    if (!state) {
        return;
    }
    if (!tls_client_open(TLS_CLIENT_SERVER, state)) {
        return;
    }
    while(!state->complete) {
        // the following #ifdef is only here so this same example can be used in multiple modes;
        // you do not need it in your code
        ProcessWeb();
    }
    free(state);
    altcp_tls_free_config(tls_config);
}*/

int cmd_tls() {
    unsigned char *tp = checkstring(cmdline, "OPEN TLS CLIENT");
    if(tp) {
           int timeout=5000;
            getargs(&tp,5,",");
            if(argc<3)error("Syntax");
            TLS_CLIENT_T *state = tls_client_init();
            strcpy(state->TLS_CLIENT_SERVER,getCstring(argv[0]));
            state->port=getint(argv[2],1,65535);
            TLS_CLIENT=state;
            if(argc==5)timeout=getint(argv[4],1,100000);
            tls_config = altcp_tls_create_config_client(NULL, 0);
            tls_client_open(state->TLS_CLIENT_SERVER, state);
            Timer4=timeout;
            while(!state->complete && Timer4) {
                // the following #ifdef is only here so this same example can be used in multiple modes;
                // you do not need it in your code
                ProcessWeb();
            }
            if(Timer4==0){
                 error("Connect timed out");
            }
        return 1;
    }
    tp=checkstring(cmdline, "TLS CLIENT REQUEST");
    if(tp){
            void *ptr1 = NULL;
            int64_t *dest=NULL;
            uint8_t *q=NULL;
            int size, timeout=5000;
            int header=1;
            TLS_CLIENT_T *state = TLS_CLIENT;
            getargs(&tp,7,",");
            if(!state)error("No connection");
            if(!state->complete)error("Open failed to complete");
            if(argc<3)error("Syntax");
            char *request=GetTempMemory(STRINGSIZE*2);
            strcpy(request,getCstring(argv[0]));
            if(argc==7)header=getint(argv[6],0,1);
            if(header){
                if(request[strlen(request)-2]==10)request[strlen(request)-2]=0;
                strcat(request," HTTP/1.1\r\nHost: ");
                strcat(request,state->TLS_CLIENT_SERVER);
                strcat(request, "\r\nConnection: close\r\n\r\n");
            }
//            strcpy(request,TLS_CLIENT_HTTP_REQUEST);
            ptr1 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
            if(vartbl[VarIndex].type & T_INT) {
                    if(vartbl[VarIndex].dims[1] != 0) error("Invalid variable");
                    if(vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                            error("Argument 2 must be integer array");
                    }
                    size=(vartbl[VarIndex].dims[0] - OptionBase)*8;
                    dest = (long long int *)ptr1;
                    dest[0]=0;
                    q=(uint8_t *)&dest[1];
            } else error("Argument 2 must be integer array");
            if(argc>=5 && *argv[4])timeout=getint(argv[4],1,100000);
            state->BUF_SIZE=size;
            state->buffer=q;
            state->buffer_len=0;
            err_t err = altcp_write(state->pcb, request, strlen(request), 0);
//            err_t err = tcp_write(state->tcp_pcb, request, strlen(request), 0);
            if (err != ERR_OK) {
                printf("error writing data, err=%d", err);
                tls_client_close(state);
                error("write failed %",err);
            }
            Timer4=timeout;
            while(!state->buffer_len && Timer4)ProcessWeb();
            if(!Timer4)error("No response from server");
            return 1;
    }
    tp=checkstring(cmdline, "CLOSE TLS CLIENT");
    if(tp){
            TLS_CLIENT_T *state = TLS_CLIENT;
            if(!state)error("No connection");
            tls_client_close(state) ;
            free(state->TLS_CLIENT_SERVER);
            free(state);
            TCP_CLIENT=NULL;
            if(tls_config)altcp_tls_free_config(tls_config);
            return 1;
     }
    return 0;
}
