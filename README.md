# YQ-ISP
-------
tiny ISP simulation

我手上的相机是NIKON D610，镜头有两个sigma 35 F1.4和NIKON 85 F1.8，考虑到FOV和shading，畸变存在的多少，后续只使用sigma 35 F1.4拍摄NEF，转换到DNG后做仿真；

## 规划
- [x] DNG/RAW支持

- [x] ubuntu/windows跨平台支持

- [x] cmake支持

- [ ] opencl支持

- [ ] 多stripe支持

![结构图](https://github.com/JonahZeng/YQ-ISP/blob/master/doc/pipeline_v1.png?raw=true)

## 使用方法
采用cmake进行跨平台构建，已在VS2017 c++工具集v141，ubuntu20.04 gcc 9.3.0下测试通过;
windows生成VS工程方法：
```bat
cd build
cmake -G "Visual Studio 15 2017 Win64" ..
```
打开.sln文件使用vs编译, 设置isp_emulation为启动项目，双击build目录下的copy_dll_release/debug.bat拷贝必需的dll至exe生成目录；
vs设置启动参数：

>>-p [pipe_number] -cfg [xml_file] -f [frame_end]


ubuntu生成makefile并编译:
```sh
cd build
cmake ..
cmake --build .
```
ubuntu上编译运行依赖libz, liblzma
```sh
sudo apt-get install libz-dev liblzma-dev
```
运行示例：
```sh
$> ./isp_emulation -p 0 -cfg ../cfg/V1_config.xml -f 0 1
[2021-09-19 18:29:19.009] [info] run command:
[2021-09-19 18:29:19.010] [info] ./isp_emulation -p 0 -cfg ../cfg/V1_config.xml -f 0 1
[2021-09-19 18:29:19.010] [info] init run start
[2021-09-19 18:29:19.010] [info] init run end
[2021-09-19 18:29:19.011] [info] root node name: pipeline_config, type:1
[2021-09-19 18:29:19.011] [info] component name component, type:1
[2021-09-19 18:29:19.011] [info] tag name:inst_name text:raw_in
[2021-09-19 18:29:19.011] [info] tag name:bayer_type text:RGGB
[2021-09-19 18:29:19.011] [info] tag name:bit_depth text:14
[2021-09-19 18:29:19.012] [info] tag name:file_type text:DNG
[2021-09-19 18:29:19.012] [info] tag name:file_name text:../raw_image/D700FAR4256convert.dng
[2021-09-19 18:29:19.012] [info] tag name:img_width text:4284
[2021-09-19 18:29:19.012] [info] tag name:img_height text:2844
[2021-09-19 18:29:19.012] [info] component name component, type:1
[2021-09-19 18:29:19.012] [info] ~fileRead deinit end
[2021-09-19 18:29:19.012] [info] deinit module raw_in
```

## 镜头的vignette校正参数
首先是没有均匀灯箱，所以不可能自己测试各个光圈下的vignette，只能采用别人已经测量好的参数，adobe的lcp（lens correction profile)是文本方式（确切的说是xml格式），只要安装以下任意一个软件都可以获得：

1. photoshop -- 收费
2. lightroom -- 收费
3. Adobe DNG Converter -- 免费
   
存放lens profiles的路径：
```sh
C:\ProgramData\Adobe\CameraRaw\LensProfiles\1.0\Sigma\Nikon #我的是nikon相机配sigma镜头
```
内容节选：
```xml
<rdf:li>
 <rdf:Description
  stCamera:FocalLength="35"
  stCamera:FocusDistance="999999995904"
  stCamera:ApertureValue="0.970854">
 <stCamera:PerspectiveModel>
  <stCamera:VignetteModel>
   <rdf:Description
    stCamera:ResidualMeanError="0.021197"
    stCamera:VignetteModelParam1="-5.668678"
    stCamera:VignetteModelParam2="19.64465"
    stCamera:VignetteModelParam3="-28.093815">
   <stCamera:VignetteModelPiecewiseParam>
    <rdf:Seq>
     <rdf:li>0.000000, 1.000000</rdf:li>
     <rdf:li>0.133789, 0.858700</rdf:li>
     <rdf:li>0.273387, 0.678600</rdf:li>
     <rdf:li>0.570885, 0.257700</rdf:li>
     <rdf:li>0.610752, 0.206000</rdf:li>
    </rdf:Seq>
   </stCamera:VignetteModelPiecewiseParam>
   </rdf:Description>
  </stCamera:VignetteModel>
  </rdf:Description>
 </stCamera:PerspectiveModel>
 </rdf:Description>
</rdf:li>
```

abobe的vignette model:

令：
$$
L(x_{d}, y_{d}) = 1 + a_{1}r_{d}^{2} + a_{2}r_{d}^{4} + a_{3}r_{d}^{6} \\

I(x_{d}, y_{d}) = I_{ideal}(x_{d}, y_{d}) * L(x_{d}, y_{d}) \\

r_{d}^{2} = x_{d}^{2} + y_{d}^{2} \\

\begin{bmatrix}
x_{d} \\
y_{d} 
\end{bmatrix} = 
\begin{bmatrix}
(u_{d} - u_{0}) / (f_{x}*res_{x})\\
(v_{d} - v_{0}) / (f_{y}*res_{y})
\end{bmatrix}

$$

$a_{1}, a_{2}, a_{3}$分别对应上面的lcp中的`VignetteModelParam1, VignetteModelParam2,VignetteModelParam3`，$r_{d}$表示距离光学中心的距离(mm)和焦距(mm)的比值， $u_{0}, v_{0}$表示圆心坐标(pixel)， $u_{d}, v_{d}$表示图像像素坐标(pixel)，$res_{x}, res_{y}$表示像素密度(pixs/mm)



~~如果没有安装ps或者lr，则参考如下第三方结果：~~

~~https://github.com/lensfun/lensfun/blob/master/data/db/slr-sigma.xml~~
~~里面列举了部分镜头的畸变和vignette参数，比如sigma 35 F1.4:~~
~~同时还提供了lcp转换工具：https://lensfun.github.io/manual/latest/lensfun-convert-lcp.html~~
