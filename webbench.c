/*
 * (C) Radim Kolar 1997-2004
 * This is free software, see GNU Public License version 2 for
 * details.
 *
 * Simple forking WWW Server benchmark:
 *
 * Usage:
 *   webbench --help
 *
 * Return codes:
 *    0 - sucess
 *    1 - benchmark failed (server is not on-line)
 *    2 - bad param
 *    3 - internal error, fork failed
 * 
 */ 
#include "socket.c"
#include <unistd.h>
#include <sys/param.h>
#include <rpc/types.h>
#include <getopt.h>
#include <strings.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "parson.h"
#include "url.h"

/* values */
volatile int timerexpired=0;
int speed=0;
int failed=0;
int bytes=0;
/* globals */
int http10=1; /* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */
/* Allow: GET, HEAD, OPTIONS, TRACE, POST, DELET, PUT, CONNECT */
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3
#define METHOD_POST 4
#define METHOD_DELET 5
#define METHOD_PUT 6
#define METHOD_CONNECT 7
#define PROGRAM_VERSION "1.5.1"
int method=METHOD_GET;
int clients=1;
int force=0;
int force_reload=0;
int proxyport=80;
char *proxyhost=NULL;
int benchtime=30;
/* internal */
int mypipe[2];
char host[MAXHOSTNAMELEN];
#define REQUEST_SIZE 8192
#define FIELD_SIZE 100
char request[REQUEST_SIZE];
char *requestBody=NULL;
int json_file_total_lines=1;
int json_file_current_lines=0;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static const struct option long_options[]=
{
 {"force",no_argument,&force,1},
 {"reload",no_argument,&force_reload,1},
 {"time",required_argument,NULL,'t'},
 {"help",no_argument,NULL,'?'},
 {"http09",no_argument,NULL,'9'},
 {"http10",no_argument,NULL,'1'},
 {"http11",no_argument,NULL,'2'},
 {"get",no_argument,&method,METHOD_GET},
 {"head",no_argument,&method,METHOD_HEAD},
 {"options",no_argument,&method,METHOD_OPTIONS},
 {"trace",no_argument,&method,METHOD_TRACE},
 {"post",no_argument,&method,METHOD_POST},
 {"delet",no_argument,&method,METHOD_DELET},
 {"put",no_argument,&method,METHOD_PUT},
 {"connect",no_argument,&method,METHOD_CONNECT},
 {"version",no_argument,NULL,'V'},
 {"proxy",required_argument,NULL,'p'},
 {"clients",required_argument,NULL,'c'},
 {"data",required_argument,NULL,'d'},/*read POST data*/
 {NULL,0,NULL,0}
};

/* prototypes */
static void benchcore(const char* host,const int port, const char *request);
static int bench(void);
static void build_request(const char *url, const char *request_body_param, int UA);
static char * read_json_file(const char *filename, char field[FIELD_SIZE][FIELD_SIZE]);

static void alarm_handler(int signal)
{
   timerexpired=1;
}	

static void usage(void)
{
   fprintf(stderr,
	"webbench [option]... URL\n"
	"  -f|--force               Don't wait for reply from server.\n"
	"  -r|--reload              Send reload request - Pragma: no-cache.\n"
    "  -d|--data                Read POST body from csv or json file.\n"
    "  -F|--Field               Read the Field added to request body from csv or json file.\n"
	"  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
	"  -p|--proxy <server:port> Use proxy server for request.\n"
	"  -c|--clients <n>         Run <n> HTTP clients at once. Default one.\n"
	"  -9|--http09              Use HTTP/0.9 style requests.\n"
	"  -1|--http10              Use HTTP/1.0 protocol.\n"
	"  -2|--http11              Use HTTP/1.1 protocol.\n"
    "  -u|--User-Agent          Change User-Agent.\n"
           "  1 for WeCaht iPhone\t 2 for WeChat Android\t 3 for iPhone Safari\n"
           "  4 for Android Chrome\t 5 for Windows IE11\t 6 for Windows IE10\n"
           "  7 for Windows Edge\t 8 for Windows Chrome\t 9 for Windows FireFox\n"
           "  10 for Mac Safari\t 11 for Mac Chrome\t 12 for Mac FireFox\n"
           "  Empty for WebBench Program Version.\n"
	"  --get                    Use GET request method.\n"
	"  --head                   Use HEAD request method.\n"
	"  --options                Use OPTIONS request method.\n"
	"  --trace                  Use TRACE request method.\n"
	"  --post                   Use TRACE request method.\n"
	"  --delet                  Use TRACE request method.\n"
	"  --put                    Use TRACE request method.\n"
	"  --connect                Use TRACE request method.\n"
	"  -?|-h|--help             This information.\n"
	"  -V|--version             Display program version.\n"
	);
}

