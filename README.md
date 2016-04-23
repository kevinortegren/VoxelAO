# VoxelAO

Small ambient occlusion demo using voxelized geometry and a ray marching kernel. WIP, needs some filtering/blur/noise to smooth the "blocky" results. The sample kernel is very basic and need tweaking.

Results are completely independent from the viewing angle.

###Run Demo

There is a runable exe in bin/Release. Download latest master [here.](https://github.com/kevinortegren/VoxelAO/archive/master.zip) (~55MB).

Use W,A,S,D and hold+move mouse to move camera. LSHIFT and Space to go up and down. Esc to exit.

###Build

Run generate_vsproj.bat to generate Visual Studio 2015 project in build/ dir. Uses DX11.

###Screen shots

####With AO
![WithAO](http://kevinortegren.com/Downloads/Img/withAO.png)

####Without AO
![WithoutAO](http://kevinortegren.com/Downloads/Img/withoutAO.png)

####Only AO
![OnlyAO](http://kevinortegren.com/Downloads/Img/onlyAO.png)

####Voxels
![Voxels](http://kevinortegren.com/Downloads/Img/image02.png)


