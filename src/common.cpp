/********************************************************
*
* Some non-specific useful functions
*
**********************************************************/

#include "stdafx.h"

char str_buffer[1024];

char * buf(){
	return str_buffer;
}

float sqr(float x){
	return x*x;
}

float dist2(float x1, float y1, float x2, float y2){
	return sqrt(sqr(x1 - x2) + sqr(y1 - y2));
}

float dist3(float x1, float y1, float z1, float x2, float y2, float z2){
	return sqrt(sqr(x1 - x2) + sqr(y1 - y2) + sqr(z1 - z2));
}

int signum(int x){
	return (x > 0) ? 1 : (x < 0) ? -1 : 0;
}

void order2intAsc(int* p1, int* p2){
	if ((*p1) > (*p2)) {
		int tmp = (*p2);
		(*p2) = (*p1);
		(*p1) = tmp;
	}
}

void ClearFile(const char * filename){
	FILE * f = fopen(filename, "wt");
	fclose(f);
}

FileFormatInfo GetFileFormatInfo(char* s){
	FileFormatInfo finfo;
	strcpy(finfo.origFilename, s);
	finfo.fmt = FMT_UNKNOWN;

	int l = strlen(s);

	if (l >= 5){
		char * end4 = s + l - 4;
		char buf1[512];
		strcpy(buf1, s);

		if (strcmpi(end4, ".hdr") == 0 || strcmpi(end4, ".img") == 0){
			buf1[l - 4] = 0;
			finfo.fmt = FMT_ANALYZE;
			sprintf(finfo.noextFilename, "%s", buf1);
			sprintf(finfo.ext, "%s", end4 + 1);
			sprintf(finfo.hdrFilename, "%s.hdr", buf1);
			sprintf(finfo.imgFilename, "%s.img", buf1);
			return finfo;
		}

		if (strcmpi(end4, ".nii") == 0) {
			buf1[l - 4] = 0;
			finfo.fmt = FMT_NII;
			sprintf(finfo.noextFilename, "%s", buf1);
			sprintf(finfo.ext, "%s", end4 + 1);
			return finfo;
		}
	}

	if (l >= 8){
		char * end7 = s + l - 7;
		char buf1[512];
		strcpy(buf1, s);

		if (strcmpi(end7, ".nii.gz") == 0) {
			buf1[l - 7] = 0;
			finfo.fmt = FMT_NIIGZ;
			sprintf(finfo.noextFilename, "%s", buf1);
			sprintf(finfo.ext, "%s", end7 + 1);
			return finfo;
		}
	}

	return finfo;
}