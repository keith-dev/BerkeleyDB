# Microsoft Developer Studio Project File - Name="db_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=db_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "db_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "db_dll.mak" CFG="db_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "db_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db_dll - Win32 ASCII Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db_dll - Win32 ASCII Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db_dll - x64 Debug AMD64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db_dll - x64 Release AMD64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db_dll - x64 Debug IA64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db_dll - x64 Release IA64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "db_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release/db_dll"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release/db_dll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob2 /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS"  /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /machine:I386 /out:"Release/libdb45.dll" /libpath:"$(OUTDIR)"

!ELSEIF  "$(CFG)" == "db_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug/db_dll"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug/db_dll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "DIAGNOSTIC" /D "CONFIG_TEST" /D "WIN32" /D "_DEBUG" /D "_WINDOWS"  /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /export:__db_assert /pdb:none /debug /machine:I386 /out:"Debug/libdb45d.dll" /fixed:no /libpath:"$(OUTDIR)"

!ELSEIF  "$(CFG)" == "db_dll - Win32 ASCII Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_ASCII"
# PROP BASE Intermediate_Dir "Debug_ASCII/db_dll"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_ASCII"
# PROP Intermediate_Dir "Debug_ASCII/db_dll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I ".." /D "DB_CREATE_DLL" /D "DIAGNOSTIC" /D "CONFIG_TEST" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS"  /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I ".." /D "DB_CREATE_DLL" /D "DIAGNOSTIC" /D "CONFIG_TEST" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS"  /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /pdb:none /debug /machine:I386 /out:"Debug_ASCII/libdb45d.dll" /fixed:no
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /export:__db_assert /pdb:none /debug /machine:I386 /out:"Debug_ASCII/libdb45d.dll" /fixed:no /libpath:"$(OUTDIR)"

!ELSEIF  "$(CFG)" == "db_dll - Win32 ASCII Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_ASCII"
# PROP BASE Intermediate_Dir "Release_ASCII/db_dll"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_ASCII"
# PROP Intermediate_Dir "Release_ASCII/db_dll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /Ob2 /I "." /I ".." /D "DB_CREATE_DLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS"  /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob2 /I "." /I ".." /D "DB_CREATE_DLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS"  /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /machine:I386 /out:"Release_ASCII/libdb45.dll"
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /machine:I386 /out:"Release_ASCII/libdb45.dll" /libpath:"$(OUTDIR)"

!ELSEIF  "$(CFG)" == "db_dll - x64 Debug AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_AMD64"
# PROP BASE Intermediate_Dir "Debug_AMD64/db_dll"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_AMD64"
# PROP Intermediate_Dir "Debug_AMD64/db_dll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "DIAGNOSTIC" /D "CONFIG_TEST" /D "WIN32" /D "_DEBUG" /D "_WINDOWS"  /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "DIAGNOSTIC" /D "CONFIG_TEST" /D "WIN32" /D "_DEBUG" /D "_WINDOWS"  /FD /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /debug /machine:AMD64 /out:"Debug_AMD64/libdb45d.dll" /fixed:no
# ADD LINK32 ws2_32.lib bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /export:__db_assert /debug /machine:AMD64 /out:"Debug_AMD64/libdb45d.dll" /fixed:no /libpath:"$(OUTDIR)"

!ELSEIF  "$(CFG)" == "db_dll - x64 Release AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_AMD64"
# PROP BASE Intermediate_Dir "Release_AMD64/db_dll"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_AMD64"
# PROP Intermediate_Dir "Release_AMD64/db_dll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /EHsc /O2 /Ob2 /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS"  /Wp64 /FD /c
# ADD CPP /nologo /MD /W3 /EHsc /O2 /Ob2 /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS"  /Wp64 /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /machine:AMD64 /out:"Release_AMD64/libdb45.dll"
# ADD LINK32 ws2_32.lib bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /machine:AMD64 /out:"Release_AMD64/libdb45.dll" /libpath:"$(OUTDIR)"

