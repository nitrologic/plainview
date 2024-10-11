#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>  // For std::memcmp
#include <cstdint>  // For fixed-width types like uint32_t, float

const char *lwo2Version = "0.1";

static bool terminateApp = false;

#include "json.h"

// Structure to store 3D points
struct Point {
    float x, y, z;
};

// Structure to store a polygon
struct Polygon {
    std::vector<uint16_t> vertexIndices;
};

// Structure to store a patch
struct Patch {
	std::vector<uint16_t> controlPointIndices;
};

struct Bone {
	std::string name;
	uint16_t parentIndex;
	float position[3];
	float orientation[3];
	float length;
};
struct swapReader {
	std::ifstream& file;
	swapReader(std::ifstream& f) : file(f) {};
	uint16_t read16() {
		uint16_t value16;
		char* p = (char*)&value16;
		file.read(p + 1, 1);
		file.read(p + 0, 1);
		return value16;
	}
	uint32_t read32() {
		uint32_t value32;
		char* p = (char*)&value32;
		file.read(p + 3, 1);
		file.read(p + 2, 1);
		file.read(p + 1, 1);
		file.read(p + 0, 1);
		return value32;
	}

	uint32_t read32f() {
		uint32_t value = read32();
		float f32 = *(float*)&value;
		return f32;
	}
};

// Simple loader for LWO2 format
class LwoLoader {
public:
    bool load(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            std::cerr << "Error opening file: " << filePath << std::endl;
            return false;
        }
		swapReader infile(file);

        char chunkID[4];
        while (file.read(chunkID, 4)) {
            uint32_t chunkSize = infile.read32();

            if (std::memcmp(chunkID, "FORM", 4) == 0) {
                char formType[4];
                file.read(formType, 4);
                if (std::memcmp(formType, "LWO2", 4) != 0) {
                    std::cerr << "Not a valid LWO2 file" << std::endl;
                    return false;
                }
            } else if (std::memcmp(chunkID, "PNTS", 4) == 0) {
                readPoints(infile, chunkSize);
            } else if (std::memcmp(chunkID, "POLS", 4) == 0) {
                readPolygons(infile, chunkSize);
			}
			else if (std::memcmp(chunkID, "PTAG", 4) == 0) {
				readPtags(file, chunkSize);
			} else {
				std::cerr << "Unknown chunk: " << chunkID[0] << chunkID[1] << chunkID[2] << chunkID[3] << std::endl;
                file.seekg(chunkSize, std::ios::cur);  // Skip unknown chunk
            }
        }

        return true;
    }

private:

    std::vector<Polygon> polygons;

    void readPolygons(swapReader &in, uint32_t chunkSize) {

		char polID[4];
		in.file.read(polID, 4);
		std::cout << "Polygon ID: " << polID[0] << polID[1] << polID[2] << polID[3] << std::endl;

		if(std::memcmp(polID, "PTCH", 4) == 0){
			std::cerr << "PTCH POLS chunk" << std::endl;
			readPatches(in, chunkSize - 4);  // Subtract 4 bytes for the polID
			return;
		}
		else if (std::memcmp(polID, "FACE", 4) == 0) {
			std::cerr << "FACE POLS chunk" << std::endl;
			return;
		}else if (std::memcmp(polID, "BONE", 4) == 0) {
			std::cerr << "BONE POLS chunk" << std::endl;
			readBones(in, chunkSize - 4);  // Subtract 4 bytes for the polID
			return;
		}else{
			std::cerr << "Not a valid POLS chunk" << std::endl;
			return;
		}
        polygons.clear();
		uint32_t bytes = chunkSize;
		while (bytes > 0) {
			uint16_t vertexCount = in.read16();
			bytes -= 2;
			Polygon p;
			p.vertexIndices.resize(vertexCount);
			for (uint16_t j = 0; j < vertexCount; ++j) {
				uint16_t i16 = in.read16();
				p.vertexIndices[j] = i16;
				bytes -= 2;
			}
			polygons.push_back(p);
		}
        std::cout << "Loaded " << polygons.size() << " polygons." << std::endl;
    }

	std::vector<Bone> bones;

/*

				If token="FACE" Or token="BONE"
					inface=True
					While size>0
						p.lwpoly=NewLWPoly(l)
						plist(pcount)=p
						p\n=read16(l) And 1023
						size=size-2
						If p\n>15 RuntimeError "Can't handle 15+ sided polygons"
						For j=0 To p\n-1
							p\v[j]=vlist(ll\vbase+read16(l))
						Next
						pcount=pcount+1
						size=size-2*p\n
					Wend

*/

	void readBones(swapReader& in, uint32_t chunkSize) {
		uint32_t bytes = chunkSize;
		while (bytes > 0) {
			Bone bone;
			// read number of bones (n16
			uint16_t n16 = in.read16();
			int n = n16 & 1023;
			bytes -= 2;
			for (int j = 0; j < n; j++) {
				uint16_t v = in.read16();
				bytes -= 2;
			}
			bones.push_back(bone);
		}
		std::cout << "Loaded " << bones.size() << " bones." << std::endl;
	}

	void readPatches(swapReader& in, uint32_t chunkSize) {
		std::vector<Patch> patches;
		uint32_t bytes = chunkSize;
		while (bytes > 0) {
			uint16_t controlPointCount = in.read16();
			bytes -= 2;
			Patch p;
			p.controlPointIndices.resize(controlPointCount);
			for (uint16_t j = 0; j < controlPointCount; ++j) {
				uint16_t i16 = in.read16();
				p.controlPointIndices[j] = i16;
				bytes -= 2;
			}
			patches.push_back(p);
		}
		std::cout << "Loaded " << patches.size() << " patches." << std::endl;
	}
    
	std::vector<Point> points;
    
    void readPoints(swapReader &in, uint32_t chunkSize) {
        int numPoints = chunkSize / 12;  // Each point is 3 floats (12 bytes)
        points.resize(numPoints);
		size_t ints= numPoints * sizeof(Point);
		std::vector<uint32_t> i3(3);
		char* p = reinterpret_cast<char*>(i3.data());
		for (size_t i = 0; i < numPoints; ++i) {
			float x = in.read32f();
			float y = in.read32f();
			float z = in.read32f();
			points[i] = { x, y, z };
        }
        for(auto &p:points){
//            std::cout << "Point: " << p.x << ", " << p.y << ", " << p.z << std::endl;
        }
        std::cout << "Loaded " << numPoints << " points." << std::endl;
    }

    void _readPolygons(std::ifstream& file, uint32_t chunkSize) {
        // In this simple example, we skip polygon data but you'd parse it similarly.
        std::cout << "Skipping " << chunkSize << " bytes of polygon data." << std::endl;
        file.seekg(chunkSize, std::ios::cur);  // Skip for now
    }

	void readPtags(std::ifstream& file, uint32_t chunkSize) {
		// ptag chunk has polygon attributes
		std::cout << "Skipping " << chunkSize << " bytes of ptag data." << std::endl;
		file.seekg(chunkSize, std::ios::cur);  // Skip for now
	}

};

int main() {
    LwoLoader loader;
	//const char* path = "C:\\Program Files\\LightWaveDigital\\LightWave_2024.1.0\\support\\genoma\\shapes\\Skelegon.lwo";
	const char* path = "../geom/Ant_Rig.lwo";
	loader.load(path);
    return 0;
}

