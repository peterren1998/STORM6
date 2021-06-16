# Package installation:

1. download the newest version of Anaconda3 from:

2. Install anaconda to ```C:\Users\neoSTORM6\anaconda3```

3. create a conda environment called "halenv" by:

```cmd
conda create -n halenv
```

4. Install required package by: 

```cmd
conda install numpy pip pillow pywin32 pyserial scipy
conda install tifffile
pip install PyQt5
pip install PyDAQmx
```

5. clone the github repo by: 

```cmd
git clone https://github.com/zhengpuas47/STORM6.git
```

6. add this STORM6 folder into PATH enviromental variable (I have done it manually in Windows)

7. wire the National Instruments Daq card properly

8. 