echo "Building flipman ..."
export PATH=$PATH:/Users/mikaelsundell/Qt6.8/Tools/CMake/CMake.app/Contents/bin
cmake .. -DCMAKE_PREFIX_PATH="/Volumes/Build/pipeline/3rdparty/build/macosx/arm64.debug;/Applications/Blackmagic DeckLink SDK 10.1.4/Mac" -G Xcode
