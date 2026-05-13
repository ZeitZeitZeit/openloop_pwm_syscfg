if not exist "C:\ti\ccs2050\ccs\eclipse\dropins-gc" mkdir "C:\ti\ccs2050\ccs\eclipse\dropins-gc"
if not exist "C:\ti\ccs2050\ccs\theia\ccs_plugins" mkdir "C:\ti\ccs2050\ccs\theia\ccs_plugins"
xcopy ".\syscfg\signalsight\gui\" "C:\ti\ccs2050\ccs\eclipse\dropins-gc\mymcusignalsight0\" /E /Y
xcopy ".\syscfg\signalsight\gui\" "C:\ti\ccs2050\ccs\theia\ccs_plugins\mymcusignalsight0\" /E /Y
xcopy "C:\ti\c2000\C2000Ware_26_00_00_00\utilities\transfer\.meta\gui\core\" "C:\ti\ccs2050\ccs\eclipse\dropins-gc\mymcusignalsight0\" /E /Y
xcopy "C:\ti\c2000\C2000Ware_26_00_00_00\utilities\transfer\.meta\gui\core\" "C:\ti\ccs2050\ccs\theia\ccs_plugins\mymcusignalsight0\" /E /Y
if not exist "C:\ti\ccs2050\ccs\eclipse\dropins-gc\mymcusignalsight0\assets" mkdir "C:\ti\ccs2050\ccs\eclipse\dropins-gc\mymcusignalsight0\assets"
if not exist "C:\ti\ccs2050\ccs\theia\ccs_plugins\mymcusignalsight0\assets" mkdir "C:\ti\ccs2050\ccs\theia\ccs_plugins\mymcusignalsight0\assets"
xcopy "C:\ti\c2000\C2000Ware_26_00_00_00\utilities\transfer\.meta\gui\assets\" "C:\ti\ccs2050\ccs\eclipse\dropins-gc\mymcusignalsight0\assets\" /E /Y
xcopy "C:\ti\c2000\C2000Ware_26_00_00_00\utilities\transfer\.meta\gui\assets\" "C:\ti\ccs2050\ccs\theia\ccs_plugins\mymcusignalsight0\assets\" /E /Y
xcopy "C:\ti\c2000\C2000Ware_26_00_00_00\utilities\transfer\.meta\signalsight\plugin_static\" "C:\ti\ccs2050\ccs\eclipse\dropins-gc\mymcusignalsight0\" /E /Y
xcopy "C:\ti\c2000\C2000Ware_26_00_00_00\utilities\transfer\.meta\signalsight\plugin_static\" "C:\ti\ccs2050\ccs\theia\ccs_plugins\mymcusignalsight0\" /E /Y
