#include <algorithm>
#include <memory.h>
#include <cstdio>
#include <cmath>

#include "mat3_3lf.h"

Mat3_3lf::Mat3_3lf() :data{ 0 }
{

}

Mat3_3lf::~Mat3_3lf()
{

}

Mat3_3lf::Mat3_3lf(const Mat3_3lf& cp)
{
    memcpy(data, cp.data, sizeof(double) * 9);
}

Mat3_3lf::Mat3_3lf(const double* d)
{
    memcpy(data, d, sizeof(double) * 9);
}

Mat3_3lf::Mat3_3lf(double d0, double d1, double d2,
    double d3, double d4, double d5,
    double d6, double d7, double d8)
{
    data[0] = d0; data[1] = d1; data[2] = d2;
    data[3] = d3; data[4] = d4; data[5] = d5;
    data[6] = d6; data[7] = d7; data[8] = d8;
}

Mat3_3lf& Mat3_3lf::operator=(const Mat3_3lf& cp)
{
    memcpy(data, cp.data, sizeof(double) * 9);
    return *this;
}


Mat3_3lf Mat3_3lf::operator+(const Mat3_3lf& cp)
{
    Mat3_3lf res;
    for (int i = 0; i < 9; i++)
    {
        res.data[i] = data[i] + cp.data[i];
    }
    return res;
}

Mat3_3lf Mat3_3lf::operator+(double cp)
{
    Mat3_3lf res;
    for (int i = 0; i < 9; i++)
    {
        res.data[i] = data[i] + cp;
    }
    return res;
}

Mat3_3lf Mat3_3lf::operator-(const Mat3_3lf& cp)
{
    Mat3_3lf res;
    for (int i = 0; i < 9; i++)
    {
        res.data[i] = data[i] - cp.data[i];
    }
    return res;
}

Mat3_3lf Mat3_3lf::operator-(double cp)
{
    Mat3_3lf res;
    for (int i = 0; i < 9; i++)
    {
        res.data[i] = data[i] - cp;
    }
    return res;
}

Mat3_3lf Mat3_3lf::operator*(const Mat3_3lf& cp)
{
    Mat3_3lf res;
    for (int i = 0; i < 9; i++)
    {
        res.data[i] = data[i] * cp.data[i];
    }
    return res;
}

Mat3_3lf Mat3_3lf::operator*(double cp)
{
    Mat3_3lf res;
    for (int i = 0; i < 9; i++)
    {
        res.data[i] = data[i] * cp;
    }
    return res;
}

Mat3_3lf Mat3_3lf::operator/(const Mat3_3lf& cp)
{
    Mat3_3lf res;
    for (int i = 0; i < 9; i++)
    {
        res.data[i] = data[i] / cp.data[i];
    }
    return res;
}

Mat3_3lf Mat3_3lf::operator/(double cp)
{
    Mat3_3lf res;
    for (int i = 0; i < 9; i++)
    {
        res.data[i] = data[i] / cp;
    }
    return res;
}


Mat3_3lf Mat3_3lf::transpose()
{
    Mat3_3lf res(*this);
    std::swap(res.data[1], res.data[3]);
    std::swap(res.data[2], res.data[6]);
    std::swap(res.data[5], res.data[7]);
    return res;
}

Mat3_3lf Mat3_3lf::multiply(const Mat3_3lf& cp)
{
    Mat3_3lf res;
    res.data[0] = data[0] * cp.data[0] + data[1] * cp.data[3] + data[2] * cp.data[6];
    res.data[1] = data[0] * cp.data[1] + data[1] * cp.data[4] + data[2] * cp.data[7];
    res.data[2] = data[0] * cp.data[2] + data[1] * cp.data[5] + data[2] * cp.data[8];

    res.data[3] = data[3] * cp.data[0] + data[4] * cp.data[3] + data[5] * cp.data[6];
    res.data[4] = data[3] * cp.data[1] + data[4] * cp.data[4] + data[5] * cp.data[7];
    res.data[5] = data[3] * cp.data[2] + data[4] * cp.data[5] + data[5] * cp.data[8];

    res.data[6] = data[6] * cp.data[0] + data[7] * cp.data[3] + data[8] * cp.data[6];
    res.data[7] = data[6] * cp.data[1] + data[7] * cp.data[4] + data[8] * cp.data[7];
    res.data[8] = data[6] * cp.data[2] + data[7] * cp.data[5] + data[8] * cp.data[8];
    return res;
}

Vector3lf Mat3_3lf::multiply(const Vector3lf& cp)
{
    Vector3lf res;
    res.data[0] = data[0] * cp.data[0] + data[1] * cp.data[1] + data[2] * cp.data[2];
    res.data[1] = data[3] * cp.data[0] + data[4] * cp.data[1] + data[5] * cp.data[2];
    res.data[2] = data[6] * cp.data[0] + data[7] * cp.data[1] + data[8] * cp.data[2];
    return res;
}

Mat3_3lf Mat3_3lf::inv()
{
    double det_val = fabs(det());
    double cofactor0 = data[4] * data[8] - data[5] * data[7];
    double cofactor1 = data[5] * data[6] - data[3] * data[8];
    double cofactor2 = data[3] * data[7] - data[4] * data[6];
    double cofactor3 = data[2] * data[7] - data[1] * data[8];
    double cofactor4 = data[0] * data[8] - data[2] * data[6];
    double cofactor5 = data[1] * data[6] - data[0] * data[7];
    double cofactor6 = data[1] * data[5] - data[2] * data[4];
    double cofactor7 = data[2] * data[3] - data[0] * data[5];
    double cofactor8 = data[0] * data[4] - data[1] * data[3];

    Mat3_3lf res(cofactor0 / det_val, cofactor3 / det_val, cofactor6 / det_val,
        cofactor1 / det_val, cofactor4 / det_val, cofactor7 / det_val,
        cofactor2 / det_val, cofactor5 / det_val, cofactor8 / det_val);
    return res;
}

double Mat3_3lf::det()
{
    double res = data[0] * data[4] * data[8] + data[1] * data[5] * data[6] + data[2] * data[3] * data[7];
    res = res - data[0] * data[5] * data[7] - data[1] * data[3] * data[8] - data[2] * data[4] * data[6];
    return res;
}