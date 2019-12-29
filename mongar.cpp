#include <sys/time.h>
#include <glib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ostream>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;

#define MAX_BUF 64     //This is plenty large

int port;
char* hostname = "192.168.1.2";
char *stringString = "local.garden.water.counter";


struct CountUp {
    string gpio;
    long int count;
    string reportingString;
    guint id;
};

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int send(int portno, struct hostent* server, char *msg)
{
    int sockfd, n;

    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    n = write(sockfd,msg,strlen(msg));
    if (n < 0)
         error("ERROR writing to socket");
    return 0;
}
//Function definitions
int readADC(unsigned int pin)
{
    int fd;          //file pointer
    char buf[MAX_BUF];     //file buffer
    char val[4];     //holds up to 4 digits for ADC value

    //Create the file path by concatenating the ADC pin number to the end of the string
    //Stores the file path name string into "buf"
    snprintf(buf, sizeof(buf), "/sys/devices/ocp.2/helper.14/AIN%d", pin);     //Concatenate ADC file name

    fd = open(buf, O_RDONLY);     //open ADC as read only

    //Will trigger if the ADC is not enabled
    if (fd < 0) {
        perror("ADC - problem opening ADC");
    }//end if

    read(fd, &val, 4);     //read ADC ing val (up to 4 digits 0-1799)
    close(fd);     //close file and stop reading

    return atoi(val);     //returns an integer value (rather than ascii)
}//end read ADC()

static gboolean sendTimerCallback (gpointer user_data) {
    CountUp* countups = (CountUp*)user_data;
    int lenArr = sizeof(countups)/sizeof(countups[0]);
    for(int i=0; i < lenArr; i++) {
        long int cnt = countups[i].count;
        string reportString = countups[i].reportingString;

        char buf[255];
        long ts = time(NULL);
        sprintf(buf,"%s %d  %u\n",reportString.c_str(),cnt,ts);
        struct hostent *server = gethostbyname("192.168.1.2");
        send(2003, server, buf);
    }

    return 1;
}

static gboolean onTransitionEvent( GIOChannel *channel,
               GIOCondition condition,
               gpointer user_data )
{
    GError *error = 0;
    gsize bytes_read = 0;
    int buf_sz = 255;
    char buf[buf_sz];
    CountUp* pCountUpStruct = (CountUp*) user_data;
    
    g_io_channel_seek_position( channel, 0, G_SEEK_SET, 0 );
    GIOStatus rc = g_io_channel_read_chars( channel,
                                            buf, buf_sz - 1,
                                            &bytes_read,
                                            &error );
    if(rc == G_IO_STATUS_NORMAL) {
        cerr << "  data:" << buf << endl;
    } else {
        cerr << "something was wrong, rc = " << rc << endl;
    }
    pCountUpStruct->count++;

    // thank you, call again!
    return 1;
}


int main( int argc, char** argv )
{

    char *portString = "2003";
    char *intervalString = "60";
    char *hostname = "192.168.1.2";
    char *intervalString = "120";

    int opt;
    while ((opt = getopt(argc, argv, "p:h:s:i:")) != -1) {
        switch (opt) {
            case 'p':
                portString = optarg;
                break;
            case 'h':
                hostname = optarg;
                break;
            case 's':
                stringString = optarg;
                break;
            case 'i':
                intervalString = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s \n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    port = atoi(portString);
    interval = atoi(intervalString);

    CountUp countups[]={
            {  "gpio30", 0, "waterCount" },
            {  "James", 0, "waterCount"  },
            { "John", 0, "waterCount"  },
            { "Mike", 0, "waterCount"  }
    };

    GMainLoop* loop = g_main_loop_new( 0, 0 );


    int lenArr = sizeof(countups)/sizeof(countups[0]);
    for(int i=0; i < lenArr; i++) {
        string gpioDesc = "/sys/class/gpio/"+countups[i].gpio+"/value";
        int fd = open( gpioDesc.c_str(), O_RDONLY | O_NONBLOCK );
        GIOChannel* channel = g_io_channel_unix_new( fd );
        GIOCondition cond = GIOCondition( G_IO_PRI );
        guint id = g_io_add_watch( channel, cond, onTransitionEvent, &(countups[i]) );
    }

    guint sendIntervalSecs = 60 *2;
    g_timeout_add_seconds(sendIntervalSecs,sendTimerCallback,countups);
    g_main_loop_run( loop );
}
