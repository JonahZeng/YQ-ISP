#include "gtm_fw.h"
#include "pipe_register.h"

gtm_fw::gtm_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

void gtm_fw::fw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                       UINT_32,          &this->bypass                }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->fwCfgList.push_back(config[i]);
    }

    log_info("%s init run end\n", name);
}

void gtm_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    dng_md_t* all_dng_md = &(global_ref_out->dng_meta_data);
    gtm_reg_t* gtm_reg = &(regs->gtm_reg);

    //uint32_t rgb2y[3] = { 306, 601, 117 };
    //uint32_t rgb2y[3] = { 217,  732,  75 };
    gtm_reg->bypass = 0;
    gtm_reg->rgb2y[0] = 217;
    gtm_reg->rgb2y[1] = 732;
    gtm_reg->rgb2y[2] = 75;
    if (frame_cnt == 0 || stat_in->gtm_stat.stat_en == 0)
    {
        uint32_t gain_map[257] =
        {
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024
        };
        memcpy(gtm_reg->gain_lut, gain_map, sizeof(uint32_t) * 257);
    }
    else 
    {
        uint32_t* luma_hist = stat_in->gtm_stat.luma_hist;
        if(stat_in->gtm_stat.stat_en == 0)
        {
            uint32_t gain_map[257] =
            {
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024
            };
            memcpy(gtm_reg->gain_lut, gain_map, sizeof(uint32_t) * 257);
        }
        else
        {
            uint32_t luma_sum = 0;
            for (uint32_t i = 0; i < 256; i++)
            {
                luma_sum += luma_hist[i] * i;
            }
            uint32_t luma_mean = luma_sum / stat_in->gtm_stat.total_pixs;

            double Bv = all_dng_md->ae_md.Bv;
            log_debug("luma mean = %d, Bv=%lf\n", luma_mean, Bv);

            double Bv_thr[10] = { -2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 };
            double luma_tar[10] = { 15.0, 25.0, 35.0, 45.0, 45.0, 50.0, 55.0, 55.0, 55.0, 55.0 };

            int32_t idx0, idx1;
            double luma_out = 0.0;
            if (Bv < Bv_thr[0])
            {
                idx0 = 0;
                idx1 = 0;
                luma_out = luma_tar[0];
            }
            else if (Bv >= Bv_thr[9])
            {
                idx0 = 9;
                idx1 = 9;
                luma_out = luma_tar[9];
            }
            else
            {
                int32_t i = 0;
                for (; i < 9; i++)
                {
                    if (Bv >= Bv_thr[i] && Bv < Bv_thr[i + 1])
                    {
                        break;
                    }
                }
                idx0 = i;
                idx1 = i + 1;
                double delta = Bv - Bv_thr[idx0];
                double step = Bv_thr[idx1] - Bv_thr[idx0];

                luma_out = luma_tar[idx0] + (luma_tar[idx1] - luma_tar[idx0]) * delta / step;
            }

            double global_gain = luma_out / luma_mean;
            uint32_t gain1p5[257] = {
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1535, 1535, 1535, 1535, 1535,
                1534, 1534, 1533, 1532, 1532, 1531, 1530, 1530, 1529, 1528,
                1527, 1526, 1525, 1524, 1522, 1521, 1520, 1519, 1517, 1516,
                1515, 1513, 1512, 1510, 1509, 1507, 1505, 1504, 1502, 1500,
                1499, 1497, 1495, 1493, 1492, 1490, 1488, 1486, 1484, 1482,
                1480, 1478, 1476, 1474, 1472, 1470, 1468, 1466, 1463, 1461,
                1459, 1457, 1455, 1452, 1450, 1448, 1445, 1443, 1441, 1438,
                1436, 1433, 1431, 1429, 1426, 1424, 1421, 1419, 1416, 1414,
                1411, 1408, 1406, 1403, 1401, 1398, 1395, 1393, 1390, 1387,
                1384, 1382, 1379, 1376, 1373, 1371, 1368, 1365, 1362, 1359,
                1356, 1353, 1351, 1348, 1345, 1342, 1339, 1336, 1333, 1330,
                1327, 1324, 1321, 1318, 1315, 1312, 1308, 1305, 1302, 1299,
                1296, 1293, 1290, 1287, 1283, 1280, 1277, 1274, 1270, 1267,
                1264, 1261, 1257, 1254, 1251, 1247, 1244, 1241, 1237, 1234,
                1231, 1227, 1224, 1220, 1217, 1214, 1210, 1207, 1203, 1200,
                1196, 1193, 1189, 1186, 1182, 1179, 1175, 1171, 1168, 1164,
                1161, 1157, 1153, 1150, 1146, 1142, 1139, 1135, 1131, 1128,
                1124, 1120, 1116, 1113, 1109, 1105, 1101, 1098, 1094, 1090,
                1086, 1082, 1078, 1075, 1071, 1067, 1063, 1059, 1055, 1051,
                1047, 1043, 1039, 1035, 1031, 1027, 1024 };

            uint32_t gain2[257] = {
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2047, 2047,
                2046, 2045, 2044, 2042, 2041, 2039, 2036, 2034, 2031,
                2028, 2025, 2022, 2019, 2016, 2012, 2008, 2004, 2001,
                1996, 1992, 1988, 1984, 1979, 1975, 1970, 1965, 1960,
                1956, 1951, 1946, 1941, 1935, 1930, 1925, 1920, 1914,
                1909, 1904, 1898, 1893, 1887, 1882, 1876, 1870, 1865,
                1859, 1853, 1848, 1842, 1836, 1830, 1824, 1819, 1813,
                1807, 1801, 1795, 1789, 1783, 1777, 1771, 1765, 1759,
                1753, 1747, 1741, 1735, 1729, 1723, 1718, 1712, 1706,
                1700, 1694, 1687, 1682, 1676, 1670, 1664, 1658, 1652,
                1646, 1640, 1634, 1628, 1622, 1616, 1610, 1604, 1598,
                1592, 1586, 1580, 1574, 1568, 1562, 1557, 1551, 1545,
                1539, 1533, 1527, 1522, 1516, 1510, 1504, 1498, 1493,
                1487, 1481, 1475, 1470, 1464, 1458, 1453, 1447, 1441,
                1436, 1430, 1425, 1419, 1414, 1408, 1402, 1397, 1391,
                1386, 1381, 1375, 1370, 1364, 1359, 1353, 1348, 1343,
                1337, 1332, 1327, 1321, 1316, 1311, 1306, 1301, 1295,
                1290, 1285, 1280, 1275, 1270, 1265, 1260, 1254, 1249,
                1244, 1239, 1235, 1230, 1225, 1220, 1215, 1210, 1205,
                1200, 1195, 1191, 1186, 1181, 1176, 1172, 1167, 1162,
                1158, 1153, 1148, 1144, 1139, 1135, 1130, 1126, 1121,
                1117, 1112, 1108, 1103, 1099, 1095, 1090, 1086, 1082,
                1077, 1073, 1069, 1065, 1060, 1056, 1052, 1048, 1044,
                1040, 1036, 1032, 1028, 1024 };

            uint32_t gain3[257] = {
                3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3071,
                3068, 3064, 3058, 3051, 3044, 3035, 3027, 3018, 3008, 2998, 2988, 2977,
                2967, 2956, 2945, 2934, 2923, 2912, 2900, 2889, 2877, 2866, 2854, 2843,
                2831, 2820, 2808, 2796, 2785, 2773, 2761, 2750, 2738, 2726, 2715, 2703,
                2691, 2680, 2668, 2657, 2645, 2634, 2622, 2610, 2599, 2588, 2576, 2565,
                2553, 2542, 2531, 2519, 2508, 2497, 2486, 2474, 2463, 2452, 2441, 2430,
                2419, 2408, 2397, 2386, 2375, 2364, 2353, 2342, 2332, 2321, 2310, 2300,
                2289, 2278, 2268, 2257, 2247, 2236, 2226, 2215, 2205, 2195, 2184, 2174,
                2164, 2154, 2144, 2134, 2124, 2113, 2103, 2094, 2084, 2074, 2064, 2054,
                2044, 2035, 2025, 2015, 2006, 1996, 1986, 1977, 1967, 1958, 1949, 1939,
                1930, 1921, 1911, 1902, 1893, 1884, 1875, 1866, 1857, 1848, 1839, 1830,
                1821, 1812, 1803, 1795, 1786, 1777, 1769, 1760, 1752, 1743, 1735, 1726,
                1718, 1709, 1701, 1693, 1684, 1676, 1668, 1660, 1652, 1644, 1636, 1628,
                1620, 1612, 1604, 1596, 1589, 1581, 1573, 1565, 1558, 1550, 1543, 1535,
                1528, 1520, 1513, 1505, 1498, 1491, 1484, 1476, 1469, 1462, 1455, 1448,
                1441, 1434, 1427, 1420, 1413, 1407, 1400, 1393, 1386, 1380, 1373, 1367,
                1360, 1354, 1347, 1341, 1334, 1328, 1322, 1315, 1309, 1303, 1297, 1291,
                1285, 1279, 1273, 1267, 1261, 1255, 1249, 1243, 1238, 1232, 1226, 1221,
                1215, 1209, 1204, 1198, 1193, 1188, 1182, 1177, 1172, 1166, 1161, 1156,
                1151, 1146, 1141, 1136, 1131, 1126, 1121, 1116, 1111, 1107, 1102, 1097,
                1093, 1088, 1083, 1079, 1074, 1070, 1065, 1061, 1057, 1052, 1048, 1044,
                1040, 1036, 1032, 1028, 1024};

            if (global_gain < 1.0)
            {
                for (int32_t i = 0; i < 257; i++)
                {
                    gtm_reg->gain_lut[i] = uint32_t(1024 * global_gain + 0.5);
                }
            } 
            else if (global_gain >= 1.0 && global_gain < 1.5)
            {
                double w1 = global_gain - 1.0;
                
                for (int32_t i = 0; i < 257; i++)
                {
                    gtm_reg->gain_lut[i] = uint32_t(1024 + (gain1p5[i] - 1024)*w1/0.5);
                }
            }
            else if (global_gain >= 1.5 && global_gain < 2.0)
            {
                double w1 = global_gain - 1.5;

                for (int32_t i = 0; i < 257; i++)
                {
                    gtm_reg->gain_lut[i] = uint32_t(gain1p5[i] + (gain2[i] - gain1p5[i]) * w1 / 0.5);
                }
            }
            else if (global_gain >= 2.0 && global_gain < 3.0)
            {
                double w1 = global_gain - 2.0;

                for (int32_t i = 0; i < 257; i++)
                {
                    gtm_reg->gain_lut[i] = uint32_t(gain2[i] + (gain3[i] - gain2[i]) * w1);
                }
            }
            else
            {
                for (int32_t i = 0; i < 257; i++)
                {
                    gtm_reg->gain_lut[i] = gain3[i];
                }
            }
        }
    }
}

gtm_fw::~gtm_fw()
{
    log_info("%s deinit\n", name);
}