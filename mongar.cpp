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
#include <pthread.h>

using namespace std;

struct timeval startTV;
struct timeval nextReportTV;
struct timeval intervalTV;
long int count =0;
pthread_t thread1;
int port;
char* hostname = "192.168.1.2";
char *stringString = "local.garden.water.counter";


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

int timeval_subtract (struct timeval *result,struct timeval *x,struct timeval *y )
{
  // Perform the carry for the later subtraction by updating y. 
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  // Compute the time remaining to wait.  tv_usec is certainly positive. 
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  // Return 1 if result is negative. 
  return x->tv_sec < y->tv_sec;
}

void timeval_add(struct timeval *result,struct timeval *x,struct timeval *y )
{
  long int sec =0;
  long int usec_sum = x->tv_usec + y->tv_usec;
  
  while(usec_sum > 1000000 ) {
     usec_sum = usec_sum - 1000000;
     sec = sec + 1;
  }

  sec = y->tv_sec + x->tv_sec + sec;

  // Compute the time remaining to wait.  tv_usec is certainly positive.
  result->tv_sec = sec;
  result->tv_usec = usec_sum;;

}

void report(long int cnt, struct timeval *now, struct timeval *interval) {
   char buf[255];
   long ts = time(NULL);
   sprintf(buf,"%s  %d %u\n",stringString, cnt,ts);   
   struct hostent *server = gethostbyname(hostname);
   send(port, server, buf);
}

void* reportThreadRun(void *ptr) {
    
    while(true) {
       struct timeval nowTV;
       gettimeofday(&nowTV,NULL);
       if(nextReportTV.tv_sec ==0) {
          // first time out
          timeval_add(&nextReportTV, &nowTV, &intervalTV);
          cerr << "first time" << endl;
       } else {
          // ok now we have something real.
          struct timeval deltaTV;
          int neg = timeval_subtract (&deltaTV,&nextReportTV,&nowTV );
          cerr << "other times,  neg:" << neg << " deltaTV " << deltaTV.tv_sec << " count: "<< count << endl;
          if(neg ) {
             // the future
             timeval_add(&nextReportTV, &nowTV, &intervalTV);
             // add the delta which is now negative to get next point
             timeval_add(&nextReportTV, &nextReportTV, &deltaTV);
             cerr << "-- report " << endl;
             report(count,&intervalTV,&nowTV);
          }
       }
       sleep(1);
       cerr << " and loop" << endl;
    }
   
}
static gboolean onTransitionEvent( GIOChannel *channel,
               GIOCondition condition,
               gpointer user_data )
{
    GError *error = 0;
    gsize bytes_read = 0;
    int buf_sz = 255;
    char buf[buf_sz];
    
    g_io_channel_seek_position( channel, 0, G_SEEK_SET, 0 );
    GIOStatus rc = g_io_channel_read_chars( channel,
                                            buf, buf_sz - 1,
                                            &bytes_read,
                                            &error );
    count++;

    if(rc == G_IO_STATUS_NORMAL) {
        cerr << "  data:" << buf << endl;
    } else {
        cerr << "something was wrong, rc = " << rc << endl;
    }    
    // thank you, call again!
    return 1;
}

int main( int argc, char** argv )
{
    int  iret1;
    GMainLoop* loop = g_main_loop_new( 0, 0 );

    intervalTV.tv_sec=60;
    intervalTV.tv_usec =0;

    char *portString = "2003";
    char *intervalString = "60";
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
    intervalTV.tv_sec = atoi(intervalString);

    cout << " will send data to " << hostname << ":" << port << " to key " << stringString << " at interval " << intervalTV.tv_sec << " seconds " << endl;

    gettimeofday(&startTV,NULL);    
    nextReportTV.tv_sec =0;;    
    int fd = open( "/sys/class/gpio/gpio30/value", O_RDONLY | O_NONBLOCK );
    GIOChannel* channel = g_io_channel_unix_new( fd );
    GIOCondition cond = GIOCondition( G_IO_PRI );
    guint id = g_io_add_watch( channel, cond, onTransitionEvent, 0 );
  

    iret1 = pthread_create( &thread1, NULL, reportThreadRun, NULL);
 
    g_main_loop_run( loop );
    pthread_join( thread1, NULL);
}
