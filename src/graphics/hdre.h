#pragma once

#include <map>
#include <string>

#define N_LEVELS 6
#define N_FACES 6

struct sHDREHeader {

	char signature[4];
	float version;

	short width;
	short height;

	float maxFileSize;

	short numChannels;
	short bitsPerChannel;
	short headerSize;
	short endianEncoding;

	float maxLuminance;
	short type;

	short includesSH;
	float numCoeffs;
	float coeffs[27];
};

struct sHDRELevel {
	int width;
	int height;
	float* data;
	float** faces;
};

class HDRE {

private:

	float* data = nullptr; // only f32 now
    float* pixels[N_LEVELS][N_FACES] = {}; // Xpos, Xneg, Ypos, Yneg, Zpos, Zneg
    float* faces_array[N_LEVELS] = {};

	sHDREHeader header;

    bool process_header();
	bool clean();

public:

	int width = 0;
	int height = 0;

    std::string name;
	float version = 0.0f;
	short numChannels = 0;
	short bitsPerChannel = 0;
	float maxLuminance = 0;

	short type = 0;
	float numCoeffs = 0.0f;
	float* coeffs = nullptr;

    HDRE() {};
	~HDRE();

	// class manager
	static std::map<std::string, HDRE*> sHDRELoaded;

	bool load(const std::string& filename);

	static HDRE* Get(const std::string& filename);
    void setName(const std::string& name);

	// useful methods
	float getMaxLuminance() { return this->header.maxLuminance; };
	//float* getSHCoeffs() { if (this->numCoeffs > 0) return this->header.coeffs; return nullptr; }

	float* getData(); // All pixel data
	float* getFace(int level, int face);	// Specific level and face
	float** getFaces(int level = 0);		// [[]]: Array per face with all level data

	sHDRELevel getLevel(int level = 0);
};
