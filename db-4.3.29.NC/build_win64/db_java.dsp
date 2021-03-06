# Microsoft Developer Studio Project File - Name="db_java" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
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
!MESSAGE "db_java - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db_java - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
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
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DB_CREATE_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib   /nologo /subsystem:windows /dll /machine:IA64
# ADD LINK32 Release/libdb43.lib /nologo /base:"0x13000000" /subsystem:windows /dll /machine:IA64 /out:"Release/libdb_java43.dll"
# Begin Custom Build - Compiling java files using javac
ProjDir=.
InputPath=.\Release\libdb_java43.dll
SOURCE="$(InputPath)"

"force_compilation.txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo compiling Berkeley DB classes 
	mkdir "$(OUTDIR)\classes"
	javac -O -d "$(OUTDIR)\classes" -classpath "$(OUTDIR)/classes" ..\java\src\com\sleepycat\db\*.java ..\java\src\com\sleepycat\db\internal\*.java ..\java\src\com\sleepycat\bind\*.java ..\java\src\com\sleepycat\bind\serial\*.java ..\java\src\com\sleepycat\bind\tuple\*.java ..\java\src\com\sleepycat\collections\*.java ..\java\src\com\sleepycat\compat\*.java ..\java\src\com\sleepycat\util\*.java
	echo compiling examples 
	mkdir "$(OUTDIR)\classes.ex"
	javac -O -d "$(OUTDIR)\classes.ex" -classpath "$(OUTDIR)\classes;$(OUTDIR)\classes.ex" ..\examples_java\src\com\sleepycat\examples\db\*.java ..\examples_java\src\com\sleepycat\examples\db\GettingStarted\*.java ..\examples_java\src\com\sleepycat\examples\collections\access\*.java ..\examples_java\src\com\sleepycat\examples\collections\hello\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\basic\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\entity\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\tuple\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\sentity\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\marshal\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\factory\*.java
	echo creating jar files 
	jar cf "$(OUTDIR)\db.jar" -C "$(OUTDIR)\classes" .
	jar cf "$(OUTDIR)\dbexamples.jar" -C "$(OUTDIR)\classes.ex" .
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
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DB_CREATE_DLL" /D "_WINDLL" /D "_AFXDLL" /YX"config.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib   /nologo /subsystem:windows /dll /debug /machine:IA64 /pdbtype:sept
# ADD LINK32 Debug/libdb43d.lib /nologo /base:"0x13000000" /subsystem:windows /dll  /debug /machine:IA64 /out:"Debug/libdb_java43d.dll" /fixed:no
# Begin Custom Build - Compiling java files using javac
ProjDir=.
InputPath=.\Debug\libdb_java43d.dll
SOURCE="$(InputPath)"

"force_compilation.txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo compiling Berkeley DB classes 
	mkdir "$(OUTDIR)\classes"
	javac -g -d "$(OUTDIR)\classes" -classpath "$(OUTDIR)/classes" ..\java\src\com\sleepycat\db\*.java ..\java\src\com\sleepycat\db\internal\*.java ..\java\src\com\sleepycat\bind\*.java ..\java\src\com\sleepycat\bind\serial\*.java ..\java\src\com\sleepycat\bind\tuple\*.java ..\java\src\com\sleepycat\collections\*.java ..\java\src\com\sleepycat\compat\*.java ..\java\src\com\sleepycat\util\*.java
	echo compiling examples 
	mkdir "$(OUTDIR)\classes.ex"
	javac -g -d "$(OUTDIR)\classes.ex" -classpath "$(OUTDIR)\classes;$(OUTDIR)\classes.ex" ..\examples_java\src\com\sleepycat\examples\db\*.java ..\examples_java\src\com\sleepycat\examples\db\GettingStarted\*.java ..\examples_java\src\com\sleepycat\examples\collections\access\*.java ..\examples_java\src\com\sleepycat\examples\collections\hello\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\basic\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\entity\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\tuple\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\sentity\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\marshal\*.java ..\examples_java\src\com\sleepycat\examples\collections\ship\factory\*.java
	echo creating jar files 
	jar cf "$(OUTDIR)\db.jar" -C "$(OUTDIR)\classes" .
	jar cf "$(OUTDIR)\dbexamples.jar" -C "$(OUTDIR)\classes.ex" .
	echo Java build finished 

# End Custom Build

!ENDIF 

# Begin Target

# Name "db_java - Win32 Release"
# Name "db_java - Win32 Debug"
# Begin Source File

SOURCE=..\libdb_java\db_java_wrap.c
# End Source File
# End Target
# End Project
