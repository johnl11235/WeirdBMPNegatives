# WeirdBMPNegatives
Supposed to be a BMP file converter that returns a negative of an image. Instead, it does nothing useful.

Compilation
Run it through any C++ compiler (I used GNU) with the following flags:
-std=c++11, -o
For those who want to use GNU in one cut and paste: g++ -std=c++11 BMPThing.cpp -o BMPThing.exe

To make the executable do funny things, simply call it in command line and feed it either '--help' to get a message, or the name of a bmp file (including the file extension).
It does not return anything useful due to some parsing error.
