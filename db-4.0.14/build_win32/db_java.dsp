# Microsoft Developer Studio Project File - Name="db_java" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=db_java - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "db_java.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "db_java.mak" CFG="db_java - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "db_java - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db_java - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "db_java - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob2 /I "." /I "../include" /I "../include_auto" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DB_CREATE_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 Release/libdb40.lib /nologo /base:"0x13000000" /subsystem:windows /dll /machine:I386 /out:"Release/libdb_java40.dll"
# Begin Custom Build - Compiling java files using javac
ProjDir=.
InputPath=.\Release\libdb_java40.dll
SOURCE=$(InputPath)

"force_compilation.txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mkdir $(ProjDir)\Release\classes 
	echo compiling Berkeley DB classes 
	javac -g -d $(ProjDir)/Release/classes -classpath\
   "$(CLASSPATH);$(ProjDir)/Release/classes"\
   ..\java\src\com\sleepycat\db\*.java 
	echo compiling examples 
	javac -g -d $(ProjDir)/Release/classes -classpath\
   "$(CLASSPATH);$(ProjDir)/Release/classes"\
   ..\java\src\com\sleepycat\examples\*.java 
	echo creating jar files 
	cd $(ProjDir)\Release\classes 
	jar cf ../db.jar com\sleepycat\db\*.class 
	jar cf ../dbexamples.jar com\sleepycat\examples\*.class 
	echo Java build finished 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "db_java - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I "../include" /I "../include_auto" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DB_CREATE_DLL" /D "_WINDLL" /D "_AFXDLL" /YX"config.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Debug/libdb40d.lib /nologo /base:"0x13000000" /subsystem:windows /dll /pdb:none /debug /machine:I386 /out:"Debug/libdb_java40d.dll" /fixed:no
# Begin Custom Build - Compiling java files using javac
ProjDir=.
InputPath=.\Debug\libdb_java40d.dll
SOURCE=$(InputPath)

"force_compilation.txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mkdir $(ProjDir)\Debug\classes 
	echo compiling Berkeley DB classes 
	javac -g -d $(ProjDir)/Debug/classes -classpath\
   "$(CLASSPATH);$(ProjDir)/Debug/classes"\
   ..\java\src\com\sleepycat\db\*.java 
	echo compiling examples 
	javac -g -d $(ProjDir)/Debug/classes -classpath\
   "$(CLASSPATH);$(ProjDir)/Debug/classes"\
   ..\java\src\com\sleepycat\examples\*.java 
	echo creating jar files 
	cd $(ProjDir)\Debug\classes 
	jar cf ../db.jar com\sleepycat\db\*.class 
	jar cf ../dbexamples.jar com\sleepycat\examples\*.class 
	echo Java build finished 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "db_java - Win32 Release"
# Name "db_java - Win32 Debug"
# Begin Source File

SOURCE=..\libdb_java\java_Db.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_DbEnv.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_DbLock.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_DbLogc.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_DbLsn.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_DbTxn.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_DbUtil.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_Dbc.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_Dbt.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_info.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_locked.c
# End Source File
# Begin Source File

SOURCE=..\libdb_java\java_util.c
# End Source File
# End Target
# End Project
