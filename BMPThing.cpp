/*
Simnple BMP file negative creator
Does not work due to strange numerical values popping up

*/
#include <iostream>
#include <fstream>
#include <string>
//Using cstdint for specific bit limitations
#include <cstdint>
//Some functionality from cstdlib is required
#include <cstdlib>
#include <vector>

using namespace std;
/*
Method
Extract the BM signature first, or its variants; if it is not any of these, reject immediately.

Then, find the DIB header
Extract the header size (in bytes)
Extract bitmap width in pixels (4 bytes sized signed integer)
Extract the bitmap height in pixels (4 bytes signed integer)
Check next 2 bytes for corruption
Check how many bits per pixel is used.

*/

//Helper structs. These each represent individual contiguous parts of the BMP file.
struct BMP_header
{
	/*
	BMP header descriptor
		2 bytes	- Identifier (Should be BM/BA/CI/CP/IC/PT
		4 bytes - Size of file (in bytes)
		2 bytes - Reserved1, for application ID
		2 bytes - Reserved2, for application ID
		4 bytes - Offset of byte for pixel array
	*/
	uint16_t filetype{0x4D42}; 	//Hex for "BM", which should be at the top.
	uint32_t filesize{0};		//Size of file in bytes
	uint16_t reserved1{0}; 		//Reserved area, for programs.
	uint16_t reserved2{0};		//Same as above, additional field.
	uint32_t offset{0};			//Start position of the pixel array. Extremely important.
};

struct InfoHeader
{
	/*
		DIB header. Base size should be about 40 bytes.
		4 bytes - Size of DIB header itself
		4 bytes - Size of image width in pixels (signed int)
		4 bytes - Size of image height in pixels (signed int)
		2 bytes - Number of color planes being used. Should be 1.
		2 bytes - Number of bits per pixel/color depth. Can be 1, 4, 8, 16, 24, 32, or some other. 24 is the one we want.
		4 bytes - Compression method. Can be the following values:
			0 - BI_RGB				- Uncompressed 						- Most Common
			1 - BI-RLE8				- RLE 8bit 							- 8 bit pixel bitmaps only
			2 - BI_RLE4 			- RLE 4bit 							- 4 bit pixel bitmaps only
			3 - BI_BITFIELDS 		- Bitfield/Huffman 1D compression 	- bit masks/Huffman for BITMAPCOREHEADER2
			4 - BI_JPEG 			- JPEG/RLE-24 compression 			- is a JPEG/RLE-24 compressed BMP for BITMAPCOREHEADER2
			5 - BI_PNG 				- PNG 								- is a PNG image
			6 - BI_ALPHABITFIELDS 	- Bit Field 						- valid for Win CE .NET 4.0+
		4 bytes - Image Size, can be actual size or 0 if uncompressed, otherwise size of compressed image
		4 bytes - Pixels per meter (horizontal)
		4 bytes - Pixels per meter (vertical)
		4 bytes - # of Colors in palette
		4 bytes - Important colors (0 if all colors are important, usually not used)
		Additional 3-4 bitmasks (BI_BITFIELDS is 12 bytes and BI_ALPHABITFIELDS is 16 bytes)
	*/
	
	uint32_t InfoHeader_size{0};		//Size of the header itself. Use to skip unimportant stuff.
	int32_t width{0};				//width of bitmap in pixels. CAN BE NEGATIVE!
	int32_t height{0};				//height of bitmap in pixels. CAN BE NEGATIVE!
	uint16_t planes{1};				//Color planes. Should be 1. If not, something is wonky.
	uint16_t bits_per_pixel{24};		//Number of bits allocated per pixel. We want 24 in our case.
	uint32_t compression {0};		//Compression method. Described above.
	uint32_t size_image{0};			//Compressed size of image. 0 if uncompressed.
	int32_t h_ppm{0};				//pixels per given meter on the horizontal (x) plane.
	int32_t v_ppm{0};				//pixels per given meter on the vertical (y) plane.
	uint32_t color_used{0};			//# of color indices in the color table. 0 for max colors allowed by bits per pixel.
	uint32_t color_important{0};	//# of colors used for displaying bitmap. If set to 0 all colors are shown.
};

struct BMP
{
	//Padding length. Used to figure out which bits to ignore per row.
	int pad_len = 0;
	int real_width = 0;
	int real_height = 0;
	
	//BMP Header, for copying over.
	BMP_header header;
	//DIB header, for copying over and to find dimensions of array.
	InfoHeader info_header;
	
	//Container for holding the image itself as bytes represented in the form of ints.
	vector<uint8_t> img_data;
	