!ELSEIF  "$(CFG)" == "db_dll - x64 Debug IA64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_IA64"
# PROP BASE Intermediate_Dir "Debug_IA64/db_dll"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_IA64"
# PROP Intermediate_Dir "Debug_IA64/db_dll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "DIAGNOSTIC" /D "CONFIG_TEST" /D "WIN32" /D "_DEBUG" /D "_WINDOWS"  /Wp64 /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /W3 /EHsc /Z7 /Od /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "DIAGNOSTIC" /D "CONFIG_TEST" /D "WIN32" /D "_DEBUG" /D "_WINDOWS"  /Wp64 /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /debug /machine:IA64 /out:"Debug_IA64/libdb45d.dll" /fixed:no
# ADD LINK32 ws2_32.lib bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /export:__db_assert /debug /machine:IA64 /out:"Debug_IA64/libdb45d.dll" /fixed:no /libpath:"$(OUTDIR)"

!ELSEIF  "$(CFG)" == "db_dll - x64 Release IA64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_IA64"
# PROP BASE Intermediate_Dir "Release_IA64/db_dll"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_IA64"
# PROP Intermediate_Dir "Release_IA64/db_dll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /EHsc /O2 /Ob2 /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS"  /Wp64 /FD /c
# ADD CPP /nologo /MD /W3 /EHsc /O2 /Ob2 /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS"  /Wp64 /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /machine:IA64 /out:"Release_IA64/libdb45.dll"
# ADD LINK32 ws2_32.lib bufferoverflowU.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /machine:IA64 /out:"Release_IA64/libdb45.dll" /libpath:"$(OUTDIR)"

!ENDIF 

# Begin Target

# Name "db_dll - Win32 Release"
# Name "db_dll - Win32 Debug"
# Name "db_dll - Win32 ASCII Debug"
# Name "db_dll - Win32 ASCII Release"
# Name "db_dll - x64 Debug AMD64"
# Name "db_dll - x64 Release AMD64"
# Name "db_dll - x64 Debug IA64"
# Name "db_dll - x64 Release IA64"
# Begin Source File

SOURCE=..\btree\bt_compact.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_compare.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_conv.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_curadj.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_cursor.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_delete.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_method.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_open.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_put.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_rec.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_reclaim.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_recno.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_rsearch.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_search.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_split.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_stat.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_upgrade.c
# End Source File
# Begin Source File

SOURCE=..\btree\bt_verify.c
# End Source File
# Begin Source File

SOURCE=..\btree\btree_auto.c
# End Source File
# Begin Source File

SOURCE=.\libdb.def
# End Source File
# Begin Source File

SOURCE=.\libdb.rc
# End Source File
# Begin Source File

SOURCE=..\clib\ctime.c
# End Source File
# Begin Source File

SOURCE=..\clib\getaddrinfo.c
# End Source File
# Begin Source File

SOURCE=..\clib\strcasecmp.c
# End Source File
# Begin Source File

SOURCE=..\clib\strsep.c
# End Source File
# Begin Source File

SOURCE=..\common\db_byteorder.c
# End Source File
# Begin Source File

SOURCE=..\common\db_clock.c
# End Source File
# Begin Source File

SOURCE=..\common\db_err.c
# End Source File
# Begin Source File

SOURCE=..\common\db_getlong.c
# End Source File
# Begin Source File

SOURCE=..\common\db_idspace.c
# End Source File
# Begin Source File

SOURCE=..\common\db_log2.c
# End Source File
# Begin Source File

SOURCE=..\common\mkpath.c
# End Source File
# Begin Source File

SOURCE=..\common\util_cache.c
# End Source File
# Begin Source File

SOURCE=..\common\util_log.c
# End Source File
# Begin Source File

SOURCE=..\common\util_sig.c
# End Source File
# Begin Source File

SOURCE=..\crypto\aes_method.c
# End Source File
# Begin Source File

SOURCE=..\crypto\crypto.c
# End Source File
# Begin Source File

SOURCE=..\crypto\mersenne\mt19937db.c
# End Source File
# Begin Source File

SOURCE=..\crypto\rijndael\rijndael-alg-fst.c
# End Source File
# Begin Source File