int main(int argc, char *argv[])
{
 int opt=0;
 int options_index=0;
 char *tmp=NULL;
 int UA=0;//移动requestBody到全局变量，因为考虑到会循环更改。但是UA filename field只会读取／赋值一次，因此仍然放在main函数
 char *filename=NULL;
 char *sep = ",";
 char field[FIELD_SIZE][FIELD_SIZE]={{0}};
 char *optarg_param=NULL;
 int tmpbool=0;
 unsigned long j=0;
 unsigned long x=0;
 unsigned long k=0;

 if(argc==1)
 {
	  usage();
          return 2;
 } 

 while((opt=getopt_long(argc,argv,"912Vfrt:p:c:d:u:F:?h",long_options,&options_index))!=EOF )
 {
  switch(opt)
  {
   case  0 : break;
   case 'f': force=1;break;
   case 'r': force_reload=1;break; 
   case '9': http10=0;break;
   case '1': http10=1;break;
   case '2': http10=2;break;
   case 'V': printf(PROGRAM_VERSION"\n");exit(0);
   case 't': benchtime=atoi(optarg);break;	     
   case 'p': 
	     /* proxy server parsing server:port */
	     tmp=strrchr(optarg,':');
	     proxyhost=optarg;
	     if(tmp==NULL)
	     {
		     break;
	     }
	     if(tmp==optarg)
	     {
		     fprintf(stderr,"Error in option --proxy %s: Missing hostname.\n",optarg);
		     return 2;
	     }
	     if(tmp==optarg+strlen(optarg)-1)
	     {
		     fprintf(stderr,"Error in option --proxy %s Port number is missing.\n",optarg);
		     return 2;
	     }
	     *tmp='\0';
	     proxyport=atoi(tmp+1);break;
   case ':':
   case 'h':
   case '?': usage();return 2;break;
   case 'c': clients=atoi(optarg);break;
   case 'F': 
        tmpbool=1;//mark that param F is called
         //optarg=strcat(optarg,","); 
        for(x=strlen(optarg)+1;x>0;x--){
             optarg[x]=optarg[x-1];
         }
        optarg[0]=',';
         //printf("%s\n",optarg);
         //Add "," as the first character in optarg
         x=0;
        do {
        
            tmp = strrchr(optarg, ',');
            for(j=0;j<strlen(optarg);j++){
                field[x][j]=tmp[j]; 
            }  
            //printf("%s\n ",tmp);
            optarg[strlen(optarg)-strlen(tmp)]='\0';
        
            ++x;
        } while(optarg && *optarg); 
        //Slice optarg using "," and store into field[][];
        for(j=0;j<x;j++){
            for(k=0;k<strlen(field[j]);k++){
                field[j][k]=field[j][k+1];
            }
        }
        // for(j=0;j<x;j++){
        //     for(k=0;k<strlen(field[j]);k++){
        //         printf("%c",field[j][k]);
        //     }
        //     printf("\n");
        // }
        //remove all "," in field
        //在这里应该把所有的参数都切分开来，并且删除掉逗号，存到field二维数组中
        //“n”,"a","m","e",NULL......
        //"a","g","e",NULL.....
        //"s","e","x",NULL.....
          break;
   case 'd': 
         //getTheFileName and store to a var
          filename=optarg;
          break;
   case 'u':
          UA=atoi(optarg);
          break;
  }
 }
 //continue with line184 and line 187 we call the function read_json_file to add to requestBody
 if(filename!=NULL && tmpbool==1) {
     tmp=read_json_file(filename,field);
     strcat(request,tmp); 
 }else if(tmpbool==0){
     printf(ANSI_COLOR_RED "\r\nField is not specified, WILL NOT READ JSON FILE\r\n\r\nPLEASE USING -F TO SPECIFY FIELD\r\n\r\n" ANSI_COLOR_RESET ANSI_COLOR_RESET);
 }else if(filename==NULL){
     printf(ANSI_COLOR_RED "\r\nFile name is not specified, WILL NOT READ JSON FILE\r\n\r\nPLEASE USING -d TO OPEN JSON FILE\r\n\r\n" ANSI_COLOR_RESET ANSI_COLOR_RESET);
 }

 if(optind==argc) {
    fprintf(stderr,"webbench: Missing URL!\n");
    usage();
    return 2;
 }

 if(clients==0) clients=1;
 if(benchtime==0) benchtime=60;
 /* Copyright */
 fprintf(stderr,"Webbench - Simple Web Benchmark "PROGRAM_VERSION"\n"
	 "Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.\n"
	 );
 
 /* print bench info */
 printf("\nBenchmarking: ");
 switch(method)
 {
	 case METHOD_GET:
	 default:
		 printf("GET");break;
	 case METHOD_OPTIONS:
		 printf("OPTIONS");break;
	 case METHOD_HEAD:
		 printf("HEAD");break;
	 case METHOD_TRACE:
		 printf("TRACE");break;
     case METHOD_POST:
		 printf("POST");break;
     case METHOD_DELET:
		 printf("DELET");break;
     case METHOD_PUT:
		 printf("PUT");break;
     case METHOD_CONNECT:
		 printf("CONNECT");break;
 }
 build_request(argv[optind],requestBody,UA);
 printf(" %s",argv[optind]);
 switch(http10)
 {
	 case 0: printf(" (using HTTP/0.9)");break;
	 case 2: printf(" (using HTTP/1.1)");break;
 }
 printf("\n");
 if(clients==1) printf("1 client");
 else
   printf("%d clients",clients);

 printf(", running %d sec", benchtime);
 if(force) printf(", early socket close");
 if(proxyhost!=NULL) printf(", via proxy server %s:%d",proxyhost,proxyport);
 if(force_reload) printf(", forcing reload");
 printf(".\n");
 return bench();
}

