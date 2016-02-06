#ifndef VVDREADER_H
#define VVDREADER_H

#include <string>
#include <vector>

class VVDReader
{
        struct fileInfo
        {
            std::string fileName;
            int length;
        };
        std::vector<fileInfo> files;
        struct common_data* vvdconfig;
        FILE* selectedFile;
        int selectedIndex;
        float* data_floor;
        float* data_ceil;
    public:
        VVDReader();
        ~VVDReader();
        bool addVVD(const std::string& fileName);
        bool selectVVD(int index);
        int getFrameSize();
        bool getSegment(int index,void* data);
        bool getSegment(float fractionalIndex,void* data);
        float getSelectedLength();
        int getSamplerate();
        float getFramePeriod();
        int getCepstrumLength();
    protected:
    private:
};



#endif // VVDREADER_H
