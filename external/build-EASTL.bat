set PATH=%PATH%;C:\Program Files\CMake\bin
set build_folder=out
cd EASTL
mkdir %build_folder%
pushd %build_folder%
call cmake .. -DEASTL_BUILD_TESTS:BOOL=ON
call cmake --build . --config Release
call cmake --build . --config Debug
call cmake --build . --config RelWithDebInfo
call cmake --build . --config MinSizeRel
pushd test
call ctest -C Release
call ctest -C Debug
call ctest -C RelWithDebInfo
call ctest -C MinSizeRel
popd
popd