void build_request(const char *url, const char *request_body_param, int UA)
{
  char tmp[10];
  int i;

  bzero(host,MAXHOSTNAMELEN);
  bzero(request,REQUEST_SIZE);

  if(force_reload && proxyhost!=NULL && http10<1) http10=1;
  if(method==METHOD_HEAD && http10<1) http10=1;
  if(method==METHOD_OPTIONS && http10<2) http10=2;
  if(method==METHOD_TRACE && http10<2) http10=2;
  if(method==METHOD_POST && http10<2) http10=2;
  if(method==METHOD_DELET && http10<2) http10=2;
  if(method==METHOD_PUT && http10<2) http10=2;
  if(method==METHOD_CONNECT && http10<2) http10=2;

  switch(method)
  {
	  default:
	  case METHOD_GET: strcpy(request,"GET");break;
	  case METHOD_HEAD: strcpy(request,"HEAD");break;
	  case METHOD_OPTIONS: strcpy(request,"OPTIONS");break;
	  case METHOD_TRACE: strcpy(request,"TRACE");break;
	  case METHOD_POST: strcpy(request,"POST");break;
	  case METHOD_DELET: strcpy(request,"DELET");break;
	  case METHOD_PUT: strcpy(request,"PUT");break;
	  case METHOD_CONNECT: strcpy(request,"CONNECT");break;
  }
		  
  strcat(request," ");

  if(NULL==strstr(url,"://"))
  {
	  fprintf(stderr, "\n%s: is not a valid URL.\n",url);
	  exit(2);
  }
  if(strlen(url)>1500)
  {
         fprintf(stderr,"URL is too long.\n");
	 exit(2);
  }
  if(proxyhost==NULL)
	   if (0!=strncasecmp("http://",url,7)) 
	   { fprintf(stderr,"\nOnly HTTP protocol is directly supported, set --proxy for others.\n");
             exit(2);
           }
  /* protocol/host delimiter */
  i=strstr(url,"://")-url+3;
  /* printf("%d\n",i); */

  if(strchr(url+i,'/')==NULL) {
                                fprintf(stderr,"\nInvalid URL syntax - hostname don't ends with '/'.\n");
                                exit(2);
                              }
  if(proxyhost==NULL)
  {
   /* get port from hostname */
   if(index(url+i,':')!=NULL &&
        index(url+i,':')<index(url+i,'/'))
   {
	    strncpy(host,url+i,strchr(url+i,':')-url-i);
	    bzero(tmp,10);
	    strncpy(tmp,index(url+i,':')+1,strchr(url+i,'/')-index(url+i,':')-1);
	    /* printf("tmp=%s\n",tmp); */
	    proxyport=atoi(tmp);
	    if(proxyport==0) proxyport=80;
   } else
   {
     strncpy(host,url+i,strcspn(url+i,"/"));
   }
   // printf("Host=%s\n",host);
   strcat(request+strlen(request),url+i+strcspn(url+i,"/"));
  } else
  {
   // printf("ProxyHost=%s\nProxyPort=%d\n",proxyhost,proxyport);
   strcat(request,url);
  }
  if(http10==1)
	  strcat(request," HTTP/1.0");
  else if (http10==2)
	  strcat(request," HTTP/1.1");
  strcat(request,"\r\n");
  if(http10>0)
	  switch(UA){
          case 0:
              strcat(request,"User-Agent: Mozilla/5.0 (iPhone; CPU iPhone OS 9_2 like Mac OS X) AppleWebKit/601.1.46 (KHTML, like Gecko) Mobile/13C75 MicroMessager/6.3.9");//ForWeCahtIniPhone
              break;
          case 1:
              strcat(request,"User-Agent: Mozilla/5.0 (Linux; Android 4.4.2; HUAWEI G750-T20 Build/HuaweiG750-T20) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/30.0.0.0 Mobile MicroMessager/6.3.9 NetType/WIFI Language/zh_CN");//ForWeCahtInAndroid
              break;
          case 2:
              strcat(request,"User-Agent: Mozilla/5.0 (iPhone; CPU iPhone OS 9_2 like Mac OS X) AppleWebKit/601.1.46 (KHTML, like Gecko) Mobile/13C75");
              break;
          case 3:
              strcat(request,"User-Agent: Mozilla/5.0 (Linux; Android 4.4.2; HUAWEI G750-T20 Build/HuaweiG750-T20) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/30.0.0.0 Mobile Safari/537.36");
              break;
          case 4:
              strcat(request,"User-Agent: Mozilla/5.0 (Linux; Android 4.4.2; HUAWEI G750-T20 Build/HuaweiG750-T20) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/30.0.0.0 Mobile Safari/537.36");//forWPNotFoundOne
              break;
          case 5:
              strcat(request,"User-Agent: Mozilla/5.0 (IE 11.0; Windows NT 6.3; Trident/7.0; .NET4.0E; .NET4.0C; rv:11.0) like Gecko");//forDesktopIE11
              break;
          case 6:
              strcat(request,"User-Agent: Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; Trident/6.0)");//forDesktopIE10
              break;
          case 7:
              strcat(request,"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2486.0 Safari/537.36 Edge/13.10586");//forDesktopEged
              break;
          case 8:
              strcat(request,"User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.111 Safari/537.36");//forPCDesktopChrome
              break;
          case 9:
              strcat(request,"User-Agent: Mozilla/5.0 (Linux; Android 4.4.2; HUAWEI G750-T20 Build/HuaweiG750-T20) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/30.0.0.0 Mobile Safari/537.36");//forPCDesktopFireFox
              break;
          case 10:
              strcat(request,"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_2) AppleWebKit/601.3.9 (KHTML, like Gecko) Version/9.0.2 Safari/601.3.9");//forMacSafari
              break;
          case 11:
              strcat(request,"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.111 Safari/537.36");//forMacChrome
              break;
          case 12:
              strcat(request,"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_2) AppleWebKit/601.3.9 (KHTML, like Gecko) FireFox/43.0.4.5848 Safari/601.3.9");//forMacFireFox
              break;
          default:
              strcat(request,"User-Agent: WebBench "PROGRAM_VERSION"");
              break;
      }
  strcat(request, "\r\n");
      
      
  if(proxyhost==NULL && http10>0)
  {
	  strcat(request,"Host: ");
	  strcat(request,host);
	  strcat(request,"\r\n");
  }
  if(request_body_param != NULL) 
  {
      strcat(request,"Content-Type:text/json\r\n");
      strcat(request, request_body_param);//\r\n username=xxx&password=yyy
      strcat(request, "\r\n");
  }
  if(force_reload && proxyhost!=NULL)
  {
	  strcat(request,"Pragma: no-cache\r\n");
  }
  if(http10>1)
	  strcat(request,"Connection: close\r\n");
  /* add empty line at end */
  if(http10>0) strcat(request,"\r\n"); 
  // printf("Req=%s\n",request);
}

