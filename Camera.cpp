#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMPlugin.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 12345
#define MSG_ADD_DATA_REF 0x01000000 // Custom message for adding a data reference

static XPLMCommandRef gWeaponSelectUp = NULL;
static XPLMCommandRef gFireAnyShell = NULL;
static XPLMDataRef gParachuteFlares = NULL;
static XPLMDataRef gDeployParachute = NULL;

int sock = 0, valread, client_fd;
struct sockaddr_in serv_addr;

void DebugCallback(const char* msg);
void* socketThread(void* arg);
void processCommand(char* command);

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
    strcpy(outName, "SocketOn");
    strcpy(outSig, "Plugin");
    strcpy(outDesc, "For GCU connect");

    gWeaponSelectUp = XPLMFindCommand("sim/weapons/weapon_select_up");
    gFireAnyShell = XPLMFindCommand("sim/weapons/fire_any_shell");
    gParachuteFlares = XPLMFindDataRef("sim/cockpit/switches/parachute_on");
    gDeployParachute = XPLMFindDataRef("sim/cockpit2/switches/parachute_deploy");

    XPLMSetErrorCallback(DebugCallback);

    int server_fd, opt = 1;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        printf("\nXPlaneSocketPlugin > Server socket creation error \n");
        return 0;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        printf("\nXPlaneSocketPlugin > setsockopt error \n");
        return 0;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        printf("\nXPlaneSocketPlugin > bind failed \n");
        return 0;
    }
    if (listen(server_fd, 3) < 0)
    {
        printf("\nXPlaneSocketPlugin > listen failed \n");
        return 0;
    }
    if ((sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    {
        printf("\nXPlaneSocketPlugin > accept failed \n");
        return 0;
    }

    printf("\nXPlaneSocketPlugin > Server socket created and listening on IP: %s, Port: %d\n", "127.0.0.1", PORT);

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, socketThread, NULL);

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API int XPluginEnable(void)
{
    return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void *inParam)
{
   if(inMessage == MSG_ADD_DATA_REF) {
      char* command = (char*)inParam;

      if (strcmp(command, "weapon_select_up") == 0) {
          XPLMCommandOnce(gWeaponSelectUp);
      } else if (strcmp(command, "fire_any_shell") == 0) {
          XPLMCommandOnce(gFireAnyShell);
      } else if (strcmp(command, "parachute_flares") == 0) {
          XPLMSetDatai(gParachuteFlares, 1);
      } else if (strcmp(command, "deploy_parachute") == 0) {
          XPLMSetDatai(gDeployParachute, 1);
      }
   }
}

void DebugCallback(const char* msg)
{
}

void processCommand(char* command) {
   char* commandCopy = strdup(command); 
   XPLMSendMessageToPlugin(XPLMGetMyID(), MSG_ADD_DATA_REF, commandCopy);
}

void* socketThread(void* arg)
{
   char command[2048];
   while (1) {
       memset(command, 0, sizeof(command));
       valread = read(sock , command, 2048); 
    
       if(valread == 0) {
           printf("\nClient disconnected\n");
           break;
       }

       printf("\nReceived from client: %s\n", command); 

       processCommand(command);
   }
   
   return NULL;
}