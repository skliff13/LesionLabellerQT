#include "stdafx.h"

char logfile[1024];

UserData userData;

UserData * getUserData(){
    return &userData;
}

struct bytes2{
	unsigned char b1;
	unsigned char b2;
};

union short12{
	short shrt;
	bytes2 bt2;
};

void LOG(const char * mess, bool onScreen, bool toFile){
	if (onScreen){
		printf("%s\n", mess);
	}

	if (toFile){
		time_t t = time(NULL);
		char s_time[30];
		strcpy(s_time, ctime(&t));
		s_time[strlen(ctime(&t)) - 1] = 0;

        FILE * f = fopen(logfile, "at");
		fprintf(f, "%s  --  %s\n", s_time, mess);
		fclose(f);
	}
}

void LOG_Begin(char * filepath){
    strcpy(logfile, filepath);

	time_t t = time(NULL);
	char s_time[30];
	strcpy(s_time, ctime(&t));
	s_time[strlen(ctime(&t)) - 1] = 0;

    FILE * f = fopen(logfile, "at");
	fprintf(f, "\n\n");
	fprintf(f, "*****************************************************\n");
    fprintf(f, "* ANALYZE LESIONS LABELLER                          *\n");
	fprintf(f, "*****************************************************\n");
	fprintf(f, "%s  --  START\n", s_time);
	fclose(f);
}