/* this function read json file using parson*/
static char *  read_json_file(const char *filename, char field[FIELD_SIZE][FIELD_SIZE]) 
{//在这个函数整理所有的读入的json文件，然后储存到tmp中，做到返回一个字符串
/*先调用url_encode()方法把:后面的每一项的都执行转义
 *这里是返回值：name:myUserName\r\npassword:myPassword
 *ps:如果测试中只发送英文的字段其实可以不用转义
 *
 */
 char *tmp=NULL;
 char *json_content=NULL;
 JSON_Value *root_value;
 JSON_Array *values;
 JSON_Object *value;
 size_t i;
 int j;

 
 strcat(json_content,"{");   
 root_value = json_parse_file(filename);
 values = json_value_get_array(root_value);
 json_file_total_lines = json_array_get_count(values);
 if (json_value_get_type(root_value) != JSONArray) {
    exit(1);
 }
 int count_i;
 int count_j;
 for(count_i=json_file_current_lines;count_i<json_file_total_lines;count_i++){
     for(count_j=0;count_j<FIELD_SIZE;count_j++){
         if(field[count_i][count_j]==field[count_i][count_j+1]){
             break;
         }
         for(j=0;j<count_j;j++){
             strcat(json_content,&field[count_i][j]);
             json_file_current_lines=count_i;
         }
         //field[count_i][count_j]就是一个json Key所以这里可以直接加到json_content中  
         value = json_array_get_object(values, count_i);
         //do the url_encode here to encode commit
         strcat(json_content," : ");           
         strcat(json_content, json_object_get_string(value, &field[count_i][count_j]));           
         strcat(json_content,",\r\n");
     }
 }
 
 tmp=json_content;
 /* cleanup code */
 json_value_free(root_value);

 strcat(json_content,"}");
 return json_content;  
}