	void read_bmp(string filename)
	{
		cout << "beginning file reading..." << endl;
		ifstream img(filename, ios::binary);
		if(img.is_open())
		{
			//File opened, do stuff
			//First, read all of the data as byte chunks (char pointers here) and throw it into the predefined struct.
			img.read((char*) &header, sizeof(header));
			
			//cout << "BMP Header read." << endl;
			//cout << "BMP header data: " << header.filesize << " " << header.reserved1 << " " << header.reserved2 << " " << header.offset << endl;
			
			if(header.filetype !=0x4D42) //Not a BMP file
			{
				cout << "This is not a BMP file!" << endl;
				exit(1);
			}
			//If it is a BMP file, we need to retrieve the DIB header asd well.
			//cout << "Start of pixel array: " << header.offset << endl;
			img.read((char*) &info_header, sizeof(info_header));
			/*
			if(info_header.compression != 0 || info_header.compression != 3)
			{
				cout << "This file is compressed!" << endl;
				exit(1);
			}
			*/
			//cout << "Size of DIB header: " << info_header.InfoHeader_size << endl;
			
			//Check that it has the right bit amount!
			//cout << "Checking bits per pixel..." << endl;
			//cout << info_header.bits_per_pixel << endl;
			/*
			if(info_header.bits_per_pixel != 24)
			{
				cout << "This is not a 24 bit BMP file!" << endl;
				exit(1);
			}
			*/
			
			//Readjust header fields, cuts out miscellanous details.
			info_header.InfoHeader_size = sizeof(InfoHeader);
			header.offset = sizeof(BMP_header) + sizeof(InfoHeader);
			
			//Extract the real width and height values of the array.
			//Negative values simply indicate the direction in which the image is drawn and will be factored in.
			real_width = abs(info_header.width);
			real_height = abs(info_header.height);
			cout << real_width << " " << real_height << endl;
			//cout << "real width and height found." << endl;
			
			//Now lets get to the image part. All the relevant data has been extracted.
			//Jump to the beginning of the pixel array.
			cout << header.offset << endl;
			img.seekg(header.offset, img.beg);
			
			//cout << "seekg run." << endl;
			
			//With the information, calculate the padding per row.
			//This becomes important when doing edits. If it needs no padding this will be 0.
			pad_len = real_width % 4;
			
			//Now we need to resize the container to hold all the pixel data.
			cout << "Resizing vector..." << endl;
			img_data.resize((real_width + pad_len) * real_height);
			
			//Copy everything into the vector.
			for(int y = 0; y < real_height; y++)
			{
				img.read((char*)img_data.data(), (real_width + pad_len));
			}
			//cout << "Image read, sample of 5th pixel: " << img_data[13] << " " << img_data[14] << " " << img_data [15] << endl;
			//Everything including padding should now be in the vector.
		}
		else
		{
			cout << "Unable to open file!" << endl;
			exit(1);
		}
		//Close the file.
		img.close();
		cout << "Image closed, reading finished." << endl;
	}
	
	void negative_img()
	{
		//Takes full image, and inverts all the colors and stuff. Ignores padding.
		cout << "Outside of negative loop..." << endl;
		for(int y = 0; y < real_height; y++)
		{
			cout << "Inside of Y loop." << endl;
			for(int x = 0; x < real_width; x++)
			{
				//cout << "Inside of X loop." << endl;
				uint8_t org_value = img_data[x + (y * (x + pad_len))];
				//cout << "original value of pixel color is " << org_value << endl;
				//Subtract the original value from 255, the maximum value of a byte. This works for 24 bit.
				img_data[x + (y * (x + pad_len))] = (255 - org_value);
				//This effectively inverts all colors and we don't have to be specific about about which color we are editing.
			}
		}
	}
	
	void write_bmp(string filename)
	{
		//Basically this is the read function but in reverse.
		/*
			First we create a new file of the specified name. We appedn the bmp suffix as well.
			Then we write back all the headers in exactly the same order as we receieved them. Since this is 24 bit we discard the color table.
			After that, we take the fully modified data and stream it back in. Once the vector is clear we simply close the file.
			The end result should be a negative image if this was done correctly.
		*/
		//First, make the blank file.
		//fstream makeshiftfile(filename, ios_base::out);
		//makeshiftfile.close();
		
		ofstream img_o(filename, ios::binary);
		if(img_o.is_open())
		{
			//Write in the image's original header.
			img_o.write((const char*)&header, sizeof(header));
			
			//Next, write in the DIB header. We leave it more or less unchanged.
			img_o.write((const char*)&info_header, sizeof(info_header));
			
			//Do a little clean-up
			img_o.flush();
			
			//Since we did not use a smaller bit depth than 8 per color, we omit the color table.
			//Now write in the actual array itself.
			img_o.write((const char*)img_data.data(), img_data.size());
			
		/*
			Why does this work?
			Because we left the padding in there without any additional modifications, and simply skipped over padding when modifying data.
			This means we can simply dump the entire vector's contents into the file and call it a day.
		*/
		}
		else
		{
			throw runtime_error("Unable to create file!");
		}
		//All done, cleanup.
		img_o.close();
	}
	
	//Inversion call, to make life easy.
	void make_negative(string f_name)
	{
		cout << "Reading in BMP file..." << endl;
		read_bmp(f_name);
		cout << "Making negative of image..." << endl;
		negative_img();
		//Modify the base file name to make a new file, one that reflects its new negative status.
		string old_filename = f_name;
		f_name.erase((f_name.size()-4), 4) += "_n.bmp";
		cout << "Converting " << old_filename << " to " << f_name << "." << endl;
		//Write the file.
		cout << "Writing new file..." << endl;
		write_bmp(f_name);
	}
	
	//Constructor
	BMP(string file)
	{
		make_negative(file);
	}

};

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		printf("Usage: %s bmp_file", argv[0]);
	}
	else
	{
		const string argument(argv[1]);
		if(argument == "--help")
		{
			cout << "Welcome to help. To use this program simply specify a <file_name> in place of '--help' to convert the image to a negative image." << endl
				<< "Please remember to include the .bmp extension in the name." << endl
				<< "This program only supports 24 bit uncompressed bitmap files." << endl;
		}
		else
		{
			BMP new_image_file(argument);
		}
	}
}