# usb-mon
Simple Application to monitor USB Devices using libudev in C++

# Clone Project
```
$ git clone --recurse-submodules https://github.com/b2open/usb-mon.git
Cloning into 'usb-mon'...
remote: Enumerating objects: 13, done.
remote: Counting objects: 100% (13/13), done.
remote: Compressing objects: 100% (11/11), done.
Unpacking objects: 100% (13/13), done.
remote: Total 13 (delta 2), reused 8 (delta 1), pack-reused 0
Checking connectivity... done.
Submodule '3rdparty/fmt' (https://github.com/fmtlib/fmt) registered for path '3rdparty/fmt'
Cloning into '3rdparty/fmt'...
remote: Enumerating objects: 28199, done.
remote: Counting objects: 100% (938/938), done.
remote: Compressing objects: 100% (291/291), done.
remote: Total 28199 (delta 574), reused 839 (delta 515), pack-reused 27261
Receiving objects: 100% (28199/28199), 13.77 MiB | 1.87 MiB/s, done.
Resolving deltas: 100% (19009/19009), done.
Checking connectivity... done.
Caminho do sub-módulo '3rdparty/fmt': confirmado '7bdf0628b1276379886c7f6dda2cef2b3b374f0b'
```

# Compile
```
cd usb-mon
usb-mon $ mkdir build
usb-mon $ cd build
usb-mon/build $ cmake ../
usb-mon/build $ make 
usb-mon/build $ make -j$(nproc)
Scanning dependencies of target fmt
[ 20%] Building CXX object 3rdparty/fmt/CMakeFiles/fmt.dir/src/format.cc.o
[ 40%] Building CXX object 3rdparty/fmt/CMakeFiles/fmt.dir/src/os.cc.o
[ 60%] Linking CXX static library libfmt.a
[ 60%] Built target fmt
Scanning dependencies of target usb-mon
[ 80%] Building CXX object CMakeFiles/usb-mon.dir/main.cpp.o
[100%] Linking CXX executable usb-mon
[100%] Built target usb-mon
```

# Dependencies
- libudev


# 
The code is based on the link project below:
- https://github.com/systemd/systemd/blob/main/src/udev/udevadm-monitor.c