/* vraci system rc error kod */
static int bench(void)
{
  int i,j,k;	
  pid_t pid=0;
  FILE *f;

  /* check avaibility of target server */
  i=Socket(proxyhost==NULL?host:proxyhost,proxyport);
  if(i<0) { 
	   fprintf(stderr,"\nConnect to server failed. Aborting benchmark.\n");
           return 1;
         }
  close(i);
  /* create pipe */
  if(pipe(mypipe))
  {
	  perror("pipe failed.");
	  return 3;
  }

  /* not needed, since we have alarm() in childrens */
  /* wait 4 next system clock tick */
  /*
  cas=time(NULL);
  while(time(NULL)==cas)
        sched_yield();
  */

  /* fork childs */
  for(i=0;i<clients;i++)
  {
	   pid=fork();
	   if(pid <= (pid_t) 0)
	   {
		   /* child process or error*/
	           sleep(1); /* make childs faster */
		   break;
	   }
  }

  if( pid< (pid_t) 0)
  {
          fprintf(stderr,"problems forking worker no. %d\n",i);
	  perror("fork failed.");
	  return 3;
  }

  if(pid== (pid_t) 0)
  {
    /* I am a child */
    if(proxyhost==NULL)
      benchcore(host,proxyport,request);
         else
      benchcore(proxyhost,proxyport,request);

         /* write results to pipe */
	 f=fdopen(mypipe[1],"w");
	 if(f==NULL)
	 {
		 perror("open pipe for writing failed.");
		 return 3;
	 }
	 /* fprintf(stderr,"Child - %d %d\n",speed,failed); */
	 fprintf(f,"%d %d %d\n",speed,failed,bytes);
	 fclose(f);
	 return 0;
  } else
  {
	  f=fdopen(mypipe[0],"r");
	  if(f==NULL) 
	  {
		  perror("open pipe for reading failed.");
		  return 3;
	  }
	  setvbuf(f,NULL,_IONBF,0);
	  speed=0;
          failed=0;
          bytes=0;

	  while(1)
	  {
		  pid=fscanf(f,"%d %d %d",&i,&j,&k);
		  if(pid<2)
                  {
                       fprintf(stderr,"Some of our childrens died.\n");
                       break;
                  }
		  speed+=i;
		  failed+=j;
		  bytes+=k;
		  /* fprintf(stderr,"*Knock* %d %d read=%d\n",speed,failed,pid); */
		  if(--clients==0) break;
	  }
	  fclose(f);

  printf("\nSpeed=%d pages/min, %d bytes/sec.\nRequests: %d susceed, %d failed.\n",
		  (int)((speed+failed)/(benchtime/60.0f)),
		  (int)(bytes/(float)benchtime),
		  speed,
		  failed);
  }
  return i;
}