SOURCE=..\crypto\rijndael\rijndael-api-fst.c
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_db.cpp
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_dbc.cpp
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_dbt.cpp
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_env.cpp
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_except.cpp
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_lock.cpp
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_logc.cpp
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_mpool.cpp
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_multi.cpp
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_seq.cpp
# End Source File
# Begin Source File

SOURCE=..\cxx\cxx_txn.cpp
# End Source File
# Begin Source File

SOURCE=..\db\crdel_auto.c
# End Source File
# Begin Source File

SOURCE=..\db\crdel_rec.c
# End Source File
# Begin Source File

SOURCE=..\db\db.c
# End Source File
# Begin Source File

SOURCE=..\db\db_am.c
# End Source File
# Begin Source File

SOURCE=..\db\db_auto.c
# End Source File
# Begin Source File

SOURCE=..\db\db_cam.c
# End Source File
# Begin Source File

SOURCE=..\db\db_cds.c
# End Source File
# Begin Source File

SOURCE=..\db\db_conv.c
# End Source File
# Begin Source File

SOURCE=..\db\db_dispatch.c
# End Source File
# Begin Source File

SOURCE=..\db\db_dup.c
# End Source File
# Begin Source File

SOURCE=..\db\db_iface.c
# End Source File
# Begin Source File

SOURCE=..\db\db_join.c
# End Source File
# Begin Source File

SOURCE=..\db\db_meta.c
# End Source File
# Begin Source File

SOURCE=..\db\db_method.c
# End Source File
# Begin Source File

SOURCE=..\db\db_open.c
# End Source File
# Begin Source File

SOURCE=..\db\db_overflow.c
# End Source File
# Begin Source File

SOURCE=..\db\db_ovfl_vrfy.c
# End Source File
# Begin Source File

SOURCE=..\db\db_pr.c
# End Source File
# Begin Source File

SOURCE=..\db\db_rec.c
# End Source File
# Begin Source File

SOURCE=..\db\db_reclaim.c
# End Source File
# Begin Source File

SOURCE=..\db\db_remove.c
# End Source File
# Begin Source File

SOURCE=..\db\db_rename.c
# End Source File
# Begin Source File

SOURCE=..\db\db_ret.c
# End Source File
# Begin Source File

SOURCE=..\db\db_setid.c
# End Source File
# Begin Source File

SOURCE=..\db\db_setlsn.c
# End Source File
# Begin Source File

SOURCE=..\db\db_stati.c
# End Source File
# Begin Source File

SOURCE=..\db\db_truncate.c
# End Source File
# Begin Source File

SOURCE=..\db\db_upg.c
# End Source File
# Begin Source File

SOURCE=..\db\db_upg_opd.c
# End Source File
# Begin Source File

SOURCE=..\db\db_vrfy.c
# End Source File
# Begin Source File

SOURCE=..\db\db_vrfyutil.c
# End Source File
# Begin Source File

SOURCE=..\dbm\dbm.c
# End Source File
# Begin Source File

SOURCE=..\dbreg\dbreg.c
# End Source File
# Begin Source File

SOURCE=..\dbreg\dbreg_auto.c
# End Source File
# Begin Source File

SOURCE=..\dbreg\dbreg_rec.c
# End Source File
# Begin Source File

SOURCE=..\dbreg\dbreg_stat.c
# End Source File
# Begin Source File

SOURCE=..\dbreg\dbreg_util.c
# End Source File
# Begin Source File

SOURCE=..\env\db_salloc.c
# End Source File
# Begin Source File

SOURCE=..\env\db_shash.c
# End Source File
# Begin Source File

SOURCE=..\env\env_config.c
# End Source File
# Begin Source File

SOURCE=..\env\env_failchk.c
# End Source File
# Begin Source File

SOURCE=..\env\env_file.c
# End Source File
# Begin Source File

SOURCE=..\env\env_method.c
# End Source File
# Begin Source File

SOURCE=..\env\env_open.c
# End Source File
# Begin Source File

SOURCE=..\env\env_recover.c
# End Source File
# Begin Source File

