#include <stdio.h>
#include <math.h>

int main()
{
    //double a1 = -0.584409;
    //double a2 = 1.421628;
    //double a3 = -4.45655;
    const int width = 6034;
    const int height = 4028;
    const int focusLength = 35;

    double a1=-0.49268299999999998, a2=1.2963659999999999, a3=-4.1669510000000001;

    int c_x = width / 2;
    int c_y = height / 2;

    double* buffer = new double[height*width];

    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            double r_x = double(x - c_x)/(167.50150*focusLength);
            double r_y = double(y - c_y)/(167.50150*focusLength);
            double r2 = r_x*r_x + r_y*r_y;

            if(x==0 && y==0)
            {
                printf("r2 = %lf\n", r2);
            }
            double g = 1.0 + a1 * r2 + r2 * r2 * a2 + r2 * r2 * r2 * a3;
            g = 1/g;
            if(x==0 && y==0)
            {
                printf("g = %lf\n", g);
            }
            buffer[y*width+x] = g;
        }
    }

    FILE* fp = fopen("./gain_map.raw", "wb");
    fwrite(buffer, sizeof(double), height*width, fp);
    fclose(fp);

    delete[] buffer;
    }