# Boost must be installed as DLL , otherwise you'll get error:

# {System.BadImageFormatException: Could not load file or assembly 'OrderBook_CLI.dll' or one of its dependencies.  is not a valid Win32 application. ## # #(Exception from HRESULT: 0x800700C1) File name: 'OrderBook_CLI.dll' at OrderBook_WPF.MainWindow..ctor()}

cd c:\boost\boost_1_60
bjam --toolset=msvc-14.0 --build-type=complete debug release link=shared runtime-link=shared --prefix=C:\boost  install
bjam --toolset=msvc-14.0 --build-type=complete debug release link=static runtime-link=shared --prefix=C:\boost  install

There should be 2 sets of libraries :
boost_..._mt_.dll - this is pure DLL 
boost_..._mt_.lib - this is export info of DLL

libboost_..._mt_.lib - these are static libraries for OrderBook.exe
