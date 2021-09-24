#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main()
{
    FILE* ppm_f = fopen("./out.pnm", "wb");
    const char header[] = "P6\n128   128\n1023\n"; //必须为偶数个char
    printf("%d\n", sizeof(header)/sizeof(char)-1);
    fwrite(header, sizeof(char), sizeof(header)/sizeof(char)-1, ppm_f);
    uint16_t* buffer = new uint16_t[128*128*3];
    for(int i=0; i<128*128; i++)
    {
        buffer[i*3 + 0] = 0xff03; //必须使用big-endian
        buffer[i*3 + 1] = 0;
        buffer[i*3 + 2] = 0;
    }
    fwrite(buffer, sizeof(uint16_t), 128*128*3, ppm_f);
    fclose(ppm_f);
    delete[] buffer;
    return 0;
}