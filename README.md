# YQ-ISP
tiny ISP simulation

我手上的相机是NIKON D610，镜头有两个sigma 35 F1.4和NIKON 85 F1.8，考虑到FOV和shading，畸变存在的多少，后续只使用sigma 35 F1.4拍摄NEF，转换到DNG后做仿真；

**第一个难题 今天的vignette校正参数**
首先是没有均匀灯箱，所以不可能自己测试各个光圈下的vignette，只能采用别人已经测量好的参数，adobe的lcp（lens correction profile)不知是二进制还是文本方式，如果是文本方式可以打开看一下；
google到一个比较有意思的github repo：
https://github.com/lensfun/lensfun/blob/master/data/db/slr-sigma.xml
里面列举了部分镜头的畸变和vignette参数，比如sigma 35 F1.4:
```xml
    <lens>
        <maker>Sigma</maker>
        <model>Sigma 35mm f/1.4 DG HSM</model>
        <mount>Nikon F AF</mount>
        <mount>Sigma SA</mount>
        <mount>Canon EF</mount>
        <mount>Sony Alpha</mount>
        <mount>Pentax KAF</mount>
        <cropfactor>1</cropfactor>
        <calibration>
            <!-- Taken with Pentax K-1 -->
            <distortion model="poly3" focal="35" k1="-0.00111476"/>
            <tca model="poly3" focal="35" vr="1.0001811" vb="0.9999722"/>
            <!-- Taken with Canon 5DmIV -->
            <vignetting model="pa" focal="35" aperture="1.4" distance="0.3" k1="-1.2015" k2="0.9026" k3="-0.3583"/>
            <vignetting model="pa" focal="35" aperture="1.4" distance="1000" k1="-1.8858" k2="1.9295" k3="-0.8556"/>
            <vignetting model="pa" focal="35" aperture="2" distance="0.3" k1="-0.1579" k2="-0.3705" k3="0.0878"/>
            <vignetting model="pa" focal="35" aperture="2" distance="1000" k1="-0.2270" k2="-1.0931" k3="0.6582"/>
            <vignetting model="pa" focal="35" aperture="2.8" distance="0.3" k1="-0.2655" k2="0.0048" k3="0.0296"/>
            <vignetting model="pa" focal="35" aperture="2.8" distance="1000" k1="-0.2521" k2="-0.1862" k3="-0.0193"/>
            <vignetting model="pa" focal="35" aperture="4" distance="0.3" k1="-0.2681" k2="0.0047" k3="0.0334"/>
            <vignetting model="pa" focal="35" aperture="4" distance="1000" k1="-0.3937" k2="0.2511" k3="-0.2028"/>
            <vignetting model="pa" focal="35" aperture="5.6" distance="0.3" k1="-0.2729" k2="0.0160" k3="0.0253"/>
            <vignetting model="pa" focal="35" aperture="5.6" distance="1000" k1="-0.3411" k2="0.0175" k3="0.0299"/>
            <vignetting model="pa" focal="35" aperture="8" distance="0.3" k1="-0.2754" k2="0.0196" k3="0.0226"/>
            <vignetting model="pa" focal="35" aperture="8" distance="1000" k1="-0.3436" k2="0.0164" k3="0.0327"/>
            <vignetting model="pa" focal="35" aperture="11" distance="0.3" k1="-0.2803" k2="0.0277" k3="0.0162"/>
            <vignetting model="pa" focal="35" aperture="11" distance="1000" k1="-0.3530" k2="0.0292" k3="0.0271"/>
            <vignetting model="pa" focal="35" aperture="16" distance="0.3" k1="-0.2898" k2="0.0395" k3="0.0103"/>
            <vignetting model="pa" focal="35" aperture="16" distance="1000" k1="-0.3670" k2="0.0498" k3="0.0171"/>
        </calibration>
    </lens>
```
lcp转换工具：https://lensfun.github.io/manual/latest/lensfun-convert-lcp.html
