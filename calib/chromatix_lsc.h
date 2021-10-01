#pragma once
typedef struct lsc_focusDist_coeff_s
{
    double focusDist;
    double a1;
    double a2;
    double a3;
}lsc_focusDist_coeff_t;

typedef struct lsc_coeff_element_s
{
    double apertureVal;
    lsc_focusDist_coeff_t focusElement[6];
}lsc_coeff_element_t;


typedef struct chromatix_lsc_s
{
    double center_x;
    double center_y;

    lsc_coeff_element_t element[4];
}chromatix_lsc_t;