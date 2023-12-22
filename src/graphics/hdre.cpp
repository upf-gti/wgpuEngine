#include <fstream>
#include <cmath>
#include <cassert>
#include <algorithm>

#include "hdre.h"

#include "spdlog/spdlog.h"

std::map<std::string, HDRE*> HDRE::sHDRELoaded;

HDRE::~HDRE()
{
	clean();
}

sHDRELevel HDRE::getLevel(int n)
{
	sHDRELevel level;

	float size = (int)(this->width / pow(2.0, n));

	level.width = size;
	level.height = size; // cubemap sizes!
	level.data = this->faces_array[n];
	level.faces = this->getFaces(n);
	
	return level;
}

float* HDRE::getData()
{
	return this->data;
}

float* HDRE::getFace(int level, int face)
{
	return this->pixels[level][face];
}

float** HDRE::getFaces(int level)
{
	return this->pixels[level];
}

void HDRE::setName(const std::string& name)
{
    sHDRELoaded[name] = this;
    this->name = name;
}

HDRE* HDRE::Get(const std::string& filename)
{
	auto it = sHDRELoaded.find(filename);
	if (it != sHDRELoaded.end())
		return it->second;

	HDRE* hdre = new HDRE();
	if (!hdre->load(filename))
	{
		delete hdre;
		return nullptr;
	}

    hdre->setName(filename);

	return hdre;
}

void flipYsides(float ** data, unsigned int size, short num_channels)
{
	// std::cout << "Flipping Y sides" << std::endl;

	size_t face_size = size * size * num_channels * 4; // Bytes!!
	float* temp = new float[face_size];
	memcpy(temp, data[2], face_size);
	memcpy(data[2], data[3], face_size);
	memcpy(data[3], temp, face_size);

	delete[] temp;
}

void flipY(float** data, unsigned int size, short num_channels, bool flip_sides)
{
	assert(data);
	// std::cout << "Flipping Y" << std::endl;

	// bytes
	size_t l = floor(size*0.5);

	int pos = 0;
	int lastpos = size * (size - 1) * num_channels;
	float* temp = new float[size * num_channels];
	size_t row_size = size * num_channels * 4;

	for (unsigned int f = 0; f < 6; ++f)
		for (size_t i = 0; i < l; ++i)
		{
			float* fdata = data[f];

			memcpy(temp, fdata + pos, row_size);
			memcpy(fdata + pos, fdata + lastpos, row_size);
			memcpy(fdata + lastpos, temp, row_size);

			pos += size * num_channels;
			lastpos -= size * num_channels;
			if (pos > lastpos)
			{
				pos = 0;
				lastpos = size * (size - 1) * num_channels;
				continue;
			}
		}

	delete[] temp;

	if (flip_sides)
		flipYsides(data, size, num_channels);
}

bool HDRE::process_header()
{
    if (header.type != 3)
    {
        spdlog::error("ArrayType not supported. Please export in Float32Array");
        return false;
    }


    if (header.version < 2.0)
    {
        spdlog::error("Versions below 2.0 are no longer supported. Please, reexport the environment");
        return false;
    }

    type = header.type;
    version = header.version;
    numChannels = header.numChannels;
    bitsPerChannel = header.bitsPerChannel;
    maxLuminance = header.maxLuminance;

    if (header.includesSH)
    {
        numCoeffs = header.numCoeffs;
        coeffs = header.coeffs;
    }

    return true;
}

bool HDRE::load(const std::string& filename)
{
    std::ifstream f(filename, std::ios::binary);
	if (!f.good())
		return false;

	f.read((char*)&header, sizeof(sHDREHeader));

    bool ok = process_header();

    if (!ok) return false;

    int width = this->width = header.width;
    int height = this->height = header.height;

    int dataSize = 0;
    int w = width;
    int h = height;

    // Get number of floats inside the HDRE
    // Per channel & Per face
    for (int i = 0; i < N_LEVELS; i++)
    {
        int mip_level = i + 1;
        dataSize += w * w * N_FACES * header.numChannels;
        w = (int)(width / pow(2.0, mip_level));
    }

    data = new float[dataSize];

    f.seekg(header.headerSize, std::ios::beg);
	f.read((char*)data, sizeof(float) * dataSize);

    f.close();

	// get separated levels

	w = width;
	int mapOffset = 0;
	
	for (int i = 0; i < N_LEVELS; i++)
	{
		int mip_level = i + 1;
		int faceSize = w * w * header.numChannels;
		int mapSize = faceSize * N_FACES;
		
		int faceOffset = 0;
		int facePixel = 0;

		faces_array[i] = new float[mapSize];

		for (int j = 0; j < N_FACES; j++)
		{
			// allocate memory
			pixels[i][j] = new float[faceSize];

			// set data
			for (int k = 0; k < faceSize; k++) {

				float value = data[mapOffset + faceOffset + k];

				pixels[i][j][k] = value;
				faces_array[i][facePixel] = value;
				facePixel++;
			}

			// update face offset
			faceOffset += faceSize;
		}

		// update level offset
		mapOffset += mapSize;

		// refactored code for writing HDRE 
		// removing Y flipping for webGl at Firefox
		if (version < 3.0)
		{
			flipY(pixels[i], w, numChannels, true);
		}
		else
		{
			if (i != 0) // original is already flipped
				flipY(pixels[i], w, numChannels, false);
		}

		// reassign width for next level
		w = (int)(width / pow(2.0, mip_level));
	}

    spdlog::info("HDRE loaded: {} (v{})", filename, version);

	return true;
}

bool HDRE::clean()
{
	if (!data)
		return false;

	try {
		delete data;

		for (int i = 0; i < N_LEVELS; i++)
		{
			delete faces_array[i];

			for (int j = 0; j < N_FACES; j++)
				delete pixels[i][j];
		}

		return true;
	}
	catch (const std::exception&) {
        spdlog::error("Error cleaning");
		return false;
	}

	return false;
}
