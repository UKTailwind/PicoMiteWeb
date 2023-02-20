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

#if LWIP_TCP

/** Define this to a compile-time IP address initialization
 * to connect anything else than IPv4 loopback
 */
#ifndef LWIP_MQTT_EXAMPLE_IPADDR_INIT
#if LWIP_IPV4
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT = IPADDR4_INIT(PP_HTONL(0x144F466D))
#else
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT
#endif
#endif

static ip_addr_t mqtt_ip LWIP_MQTT_EXAMPLE_IPADDR_INIT;

static mqtt_client_t* mqtt_client;


typedef struct mqtt_dns_t_ {
        ip_addr_t *remote;
        int complete;
} mqtt_dns_t;

mqtt_dns_t mqtt_dns = {
        NULL,
        0
}; 

static void mqtt_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    mqtt_dns_t *dns=(mqtt_dns_t *)arg;
    if (ipaddr) {
        dns->remote = (ip_addr_t *)ipaddr;
        dns->complete=1;
        char buff[STRINGSIZE]={0};
        sprintf(buff,"tcp address %s\r\n", ip4addr_ntoa(ipaddr));
        MMPrintString(buff);
    } 
} 

struct mqtt_connect_client_info_t mqtt_client_info =
{
  NULL,
  NULL, /* user */
  NULL, /* pass */
  100,  /* keep alive */
  "A new topic", /* will_topic */
  "Gone", /* will_msg */
  1,    /* will_qos */
  1    /* will_retain */

#if LWIP_ALTCP && LWIP_ALTCP_TLS
  , NULL
#endif
};

static void
mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(data);

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" data cb: len %d, flags %d\n", client_info->client_id,
          (int)len, (int)flags));
}

static void
mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" publish cb: topic %s, len %d\n", client_info->client_id,
          topic, (int)tot_len));
}

static void
mqtt_request_cb(void *arg, err_t err)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" request cb: err %d\n", client_info->client_id, (int)err));
}

static void
mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(client);

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" mqtt connection cb: status %d\n", client_info->client_id, (int)status));
}
#endif /* LWIP_TCP */

void
mqtt_example_init(ip_addr_t *mqtt_ip, int port)
{
#if LWIP_TCP
  mqtt_client = mqtt_client_new();

  mqtt_set_inpub_callback(mqtt_client,
          mqtt_incoming_publish_cb,
          mqtt_incoming_data_cb,
          LWIP_CONST_CAST(void*, &mqtt_client_info));
    printf("Connecting to %s port %u\r\n", ip4addr_ntoa(mqtt_ip), port);

  mqtt_client_connect(mqtt_client,
          mqtt_ip, port,
          mqtt_connection_cb, LWIP_CONST_CAST(void*, &mqtt_client_info),
          &mqtt_client_info);
          Timer4=5000;
          while(Timer4){ProcessWeb();
          }
  mqtt_publish(mqtt_client, mqtt_client_info.will_topic, mqtt_client_info.will_msg, strlen(mqtt_client_info.will_msg), mqtt_client_info.will_qos, mqtt_client_info.will_retain,
            mqtt_request_cb , (void *)&mqtt_client_info);

#endif /* LWIP_TCP */
}
void cmd_mqtt(unsigned char *tp){
                char *IP=GetMemory(STRINGSIZE);
                char *ID=GetMemory(STRINGSIZE);
                int timeout=2000;
                getargs(&tp,7,",");
                if(argc!=7)error("Syntax");
                IP=getCstring(argv[0]);
                int port=getint(argv[2],1,65535);

                TCP_CLIENT_T *state = tcp_client_init();
                TCP_CLIENT=state;
                state->TCP_PORT=port;
                mqtt_client_info.client_user=getCstring(argv[4]);
                mqtt_client_info.client_pass=getCstring(argv[6]);
                strcpy(ID,"PicoMiteWeb");
                strcat(ID,id_out);
                IntToStr(&ID[strlen(ID)],time_us_64(),16);
                mqtt_client_info.client_id=ID;
                 if(!isalpha(*IP) && strchr(IP,'.') && strchr(IP,'.')<IP+4){
                        if(!ip4addr_aton(IP, &state->remote_addr))error("Invalid address format");
                } else {
                        int err = dns_gethostbyname(IP, &state->remote_addr, tcp_dns_found, state);
                        Timer4=timeout;
                        while(!state->complete && Timer4 && !(err==ERR_OK)){{if(startupcomplete)cyw43_arch_poll();}};
                        if(!Timer4)error("Failed to convert web address");
                        state->complete=0;
                }
                mqtt_example_init(&state->remote_addr,state->TCP_PORT);
                return;

}