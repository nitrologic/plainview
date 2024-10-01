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

// Simple loader for LWO2 format
class LwoLoader {
public:
    bool load(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            std::cerr << "Error opening file: " << filePath << std::endl;
            return false;
        }

        char chunkID[4];
        while (file.read(chunkID, 4)) {
            uint32_t chunkSize = 0;
            file.read(reinterpret_cast<char*>(&chunkSize), 4);
            chunkSize = swapEndian(chunkSize);  // Convert big endian to host byte order

            if (std::memcmp(chunkID, "FORM", 4) == 0) {
                char formType[4];
                file.read(formType, 4);
                if (std::memcmp(formType, "LWO2", 4) != 0) {
                    std::cerr << "Not a valid LWO2 file" << std::endl;
                    return false;
                }
            } else if (std::memcmp(chunkID, "PNTS", 4) == 0) {
                readPoints(file, chunkSize);
            } else if (std::memcmp(chunkID, "POLS", 4) == 0) {
                readPolygons(file, chunkSize);
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
    std::vector<Point> points;
    
    // Helper to swap endianess for big endian values
    uint32_t swapEndian(uint32_t val) {
        return (val >> 24) |
               ((val << 8) & 0x00FF0000) |
               ((val >> 8) & 0x0000FF00) |
               (val << 24);
    }

    void readPoints(std::ifstream& file, uint32_t chunkSize) {
        int numPoints = chunkSize / 12;  // Each point is 3 floats (12 bytes)
        points.resize(numPoints);
        for (int i = 0; i < numPoints; ++i) {
            file.read(reinterpret_cast<char*>(&points[i]), sizeof(Point));
            points[i].x = swapEndian(points[i].x);  // Swap endianness for each float
            points[i].y = swapEndian(points[i].y);
            points[i].z = swapEndian(points[i].z);
        }
        std::cout << "Loaded " << numPoints << " points." << std::endl;
    }

    void readPolygons(std::ifstream& file, uint32_t chunkSize) {
        // In this simple example, we skip polygon data but you'd parse it similarly.
        std::cout << "Skipping " << chunkSize << " bytes of polygon data." << std::endl;
        file.seekg(chunkSize, std::ios::cur);  // Skip for now
    }

	void readPtags(std::ifstream& file, uint32_t chunkSize) {
		// In this simple example, we skip polygon data but you'd parse it similarly.
		std::cout << "Skipping " << chunkSize << " bytes of ptag data." << std::endl;
		file.seekg(chunkSize, std::ios::cur);  // Skip for now
	}

};

int main() {
    LwoLoader loader;
	const char* path = "C:\\Program Files\\LightWaveDigital\\LightWave_2024.1.0\\support\\genoma\\shapes\\Skelegon.lwo";
    loader.load(path);
    return 0;
}

