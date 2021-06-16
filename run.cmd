adb push main /data/local/tmp/main
adb shell chmod 777 /data/local/tmp/main
%adb shell < trans.cmd%
start ndkBuildparam.exe "adb shell < trans.cmd"
pause