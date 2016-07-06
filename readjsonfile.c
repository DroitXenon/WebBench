#include<stdio.h>
#include<string.h>
#include<stdlib.h>
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

#  define NULL ((void*)0)

int mypipe[2];
char host[MAXHOSTNAMELEN];
#define REQUEST_SIZE 8192
#define FIELD_SIZE 100
char request[REQUEST_SIZE];
char *requestBody=NULL;
int json_file_total_lines=1;
int json_file_current_lines=0;
unsigned long F_param_number=0;
char field[100][100]={
    'o','b','j','c','t','I','d','\0'
};

static char *  read_json_file(const char *filename, char field[FIELD_SIZE][FIELD_SIZE]) 
{//在这个函数整理所有的读入的json文件，然后储存到tmp中，做到返回一个字符串
	/*先调用url_encode()方法把:后面的每一项的都执行转义
	   *这里是返回值：name:myUserName\r\npassword:myPassword
	    *ps:如果测试中只发送英文的字段其实可以不用转义
	     *
	      */
	 printf("read_json_file function start");
	  char *tmp=NULL;
	   char *json_content=NULL;
	    JSON_Value *root_value;
	     JSON_Array *values;
	      JSON_Object *value;
	       size_t i;

	        
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
			            for(count_j=0;count_j<F_param_number;count_j++){
					             if(field[count_i][count_j]=='\0'){
							                  break;
									           }
						              //for(j=0;j<count_j;j++){
						              //    strcat(json_content,&field[count_i][j]);
						              //    json_file_current_lines=count_i;
						              //}
						              //field[count_i][]就是一个json Key所以这里可以直接加到json_content中  
						              printf("start read json!\n");
							               value = json_array_get_object(values, count_i);
								                printf("finish get json object\n");
										         //do the url_encode here to encode commit
										         strcat(json_content," : ");           
											          strcat(json_content, json_object_get_string(value, field[count_i]));           
												           strcat(json_content,",\r\n");
													        }
				     }
		        
		        tmp=json_content;
			 /* cleanup code */
			 json_value_free(root_value);

			  strcat(json_content,"}");
			   return json_content;  
}

int main()
{
	read_json_file("/Users/qinyuhang/Downloads/qSplL8zCvUGfnvK6TqINkj93-gzGzoHsz-40974552721639855/Promo1x.json",field);
	return 0;
}