void benchcore(const char *host,const int port,const char *req)
{
 int rlen;
 char buf[1500];
 int s,i;
 struct sigaction sa;

 /* setup alarm signal handler */
 sa.sa_handler=alarm_handler;
 sa.sa_flags=0;
 if(sigaction(SIGALRM,&sa,NULL))
    exit(3);
 alarm(benchtime);

 rlen=strlen(req);
 nexttry:while(1)
 {
    if(timerexpired)
    {
       if(failed>0)
       {
          /* fprintf(stderr,"Correcting failed by signal\n"); */
          failed--;
       }
       return;
    }
    s=Socket(host,port);                          
    if(s<0) { failed++;continue;} 
    if(rlen!=write(s,req,rlen)) {failed++;close(s);continue;}
    if(http10==0) 
	    if(shutdown(s,1)) { failed++;close(s);continue;}
    if(force==0) 
    {
            /* read all available data from socket */
	    while(1)
	    {
              if(timerexpired) break; 
	      i=read(s,buf,1500);
              /* fprintf(stderr,"%d\n",i); */
	      if(i<0) 
              { 
                 failed++;
                 close(s);
                 goto nexttry;
              }
	       else
		       if(i==0) break;
		       else
			       bytes+=i;
	    }
    }
    if(close(s)) {failed++;continue;}
    speed++;
 }
}
