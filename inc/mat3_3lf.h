#pragma once

typedef struct _Vector3lf
{
    double data[3];
}Vector3lf;

class Mat3_3lf
{
private:
    double data[9];
public:
    Mat3_3lf();
    ~Mat3_3lf();
    Mat3_3lf(const Mat3_3lf& cp);
    Mat3_3lf(const double* d);
    Mat3_3lf(double d0, double d1, double d2,
        double d3, double d4, double d5,
        double d6, double d7, double d8);

    Mat3_3lf& operator=(const Mat3_3lf& cp);

    Mat3_3lf operator+(const Mat3_3lf& cp);
    Mat3_3lf operator+(double cp);

    Mat3_3lf operator-(const Mat3_3lf& cp);
    Mat3_3lf operator-(double cp);

    Mat3_3lf operator*(const Mat3_3lf& cp);
    Mat3_3lf operator*(double cp);

    Mat3_3lf operator/(const Mat3_3lf& cp);
    Mat3_3lf operator/(double cp);


    Mat3_3lf transpose();
    Mat3_3lf multiply(const Mat3_3lf& cp);
    Vector3lf multiply(const Vector3lf& cp);
    Mat3_3lf inv();
    double det();
    double* get() { return data; }
};