SOURCE=..\env\env_region.c
# End Source File
# Begin Source File

SOURCE=..\env\env_register.c
# End Source File
# Begin Source File

SOURCE=..\env\env_stat.c
# End Source File
# Begin Source File

SOURCE=..\fileops\fileops_auto.c
# End Source File
# Begin Source File

SOURCE=..\fileops\fop_basic.c
# End Source File
# Begin Source File

SOURCE=..\fileops\fop_rec.c
# End Source File
# Begin Source File

SOURCE=..\fileops\fop_util.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_auto.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_conv.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_dup.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_func.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_meta.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_method.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_open.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_page.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_rec.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_reclaim.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_stat.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_upgrade.c
# End Source File
# Begin Source File

SOURCE=..\hash\hash_verify.c
# End Source File
# Begin Source File

SOURCE=..\hmac\hmac.c
# End Source File
# Begin Source File

SOURCE=..\hmac\sha1.c
# End Source File
# Begin Source File

SOURCE=..\hsearch\hsearch.c
# End Source File
# Begin Source File

SOURCE=..\lock\lock.c
# End Source File
# Begin Source File

SOURCE=..\lock\lock_deadlock.c
# End Source File
# Begin Source File

SOURCE=..\lock\lock_failchk.c
# End Source File
# Begin Source File

SOURCE=..\lock\lock_id.c
# End Source File
# Begin Source File

SOURCE=..\lock\lock_list.c
# End Source File
# Begin Source File

SOURCE=..\lock\lock_method.c
# End Source File
# Begin Source File

SOURCE=..\lock\lock_region.c
# End Source File
# Begin Source File

SOURCE=..\lock\lock_stat.c
# End Source File
# Begin Source File

SOURCE=..\lock\lock_timer.c
# End Source File
# Begin Source File

SOURCE=..\lock\lock_util.c
# End Source File
# Begin Source File

SOURCE=..\log\log.c
# End Source File
# Begin Source File

SOURCE=..\log\log_archive.c
# End Source File
# Begin Source File

SOURCE=..\log\log_compare.c
# End Source File
# Begin Source File

SOURCE=..\log\log_debug.c
# End Source File
# Begin Source File

SOURCE=..\log\log_get.c
# End Source File
# Begin Source File

SOURCE=..\log\log_method.c
# End Source File
# Begin Source File

SOURCE=..\log\log_put.c
# End Source File
# Begin Source File

SOURCE=..\log\log_stat.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_alloc.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_bh.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_fget.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_fmethod.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_fopen.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_fput.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_fset.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_method.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_mvcc.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_region.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_register.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_stat.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_sync.c
# End Source File
# Begin Source File

SOURCE=..\mp\mp_trickle.c
# End Source File
# Begin Source File

SOURCE=..\mutex\mut_alloc.c
# End Source File
# Begin Source File

SOURCE=..\mutex\mut_failchk.c
# End Source File
# Begin Source File

SOURCE=..\mutex\mut_method.c
# End Source File
# Begin Source File

SOURCE=..\mutex\mut_region.c
# End Source File
# Begin Source File

SOURCE=..\mutex\mut_stat.c
# End Source File
# Begin Source File

SOURCE=..\mutex\mut_win32.c
# End Source File
# Begin Source File

SOURCE=..\os\os_alloc.c
# End Source File
# Begin Source File

SOURCE=..\os\os_fzero.c
# End Source File
# Begin Source File

SOURCE=..\os\os_mkdir.c
# End Source File
# Begin Source File

SOURCE=..\os\os_oflags.c
# End Source File
# Begin Source File

SOURCE=..\os\os_pid.c
# End Source File
# Begin Source File

SOURCE=..\os\os_region.c
# End Source File
# Begin Source File

SOURCE=..\os\os_root.c
# End Source File
# Begin Source File

SOURCE=..\os\os_rpath.c
# End Source File
# Begin Source File

SOURCE=..\os\os_tmpdir.c
# End Source File
# Begin Source File

