
#define FMT_UNKNOWN 1000
#define FMT_ANALYZE 1001
#define FMT_NII 1002
#define FMT_NIIGZ 1003

struct FileFormatInfo {
	int fmt;
	char origFilename[512];
	char noextFilename[512];
	char hdrFilename[512];
	char imgFilename[512];
	char ext[512];
};

char * buf();

float sqr(float x);

float dist2(float x1, float y1, float x2, float y2);

float dist3(float x1, float y1, float z1, float x2, float y2, float z2);

int signum(int x);

void order2intAsc(int* p1, int* p2);

void ClearFile(const char * filename);

FileFormatInfo GetFileFormatInfo(char* s);