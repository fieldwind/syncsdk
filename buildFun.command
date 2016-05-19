#sh killHsm.sh
#chmod +x killHSM.commmand  set app to execute privilige


#pid=$(ps -ef | grep 'HTC\ Sync\ Manager' | grep -v 'grep' | awk '{print $2}')
#kill $pid


cd /Users/frankweng/Code/workCode/syncsdkbuild

lipo -create /Users/frankweng/Code/workCode/syncsdkbuild/Build/Products/iPhone-Release-iphoneos/libfunambol.a /Users/frankweng/Code/workCode/syncsdkbuild/Build/Products/iPhone-Release-iphonesimulator/libfunambol.a -output ./libfunambol.a

lipo -info ./libfunambol.a
