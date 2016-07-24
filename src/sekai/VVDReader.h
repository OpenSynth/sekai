/*
	Sekai - addons for the WORLD speech toolkit
    Copyright (C) 2016 Tobias Platen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/ 

#ifndef SEKAI_VVDREADER_H
#define SEKAI_VVDREADER_H

#include <string>
#include <vector>

class VVDReader
{
	protected:
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
