/********************************************************
*
* Functions to handle Analyze75 file format
*
*********************************************************/

#include "../stdafx.h"

using namespace cv;

void Analyze75ShowVector(Vector<Mat> &v, short * dims, double alpha_, double beta_);

bool NoAnalyzeExt(const char * filename){
	bool noExt = true;
	const char * pch;

	pch = strstr(filename, ".img");
	noExt = noExt && (pch == NULL || pch != (filename + strlen(filename) - 4));

	pch = strstr(filename, ".hdr");
	noExt = noExt && (pch == NULL || pch != (filename + strlen(filename) - 4));

	return noExt;
}

void MakeExtFilename(const char * filename, char * extFilename, const char * ext){
	if (NoAnalyzeExt(filename))
		sprintf(extFilename, "%s.%s\0", filename, ext);
	else{
		strncpy(extFilename, filename, strlen(filename) - 4);
		extFilename[strlen(filename) - 4] = '\0';
		strcat(extFilename, ".");
		strcat(extFilename, ext);
	}
}

void MakeHDRFilename(const char * filename, char * hdrFilename){
	MakeExtFilename(filename, hdrFilename, "hdr");
}

void MakeIMGFilename(const char * filename, char * imgFilename){
	MakeExtFilename(filename, imgFilename, "img");
}

HDRInfo Analyze75Info(const char * filename)
{
	char hdrFilename[256], buf[500];
	MakeExtFilename(filename, hdrFilename, "hdr");

	FILE* pFile = fopen(hdrFilename, "rb");

	if (pFile == NULL){
		sprintf(buf, "Specified file '%s' not found", hdrFilename);
		throw buf;
	}

	HDRInfo hdrInfo;

	fread(&hdrInfo, 348, 1, pFile);
	fclose(pFile);

	return hdrInfo;
}

void Analyze75SaveHeader(const char * filename, HDRInfo * info){
	char hdrFilename[256], buf[500];
	MakeExtFilename(filename, hdrFilename, "hdr");

	FILE * f = fopen(hdrFilename, "wb");

	if (f == NULL){
		sprintf(buf, "Cannot write header info to '%s'", hdrFilename);
		throw buf;
	}

	fwrite(info, sizeof(HDRInfo), 1, f);
	fclose(f);
}

void Analyze75Bits8(AnalyzeImage * im){
    for (int k = 0; k < im->slices.size(); k++){
        im->slices[k].convertTo(im->slices[k], CV_8SC1);
    }
}

void Analyze75Bits16(AnalyzeImage * im){
    for (int k = 0; k < im->slices.size(); k++){
        im->slices[k].convertTo(im->slices[k], CV_16SC1);
    }
}


void Analyze75Read(const char * filename, AnalyzeImage * im, bool convertTo8bits){
	char str[256], buff[1024];

	sprintf(buff, "Reading analyze '%s'... ", filename);
	LOG(buff);

	MakeHDRFilename(filename, str);
	HDRInfo info = Analyze75Info(str);
	short d2 = info.dims.dim[2];
	info.dims.dim[2] = info.dims.dim[1];
	info.dims.dim[1] = d2;
	im->hdrInfo = info;

	/*if (info.dims.datatype != DT_SIGNED_SHORT)
		throw "Analyze data type is not DT_SIGNED_SHORT";
	*/

    for (int j = 0; j < im->slices.size(); j++)
        im->slices[j].release();
    im->slices.clear();

    short * dims = info.dims.dim;
	im->slices = Vector<Mat>(dims[3]);

	MakeIMGFilename(filename, str);
	FILE * f = fopen(str, "rb");
	if (f == NULL){
		sprintf(buff, "File not found '%s'", str);
		throw buff;
	}

	short sbuf[262144];
	for (int k = 0; k < dims[3]; k++){
		fread(sbuf, sizeof(short), dims[1] * dims[2], f);

		Mat m(dims[1], dims[2], CV_16S, sbuf, sizeof(short) * dims[2]);
        if (convertTo8bits)
            m.convertTo(im->slices[k], CV_8SC1);
        else
            m.copyTo(im->slices[k]);
	}

	fclose(f);

	LOG("OK");
}

void Analyze75Write(const char * filename, AnalyzeImage * im, bool convertFrom8bits){
	char str[256], buff[1024];

	sprintf(buff, "Writing analyze '%s'... ", filename);
	LOG(buff);

	HDRInfo info = im->hdrInfo;
	short d2 = info.dims.dim[2];
	info.dims.dim[2] = info.dims.dim[1];
	info.dims.dim[1] = d2;
	MakeHDRFilename(filename, str);
	Analyze75SaveHeader(filename, &info);

	MakeIMGFilename(filename, str);
	FILE * f = fopen(str, "wb");

	if (f == NULL){
		sprintf(buff, "Failed to write '%s'", str);
		throw buff;
	}

	short * dims = info.dims.dim;
	for (int k = 0; k < dims[3]; k++){
        if (convertFrom8bits){
            Mat m;
            im->slices[k].convertTo(m, CV_16SC1);
            fwrite(m.data, sizeof(short), dims[1] * dims[2], f);
        }
        else
            fwrite(im->slices[k].data, sizeof(short), dims[1] * dims[2], f);
	}

	fclose(f);

	LOG("OK");
}

void Analyze75Show(AnalyzeImage * im, double alpha_, double beta_){
	short * dims = im->hdrInfo.dims.dim;
	Analyze75ShowVector(im->slices, dims, alpha_, beta_);
}

void Analyze75ShowVector(Vector<Mat> &v, short * dims, double alpha_, double beta_){
	Mat m;

	int i = dims[3] / 2, key = -1;
	while (key != 27){
		v[i].convertTo(m, CV_32F, alpha_, beta_);
		sprintf(buf(), "%ix%ix%i", dims[1], dims[2], dims[3]);
		imshow(buf(), m);

		key = waitKey(20);

		if (key == 2424832)
			i = (i - 1 + dims[3]) % dims[3];
		if (key == 2555904)
			i = (i + 1) % dims[3];
		if (key == 27){
			destroyWindow(buf());
			return;
		}
	}
}
