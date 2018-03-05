# dumprecord
windows端程序崩溃自动抓dump模块，C++编写，接口简单，很方便集成到各种语言的程序中。

解决方案是用vs2015生成的，下载好用vs2015打开DumpRecord.sln，直接编译，会在release或者debug目录下生成dumprecord.dll模块，模块导出的接口只有一个

void _declspec(dllexport) RunCrashHandler()

在C#工程或者C++工程中直接导入这个接口，在程序初始化时调用该接口，整个程序便有了自动截取dump的功能。

注意：dump默认是生成在exe所在目录，以出错时间-进程id-线程id.dmp形式生成目录，目录下有dump文件以及搜集的部分系统库（对于C#程序mscordacwks.dll、mscordbi.dll、mscorlib.dll会自动搜集在目录中，dump分析时会用到）