SOURCE=..\os\os_uid.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_abs.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_clock.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_config.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_dir.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_errno.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_fid.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_flock.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_fsync.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_getenv.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_handle.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_map.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_open.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_rename.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_rw.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_seek.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_sleep.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_spin.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_stat.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_truncate.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_unlink.c
# End Source File
# Begin Source File

SOURCE=..\os_windows\os_yield.c
# End Source File
# Begin Source File

SOURCE=..\qam\qam.c
# End Source File
# Begin Source File

SOURCE=..\qam\qam_auto.c
# End Source File
# Begin Source File

SOURCE=..\qam\qam_conv.c
# End Source File
# Begin Source File

SOURCE=..\qam\qam_files.c
# End Source File
# Begin Source File

SOURCE=..\qam\qam_method.c
# End Source File
# Begin Source File

SOURCE=..\qam\qam_open.c
# End Source File
# Begin Source File

SOURCE=..\qam\qam_rec.c
# End Source File
# Begin Source File

SOURCE=..\qam\qam_stat.c
# End Source File
# Begin Source File

SOURCE=..\qam\qam_upgrade.c
# End Source File
# Begin Source File

SOURCE=..\qam\qam_verify.c
# End Source File
# Begin Source File

SOURCE=..\rep\rep_auto.c
# End Source File
# Begin Source File

SOURCE=..\rep\rep_backup.c
# End Source File
# Begin Source File

SOURCE=..\rep\rep_elect.c
# End Source File
# Begin Source File

SOURCE=..\rep\rep_log.c
# End Source File
# Begin Source File

SOURCE=..\rep\rep_method.c
# End Source File
# Begin Source File

SOURCE=..\rep\rep_record.c
# End Source File
# Begin Source File

SOURCE=..\rep\rep_region.c
# End Source File
# Begin Source File

SOURCE=..\rep\rep_stat.c
# End Source File
# Begin Source File

SOURCE=..\rep\rep_util.c
# End Source File
# Begin Source File

SOURCE=..\rep\rep_verify.c
# End Source File
# Begin Source File

SOURCE=..\repmgr\repmgr_elect.c
# End Source File
# Begin Source File

SOURCE=..\repmgr\repmgr_method.c
# End Source File
# Begin Source File

SOURCE=..\repmgr\repmgr_msg.c
# End Source File
# Begin Source File

SOURCE=..\repmgr\repmgr_net.c
# End Source File
# Begin Source File

SOURCE=..\repmgr\repmgr_queue.c
# End Source File
# Begin Source File

SOURCE=..\repmgr\repmgr_sel.c
# End Source File
# Begin Source File

SOURCE=..\repmgr\repmgr_stat.c
# End Source File
# Begin Source File

SOURCE=..\repmgr\repmgr_util.c
# End Source File
# Begin Source File

SOURCE=..\repmgr\repmgr_windows.c
# End Source File
# Begin Source File

SOURCE=..\sequence\seq_stat.c
# End Source File
# Begin Source File

SOURCE=..\sequence\sequence.c
# End Source File
# Begin Source File

SOURCE=..\txn\txn.c
# End Source File
# Begin Source File

SOURCE=..\txn\txn_auto.c
# End Source File
# Begin Source File

SOURCE=..\txn\txn_chkpt.c
# End Source File
# Begin Source File

SOURCE=..\txn\txn_failchk.c
# End Source File
# Begin Source File

SOURCE=..\txn\txn_method.c
# End Source File
# Begin Source File

SOURCE=..\txn\txn_rec.c
# End Source File
# Begin Source File

SOURCE=..\txn\txn_recover.c
# End Source File
# Begin Source File

SOURCE=..\txn\txn_region.c
# End Source File
# Begin Source File

SOURCE=..\txn\txn_stat.c
# End Source File
# Begin Source File

SOURCE=..\txn\txn_util.c
# End Source File
# Begin Source File

SOURCE=..\xa\xa.c
# End Source File
# Begin Source File

SOURCE=..\xa\xa_db.c
# End Source File
# Begin Source File

SOURCE=..\xa\xa_map.c
# End Source File

# End Target
# End Project
