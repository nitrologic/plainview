pushd bin
cmake ..
ninja -j8
popd

if %errorlevel% neq 0 exit /b %errorlevel%

bin\plainview.exe
