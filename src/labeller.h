
struct UserData{
    int curSlice;
    cv::Mat curSliceImage;
    int toolSize;
    int HUshift;
    char filePath[2048];
    bool unsavedChanges;
    double z2xy;
    int mainSize;
    cv::Point3i rulerPoints[2];
    float rulerDist;

    cv::Vector<cv::Vec3b> palette;
};

void LOG(const char * mess, bool onScreen = true, bool toFile = true);

void LOG_Begin(char * filepath);

UserData * getUserData();
