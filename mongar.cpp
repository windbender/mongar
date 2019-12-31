#include <errno.h>
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
char* hostname = (char*) "192.168.1.2";
char *stringString = (char*) "local.garden.water.counter";

char* windDirString = (char*) "/sys/bus/iio/devices/iio:device0/in_voltage0_raw";

struct CountUp {
    string gpio;
    long int count;
    string reportingString;
    guint id;
};

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int lenArr = 0;


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
    close(sockfd);
    return 0;
}
//Function definitions
int readADC(char* windFile)
{
    int fd;          //file pointer
    char val[4];     //holds up to 4 digits for ADC value

    fd = open(windFile, O_RDONLY);     //open ADC as read only

    //Will trigger if the ADC is not enabled
    if (fd < 0) {
        error("ADC - problem opening ADC");
    }//end if

    read(fd, &val, 4);     //read ADC ing val (up to 4 digits 0-1799)
    close(fd);     //close file and stop reading

    return atoi(val);     //returns an integer value (rather than ascii)
}//end read ADC()

static float makeDegreeFromReading(float reading) {
   struct WindDir {
      float start;
      float end;
      float degree;
   };

   WindDir winddir[]  = {
      {0,27.5,112.5},
      {27.5,33,67.5},
      {33,42,90},
      {42,63,157.5},
      {63,93,135},
      {93,122,202.5},
      {122,180,180},
      {180,251.5,22.5},
      {251.5,372.5,45},
      {372.5,496,247.5},
      {496,612.5,225},
      {612.5,853.5,337.5},
      {853.5,1123.5,0},
      {1123.5,1497,292.5},
      {1497,2230,315},
      {2230,4096,270}
   };
   int lenArr2 = sizeof(winddir)/sizeof(winddir[0]);
   for(int i =0; i < lenArr2; i++) {
      if(winddir[i].start <= reading && winddir[i].end > reading ) {
         return winddir[i].degree;
      } else {
      }
   }
   return -1;
}

float currentDir = 0;
static gboolean readWindDir(gpointer user_data) {
   int val = readADC(windDirString);
   float dir = makeDegreeFromReading(1.0* val);   
   float ratio = 0.01;
   currentDir = currentDir * ( 1 - ratio) + ratio * dir; 
   return 1; 
}

static gboolean printWindDir(gpointer user_data) {

   cerr << "wind reading :" << currentDir << endl;
   return 1;
}

static gboolean sendTimerCallback (gpointer user_data) {
    CountUp* countups = (CountUp*)user_data;
    for(int i=0; i < lenArr; i++) {
        long int cnt = countups[i].count;
        string reportString = countups[i].reportingString;

        char buf[255];
        long ts = time(NULL);
        sprintf(buf,"%s %d  %u\n",reportString.c_str(),cnt,ts);

//        cerr << "  send: " << buf << endl;

        struct hostent *server = gethostbyname("192.168.1.2");
        send(2003, server, buf);
    }
    {

        char buf[255];
        long ts = time(NULL);
        sprintf(buf,"%s %f %u\n","winddir",currentDir,ts);

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
        cerr << "  data: " << pCountUpStruct->reportingString << ": "<< pCountUpStruct->count << endl;
    } else {
        cerr << "something was wrong, rc = " << rc << endl;
    }
    pCountUpStruct->count++;

    // thank you, call again!
    return 1;
}

int enableTransitions(string gpio,const char* val) {
    string gpioProg = "/sys/class/gpio/"+gpio+"/edge";

    FILE* f = fopen(gpioProg.c_str(), "w");
    if (f == NULL) {
        fprintf(stderr, "Unable to open path for writing\n");
        return 1;
    }

    fprintf(f, "%s\n",val);
    fclose(f);
    return 0;
}

int main( int argc, char** argv )
{

    char *portString = (char*)"2003";
    char *intervalString = (char*)"60";
    char *hostname = (char*)"192.168.1.2";

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
    int interval = atoi(intervalString);

    CountUp countups[]={
            { "gpio30", 0, "waterCount" },
            { "gpio31", 0, "rainCount"  },
            { "gpio20", 0, "windCount"  }
    };

    GMainLoop* loop = g_main_loop_new( 0, 0 );
    
    lenArr = sizeof(countups)/sizeof(countups[0]);
    for(int i=0; i < lenArr; i++) {
        int x = enableTransitions(countups[i].gpio,"both");
        if(x != 0) {
           error("unable to setup transitions");
        }
    } 

    for(int i=0; i < lenArr; i++) {
        string gpioDesc = "/sys/class/gpio/"+countups[i].gpio+"/value";
        int fd = open( gpioDesc.c_str(), O_RDONLY | O_NONBLOCK );
        GIOChannel* channel = g_io_channel_unix_new( fd );
        GIOCondition cond = GIOCondition( G_IO_PRI );
        guint id = g_io_add_watch( channel, cond, onTransitionEvent, &(countups[i]) );
    }


    guint sendIntervalSecs = 60;
    //guint sendIntervalSecs = 4;
    g_timeout_add_seconds_full(G_PRIORITY_DEFAULT_IDLE,sendIntervalSecs,sendTimerCallback,countups,NULL); // G_PRIORITY_DEFAULT_IDLE
    g_timeout_add(10,readWindDir,countups);
    g_main_loop_run( loop );
}
