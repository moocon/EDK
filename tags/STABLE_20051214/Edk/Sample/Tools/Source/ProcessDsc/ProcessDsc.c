/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  ProcessDsc.c

Abstract:

  Main module for the ProcessDsc utility.

--*/

#include <windows.h>  // for GetShortPathName()
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <direct.h>   // for _mkdir()
#include <errno.h>
#include <stdlib.h>   // for getenv()
#include "DSCFile.h"
#include "FWVolume.h"
#include "Exceptions.h"
#include "Common.h"

#include "EfiUtilityMsgs.h"
#include "TianoBind.h"
//
// Disable warning for while(1) code
//
#pragma warning(disable : 4127)
//
// Disable warning for unreferenced function parameters
//
#pragma warning(disable : 4100)

extern int  errno;

#define PROGRAM_NAME  "ProcessDsc"

//
// Common symbol name definitions. For example, the user can reference
// $(BUILD_DIR) in their DSC file and we will expand it for them (usually).
// I've defined the equivalents here in case we want to change the name the
// user references, in which case we just change the string value here and
// our code still works.
//
#define BUILD_DIR                       "BUILD_DIR"
#define EFI_SOURCE                      "EFI_SOURCE"
#define DEST_DIR                        "DEST_DIR"
#define SOURCE_DIR                      "SOURCE_DIR"
#define LIB_DIR                         "LIB_DIR"
#define BIN_DIR                         "BIN_DIR"
#define OUT_DIR                         "OUT_DIR"
#define INF_FILENAME                    "INF_FILENAME"
#define SOURCE_RELATIVE_PATH            "SOURCE_RELATIVE_PATH"
#define SOURCE_BASE_NAME                "SOURCE_BASE_NAME"
#define SOURCE_FILE_NAME                "SOURCE_FILE_NAME"    // c:\FullPath\File.c
#define PROCESSOR                       "PROCESSOR"
#define FV                              "FV"
#define BASE_NAME                       "BASE_NAME"
#define GUID                            "GUID"
#define FILE_GUID                       "FILE_GUID"
#define COMPONENT_TYPE_FILE             "FILE"
#define BUILD_TYPE                      "BUILD_TYPE"
#define FFS_EXT                         "FFS_EXT"             // FV_EXT is deprecated -- extension of FFS file
#define MAKEFILE_NAME                   "MAKEFILE_NAME"       // name of component's output makefile
#define PLATFORM                        "PLATFORM"            // for more granularity
#define PACKAGE_FILENAME                "PACKAGE_FILENAME"
#define PACKAGE                         "PACKAGE"
#define PACKAGE_TAG                     "PACKAGE_TAG"         // alternate name to PACKAGE
#define SHORT_NAMES                     "SHORT_NAMES"         // for 8.3 names of symbols
#define APRIORI                         "APRIORI"             // to add to apriori list
#define OPTIONAL_COMPONENT              "OPTIONAL"            // define as non-zero for optional INF files
#define SOURCE_SELECT                   "SOURCE_SELECT"       // say SOURCE_SELECT=smm,common to select INF sources
#define NONFFS_FV                       "NONFFS_FV"           // for non-FFS FV such as working & spare block FV
#define SKIP_FV_NULL                    "SKIP_FV_NULL"        // define as nonzero to not build components with FV=NULL
#define SOURCE_COMPILE_TYPE             "SOURCE_COMPILE_TYPE" // to build a source using a custom build section in the DSC file
#define SOURCE_FILE_EXTENSION           "SOURCE_FILE_EXTENSION"
#define COMPILE_SELECT                  "COMPILE_SELECT"
#define SOURCE_OVERRIDE_PATH            "SOURCE_OVERRIDE_PATH"  // get source files from here first
#define MAKEFILE_OUT_SECTION_NAME       "makefile.out"
#define COMMON_SECTION_NAME             "common"                // shared files or functionality
#define NMAKE_SECTION_NAME              "nmake"
#define SOURCES_SECTION_NAME            "sources"
#define COMPONENTS_SECTION_NAME         "components"
#define INCLUDE_SECTION_NAME            "includes"
#define DEFINES_SECTION_NAME            "defines"
#define LIBRARIES_SECTION_NAME          "libraries"
#define LIBRARIES_PLATFORM_SECTION_NAME "libraries.platform"
#define MAKEFILE_SECTION_NAME           "makefile"
#define COMPONENT_TYPE                  "component_type"
#define PLATFORM_STR                    "\\platform\\"          // to determine EFI_SOURCE
#define MAKEFILE_OUT_NAME               "makefile.out"          // if not specified on command line
//
// When a symbol is defined as "NULL", it gets saved in the symbol table as a 0-length
// string. Use this macro to detect if a symbol has been defined this way.
//
#define IS_NULL_SYMBOL_VALUE(var) ((var != NULL) && (strlen (var) == 0))

//
// Defines for file types
//
#define FILETYPE_UNKNOWN  0
#define FILETYPE_C        1
#define FILETYPE_ASM      2 // .asm or .s
#define FILETYPE_DSC      3 // descriptor file
#define FILETYPE_H        4
#define FILETYPE_LIB      5
#define FILETYPE_I        6
#define FILETYPE_SRC      7
#define FILETYPE_VFR      8
#define FILETYPE_UNI      9

typedef struct {
  INT8  *Extension;         // file extension
  INT8  *BuiltExtension;
  INT8  FileFlags;
  int   FileType;
} FILETYPE;

//
// Define masks for the FileFlags field
//
#define FILE_FLAG_INCLUDE 0x01
#define FILE_FLAG_SOURCE  0x02

//
// This table describes a from-to list of files. For
// example, when a ".c" is built, it results in a ".obj" file.
// For unicode (.uni) files, we specify a NULL built file
// extension. This keeps it out of the list of objects in
// the output makefile.
//
static const FILETYPE mFileTypes[] = {
  {
    ".c",
    ".obj",
    FILE_FLAG_SOURCE,
    FILETYPE_C
  },
  {
    ".asm",
    ".obj",
    FILE_FLAG_SOURCE,
    FILETYPE_ASM
  },
  {
    ".s",
    ".obj",
    FILE_FLAG_SOURCE,
    FILETYPE_ASM
  },
  {
    ".dsc",
    "",
    FILE_FLAG_SOURCE,
    FILETYPE_DSC
  },
  {
    ".h",
    "",
    FILE_FLAG_INCLUDE,
    FILETYPE_H
  },
  {
    ".lib",
    "",
    FILE_FLAG_SOURCE,
    FILETYPE_LIB
  },
  {
    ".i",
    ".obj",
    FILE_FLAG_INCLUDE,
    FILETYPE_I
  },
  {
    ".src",
    ".c",
    FILE_FLAG_INCLUDE,
    FILETYPE_SRC
  },
  {
    ".vfr",
    ".obj",
    FILE_FLAG_SOURCE,
    FILETYPE_VFR
  },  // actually *.vfr -> *.c -> *.obj
  {
    ".uni",
    NULL,
    FILE_FLAG_SOURCE,
    FILETYPE_UNI
  },
  //
  //  { ".uni", ".obj", FILE_FLAG_SOURCE,  FILETYPE_UNI },
  //
  {
    NULL,
    NULL,
    0,
    0
  }
};

//
// BUGBUG -- remove when you merge with ExpandMacros() function.
//
int
ExpandMacrosRecursive (
  INT8  *SourceLine,
  INT8  *DestLine,
  int   LineLen,
  int   ExpandMode
  );

//
// Structure to split up a file into its different parts.
//
typedef struct {
  INT8  Drive[3];
  INT8  *Path;
  INT8  *BaseName;
  INT8  *Extension;
  int   ExtensionCode;
} FILE_NAME_PARTS;

//
// Maximum length for any line in any file after macro expansion
//
#define MAX_EXP_LINE_LEN  (MAX_LINE_LEN * 2)

//
// Linked list to keep track of all symbols
//
typedef struct _SYMBOL {
  struct _SYMBOL  *Next;
  int             Type; // local or global symbol
  INT8            *Name;
  INT8            *Value;
} SYMBOL;

//
// This structure is used to save globals
//
struct {
  INT8    *DscFilename;
  SYMBOL  *Symbol;
  INT8    MakefileName[MAX_PATH]; // output makefile name
  INT8    XRefFileName[MAX_PATH];
  INT8    GuidDatabaseFileName[MAX_PATH];
  FILE    *MakefileFptr;
  UINT32  Verbose;
} gGlobals;

//
// This gets dumped to the head of makefile.out
//
static const INT8 *MakefileHeader[] = {
  "#/*++",
  "#",
  "#  DO NOT EDIT",
  "#  File auto-generated by build utility",
  "#",
  "#  Module Name:",
  "#",
  "#    makefile",
  "#",
  "#  Abstract:",
  "#",
  "#    Auto-generated makefile for building of EFI components/libraries",
  "#",
  "#--*/ ",
  "",
  NULL
};

//
// Function prototypes
//
static
int
ProcessOptions (
  int  Argc,
  INT8 *Argv[]
  );

static
void
Usage (
  VOID
  );

static
INT8              *
StripLine (
  INT8 *Line
  );

static
STATUS
ParseGuidDatabaseFile (
  INT8 *FileName
  );

#define DSC_SECTION_TYPE_COMPONENTS         0
#define DSC_SECTION_TYPE_LIBRARIES          1
#define DSC_SECTION_TYPE_PLATFORM_LIBRARIES 2

static
int
ProcessSectionComponents (
  DSC_FILE *DscFile,
  int      DscSectionType,
  int      Instance
  );
static
int
ProcessComponentFile (
  DSC_FILE  *DscFile,
  INT8      *Line,
  int       DscSectionType,
  int       Instance
  );
static
int
ProcessIncludeFiles (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  );
static

int
ProcessIncludeFilesSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  );

//
// Mode flags for processing source files
//
#define SOURCE_MODE_BUILD_COMMANDS  0x01
#define SOURCE_MODE_SOURCE_FILES    0x02

static
int
ProcessSourceFiles (
  DSC_FILE  *DSCFile,
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  UINT32    Mode
  );

static
int
ProcessSourceFilesSection (
  DSC_FILE  *DSCFile,
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName,
  UINT32    Mode
  );

static
int
ProcessObjects (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  );

static
int
ProcessObjectsSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  );

static
int
ProcessLibs (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  );

static
int
ProcessLibsSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  );

static
int
ProcessIncludesSection (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  );

static
int
ProcessIncludesSectionSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  );

static
int
ProcessINFNMakeSection (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  );

static
int
ProcessINFDefinesSection (
  DSC_FILE  *ComponentFile
  );

static
int
ProcessINFDefinesSectionSingle (
  DSC_FILE  *ComponentFile,
  INT8      *SectionName
  );

static
int
ProcessSectionLibraries (
  DSC_FILE  *DscFile,
  long      Offset
  );

static
int
ProcessDSCDefinesSection (
  DSC_FILE *DscFile
  );

static
int
SetSymbolType (
  INT8  *SymbolName,
  INT8  Type
  );

static
int
RemoveLocalSymbols (
  VOID
  );

static
int
RemoveFileSymbols (
  VOID
  );

static
int
RemoveSymbol (
  INT8   *Name,
  INT8   SymbolType
  );

static
int
SetFileExtension (
  INT8 *FileName,
  INT8 *Extension
  );

static
int
GetSourceFileType (
  INT8 *FileName
  );

static
int
IsIncludeFile (
  INT8 *FileName
  );

static
int
WriteCompileCommands (
  DSC_FILE *DscFile,
  FILE     *MakeFptr,
  INT8     *FileName,
  INT8     *Processor
  );

static
int
WriteCommonMakefile (
  DSC_FILE *DscFile,
  FILE     *MakeFptr,
  INT8     *Processor
  );

static
int
WriteComponentTypeBuildCommands (
  DSC_FILE *DscFile,
  FILE     *MakeFptr,
  INT8     *SectionName
  );

static
void
StripTrailingSpaces (
  INT8 *Str
  );

static
void
FreeFileParts (
  FILE_NAME_PARTS *FP
  );

static
FILE_NAME_PARTS   *
GetFileParts (
  INT8 *FileName
  );

static
SYMBOL            *
FreeSymbols (
  SYMBOL *Syms
  );

static
int
GetEfiSource (
  VOID
  );

static
int
CreatePackageFile (
  DSC_FILE          *DSCFile
  );

static
int
IsAbsolutePath (
  INT8    *FileName
  );

static
INT8              *
BuiltFileExtension (
  INT8      *SourceFileName
  );

/*****************************************************************************/
int
main (
  int   Argc,
  INT8  *Argv[]
  )
/*++

Routine Description:
  
  Main utility entry point.

Arguments:

  Argc - Standard app entry point args.
  Argv - Standard app entry point args.

Returns:

  0  if successful
  non-zero otherwise

--*/
{
  int       i;
  DSC_FILE  DSCFile;
  SECTION   *Sect;
  INT8      Line[MAX_LINE_LEN];
  INT8      ExpLine[MAX_LINE_LEN];
  INT8      *EMsg;

  SetUtilityName (PROGRAM_NAME);

  InitExceptions ();

  DSCFileInit (&DSCFile);
  //
  // Initialize the firmware volume data
  //
  CFVConstructor ();
  //
  // Exception handling for this block of code.
  //
  TryException ();
  //
  // Process command-line options.
  //
  if (ProcessOptions (Argc, Argv)) {
    EMsg = CatchException ();
    if (EMsg != NULL) {
      fprintf (stderr, "%s\n", EMsg);
    }

    return STATUS_ERROR;
  }
  //
  // Parse the GUID database file if specified
  //
  if (gGlobals.GuidDatabaseFileName[0] != 0) {
    ParseGuidDatabaseFile (gGlobals.GuidDatabaseFileName);
  }
  //
  // Set the output cross-reference file if applicable
  //
  if (gGlobals.XRefFileName[0]) {
    CFVSetXRefFileName (gGlobals.XRefFileName);
  }
  //
  // Pre-process the DSC file to get section info.
  //
  if (DSCFileSetFile (&DSCFile, gGlobals.DscFilename) != 0) {
    goto ProcessingError;
  }
  //
  // Try to open the final output makefile
  //
  if ((gGlobals.MakefileFptr = fopen (gGlobals.MakefileName, "w")) == NULL) {
    Error (NULL, 0, 0, gGlobals.MakefileName, "failed to open output makefile for writing");
    goto ProcessingError;
  }
  //
  // Write the header out to the makefile
  //
  for (i = 0; MakefileHeader[i] != NULL; i++) {
    fprintf (gGlobals.MakefileFptr, "%s\n", MakefileHeader[i]);
  }
  //
  // Now get the EFI_SOURCE directory which we use everywhere.
  //
  if (GetEfiSource ()) {
    return STATUS_ERROR;
  }
  //
  // Process the [defines] section in the DSC file to get any defines we need
  // elsewhere
  //
  ProcessDSCDefinesSection (&DSCFile);
  if (ExceptionThrown ()) {
    goto ProcessingError;
  }
  //
  // Write out the [makefile.out] section data to the main output makefile.
  //
  Sect = DSCFileFindSection (&DSCFile, MAKEFILE_OUT_SECTION_NAME);
  if (Sect != NULL) {
    while (DSCFileGetLine (&DSCFile, Line, sizeof (Line)) != NULL) {
      ExpandMacros (
        Line,
        ExpLine,
        sizeof (ExpLine),
        EXPANDMODE_NO_DESTDIR | EXPANDMODE_NO_SOURCEDIR
        );
      //
      // Write the line to makefile.out
      //
      fprintf (gGlobals.MakefileFptr, ExpLine);
    }
  }
  //
  // Process [libraries] section in the DSC file
  //
  Sect = DSCFileFindSection (&DSCFile, LIBRARIES_SECTION_NAME);
  if (Sect != NULL) {
    fprintf (gGlobals.MakefileFptr, "libraries : \n");
    ProcessSectionComponents (&DSCFile, DSC_SECTION_TYPE_LIBRARIES, 0);
  }

  if (ExceptionThrown ()) {
    goto ProcessingError;
  }
  //
  // Process [libraries.platform] section in the DSC file
  //
  Sect = DSCFileFindSection (&DSCFile, LIBRARIES_PLATFORM_SECTION_NAME);
  if (Sect != NULL) {
    ProcessSectionComponents (&DSCFile, DSC_SECTION_TYPE_PLATFORM_LIBRARIES, 0);
  }

  fprintf (gGlobals.MakefileFptr, "\n");
  if (ExceptionThrown ()) {
    goto ProcessingError;
  }
  //
  // Process [components] section in the DSC file
  //
  Sect = DSCFileFindSection (&DSCFile, COMPONENTS_SECTION_NAME);
  if (Sect != NULL) {
    ProcessSectionComponents (&DSCFile, DSC_SECTION_TYPE_COMPONENTS, 0);
    fprintf (gGlobals.MakefileFptr, "\n");
  }

  if (ExceptionThrown ()) {
    goto ProcessingError;
  }
  //
  // Now cycle through all [components.1], [components.2], ....[components.n].
  // This is necessary to support building of firmware volumes that may contain
  // other encapsulated firmware volumes (ala capsules).
  //
  i = 1;
  while (1) {
    RemoveSymbol (FV, SYM_GLOBAL);
    sprintf (Line, "%s.%d", COMPONENTS_SECTION_NAME, i);
    Sect = DSCFileFindSection (&DSCFile, Line);
    if (Sect != NULL) {
      ProcessSectionComponents (&DSCFile, DSC_SECTION_TYPE_COMPONENTS, i);
      fprintf (gGlobals.MakefileFptr, "\n");
    } else {
      break;
    }

    if (ExceptionThrown ()) {
      goto ProcessingError;
    }

    i++;
  }

ProcessingError:
  EMsg = CatchException ();
  if (EMsg != NULL) {
    fprintf (stderr, "%s\n", EMsg);
    fprintf (stderr, "Processing aborted\n");
  }

  TryException ();
  //
  // Create the FV files if no fatal errors or errors
  //
  if (GetUtilityStatus () < STATUS_ERROR) {
    CFVWriteInfFiles (&DSCFile, gGlobals.MakefileFptr);
  }
  //
  // Close the makefile
  //
  if (gGlobals.MakefileFptr != NULL) {
    fclose (gGlobals.MakefileFptr);
    gGlobals.MakefileFptr = NULL;
  }
  //
  // Clean up
  //
  FreeSymbols (gGlobals.Symbol);
  gGlobals.Symbol = NULL;
  CFVDestructor ();
  DSCFileDestroy (&DSCFile);

  EMsg = CatchException ();
  if (EMsg != NULL) {
    fprintf (stderr, "%s\n", EMsg);
    fprintf (stderr, "Processing aborted\n");
  }

  return GetUtilityStatus ();
}

static
int
ProcessSectionComponents (
  DSC_FILE  *DSCFile,
  int       DscSectionType,
  int       Instance
  )
/*++

Routine Description:
  
  Process the [components] or [libraries] section in the description file. We
  use this function for both since they're very similar. Here we just
  read each line from the section, and if it's valid, call a function to
  do the actual processing of the component description file.

Arguments:

  DSCFile        - structure containing section info on the description file
  DscSectionType - type of description section

Returns:

  0     if successful

--*/
{
  INT8  Line[MAX_LINE_LEN];
  INT8  Line2[MAX_EXP_LINE_LEN];
  INT8  *Cptr;

  //
  // Read lines while they're valid
  //
  while (DSCFileGetLine (DSCFile, Line, sizeof (Line)) != NULL) {
    //
    // Expand macros on the line
    //
    if (ExpandMacros (Line, Line2, sizeof (Line2), 0)) {
      return STATUS_ERROR;
    }
    //
    // Strip the line
    //
    Cptr = StripLine (Line2);
    if (*Cptr) {
      Message (2, "Processing component line: %s", Line2);
      if (ProcessComponentFile (DSCFile, Line2, DscSectionType, Instance) != 0) {
        return STATUS_ERROR;
      }
    }
  }

  return 0;
}

static
int
ProcessComponentFile (
  DSC_FILE  *DSCFile,
  INT8      *ArgLine,
  int       DscSectionType,
  int       Instance
  )
/*++

Routine Description:
  
  Given a line from the [components] or [libraries] section of the description
  file, process the line to extract the component's INF filename and 
  parameters. Then open the INF file and process it to create a corresponding
  makefile.

Arguments:

  DSCFile               The project DSC file info structure.
  Libs                  Indicates whether we're processing the [components]
                        section or the [libraries] section.
  ArgLine               The actual line from the DSC file. Looks something like
                        one of the following:

   dxe\drivers\vm\vm.dsc PROCESSOR=IA32 DEST_DIR=$(DEST_DIR)\xxx FV=FV1,FV2
   $(BUILD_DIR).\FvVariable.ffs COMPONENT_TYPE=FILE
   .\FvVariable.ffs COMPONENT_TYPE=FILE
   define  VAR1=value1 VAR2=value2

Returns:

  0 if successful

--*/
{
  FILE      *MakeFptr;
  FILE      *TempFptr;
  INT8      *Cptr;
  INT8      *name;
  INT8      *End;
  INT8      *TempCptr;
  INT8      FileName[MAX_PATH];
  INT8      ComponentFilePath[MAX_PATH];
  INT8      InLine[MAX_LINE_LEN];
  INT8      Line[MAX_LINE_LEN];
  INT8      *Processor;
  INT8      SymType;
  int       Len;
  int       ComponentCreated;
  int       ComponentFilePathAbsolute;
  int       DefineLine;
  DSC_FILE  ComponentFile;
  INT8      ComponentMakefileName[MAX_PATH];
  BOOLEAN   IsForFv;

  //
  // Now remove all local symbols
  //
  RemoveLocalSymbols ();
  //
  // Null out the file pointer in case we take an exception somewhere
  // and we need to close it only if we opened it.
  //
  MakeFptr                  = NULL;
  ComponentFilePathAbsolute = 0;
  ComponentCreated          = 0;
  //
  // Skip preceeding spaces on the line
  //
  while (isspace (*ArgLine) && (*ArgLine)) {
    ArgLine++;
  }
  //
  // Find the end of the component's filename and truncate the line at that
  // point. From here on out ArgLine is the name of the component filename.
  //
  Cptr = ArgLine;
  while (!isspace (*Cptr) && *Cptr) {
    Cptr++;
  }

  End = Cptr;
  if (*Cptr) {
    End++;
    *Cptr = 0;
  }
  //
  // Exception-handle processing of this component description file
  //
  TryException ();
  //
  // Save the path from the component name for later. It may be relative or
  // absolute.
  //
  strcpy (ComponentFilePath, ArgLine);
  Cptr = ComponentFilePath + strlen (ComponentFilePath) - 1;
  while ((*Cptr != '\\') && (*Cptr != '/') && (Cptr != ComponentFilePath)) {
    Cptr--;
  }
  //
  // Terminate the path.
  //
  *Cptr = 0;
  //
  // If we have "c:\path\filename"
  //
  if (isalpha (ComponentFilePath[0]) && (ComponentFilePath[1] == ':')) {
    ComponentFilePathAbsolute = 1;
  } else if (ComponentFilePath[0] == '.') {
    //
    // or if the path starts with ".", then it's build-dir relative.
    // Prepend $(BUILD_DIR) on the file name
    //
    sprintf (Line, "%s\\%s", GetSymbolValue (BUILD_DIR), ComponentFilePath);
    strcpy (ComponentFilePath, Line);
    ComponentFilePathAbsolute = 1;
  }
  //
  // We also allow a component line format for defines of global symbols
  // instead of a component filename. In this case, the line looks like:
  // defines  x=abc y=yyy. Be nice and accept "define" and "defines" in a
  // case-insensitive manner. If it's defines, then make the symbols global.
  //
  if ((stricmp (ArgLine, "define") == 0) || (stricmp (ArgLine, "defines") == 0)) {
    SymType     = SYM_OVERWRITE | SYM_GLOBAL;
    DefineLine  = 1;
  } else {
    SymType     = SYM_OVERWRITE | SYM_LOCAL;
    DefineLine  = 0;
  }
  //
  // The rest of the component line from the DSC file should be defines
  //
  while (*End) {
    End = StripLine (End);
    if (*End) {
      //
      // If we're processing a "define abc=1 xyz=2" line, then set symbols
      // as globals per the SymType set above.
      //
      Len = AddSymbol (End, NULL, SymType);
      if (Len > 0) {
        End += Len;
      } else {
        Warning (NULL, 0, 0, ArgLine, "unrecognized option in description file");
        break;
      }
    }
  }
  //
  // If it's a define line, then we're done
  //
  if (DefineLine) {
    //
    // If there is NonFFS_FV, create the FVxxx.inf file
    // and include it in makefile.out. Remove the symbol
    // in order not to process it again next time
    //
    Cptr = GetSymbolValue (NONFFS_FV);
    if (Cptr != NULL) {
      NonFFSFVWriteInfFiles (DSCFile, Cptr);
      RemoveSymbol (NONFFS_FV, SYM_GLOBAL);
    }

    goto ComponentDone;
  }
  //
  // If DEBUG_BREAK or EFI_BREAKPOINT is defined, then do a debug breakpoint.
  //
  if ((GetSymbolValue ("DEBUG_BREAK") != NULL) || (GetSymbolValue ("EFI_BREAKPOINT") != NULL)) {
    EFI_BREAKPOINT ();
  }
  //
  // Create the destination path. Save it as the destination directory,
  // assuming that it is located near its source files.
  // The destination path is $(BUILD_DIR)\$(PROCESSOR)\component_path
  //
  if (ComponentFilePathAbsolute == 0) {
    sprintf (
      FileName,
      "%s\\%s\\%s",
      GetSymbolValue (BUILD_DIR),
      GetSymbolValue (PROCESSOR),
      ComponentFilePath
      );
  } else {
    sprintf (
      FileName,
      "%s\\%s",
      ComponentFilePath,
      GetSymbolValue (PROCESSOR)
      );
  }
  //
  // They may have defined DEST_DIR on the component INF line, so it's already
  // been defined, If that's the case, then don't set it to the path of this file.
  //
  if (GetSymbolValue (DEST_DIR) == NULL) {
    AddSymbol (DEST_DIR, FileName, SYM_OVERWRITE | SYM_LOCAL | SYM_FILEPATH);
  }
  //
  // Create the source directory path from the component file's path. If the component
  // file's path is absolute, we may have problems here. Try to account for it though.
  //
  if (ComponentFilePathAbsolute == 0) {
    sprintf (
      FileName,
      "%s\\%s",
      GetSymbolValue (EFI_SOURCE),
      ComponentFilePath
      );
  } else {
    strcpy (FileName, ComponentFilePath);
  }

  AddSymbol (SOURCE_DIR, FileName, SYM_OVERWRITE | SYM_LOCAL | SYM_FILEPATH);
  //
  // Expand symbols in the component description filename
  //
  ExpandMacros (ArgLine, Line, sizeof (Line), 0);
  //
  // Typically the given line is a component description filename. However we
  // also allow a FV filename (fvvariable.ffs COMPONENT_TYPE=FILE). If the
  // component type is "FILE", then add it to the FV list, create a package
  // file, and we're done.
  //
  Cptr = GetSymbolValue (COMPONENT_TYPE);
  if ((Cptr != NULL) && (strncmp (
                          Cptr,
                          COMPONENT_TYPE_FILE,
                          strlen (COMPONENT_TYPE_FILE)
                          ) == 0)) {
    CFVAddFVFile (
      Line,
      Cptr,
      GetSymbolValue (FV),
      Instance,
      NULL,
      NULL,
      GetSymbolValue (APRIORI),
      GetSymbolValue (BASE_NAME),
      NULL
      );
    goto ComponentDone;
  }
  //
  // Better have defined processor by this point.
  //
  Processor = GetSymbolValue (PROCESSOR);
  if (Processor == NULL) {
    Error (NULL, 0, 0, NULL, "PROCESSOR not defined for component %s", Line);
    return STATUS_ERROR;
  }
  //
  // The bin, out, and lib dirs are now = $(BUILD_DIR)/$(PROCESSOR). Set them.
  // Don't flag them as file paths (required for short 8.3 filenames) since
  // they're defined using the BUILD_DIR macro.
  //
  sprintf (InLine, "$(BUILD_DIR)\\%s", Processor);
  AddSymbol (BIN_DIR, InLine, SYM_LOCAL);
  AddSymbol (OUT_DIR, InLine, SYM_LOCAL);
  AddSymbol (LIB_DIR, InLine, SYM_LOCAL);
  //
  // See if it's been destined for an FV. It's possible to not be in an
  // FV if they just want to build it.
  //
  Cptr = GetSymbolValue (FV);
  if ((Cptr != NULL) && !IS_NULL_SYMBOL_VALUE (Cptr)) {
    IsForFv = TRUE;
  } else {
    IsForFv = FALSE;
  }
  //
  // As an optimization, if they've defined SKIP_FV_NULL as non-zero, and
  // the component is not destined for an FV, then skip it.
  // Since libraries are never intended for firmware volumes, we have to
  // build all of them.
  //
  if ((DscSectionType == DSC_SECTION_TYPE_COMPONENTS) && (IsForFv == FALSE)) {
    if ((GetSymbolValue (SKIP_FV_NULL) != NULL) && (atoi (GetSymbolValue (SKIP_FV_NULL)) != 0)) {
      Message (0, "%s not being built (FV=NULL)", FileName);
      goto ComponentDone;
    }
  }
  //
  // Prepend EFI_SOURCE to the component description file to get the
  // full path. Only do this if the path is not a full path already.
  //
  if (ComponentFilePathAbsolute == 0) {
    name = GetSymbolValue (EFI_SOURCE);
    sprintf (FileName, "%s\\%s", name, Line);
  } else {
    strcpy (FileName, Line);
  }
  //
  // Print a message, depending on verbose level.
  //
  if (DscSectionType == DSC_SECTION_TYPE_COMPONENTS) {
    Message (1, "Processing component         %s", FileName);
  } else {
    Message (1, "Processing library           %s", FileName);
  }
  //
  // Open the component's description file and get the sections. If we fail
  // to open it, see if they defined "OPTIONAL=1, in which case we'll just
  // ignore the component.
  //
  TempFptr = fopen (FileName, "r");
  if (TempFptr == NULL) {
    //
    // Better have defined OPTIONAL
    //
    if (GetSymbolValue (OPTIONAL_COMPONENT) != NULL) {
      if (atoi (GetSymbolValue (OPTIONAL_COMPONENT)) != 0) {
        Message (0, "Optional component '%s' not found", FileName);
        goto ComponentDone;
      }
    }

    ParserError (0, FileName, "failed to open component file");
    return STATUS_ERROR;
  } else {
    fclose (TempFptr);
  }

  DSCFileInit (&ComponentFile);
  ComponentCreated = 1;
  if (DSCFileSetFile (&ComponentFile, FileName)) {
    Error (NULL, 0, 0, NULL, "failed to preprocess component file '%s'", FileName);
    return STATUS_ERROR;
  }
  //
  // Add a symbol for the INF filename so users can create dependencies
  // in makefiles.
  //
  AddSymbol (INF_FILENAME, FileName, SYM_OVERWRITE | SYM_LOCAL | SYM_FILENAME);
  //
  // Process the [defines], [defines.$(PROCESSOR)], and [defines.$(PROCESSOR).$(PLATFORM)]
  // sections in the INF file
  //
  ProcessINFDefinesSection (&ComponentFile);
  //
  // Better have defined FILE_GUID if not a library
  //
  if ((GetSymbolValue (GUID) == NULL) &&
      (GetSymbolValue (FILE_GUID) == NULL) &&
      (DscSectionType == DSC_SECTION_TYPE_COMPONENTS)
      ) {
    Error (GetSymbolValue (INF_FILENAME), 1, 0, NULL, "missing FILE_GUID definition in component file");
    DSCFileDestroy (&ComponentFile);
    return STATUS_ERROR;
  }
  //
  // Better have defined base name
  //
  if (GetSymbolValue (BASE_NAME) == NULL) {
    Error (GetSymbolValue (INF_FILENAME), 1, 0, NULL, "missing BASE_NAME definition in INF file");
    DSCFileDestroy (&ComponentFile);
    return STATUS_ERROR;
  }
  //
  // Better have defined COMPONENT_TYPE, since it's used to find named sections.
  //
  if (GetSymbolValue (COMPONENT_TYPE) == NULL) {
    Error (GetSymbolValue (INF_FILENAME), 1, 0, NULL, "missing COMPONENT_TYPE definition in INF file");
    DSCFileDestroy (&ComponentFile);
    return STATUS_ERROR;
  }
  //
  // Create the output directory, then open the output component's makefile
  // we're going to create. Allow them to override the makefile name.
  //
  TempCptr = GetSymbolValue (MAKEFILE_NAME);
  if (TempCptr != NULL) {
    ExpandMacros (TempCptr, ComponentMakefileName, sizeof (ComponentMakefileName), 0);
    TempCptr = ComponentMakefileName;
  } else {
    TempCptr = "makefile";
  }

  sprintf (FileName, "%s\\%s", GetSymbolValue (DEST_DIR), TempCptr);
  //
  // Save it now with path info
  //
  AddSymbol (MAKEFILE_NAME, FileName, SYM_OVERWRITE | SYM_LOCAL | SYM_FILENAME);

  if (MakeFilePath (FileName)) {
    return STATUS_ERROR;
  }

  if ((MakeFptr = fopen (FileName, "w")) == NULL) {
    Error (NULL, 0, 0, FileName, "could not create makefile");
    return STATUS_ERROR;
  }
  //
  // At this point we should have all the info we need to create a package
  // file if setup to do so. Libraries don't use package files, so
  // don't do this for libs.
  //
  if (DscSectionType == DSC_SECTION_TYPE_COMPONENTS) {
    CreatePackageFile (DSCFile);
  }
  //
  // Write an nmake line to makefile.out
  // If this is a library, then the caller already created the "libraries :" target,
  // so print the line.
  //
  if (DscSectionType == DSC_SECTION_TYPE_COMPONENTS) {
    if (GetSymbolValue (FV) != NULL) {
      fprintf (gGlobals.MakefileFptr, "components_%d ::\n", Instance);
      fprintf (gGlobals.MakefileFptr, "  @cd %s\n", Processor);
      fprintf (gGlobals.MakefileFptr, "  $(MAKE) -f %s all\n", FileName);
      fprintf (gGlobals.MakefileFptr, "  @cd ..\n\n");
    } else {
      //
      // FV=NULL component
      //
      fprintf (gGlobals.MakefileFptr, "fv_null_components ::\n");
      fprintf (gGlobals.MakefileFptr, "  @cd %s\n", Processor);
      fprintf (gGlobals.MakefileFptr, "  $(MAKE) -f %s all\n", FileName);
      fprintf (gGlobals.MakefileFptr, "  @cd ..\n\n");
    }
  } else {
    //
    // We just add up the libraries.
    //
    fprintf (gGlobals.MakefileFptr, "  @cd %s\n", Processor);
    fprintf (gGlobals.MakefileFptr, "  $(MAKE) -f %s all\n", FileName);
    fprintf (gGlobals.MakefileFptr, "  @cd ..\n");
  }
  //
  // Copy the common makefile section from the description file to
  // the component's makefile
  //
  WriteCommonMakefile (DSCFile, MakeFptr, Processor);
  //
  // Process the component's [nmake.common] and [nmake.$(PROCESSOR)] sections
  //
  ProcessINFNMakeSection (&ComponentFile, MakeFptr);
  //
  // Create the SOURCE_FILES macro that includes the names of all source
  // files in this component. This macro can then be used elsewhere to
  // process all the files making up the component. Required for scanning
  // files for string localization.
  //
  ProcessSourceFiles (DSCFile, &ComponentFile, MakeFptr, SOURCE_MODE_SOURCE_FILES);
  //
  // Create the include paths. Process [includes.common] and
  // [includes.$(PROCESSOR)] and [includes.$(PROCESSOR).$(PLATFORM)] sections.
  //
  ProcessIncludesSection (&ComponentFile, MakeFptr);
  //
  // Process all include source files to create a dependency list that can
  // be used in the makefile.
  //
  ProcessIncludeFiles (&ComponentFile, MakeFptr);
  //
  // Process the [sources.common], [sources.$(PROCESSOR)], and
  // [sources.$(PROCESSOR).$(PLATFORM)] files and emit their build commands
  //
  ProcessSourceFiles (DSCFile, &ComponentFile, MakeFptr, SOURCE_MODE_BUILD_COMMANDS);
  //
  // Process sources again to create an OBJECTS macro
  //
  ProcessObjects (&ComponentFile, MakeFptr);
  //
  // Process all the libraries to define "LIBS = x.lib y.lib..."
  // Be generous and append ".lib" if they forgot.
  // Make a macro definition: LIBS = $(LIBS) xlib.lib ylib.lib...
  //
  ProcessLibs (&ComponentFile, MakeFptr);
  //
  // Emit commands to create the component. These are simply copied from
  // the description file to the component's makefile. First look for
  // [build.$(PROCESSOR).$(BUILD_TYPE)]. If not found, then look for if
  // find a [build.$(PROCESSOR).$(COMPONENT_TYPE)] line.
  //
  Cptr = GetSymbolValue (BUILD_TYPE);
  if (Cptr != NULL) {
    sprintf (InLine, "build.%s.%s", Processor, Cptr);
    WriteComponentTypeBuildCommands (DSCFile, MakeFptr, InLine);
  } else {
    sprintf (InLine, "build.%s.%s", Processor, GetSymbolValue (COMPONENT_TYPE));
    WriteComponentTypeBuildCommands (DSCFile, MakeFptr, InLine);
  }
  //
  // Add it to the FV if not a library
  //
  if (DscSectionType == DSC_SECTION_TYPE_COMPONENTS) {
    //
    // Create the FV filename and add it to the FV.
    // By this point we know it's in FV.
    //
    Cptr = GetSymbolValue (FILE_GUID);
    if (Cptr == NULL) {
      Cptr = GetSymbolValue (GUID);
    }

    sprintf (InLine, "%s-%s", Cptr, GetSymbolValue (BASE_NAME));
    //
    // We've deprecated FV_EXT, which should be FFS_EXT, the extension
    // of the FFS file generated by GenFFSFile.
    //
    TempCptr = GetSymbolValue (FFS_EXT);
    if (TempCptr == NULL) {
      TempCptr = GetSymbolValue ("FV_EXT");
    }

    CFVAddFVFile (
      InLine,
      GetSymbolValue (COMPONENT_TYPE),
      GetSymbolValue (FV),
      Instance,
      TempCptr,
      Processor,
      GetSymbolValue (APRIORI),
      GetSymbolValue (BASE_NAME),
      Cptr
      );
  }
  //
  // Catch any failures and print the name of the component file
  // being processed to assist debugging.
  //
ComponentDone:

  Cptr = CatchException ();
  if (Cptr != NULL) {
    fprintf (stderr, "%s\n", Cptr);
    sprintf (InLine, "Processing of component %s failed", ArgLine);
    ThrowException (InLine);
  }

  if (MakeFptr != NULL) {
    fclose (MakeFptr);
  }

  if (ComponentCreated) {
    DSCFileDestroy (&ComponentFile);
  }

  return STATUS_SUCCESS;
}

static
int
CreatePackageFile (
  DSC_FILE          *DSCFile
  )
{
  INT8    *Package;
  SECTION *TempSect;
  INT8    Str[MAX_LINE_LEN];
  INT8    StrExpanded[MAX_LINE_LEN];
  FILE    *PkgFptr;
  int     Status;

  PkgFptr = NULL;

  //
  // First find out if PACKAGE_FILENAME or PACKAGE is defined. PACKAGE_FILENAME
  // is used to specify the exact package file to use. PACKAGE is used to
  // specify the package section name.
  //
  Package = GetSymbolValue (PACKAGE_FILENAME);
  if (Package != NULL) {
    //
    // Use existing file. We're done.
    //
    return STATUS_SUCCESS;
  }
  //
  // See if PACKAGE or PACKAGE_TAG is defined
  //
  Package = GetSymbolValue (PACKAGE);
  if (Package == NULL) {
    Package = GetSymbolValue (PACKAGE_TAG);
  }

  if (Package == NULL) {
    //
    // Not defined either. Assume they are not using the package functionality
    // of this utility. However define the PACKAGE_FILENAME macro to the
    // best-guess value.
    //
    sprintf (
      Str,
      "%s\\%s.pkg",
      GetSymbolValue (SOURCE_DIR),
      GetSymbolValue (BASE_NAME)
      );
    AddSymbol (PACKAGE_FILENAME, Str, SYM_LOCAL | SYM_FILENAME);
    return STATUS_SUCCESS;
  }
  //
  // Save the position in the DSC file.
  // Find the [package.$(COMPONENT_TYPE).$(PACKAGE)] section in the DSC file
  //
  Status = STATUS_SUCCESS;
  DSCFileSavePosition (DSCFile);
  sprintf (Str, "%s.%s.%s", PACKAGE, GetSymbolValue (COMPONENT_TYPE), Package);
  TempSect = DSCFileFindSection (DSCFile, Str);
  if (TempSect != NULL) {
    //
    // So far so good. Create the name of the package file, then open it up
    // for writing. File name is c:\...\oem\platform\nt32\ia32\...\BaseName.pkg.
    //
    sprintf (
      Str,
      "%s\\%s.pkg",
      GetSymbolValue (DEST_DIR),
      GetSymbolValue (BASE_NAME)
      );
    //
    // Try to open the file, then save the file name as the PACKAGE_FILENAME
    // symbol for use elsewhere.
    //
    if ((PkgFptr = fopen (Str, "w")) == NULL) {
      Error (NULL, 0, 0, Str, "could not open package file for writing");
      Status = STATUS_ERROR;
      goto Finish;
    }

    AddSymbol (PACKAGE_FILENAME, Str, SYM_LOCAL | SYM_FILENAME);
    //
    // Now read lines in from the DSC file and write them back out to the
    // package file (with string substitution).
    //
    while (DSCFileGetLine (DSCFile, Str, sizeof (Str)) != NULL) {
      //
      // Expand macros, then write the line out to the package file
      //
      ExpandMacrosRecursive (Str, StrExpanded, sizeof (StrExpanded), EXPANDMODE_RECURSIVE);
      fprintf (PkgFptr, StrExpanded);
    }
  } else {
    Warning (
      NULL,
      0,
      0,
      NULL,
      "cannot locate package section [%s] in DSC file for %s",
      Str,
      GetSymbolValue (INF_FILENAME)
      );
    Status = STATUS_WARNING;
    goto Finish;
  }

  if (PkgFptr != NULL) {
    fclose (PkgFptr);
  }

Finish:
  //
  // Restore the position in the DSC file
  //
  DSCFileRestorePosition (DSCFile);

  return STATUS_SUCCESS;
}

static
int
ProcessINFDefinesSection (
  DSC_FILE   *ComponentFile
  )
/*++

Routine Description:

  Process the [defines.xxx] sections of the component description file. Process
  platform first, then processor. In this way, if a platform wants and override,
  that one gets parsed first, and later assignments do not overwrite the value.
  
Arguments:

  ComponentFile     - section info on the component file being processed

Returns:

 
--*/
{
  INT8  *Cptr;
  INT8  Str[MAX_LINE_LEN];

  //
  // Find a [defines.$(PROCESSOR).$(PLATFORM)] section and process it
  //
  Cptr = GetSymbolValue (PLATFORM);
  if (Cptr != NULL) {
    sprintf (
      Str,
      "%s.%s.%s",
      DEFINES_SECTION_NAME,
      GetSymbolValue (PROCESSOR),
      Cptr
      );
    ProcessINFDefinesSectionSingle (ComponentFile, Str);
  }
  //
  // Find a [defines.$(PROCESSOR)] section and process it
  //
  sprintf (Str, "%s.%s", DEFINES_SECTION_NAME, GetSymbolValue (PROCESSOR));
  ProcessINFDefinesSectionSingle (ComponentFile, Str);

  //
  // Find a [defines] section and process it
  //
  if (ProcessINFDefinesSectionSingle (ComponentFile, DEFINES_SECTION_NAME) != STATUS_SUCCESS) {
    Error (NULL, 0, 0, NULL, "missing [defines] section in component file %s", GetSymbolValue (INF_FILENAME));
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

static
int
ProcessINFDefinesSectionSingle (
  DSC_FILE  *ComponentFile,
  INT8      *SectionName
  )
{
  INT8    *Cptr;
  INT8    Str[MAX_LINE_LEN];
  SECTION *TempSect;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      //
      // Don't process blank lines.
      //
      if (*Cptr) {
        //
        // Add without overwriting macros specified on the component line
        // in the description file
        //
        AddSymbol (Str, NULL, SYM_LOCAL);
      }
    }
  } else {
    return STATUS_WARNING;
  }

  return STATUS_SUCCESS;
}

static
int
ProcessINFNMakeSection (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  )
/*++

Routine Description:

  Process the [nmake.common] and [nmake.$(PROCESSOR)] sections of the component
  description file and write and copy them to the component's makefile.
  
Arguments:

  ComponentFile     - section info on the component file being processed
  MakeFptr          - file pointer to the component' makefile we're creating

Returns:

  Always STATUS_SUCCESS right now, since the sections are optional.
  
--*/
{
  INT8    *Cptr;
  INT8    Str[MAX_LINE_LEN];
  INT8    ExpandedLine[MAX_LINE_LEN];
  SECTION *TempSect;

  //
  // Copy the [nmake.common] and [nmake.processor] sections from the
  // component file directly to the output file.
  //
  sprintf (Str, "%s.%s", NMAKE_SECTION_NAME, COMMON_SECTION_NAME);
  TempSect = DSCFileFindSection (ComponentFile, Str);
  if (TempSect != NULL) {
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      ExpandMacros (
        Cptr,
        ExpandedLine,
        sizeof (ExpandedLine),
        EXPANDMODE_NO_DESTDIR | EXPANDMODE_NO_SOURCEDIR
        );
      fprintf (MakeFptr, "%s\n", ExpandedLine);
    }

    fprintf (MakeFptr, "\n");
  } else {
    Error (GetSymbolValue (INF_FILENAME), 1, 0, Str, "section not found in component INF file");
  }

  sprintf (Str, "%s.%s", NMAKE_SECTION_NAME, GetSymbolValue (PROCESSOR));
  TempSect = DSCFileFindSection (ComponentFile, Str);
  if (TempSect != NULL) {
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      //
      // Don't print blank lines?
      //
      if (*Cptr) {
        fprintf (MakeFptr, "%s\n", Cptr);
      }
    }

    fprintf (MakeFptr, "\n");
  }
  //
  // Do the same for [nmake.processor.platform]
  //
  Cptr = GetSymbolValue (PLATFORM);
  if (Cptr != NULL) {
    sprintf (Str, "%s.%s.%s", NMAKE_SECTION_NAME, GetSymbolValue (PROCESSOR), Cptr);
    TempSect = DSCFileFindSection (ComponentFile, Str);
    if (TempSect != NULL) {
      while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
        Cptr = StripLine (Str);
        //
        // Don't print blank lines?
        //
        if (*Cptr) {
          fprintf (MakeFptr, "%s\n", Cptr);
        }
      }

      fprintf (MakeFptr, "\n");
    }
  }

  return STATUS_SUCCESS;
}

static
int
ProcessIncludesSection (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  )
/*++

Routine Description:

  Process the [includes.common], [includes.processor], and 
  [includes.processor.platform] section of the component description file 
  and write the appropriate macros to the component's makefile.

  Process in reverse order to allow overrides on platform basis.
    
Arguments:

  ComponentFile     - section info on the component file being processed
  MakeFptr          - file pointer to the component' makefile we're creating

Returns:

  Always STATUS_SUCCESS right now, since the sections are optional.
  
--*/
{
  INT8  *Cptr;
  INT8  Str[MAX_LINE_LEN];
  INT8  *Processor;
  INT8  *OverridePath;
  //
  // We use this a lot here, so get the value only once.
  //
  Processor = GetSymbolValue (PROCESSOR);
  //
  // If they're using an override source path, then add OverridePath and
  // OverridePath\$(PROCESSOR) to the list of include paths.
  //
  OverridePath = GetSymbolValue (SOURCE_OVERRIDE_PATH);
  if (OverridePath != NULL) {
    fprintf (MakeFptr, "INC = $(INC) -I %s\n", OverridePath);
    fprintf (MakeFptr, "INC = $(INC) -I %s\\%s \n", OverridePath, Processor);
  }
  //
  // Try for an [includes.processor.platform]
  //
  Cptr = GetSymbolValue (PLATFORM);
  if (Cptr != NULL) {
    sprintf (Str, "%s.%s.%s", INCLUDE_SECTION_NAME, Processor, Cptr);
    ProcessIncludesSectionSingle (ComponentFile, MakeFptr, Str);
  }
  //
  // Now the [includes.processor] section
  //
  sprintf (Str, "%s.%s", INCLUDE_SECTION_NAME, Processor);
  ProcessIncludesSectionSingle (ComponentFile, MakeFptr, Str);

  //
  // Now the [includes.common] section
  //
  sprintf (Str, "%s.%s", INCLUDE_SECTION_NAME, COMMON_SECTION_NAME);
  ProcessIncludesSectionSingle (ComponentFile, MakeFptr, Str);

  //
  // Done
  //
  return STATUS_SUCCESS;
}
//
// Process one of the [includes.xxx] sections to create a list of all
// the include paths.
//
static
int
ProcessIncludesSectionSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  )
{
  INT8    *Cptr;
  SECTION *TempSect;
  INT8    Str[MAX_LINE_LEN];
  INT8    *Processor;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    //
    // Add processor subdirectory on every include path
    //
    Processor = GetSymbolValue (PROCESSOR);
    //
    // Copy lines directly
    //
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      //
      // Don't process blank lines
      //
      if (*Cptr) {
        //
        // Strip off trailing slash
        //
        if (Cptr[strlen (Cptr) - 1] == '\\') {
          Cptr[strlen (Cptr) - 1] = 0;
        }
        //
        // Special case of ".". Replace it with source path
        // and the rest of the line (for .\$(PROCESSOR))
        //
        if (*Cptr == '.') {
          //
          // Handle case of just a "."
          //
          if (Cptr[1] == 0) {
            fprintf (MakeFptr, "INC = $(INC) -I $(SOURCE_DIR)\n");
            fprintf (
              MakeFptr,
              "INC = $(INC) -I $(SOURCE_DIR)\\%s \n",
              Processor
              );
          } else if (Cptr[1] == '.') {
            //
            // Handle case of "..\path\path\path"
            //
            fprintf (
              MakeFptr,
              "INC = $(INC) -I $(SOURCE_DIR)\\%s \n",
              Cptr
              );
            fprintf (
              MakeFptr,
              "INC = $(INC) -I $(SOURCE_DIR)\\%s\\%s \n",
              Cptr,
              Processor
              );

          } else {
            //
            // Handle case of ".\path\path\path"
            //
            fprintf (
              MakeFptr,
              "INC = $(INC) -I $(SOURCE_DIR)\\%s \n",
              Cptr + 1
              );
            fprintf (
              MakeFptr,
              "INC = $(INC) -I $(SOURCE_DIR)\\%s\\%s \n",
              Cptr + 1,
              Processor
              );
          }
        } else if ((Cptr[1] != ':') && isalpha (*Cptr)) {
          fprintf (MakeFptr, "INC = $(INC) -I $(EFI_SOURCE)\\%s \n", Cptr);
          fprintf (
            MakeFptr,
            "INC = $(INC) -I $(EFI_SOURCE)\\%s\\%s \n",
            Cptr,
            Processor
            );
        } else {
          //
          // The line is something like: $(EFI_SOURCE)\dxe\include. Add it to
          // the existing $(INC) definition. Add user includes before any
          // other existing paths.
          //
          fprintf (MakeFptr, "INC = $(INC) -I %s \n", Cptr);
          fprintf (MakeFptr, "INC = $(INC) -I %s\\%s \n", Cptr, Processor);
        }
      }
    }

    fprintf (MakeFptr, "\n");
  }

  return STATUS_SUCCESS;
}

static
int
ProcessSourceFiles (
  DSC_FILE  *DSCFile,
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  UINT32    Mode
  )
/*++

Routine Description:

  Process the [sources.common], [sources.$(PROCESSOR)], and 
  [sources.$(PROCESSOR).$(PLATFORM] sections of the component
  description file and write the appropriate build commands out to the 
  component's makefile. If $(SOURCE_SELECT) is defined, then it overrides
  the source selections. We use this functionality for SMM.
  
Arguments:

  ComponentFile     - section info on the component file being processed
  MakeFptr          - file pointer to the component' makefile we're creating
  DSCFile           - section info on the description file we're processing
  Mode              - to write build commands, or just create a list
                      of sources.

Returns:

  Always STATUS_SUCCESS right now, since the sections are optional.
  
--*/
{
  INT8  Str[MAX_LINE_LEN];
  INT8  *Processor;
  INT8  *Platform;
  INT8  *SourceSelect;
  INT8  *CStart;
  INT8  *CEnd;
  INT8  CSave;
  INT8  *CopySourceSelect;
  //
  // We use this a lot here, so get the value only once.
  //
  Processor = GetSymbolValue (PROCESSOR);
  //
  // See if they defined SOURCE_SELECT=xxx,yyy in which case we'll
  // select each [sources.xxx] and [sources.yyy] files and process
  // them.
  //
  SourceSelect = GetSymbolValue (SOURCE_SELECT);

  if (SourceSelect != NULL) {
    //
    // Make a copy of the string and break it up (comma-separated) and
    // select each [sources.*] file from the INF.
    //
    CopySourceSelect = (INT8 *) malloc (strlen (SourceSelect) + 1);
    if (CopySourceSelect == NULL) {
      Error (NULL, 0, 0, NULL, "failed to allocate memory");
      return STATUS_ERROR;
    }

    strcpy (CopySourceSelect, SourceSelect);
    CStart  = CopySourceSelect;
    CEnd    = CStart;
    while (*CStart) {
      CEnd = CStart + 1;
      while (*CEnd && *CEnd != ',') {
        CEnd++;
      }

      CSave = *CEnd;
      *CEnd = 0;
      sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, CStart);
      ProcessSourceFilesSection (DSCFile, ComponentFile, MakeFptr, Str, Mode);
      //
      // Restore the terminator and advance
      //
      *CEnd   = CSave;
      CStart  = CEnd;
      if (*CStart) {
        CStart++;
      }
    }

    free (CopySourceSelect);

  } else {
    //
    // Process all the [sources.common] source files to make them build
    //
    sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, COMMON_SECTION_NAME);
    ProcessSourceFilesSection (DSCFile, ComponentFile, MakeFptr, Str, Mode);
    //
    // Now process the [sources.$(PROCESSOR)] files.
    //
    sprintf (Str, "sources.%s", Processor);
    ProcessSourceFilesSection (DSCFile, ComponentFile, MakeFptr, Str, Mode);
    //
    // Now process the [sources.$(PROCESSOR).$(PLATFORM)] files.
    //
    Platform = GetSymbolValue (PLATFORM);
    if (Platform != NULL) {
      sprintf (Str, "sources.%s.%s", Processor, Platform);
      ProcessSourceFilesSection (DSCFile, ComponentFile, MakeFptr, Str, Mode);
    }
  }

  return STATUS_SUCCESS;
}

/*++

Routine Description:
  Given a source file line from an INF file, parse it to see if there are
  any defines on it. If so, then add them to the symbol table.
  Also, terminate the line after the file name.
  
Arguments:
  SourceFileLine - a line from a [sources.?] section of the INF file. Likely
  something like:
  
  MySourceFile.c   BUILT_NAME=$(BUILD_DIR)\MySourceFile.obj

Returns:
  Nothing.
  
--*/
static
void
AddFileSymbols (
  INT8    *SourceFileLine
  )
{
  int Len;
  //
  // Skip spaces
  //
  for (; *SourceFileLine && isspace (*SourceFileLine); SourceFileLine++)
    ;
  for (; *SourceFileLine && !isspace (*SourceFileLine); SourceFileLine++)
    ;
  if (*SourceFileLine) {
    *SourceFileLine = 0;
    SourceFileLine++;
    //
    // AddSymbol() will parse it for us, and return the length. Keep calling
    // it until it reports an error or is done.
    //
    do {
      Len = AddSymbol (SourceFileLine, NULL, SYM_FILE);
      SourceFileLine += Len;
    } while (Len > 0);
  }
}
//
// Process a single section of source files in the component INF file
//
static
int
ProcessSourceFilesSection (
  DSC_FILE  *DSCFile,
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName,
  UINT32    Mode
  )
{
  INT8    *Cptr;
  INT8    FileName[MAX_EXP_LINE_LEN];
  INT8    FilePath[MAX_PATH];
  INT8    TempFileName[MAX_PATH];
  INT8    OriginalFileName[MAX_PATH];
  SECTION *TempSect;
  INT8    Str[MAX_LINE_LEN];
  INT8    *Processor;
  INT8    *OverridePath;
  FILE    *FPtr;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    Processor = GetSymbolValue (PROCESSOR);
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      //
      // Don't process blank lines
      //
      if (*Cptr) {
        //
        // Expand macros in the filename, then parse the line for symbol
        // definitions. AddFileSymbols() will null-terminate the line
        // after the file name. Save a copy for override purposes, in which
        // case we'll need to know the file name and path (in case it's in
        // a subdirectory).
        //
        ExpandMacros (Cptr, FileName, sizeof (FileName), 0);
        AddFileSymbols (FileName);
        strcpy (OriginalFileName, FileName);
        //
        // Set the SOURCE_FILE_NAME symbol. What we have now is the name of
        // the file, relative to the location of the INF file. So prepend
        // $(SOURCE_DIR) to it first.
        //
        strcpy (TempFileName, "$(SOURCE_DIR)\\");
        strcat (TempFileName, FileName);
        AddSymbol (SOURCE_FILE_NAME, TempFileName, SYM_FILE | SYM_OVERWRITE);
        //
        // Extract path information from the source file and set internal
        // variable SOURCE_RELATIVE_PATH. Only do this if the path
        // contains a backslash.
        //
        strcpy (FilePath, FileName);
        for (Cptr = FilePath + strlen (FilePath) - 1; (Cptr > FilePath) && (*Cptr != '\\'); Cptr--)
          ;
        if (*Cptr == '\\') {
          *(Cptr + 1) = 0;
          AddSymbol (SOURCE_RELATIVE_PATH, FilePath, SYM_FILE);
        }
        //
        // Define another internal symbol for the name of the file without
        // the path and extension.
        //
        for (Cptr = FileName + strlen (FileName) - 1; (Cptr > FileName) && (*Cptr != '\\'); Cptr--)
          ;
        if (*Cptr == '\\') {
          Cptr++;
        }

        strcpy (FilePath, Cptr);
        //
        // We now have a file name with no path information. Before we do anything else,
        // see if OVERRIDE_PATH is set, and if so, see if file $(OVERRIDE_PATH)FileName
        // exists. If it does, then recursive call this function to use the override file
        // instead of the one from the INF file.
        //
        OverridePath = GetSymbolValue (SOURCE_OVERRIDE_PATH);
        if (OverridePath != NULL) {
          //
          // See if the file exists. If it does, reset the SOURCE_FILE_NAME symbol.
          //
          strcpy (TempFileName, OverridePath);
          strcat (TempFileName, "\\");
          strcat (TempFileName, OriginalFileName);
          if ((FPtr = fopen (TempFileName, "rb")) != NULL) {
            fclose (FPtr);
            AddSymbol (SOURCE_FILE_NAME, TempFileName, SYM_FILE | SYM_OVERWRITE);
            //
            // Print a message. This function is called to create build commands
            // for source files, and to create a macro of all source files. Therefore
            // do this check so we don't print the override message multiple times.
            //
            if (Mode & SOURCE_MODE_BUILD_COMMANDS) {
              fprintf (stdout, "Override: %s\n", TempFileName);
            }
          } else {
            //
            // Set override path to null to use as a flag below
            //
            OverridePath = NULL;
          }
        }

        for (Cptr = FilePath; *Cptr && (*Cptr != '.'); Cptr++)
          ;
        if (*Cptr == '.') {
          *Cptr = 0;
          AddSymbol (SOURCE_FILE_EXTENSION, Cptr + 1, SYM_FILE);
        }

        AddSymbol (SOURCE_BASE_NAME, FilePath, SYM_FILE);
        //
        // If we're just creating the SOURCE_FILES macro, then write the
        // file name out to the makefile.
        //
        if (Mode & SOURCE_MODE_SOURCE_FILES) {
          //
          // If we're processing an override file, then use the file name as-is
          //
          if (OverridePath != NULL) {
            //
            // SOURCE_FILES = $(SOURCE_FILES) c:\Path\ThisFile.c
            //
            fprintf (MakeFptr, "SOURCE_FILES = $(SOURCE_FILES) %s\n", TempFileName);
          } else {
            //
            // SOURCE_FILES = $(SOURCE_FILES) $(SOURCE_DIR)\ThisFile.c
            //
            fprintf (MakeFptr, "SOURCE_FILES = $(SOURCE_FILES) $(SOURCE_DIR)\\%s\n", FileName);
          }
        } else if (Mode & SOURCE_MODE_BUILD_COMMANDS) {
          //
          // Write the build commands for this file per the build commands
          // for this file type as defined in the description file.
          // Also create the directory for it in the build path.
          //
          WriteCompileCommands (DSCFile, MakeFptr, FileName, Processor);
          fprintf (MakeFptr, "\n");
          sprintf (Str, "%s\\%s", GetSymbolValue (DEST_DIR), FileName);
          MakeFilePath (Str);
        }
        //
        // Remove file-level symbols
        //
        RemoveFileSymbols ();
      }
    }
  }

  return STATUS_SUCCESS;
}
//
// Process the INF [sources.*] sections and emit the OBJECTS = .....
// lines to the component's makefile.
//
static
int
ProcessObjects (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  )
{
  INT8  Str[MAX_LINE_LEN];
  INT8  *Processor;
  INT8  *Platform;
  INT8  *SourceSelect;
  INT8  *CStart;
  INT8  *CEnd;
  INT8  CSave;
  INT8  *CopySourceSelect;

  //
  // Write a useful comment to the output makefile so the user knows where
  // the data came from.
  //
  fprintf (MakeFptr, "\n# Tool-generated list of object files that are created\n");
  fprintf (MakeFptr, "# from the list of source files in the [sources.*] sections\n");
  fprintf (MakeFptr, "# of the component INF file.\n#\n");
  //
  // We use this a lot here, so get the value only once.
  //
  Processor = GetSymbolValue (PROCESSOR);
  //
  // Now define the OBJECTS variable and assign it to be all the object files we're going
  // to create. Afterwards create a pseudo-target objects to let the user quickly just compile
  // the source files. This means we need to process all the common objects and
  // processor-specific objects again.
  //
  fprintf (MakeFptr, "OBJECTS = $(OBJECTS) ");
  //
  // See if they defined SOURCE_SELECT=xxx,yyy in which case well
  // select each [sources.xxx] and [sources.yyy] files and process
  // them.
  //
  SourceSelect = GetSymbolValue (SOURCE_SELECT);

  if (SourceSelect != NULL) {
    //
    // Make a copy of the string and break it up (comma-separated) and
    // select each [sources.*] file from the INF.
    //
    CopySourceSelect = (INT8 *) malloc (strlen (SourceSelect) + 1);
    if (CopySourceSelect == NULL) {
      Error (NULL, 0, 0, NULL, "failed to allocate memory");
      return STATUS_ERROR;
    }

    strcpy (CopySourceSelect, SourceSelect);
    CStart  = CopySourceSelect;
    CEnd    = CStart;
    while (*CStart) {
      CEnd = CStart + 1;
      while (*CEnd && *CEnd != ',') {
        CEnd++;
      }

      CSave = *CEnd;
      *CEnd = 0;
      sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, CStart);
      ProcessObjectsSingle (ComponentFile, MakeFptr, Str);
      //
      // Restore the terminator and advance
      //
      *CEnd   = CSave;
      CStart  = CEnd;
      if (*CStart) {
        CStart++;
      }
    }

    free (CopySourceSelect);
  } else {
    //
    // Now process all the [sources.common] files and emit build commands for them
    //
    sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, COMMON_SECTION_NAME);
    if (ProcessObjectsSingle (ComponentFile, MakeFptr, Str) != STATUS_SUCCESS) {
      Warning (GetSymbolValue (INF_FILENAME), 1, 0, NULL, "no [%s] section found in component description", Str);
    }
    //
    // Now process any processor-specific source files in [sources.$(PROCESSOR)]
    //
    sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, Processor);
    ProcessObjectsSingle (ComponentFile, MakeFptr, Str);

    //
    // Now process any [sources.$(PROCESSOR).$(PLATFORM)] files
    //
    Platform = GetSymbolValue (PLATFORM);
    if (Platform != NULL) {
      sprintf (Str, "sources.%s.%s", Processor, Platform);
      ProcessObjectsSingle (ComponentFile, MakeFptr, Str);
    }
  }

  fprintf (MakeFptr, "\n\n");
  return STATUS_SUCCESS;
}

static
INT8 *
BuiltFileExtension (
  INT8      *SourceFileName
  )
{
  int   i;
  INT8  *Cptr;
  //
  // Find the dot in the filename extension
  //
  for (Cptr = SourceFileName + strlen (SourceFileName) - 1;
       (Cptr > SourceFileName) && (*Cptr != '\\') && (*Cptr != '.');
       Cptr--
      ) {
    //
    // Do nothing
    //
  }

  if (*Cptr != '.') {
    return NULL;
  }
  //
  // Look through our list of known file types and return a pointer to
  // its built file extension.
  //
  for (i = 0; mFileTypes[i].Extension != NULL; i++) {
    if (stricmp (Cptr, mFileTypes[i].Extension) == 0) {
      return mFileTypes[i].BuiltExtension;
    }
  }

  return NULL;
}

int
ProcessObjectsSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  )
{
  INT8    *Cptr;
  INT8    *Cptr2;
  INT8    Str[MAX_LINE_LEN];
  INT8    FileName[MAX_EXP_LINE_LEN];
  SECTION *TempSect;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      //
      // Don't process blank lines
      //
      if (*Cptr) {
        //
        // Expand macros then create the output filename. We'll do a lookup
        // on the source file's extension to determine what the extension of
        // the built version of the file is. For example, .c -> .obj.
        //
        if (!IsIncludeFile (Cptr)) {
          ExpandMacros (Cptr, FileName, sizeof (FileName), 0);
          Cptr2 = BuiltFileExtension (FileName);
          if (Cptr2 != NULL) {
            SetFileExtension (FileName, Cptr2);
            if (!IsAbsolutePath (FileName)) {
              fprintf (MakeFptr, "\\\n          $(%s)\\%s   ", DEST_DIR, FileName);
            } else {
              fprintf (MakeFptr, "\\\n          %s   ", FileName);
            }
          }
        }
      }
    }
  } else {
    return STATUS_WARNING;
  }

  return STATUS_SUCCESS;
}
//
// Process all [libraries.*] sections in the component INF file to create a
// macro to the component's output makefile: LIBS = Lib1 Lib2, ...
//
static
int
ProcessLibs (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  )
{
  INT8  Str[MAX_LINE_LEN];
  INT8  *Processor;
  INT8  *Platform;

  //
  // Print a useful comment to the component's makefile so the user knows
  // where the data came from.
  //
  fprintf (MakeFptr, "# Tool-generated list of libraries that are generated\n");
  fprintf (MakeFptr, "# from the list of libraries listed in the [libraries.*] sections\n");
  fprintf (MakeFptr, "# of the component INF file.\n\n");

  Processor = GetSymbolValue (PROCESSOR);
  sprintf (Str, "%s.%s", LIBRARIES_SECTION_NAME, COMMON_SECTION_NAME);
  ProcessLibsSingle (ComponentFile, MakeFptr, Str);
  //
  // Process the [libraries.processor] libraries to define "LIBS = x.lib y.lib..."
  //
  sprintf (Str, "%s.%s", LIBRARIES_SECTION_NAME, Processor);
  ProcessLibsSingle (ComponentFile, MakeFptr, Str);
  //
  // Now process any [libraries.$(PROCESSOR).$(PLATFORM)] files
  //
  Platform = GetSymbolValue (PLATFORM);
  if (Platform != NULL) {
    sprintf (Str, "%s.%s.%s", LIBRARIES_SECTION_NAME, Processor, Platform);
    ProcessLibsSingle (ComponentFile, MakeFptr, Str);
  }
  //
  // Process any [libraries.platform] files
  //
  ProcessLibsSingle (ComponentFile, MakeFptr, LIBRARIES_PLATFORM_SECTION_NAME);

  return STATUS_SUCCESS;
}

static
int
ProcessLibsSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  )
{
  INT8    *Cptr;
  INT8    Str[MAX_LINE_LEN];
  SECTION *TempSect;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    fprintf (MakeFptr, "LIBS = $(LIBS) ");
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      //
      // Don't process blank lines
      //
      if (*Cptr) {
        if (Cptr[strlen (Cptr) - 4] != '.') {
          fprintf (MakeFptr, "    \\\n       $(LIB_DIR)\\%s.lib", Cptr);
        } else {
          fprintf (MakeFptr, "    \\\n       $(LIB_DIR)\\%s", Cptr);
        }
      }
    }

    fprintf (MakeFptr, "\n\n");
  }

  return STATUS_SUCCESS;
}

static
int
ProcessIncludeFiles (
  DSC_FILE *ComponentFile,
  FILE     *MakeFptr
  )
{
  INT8  Str[MAX_LINE_LEN];
  INT8  *Platform;
  INT8  *Processor;

  //
  // Print a useful comment to the output makefile so the user knows where
  // the info came from
  //
  fprintf (MakeFptr, "# Tool-generated include dependencies from any include files in the\n");
  fprintf (MakeFptr, "# [sources.*] sections of the component INF file\n\n");

  Processor = GetSymbolValue (PROCESSOR);
  //
  // Find all the include files in the [common.sources] and [common.$(PROCESSOR)]
  // sections.
  //
  sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, COMMON_SECTION_NAME);
  ProcessIncludeFilesSingle (ComponentFile, MakeFptr, Str);
  //
  // Now process the [sources.$(PROCESSOR)] files.
  //
  sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, Processor);
  ProcessIncludeFilesSingle (ComponentFile, MakeFptr, Str);
  //
  // Now process the [sources.$(PROCESSOR).$(PLATFORM)] files.
  //
  Platform = GetSymbolValue (PLATFORM);
  if (Platform != NULL) {
    sprintf (Str, "sources.%s.%s", Processor, Platform);
    ProcessIncludeFilesSingle (ComponentFile, MakeFptr, Str);
  }

  fprintf (MakeFptr, "\n");
  return STATUS_SUCCESS;
}

int
ProcessIncludeFilesSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  )
{
  INT8            *Cptr;
  INT8            FileName[MAX_EXP_LINE_LEN];
  INT8            TempFileName[MAX_PATH];
  SECTION         *TempSect;
  FILE_NAME_PARTS *File;
  INT8            Str[MAX_LINE_LEN];
  INT8            *OverridePath;
  FILE            *FPtr;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    //
    // See if the SOURCE_OVERRIDE_PATH has been set. If it has, and
    // they have an include file that is overridden, then add the path
    // to it to the list of include paths (prepend).
    //
    OverridePath = GetSymbolValue (SOURCE_OVERRIDE_PATH);
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      //
      // Don't process blank lines
      //
      if (*Cptr) {
        //
        // Expand macros in the filename, then get its parts
        //
        ExpandMacros (Cptr, FileName, sizeof (FileName), 0);
        AddFileSymbols (FileName);
        File = GetFileParts (FileName);
        if ((File != NULL) && IsIncludeFile (FileName)) {
          if (OverridePath != NULL) {
            strcpy (TempFileName, OverridePath);
            strcat (TempFileName, "\\");
            strcat (TempFileName, FileName);
            if ((FPtr = fopen (TempFileName, "rb")) != NULL) {
              fclose (FPtr);
              //
              // Null-terminate the file name at the last backslash and add that
              // to the beginning of the list of include paths.
              //
              for (Cptr = TempFileName + strlen (TempFileName) - 1;
                   (Cptr >= TempFileName) && (*Cptr != '\\') && (*Cptr != '/');
                   Cptr--
                  )
                ;
              if (Cptr >= TempFileName) {
                *Cptr = 0;
              }

              fprintf (MakeFptr, "INC = -I %s $(INC)\n", TempFileName);
            }
          }
          //
          // If absolute path already, don't prepend source directory
          //
          if (FileName[0] && (FileName[1] == ':')) {
            //
            // fprintf (MakeFptr, "INC_DEPS = $(INC_DEPS) %s\n", FileName);
            //
          } else {
            //
            // fprintf (MakeFptr, "INC_DEPS = $(INC_DEPS) $(SOURCE_DIR)\\%s\n", FileName);
            //
          }
        }

        FreeFileParts (File);
        RemoveFileSymbols ();
      }
    }
  }

  return STATUS_SUCCESS;
}

static
void
FreeFileParts (
  FILE_NAME_PARTS *FP
  )
{
  if (FP != NULL) {
    if (FP->Path != NULL) {
      free (FP->Path);
    }

    if (FP->BaseName != NULL) {
      free (FP->BaseName);
    }

    if (FP->Extension != NULL) {
      free (FP->Extension);
    }
  }
}

static
FILE_NAME_PARTS *
GetFileParts (
  INT8 *FileName
  )
{
  FILE_NAME_PARTS *FP;
  INT8            *Cptr;
  INT8            CopyFileName[MAX_PATH];
  INT8            *FileNamePtr;

  strcpy (CopyFileName, FileName);
  FP = (FILE_NAME_PARTS *) malloc (sizeof (FILE_NAME_PARTS));
  if (FP == NULL) {
    Error (NULL, 0, 0, NULL, "failed to allocate memory");
    return NULL;
  }

  memset ((INT8 *) FP, 0, sizeof (FILE_NAME_PARTS));
  //
  // Get extension code
  //
  FP->ExtensionCode = GetSourceFileType (CopyFileName);
  //
  // Get drive if there
  //
  FileNamePtr = CopyFileName;
  if (FileNamePtr[1] == ':') {
    FP->Drive[0]  = FileNamePtr[0];
    FP->Drive[1]  = ':';
    FileNamePtr += 2;
  }
  //
  // Start at the end and work back
  //
  for (Cptr = FileNamePtr + strlen (FileNamePtr) - 1; (Cptr > FileNamePtr) && (*Cptr != '.'); Cptr--)
    ;

  if (*Cptr == '.') {
    //
    // Don't copy the dot
    //
    FP->Extension = (char *) malloc (strlen (Cptr));
    strcpy (FP->Extension, Cptr + 1);
    *Cptr = 0;
    Cptr--;
    StripTrailingSpaces (FP->Extension);
  } else {
    //
    // Create empty string for extension
    //
    FP->Extension     = (char *) malloc (1);
    FP->Extension[0]  = 0;
  }
  //
  // Now back up and get the base name
  //
  for (; (Cptr > FileNamePtr) && (*Cptr != '\\') && (*Cptr != '/'); Cptr--)
    ;
  FP->BaseName = (char *) malloc (strlen (Cptr) + 1);
  strcpy (FP->BaseName, Cptr);
  *Cptr = 0;
  Cptr--;
  //
  // Rest is path
  //
  if (Cptr >= FileNamePtr) {
    Cptr      = FileNamePtr;
    FP->Path  = (char *) malloc (strlen (Cptr) + 1);
    strcpy (FP->Path, Cptr);
  } else {
    FP->Path    = (char *) malloc (1);
    FP->Path[0] = 0;
  }

  return FP;
}

/*****************************************************************************
******************************************************************************/
static
int
WriteCommonMakefile (
  DSC_FILE  *DSCFile,
  FILE      *MakeFptr,
  INT8      *Processor
  )
{
  INT8    InLine[MAX_LINE_LEN];
  INT8    OutLine[MAX_LINE_LEN * 2];
  SECTION *Sect;
  INT8    *Sym;
  int     i;
  //
  // Don't mess up the original file pointer, since we're processing it at a higher
  // level.
  //
  DSCFileSavePosition (DSCFile);
  //
  // Write the header to the file
  //
  for (i = 0; MakefileHeader[i] != NULL; i++) {
    fprintf (MakeFptr, "%s\n", MakefileHeader[i]);
  }

  fprintf (MakeFptr, "# Hard-coded defines output by the tool\n");
  //
  // First write the basics to the component's makefile. These includes
  // EFI_SOURCE, BIN_DIR, OUT_DIR, LIB_DIR, SOURCE_DIR, DEST_DIR.
  //
  Sym = GetSymbolValue (EFI_SOURCE);
  fprintf (MakeFptr, "%s       = %s\n", EFI_SOURCE, Sym);
  Sym = GetSymbolValue (BUILD_DIR);
  fprintf (MakeFptr, "%s        = %s\n", BUILD_DIR, Sym);
  Sym = GetSymbolValue (BIN_DIR);
  fprintf (MakeFptr, "%s          = %s\n", BIN_DIR, Sym);
  Sym = GetSymbolValue (OUT_DIR);
  fprintf (MakeFptr, "%s          = %s\n", OUT_DIR, Sym);
  Sym = GetSymbolValue (LIB_DIR);
  fprintf (MakeFptr, "%s          = %s\n", LIB_DIR, Sym);
  Sym = GetSymbolValue (SOURCE_DIR);
  fprintf (MakeFptr, "%s       = %s\n", SOURCE_DIR, Sym);
  Sym = GetSymbolValue (DEST_DIR);
  fprintf (MakeFptr, "%s         = %s\n", DEST_DIR, Sym);
  fprintf (MakeFptr, "\n");
  //
  // If there was a [makefile.common] section in the description file,
  // copy it (after macro expansion) to the output file.
  //
  sprintf (InLine, "%s.%s", MAKEFILE_SECTION_NAME, COMMON_SECTION_NAME);
  Sect = DSCFileFindSection (DSCFile, InLine);
  if (Sect != NULL) {
    //
    // fprintf (MakeFptr, "# From the [makefile.common] section of the DSC file\n");
    // Read lines, expand, then dump out
    //
    while (DSCFileGetLine (DSCFile, InLine, sizeof (InLine)) != NULL) {
      //
      // Replace macros
      //
      ExpandMacrosRecursive (InLine, OutLine, sizeof (OutLine), EXPANDMODE_RECURSIVE);
      fprintf (MakeFptr, OutLine);
    }
  }
  //
  // If there was a [makefile.platform] section in the description file,
  // copy it (after macro expansion) to the output file.
  //
  sprintf (InLine, "%s.%s", MAKEFILE_SECTION_NAME, "Platform");
  Sect = DSCFileFindSection (DSCFile, InLine);
  if (Sect != NULL) {
    //
    // Read lines, expand, then dump out
    //
    while (DSCFileGetLine (DSCFile, InLine, sizeof (InLine)) != NULL) {
      //
      // Replace macros
      //
      ExpandMacrosRecursive (InLine, OutLine, sizeof (OutLine), EXPANDMODE_RECURSIVE);
      fprintf (MakeFptr, OutLine);
    }
  }
  //
  // Do the same for any [makefile.$(PROCESSOR)]
  //
  sprintf (InLine, "%s.%s", MAKEFILE_SECTION_NAME, Processor);
  Sect = DSCFileFindSection (DSCFile, InLine);
  if (Sect != NULL) {
    //
    // Read lines, expand, then dump out
    //
    while (DSCFileGetLine (DSCFile, InLine, sizeof (InLine)) != NULL) {
      ExpandMacros (InLine, OutLine, sizeof (OutLine), 0);
      fprintf (MakeFptr, OutLine);
    }
  }
  //
  // Same thing for [makefile.$(PROCESSOR).$(PLATFORM)]
  //
  Sym = GetSymbolValue (PROCESSOR);
  if (Sym != NULL) {
    sprintf (InLine, "%s.%s.%s", MAKEFILE_SECTION_NAME, Processor, Sym);
    Sect = DSCFileFindSection (DSCFile, InLine);
    if (Sect != NULL) {
      //
      // Read lines, expand, then dump out
      //
      while (DSCFileGetLine (DSCFile, InLine, sizeof (InLine)) != NULL) {
        ExpandMacros (InLine, OutLine, sizeof (OutLine), 0);
        fprintf (MakeFptr, OutLine);
      }
    }
  }

  DSCFileRestorePosition (DSCFile);
  return 0;
}

static
int
WriteComponentTypeBuildCommands (
  DSC_FILE *DSCFile,
  FILE     *MakeFptr,
  INT8     *SectionName
  )
/*++

Routine Description:
  
   Given a section name such as [build.ia32.library], find the section in
   the description file and copy the build commands.

Arguments:

  DSCFile     - section information on the main description file
  MakeFptr    - file pointer to the makefile we're writing to
  SectionName - name of the section we're to copy out to the makefile.

Returns:

  Always successful, since the section may be optional.

--*/
{
  SECTION *Sect;
  INT8    InLine[MAX_LINE_LEN];

  //
  // Don't mess up the original file pointer, since we're processing it at a higher
  // level.
  //
  DSCFileSavePosition (DSCFile);
  Sect = DSCFileFindSection (DSCFile, SectionName);
  if (Sect != NULL) {
    //
    // Read lines, expand, then dump out
    //
    while (DSCFileGetLine (DSCFile, InLine, sizeof (InLine)) != NULL) {
      //
      // ExpandMacros(InLine, OutLine, sizeof(OutLine), 0);
      //
      fprintf (MakeFptr, InLine);
    }
  } else {
    Warning (
      NULL,
      0,
      0,
      GetSymbolValue (INF_FILENAME),
      "no [%s] build commands found in DSC file for component",
      SectionName
      );
  }

  DSCFileRestorePosition (DSCFile);
  return STATUS_SUCCESS;
}

/*****************************************************************************

******************************************************************************/
static
int
WriteCompileCommands (
  DSC_FILE  *DscFile,
  FILE      *MakeFptr,
  INT8      *FileName,
  INT8      *Processor
  )
{
  FILE_NAME_PARTS *File;
  SECTION         *Sect;
  //
  // format: [build.ia32.c]
  //
  INT8            BuildSectionName[40];
  INT8            InLine[MAX_LINE_LEN];
  INT8            OutLine[MAX_LINE_LEN * 2];
  INT8            *SourceCompileType;
  char            *CPtr;
  char            *CPtr2;
  //
  // Determine the filename, then chop it up into its parts
  //
  File = GetFileParts (FileName);
  if (File != NULL) {
    //
    // Don't mess up the original file pointer, since we're processing it at a higher
    // level.
    //
    DSCFileSavePosition (DscFile);
    //
    // Option 1: SOURCE_COMPILE_TYPE=MyCompileSection
    //           Find a section of that name from which to get the build/compile
    //           commands for this source file. Look for both
    //           [build.$(PROCESSOR).$(SOURCE_COMPILE_TYPE] and
    //           [compile.$(PROCESSOR).$(SOURCE_COMPILE_TYPE]
    // Option 2: COMPILE_SELECT=.c=MyCCompile,.asm=MyAsm
    //           Find a [compile.$(PROCESSOR).MyCompile] section from which to
    //           get the compile commands for this source file. Look for both
    //           [build.$(PROCESSOR).MyCompile] and
    //           [compile.$(PROCESSOR).MyCompile]
    // Option 3: Look for standard section types to compile the file by extension.
    // Look for both [build.$(PROCESSOR).<extension>] and
    //               [compile.$(PROCESSOR).<extension>]
    //
    Sect = NULL;
    //
    // Option 1 - use SOURCE_COMPILE_TYPE variable
    //
    SourceCompileType = GetSymbolValue (SOURCE_COMPILE_TYPE);
    if (SourceCompileType != NULL) {
      sprintf (BuildSectionName, "compile.%s.%s", Processor, SourceCompileType);
      Sect = DSCFileFindSection (DscFile, BuildSectionName);
      if (Sect == NULL) {
        sprintf (BuildSectionName, "build.%s.%s", Processor, SourceCompileType);
        Sect = DSCFileFindSection (DscFile, BuildSectionName);
      }
    }
    //
    // Option 2 - use COMPILE_SELECT variable
    //
    if (Sect == NULL) {
      SourceCompileType = GetSymbolValue (COMPILE_SELECT);
      if (SourceCompileType != NULL) {
        //
        // Parse the variable, which looks like COMPILE_SELECT=.c=MyCCompiler;.asm=MyAsm;
        // to find an entry with a matching file name extension. If you find one,
        // then use that name to find the section name.
        //
        CPtr = SourceCompileType;
        while (*CPtr && (Sect == NULL)) {
          //
          // See if we found a match with this source file name extension. File->Extension
          // does not include the dot, so skip the dot in the COMPILE_SELECT variable if there
          // is one.
          //
          if (*CPtr == '.') {
            CPtr++;
          }

          if (strnicmp (CPtr, File->Extension, strlen (File->Extension)) == 0) {
            //
            // Found a file name extension match -- extract the name from the variable, for
            // example "MyCCompiler"
            //
            while (*CPtr && (*CPtr != '=')) {
              CPtr++;
            }

            if ((*CPtr != '=') || (CPtr[1] == 0)) {
              Error (NULL, 0, 0, SourceCompileType, "malformed COMPILE_SELECT variable");
              break;
            }

            CPtr++;
            sprintf (BuildSectionName, "compile.%s.", Processor);
            for (CPtr2 = BuildSectionName + strlen (BuildSectionName);
                 *CPtr && (*CPtr != ',') && (*CPtr != ';');
                 CPtr++
                ) {
              *CPtr2 = *CPtr;
              CPtr2++;
            }

            *CPtr2  = 0;
            Sect    = DSCFileFindSection (DscFile, BuildSectionName);
            if (Sect == NULL) {
              ParserError (
                0,
                BuildSectionName,
                "could not find section in DSC file - selected by COMPILE_SELECT variable"
                );
            }
          }

          //
          // Skip to next file name extension in the COMPILE_SELECT variable
          //
          while (*CPtr && (*CPtr != ';') && (*CPtr != ',')) {
            CPtr++;
          }

          if (*CPtr) {
            CPtr++;
          }
        }
      }
    }
    //
    // Option 3 - use "Build|Compile.$(PROCESSOR).<Extension>" section
    //
    if (Sect == NULL) {
      sprintf (BuildSectionName, "compile.%s.%s", Processor, File->Extension);
      Sect = DSCFileFindSection (DscFile, BuildSectionName);
      if (Sect == NULL) {
        sprintf (BuildSectionName, "build.%s.%s", Processor, File->Extension);
        Sect = DSCFileFindSection (DscFile, BuildSectionName);
      }
    }
    //
    // Should have found something by now unless it's an include (.h) file
    //
    if (Sect != NULL) {
      //
      // Temporarily add a FILE variable to the global symbol table. Omit the
      // extension.
      //
      sprintf (InLine, "%s%s%s", File->Drive, File->Path, File->BaseName);
      AddSymbol ("FILE", InLine, SYM_OVERWRITE | SYM_LOCAL | SYM_FILENAME);
      //
      // Read lines, expand (except SOURCE_DIR and DEST_DIR), then dump out
      //
      while (DSCFileGetLine (DscFile, InLine, sizeof (InLine)) != NULL) {
        ExpandMacros (
          InLine,
          OutLine,
          sizeof (OutLine),
          EXPANDMODE_NO_DESTDIR | EXPANDMODE_NO_SOURCEDIR
          );
        fprintf (MakeFptr, OutLine);
      }
    } else {
      //
      // Be nice and ignore include files
      //
      if (!IsIncludeFile (FileName)) {
        Error (
          NULL,
          0,
          0,
          NULL,
          "no build commands section [%s] found in DSC file for %s.%s",
          BuildSectionName,
          File->BaseName,
          File->Extension
          );
      }
    }

    DSCFileRestorePosition (DscFile);
    FreeFileParts (File);
  }

  return STATUS_SUCCESS;
}

/*****************************************************************************
******************************************************************************/
static
int
SetFileExtension (
  INT8 *FileName,
  INT8 *Extension
  )
{
  INT8  *Cptr;

  Cptr = FileName + strlen (FileName) - 1;
  while ((Cptr > FileName) && (*Cptr != '.')) {
    Cptr--;

  }
  //
  // Better be a dot
  //
  if (*Cptr != '.') {
    Message (2, "Missing filename extension: %s", FileName);
    return STATUS_WARNING;
  }

  Cptr++;
  if (*Extension == '.') {
    Extension++;
  }

  strcpy (Cptr, Extension);
  return STATUS_SUCCESS;
}

/*****************************************************************************
******************************************************************************/
int
MakeFilePath (
  INT8 *FileName
  )
{
  INT8  *Cptr;
  INT8  SavedChar;
  INT8  BuildDir[MAX_PATH];
  INT8  CopyFileName[MAX_PATH];

  //
  // Expand macros in the filename
  //
  if (ExpandMacros (FileName, CopyFileName, sizeof (CopyFileName), EXPANDMODE_NO_UNDEFS)) {
    Error (NULL, 0, 0, NULL, "undefined macros in file path: %s", FileName);
    return STATUS_ERROR;
  }
  //
  // Copy it back
  //
  strcpy (FileName, CopyFileName);
  //
  // To avoid creating $(BUILD_DIR) path, see if this path is the same as
  // $(BUILD_DIR), and if it is, see if build dir exists and skip over that
  // portion if it does
  //
  Cptr = GetSymbolValue (BUILD_DIR);
  if (Cptr != NULL) {
    if (strnicmp (Cptr, FileName, strlen (Cptr)) == 0) {
      //
      // BUILD_DIR path. See if it exists
      //
      strcpy (BuildDir, FileName);
      BuildDir[strlen (Cptr)] = 0;
      if ((_mkdir (BuildDir) != 0) && (errno != EEXIST)) {
        Cptr = FileName;
      } else {
        //
        // Already done. Shortcut. Skip to next path so that we don't create
        // the BUILD_DIR as well.
        //
        Cptr = FileName + strlen (Cptr);
        if (*Cptr == '\\') {
          Cptr++;
        }
      }
    } else {
      //
      // Not build dir
      //
      Cptr = FileName;
    }
  } else {
    Cptr = FileName;
  }
  //
  // Create directories until done. Skip over "c:\" in the path if it exists
  //
  if (*Cptr && (*(Cptr + 1) == ':') && (*(Cptr + 2) == '\\')) {
    Cptr += 3;
  }

  for (;;) {
    for (; *Cptr && (*Cptr != '/') && (*Cptr != '\\'); Cptr++)
      ;
    if (*Cptr) {
      SavedChar = *Cptr;
      *Cptr     = 0;
      if ((_mkdir (FileName) != 0)) {
        //
        //        Error (NULL, 0, 0, FileName, "failed to create directory");
        //        return 1;
        //
      }

      *Cptr = SavedChar;
      Cptr++;
    } else {
      break;
    }
  }

  return STATUS_SUCCESS;
}

/*****************************************************************************
******************************************************************************/
int
ExpandMacros (
  INT8 *SourceLine,
  INT8 *DestLine,
  int  LineLen,
  int  ExpandMode
  )
{
  INT8  *FromPtr;
  INT8  *ToPtr;
  INT8  *SaveStart;
  INT8  *Cptr;
  INT8  *value;
  int   Expanded;

  FromPtr = SourceLine;
  ToPtr   = DestLine;
  while (*FromPtr && (LineLen > 0)) {
    if ((*FromPtr == '$') && (*(FromPtr + 1) == '(')) {
      //
      // Save the start in case it's undefined, in which case we copy it as-is.
      //
      SaveStart = FromPtr;
      Expanded  = 0;
      //
      // Macro expansion time. Find the end (no spaces allowed)
      //
      FromPtr += 2;
      for (Cptr = FromPtr; *Cptr && (*Cptr != ')'); Cptr++)
        ;
      if (*Cptr) {
        //
        // Truncate the string at the closing parenthesis for ease-of-use.
        // Then copy the string directly to the destination line in case we don't find
        // a definition for it.
        //
        *Cptr = 0;
        strcpy (ToPtr, SaveStart);
        if ((stricmp (SOURCE_DIR, FromPtr) == 0) && (ExpandMode & EXPANDMODE_NO_SOURCEDIR)) {
          //
          // excluded this expansion
          //
        } else if ((stricmp (DEST_DIR, FromPtr) == 0) && (ExpandMode & EXPANDMODE_NO_DESTDIR)) {
          //
          // excluded this expansion
          //
        } else if ((value = GetSymbolValue (FromPtr)) != NULL) {
          strcpy (ToPtr, value);
          LineLen -= strlen (value);
          ToPtr += strlen (value);
          Expanded = 1;
        } else if (ExpandMode & EXPANDMODE_NO_UNDEFS) {
          Error (NULL, 0, 0, Cptr, "undefined macro on line: %s", SourceLine);
          return 1;
        }

        if (!Expanded) {
          //
          // Restore closing parenthesis, and advance to next character
          //
          *Cptr   = ')';
          FromPtr = SaveStart + 1;
          ToPtr++;
        } else {
          FromPtr = Cptr + 1;
        }
      } else {
        ParserError (0, SourceLine, "missing closing parenthesis on macro");
        return STATUS_WARNING;
      }
    } else {
      *ToPtr = *FromPtr;
      FromPtr++;
      ToPtr++;
      LineLen--;
    }
  }

  if (*FromPtr == 0) {
    *ToPtr = 0;
  }

  return STATUS_SUCCESS;
}

int
ExpandMacrosRecursive (
  INT8  *SourceLine,
  INT8  *DestLine,
  int   LineLen,
  int   ExpandMode
  )
{
  static int  NestDepth = 0;
  INT8        *FromPtr;
  INT8        *ToPtr;
  INT8        *SaveStart;
  INT8        *Cptr;
  INT8        *value;
  int         Expanded;
  int         ExpandedCount;
  INT8        *LocalDestLine;
  STATUS      Status;
  int         LocalLineLen;

  NestDepth++;
  Status        = STATUS_SUCCESS;
  LocalDestLine = (INT8 *) malloc (LineLen);
  if (LocalDestLine == NULL) {
    Error (__FILE__, __LINE__, 0, "application error", "memory allocation failed");
    return STATUS_ERROR;
  }

  FromPtr = SourceLine;
  ToPtr   = LocalDestLine;
  //
  // Walk the entire line, replacing $(MACRO_NAME).
  //
  LocalLineLen  = LineLen;
  ExpandedCount = 0;
  while (*FromPtr && (LocalLineLen > 0)) {
    if ((*FromPtr == '$') && (*(FromPtr + 1) == '(')) {
      //
      // Save the start in case it's undefined, in which case we copy it as-is.
      //
      SaveStart = FromPtr;
      Expanded  = 0;
      //
      // Macro expansion time. Find the end (no spaces allowed)
      //
      FromPtr += 2;
      for (Cptr = FromPtr; *Cptr && (*Cptr != ')'); Cptr++)
        ;
      if (*Cptr) {
        //
        // Truncate the string at the closing parenthesis for ease-of-use.
        // Then copy the string directly to the destination line in case we don't find
        // a definition for it.
        //
        *Cptr = 0;
        strcpy (ToPtr, SaveStart);
        if ((stricmp (SOURCE_DIR, FromPtr) == 0) && (ExpandMode & EXPANDMODE_NO_SOURCEDIR)) {
          //
          // excluded this expansion
          //
        } else if ((stricmp (DEST_DIR, FromPtr) == 0) && (ExpandMode & EXPANDMODE_NO_DESTDIR)) {
          //
          // excluded this expansion
          //
        } else if ((value = GetSymbolValue (FromPtr)) != NULL) {
          strcpy (ToPtr, value);
          LocalLineLen -= strlen (value);
          ToPtr += strlen (value);
          Expanded = 1;
          ExpandedCount++;
        } else if (ExpandMode & EXPANDMODE_NO_UNDEFS) {
          Error (NULL, 0, 0, Cptr, "undefined macro on line: %s", SourceLine);
          strcpy (ToPtr, FromPtr);
          Status = STATUS_WARNING;
          goto Done;
        }

        if (!Expanded) {
          //
          // Restore closing parenthesis, and advance to next character
          //
          *Cptr   = ')';
          FromPtr = SaveStart + 1;
          ToPtr++;
        } else {
          FromPtr = Cptr + 1;
        }
      } else {
        Error (NULL, 0, 0, SourceLine, "missing closing parenthesis on macro");
        strcpy (ToPtr, FromPtr);
        Status = STATUS_WARNING;
        goto Done;
      }
    } else {
      *ToPtr = *FromPtr;
      FromPtr++;
      ToPtr++;
      LocalLineLen--;
    }
  }

  if (*FromPtr == 0) {
    *ToPtr = 0;
  }

  //
  // If we're in recursive mode, and we expanded at least one string successfully,
  // then make a recursive call to try again.
  //
  if ((ExpandedCount != 0) && (Status == STATUS_SUCCESS) && (ExpandMode & EXPANDMODE_RECURSIVE) && (NestDepth < 10)) {
    Status = ExpandMacros (LocalDestLine, DestLine, LineLen, ExpandMode);
    free (LocalDestLine);
    NestDepth = 0;
    return Status;
  }

Done:
  if (Status != STATUS_ERROR) {
    strcpy (DestLine, LocalDestLine);
  }

  NestDepth = 0;
  free (LocalDestLine);
  return Status;
}

INT8 *
GetSymbolValue (
  INT8 *SymbolName
  )
/*++

Routine Description:
  
  Look up a symbol in our symbol table.

Arguments:

  SymbolName - The name of symbol.

Returns:

  Pointer to the value of the symbol if found
  NULL if the symbol is not found

--*/
{
  SYMBOL  *Symbol;

  //
  // Scan once for file-level symbols
  //
  Symbol = gGlobals.Symbol;
  while (Symbol) {
    if ((stricmp (SymbolName, Symbol->Name) == 0) && (Symbol->Type & SYM_FILE)) {
      return Symbol->Value;
    }

    Symbol = Symbol->Next;
  }
  //
  // Scan once for local symbols
  //
  Symbol = gGlobals.Symbol;
  while (Symbol) {
    if ((stricmp (SymbolName, Symbol->Name) == 0) && (Symbol->Type & SYM_LOCAL)) {
      return Symbol->Value;
    }

    Symbol = Symbol->Next;
  }
  //
  // No local value found. Scan for globals.
  //
  Symbol = gGlobals.Symbol;
  while (Symbol) {
    if ((stricmp (SymbolName, Symbol->Name) == 0) && (Symbol->Type & SYM_GLOBAL)) {
      return Symbol->Value;
    }

    Symbol = Symbol->Next;
  }
  //
  // For backwards-compatibility, if it's "GUID", return FILE_GUID value
  //
  if (stricmp (SymbolName, GUID) == 0) {
    return GetSymbolValue (FILE_GUID);
  }

  return NULL;
}

static
int
RemoveLocalSymbols (
  VOID
  )
/*++

Routine Description:
  
  Remove all local symbols from the symbol table. Local symbols are those
  that are defined typically by the component's INF file.

Arguments:

  None.

Returns:

  Right now, never fails.

--*/
{
  SYMBOL  *Sym;
  int     FoundOne;

  do {
    FoundOne  = 0;
    Sym       = gGlobals.Symbol;
    while (Sym) {
      if (Sym->Type & SYM_LOCAL) {
        //
        // Going to delete it out from under ourselves, so break and restart
        //
        FoundOne = 1;
        RemoveSymbol (Sym->Name, SYM_LOCAL);
        break;
      }

      Sym = Sym->Next;
    }
  } while (FoundOne);
  return STATUS_SUCCESS;
}

static
int
RemoveFileSymbols (
  VOID
  )
/*++

Routine Description:
  
  Remove all file-level symbols from the symbol table. File-level symbols are 
  those that are defined on a source file line in an INF file.

Arguments:

  None.

Returns:

  Right now, never fails.

--*/
{
  SYMBOL  *Sym;
  int     FoundOne;

  do {
    FoundOne  = 0;
    Sym       = gGlobals.Symbol;
    while (Sym) {
      if (Sym->Type & SYM_FILE) {
        //
        // Going to delete it out from under ourselves, so break and restart
        //
        FoundOne = 1;
        RemoveSymbol (Sym->Name, SYM_FILE);
        break;
      }

      Sym = Sym->Next;
    }
  } while (FoundOne);
  return STATUS_SUCCESS;
}

static
STATUS
ParseGuidDatabaseFile (
  INT8 *FileName
  )
/*++

Routine Description:
  This function parses a GUID-to-basename text file (perhaps output by
  the GuidChk utility) to define additional symbols. The format of the 
  file should be:

  7BB28B99-61BB-11D5-9A5D-0090273FC14D EFI_DEFAULT_BMP_LOGO_GUID gEfiDefaultBmpLogoGuid
  
  This function parses the line and defines global symbol:

    EFI_DEFAULT_BMP_LOGO_GUID=7BB28B99-61BB-11D5-9A5D-0090273FC14D 
  
  This symbol (rather than the actual GUID) can then be used in INF files to 
  fix duplicate GUIDs

Arguments:
  FileName  - the name of the file to parse.

Returns:
  STATUS_ERROR    - could not open FileName
  STATUS_SUCCESS  - we opened the file

--*/
{
  FILE  *Fptr;
  INT8  Line[100];
  INT8  Guid[100];
  INT8  DefineName[80];

  Fptr = fopen (FileName, "r");
  if (Fptr == NULL) {
    Error (NULL, 0, 0, FileName, "failed to open input GUID database input file");
    return STATUS_ERROR;
  }

  while (fgets (Line, sizeof (Line), Fptr) != NULL) {
    //
    // Get the GUID string, skip the defined name (EFI_XXX_GUID), and get the
    // variable name (gWhateverProtocolGuid)
    //
    if (sscanf (Line, "%s %s %*s", Guid, DefineName) == 2) {
      AddSymbol (DefineName, Guid, SYM_GLOBAL);
    }
  }

  fclose (Fptr);
  return STATUS_SUCCESS;
}

/*****************************************************************************

  Returns:
     0 if successful standard add
    length of the parsed string if passed in " name = value  "
    < 0 on error

******************************************************************************/
int
AddSymbol (
  INT8    *Name,
  INT8    *Value,
  int     Mode
  )
{
  SYMBOL  *Symbol;
  SYMBOL  *NewSymbol;
  int     Len;
  INT8    *Start;
  INT8    *Cptr;
  INT8    CSave;
  INT8    *SaveCptr;
  INT8    ShortName[MAX_PATH];

  Len           = 0;
  SaveCptr      = NULL;
  CSave         = 0;

  ShortName[0]  = 0;
  //
  // Mode better be local or global symbol
  //
  if ((Mode & (SYM_LOCAL | SYM_GLOBAL | SYM_FILE)) == 0) {
    Error (NULL, 0, 0, "APP ERROR", "adding symbol '%s' that is not local, global, nor file level", Name);
    return -1;
  }
  //
  // If value pointer is null, then they passed us a line something like:
  //    varname = value, or simply var =
  //
  if (Value == NULL) {
    Start = Name;
    while (*Name && isspace (*Name)) {
      Name++;

    }

    if (!*Name) {
      return -1;
    }
    //
    // Find the end of the name. Either space or a '='.
    //
    for (Value = Name; *Value && !isspace (*Value) && (*Value != '='); Value++)
      ;
    if (!*Value) {
      return -1;
    }
    //
    // Look for the '='
    //
    Cptr = Value;
    while (*Value && (*Value != '=')) {
      Value++;
    }

    if (!*Value) {
      return -1;
    }
    //
    // Now truncate the name
    //
    *Cptr = 0;
    //
    // Skip over the = and then any spaces
    //
    Value++;
    while (*Value && isspace (*Value)) {
      Value++;

    }
    //
    // Find end of string, checking for quoted string
    //
    if (*Value == '\"') {
      Value++;
      for (Cptr = Value; *Cptr && *Cptr != '\"'; Cptr++)
        ;
    } else {
      for (Cptr = Value; *Cptr && !isspace (*Cptr); Cptr++)
        ;
    }
    //
    // Null terminate the value string
    //
    CSave     = *Cptr;
    SaveCptr  = Cptr;
    *Cptr     = 0;
    Len       = (int) (Cptr - Start);
  }

  //
  // If file name or file path, and we're shortening, then print it
  //
  if ((Mode & (SYM_FILEPATH | SYM_FILENAME)) && (GetSymbolValue (SHORT_NAMES) != NULL)) {
    if (GetShortPathName (Value, ShortName, sizeof (ShortName)) > 0) {
      //
      // fprintf (stdout, "String value '%s' shortened to '%s'\n",
      //    Value, ShortName);
      //
      Value = ShortName;
    } else {
      //
      // fprintf (stdout, "WARNING: Failed to get short name for %s\n", Value);
      //
    }
  }
  //
  // We now have a symbol name and a value. Look for an existing variable of
  // the same type (global or local) and overwrite it.
  //
  Symbol = gGlobals.Symbol;
  while (Symbol) {
    //
    // Check for symbol name match
    //
    if (stricmp (Name, Symbol->Name) == 0) {
      //
      // See if this symbol is of the same type (global or local) as what
      // they're requesting
      //
      if ((Symbol->Type & (SYM_LOCAL | SYM_GLOBAL)) == (Mode & (SYM_LOCAL | SYM_GLOBAL))) {
        //
        // Did they say we could overwrite it?
        //
        if (Mode & SYM_OVERWRITE) {
          free (Symbol->Value);
          Symbol->Value = (INT8 *) malloc (strlen (Value) + 1);
          if (Symbol->Value == NULL) {
            Error (NULL, 0, 0, NULL, "failed to allocate memory");
            return -1;
          }

          strcpy (Symbol->Value, Value);
          //
          // If value == "NULL", then make it a 0-length string
          //
          if (stricmp (Symbol->Value, "NULL") == 0) {
            Symbol->Value[0] = 0;
          }

          return Len;
        } else {
          return STATUS_ERROR;
        }
      }
    }

    Symbol = Symbol->Next;
  }
  //
  // Does not exist, create a new one
  //
  NewSymbol = (SYMBOL *) malloc (sizeof (SYMBOL));
  if (NewSymbol == NULL) {
    Error (NULL, 0, 0, NULL, "failed to allocate memory");
    return -1;
  }

  memset ((INT8 *) NewSymbol, 0, sizeof (SYMBOL));
  NewSymbol->Name   = (INT8 *) malloc (strlen (Name) + 1);
  NewSymbol->Value  = (INT8 *) malloc (strlen (Value) + 1);
  //
  // Simply use the mode bits as the type.
  //
  NewSymbol->Type = Mode;
  if ((NewSymbol->Name == NULL) || (NewSymbol->Value == NULL)) {
    Error (NULL, 0, 0, NULL, "failed to allocate memory");
    return -1;
  }

  strcpy (NewSymbol->Name, Name);
  strcpy (NewSymbol->Value, Value);
  //
  // Remove trailing spaces
  //
  Cptr = NewSymbol->Value + strlen (NewSymbol->Value) - 1;
  while (Cptr > NewSymbol->Value) {
    if (isspace (*Cptr)) {
      *Cptr = 0;
      Cptr--;
    } else {
      break;
    }
  }
  //
  // Add it to the head of the list.
  //
  NewSymbol->Next = gGlobals.Symbol;
  gGlobals.Symbol = NewSymbol;
  //
  // If value == "NULL", then make it a 0-length string
  //
  if (stricmp (NewSymbol->Value, "NULL") == 0) {
    NewSymbol->Value[0] = 0;
  }
  //
  // Restore the terminator we inserted if they passed in var=value
  //
  if (SaveCptr != NULL) {
    *SaveCptr = CSave;
  }

  return Len;
}

/*****************************************************************************
******************************************************************************/
static
int
RemoveSymbol (
  INT8 *Name,
  INT8 SymbolType
  )
{
  SYMBOL  *Symbol;
  SYMBOL  *PrevSymbol;

  PrevSymbol  = NULL;
  Symbol      = gGlobals.Symbol;
  while (Symbol) {
    if ((stricmp (Name, Symbol->Name) == 0) && (Symbol->Type & SymbolType)) {
      if (Symbol->Value) {
        free (Symbol->Value);
      }

      free (Symbol->Name);
      if (PrevSymbol) {
        PrevSymbol->Next = Symbol->Next;
      } else {
        gGlobals.Symbol = Symbol->Next;
      }

      free (Symbol);
      return STATUS_SUCCESS;
    }

    PrevSymbol  = Symbol;
    Symbol      = Symbol->Next;
  }

  return STATUS_WARNING;
}

#if 0

/*****************************************************************************
******************************************************************************/
static
void
FreeSections (
  SECTION *Sect
  )
{
  SECTION *Next;

  while (Sect != NULL) {
    Next = Sect->Next;
    if (Sect->Name != NULL) {
      delete[] Sect->Name;
    }

    delete Sect;
    Sect = Next;
  }
}
#endif

/*****************************************************************************
******************************************************************************/
static
INT8 *
StripLine (
  INT8 *Line
  )
{
  INT8  *Cptr;
  int   Len;

  Cptr = Line;
  //
  // Look for '#' comments in first character of line
  //
  if (*Cptr == '#') {
    *Cptr = 0;
    return Cptr;
  }

  while (isspace (*Cptr)) {
    Cptr++;
  }
  //
  // Hack off newlines
  //
  Len = strlen (Cptr);
  if ((Len > 0) && (Cptr[Len - 1] == '\n')) {
    Cptr[Len - 1] = 0;
  }
  //
  // Hack off trailing spaces
  //
  StripTrailingSpaces (Cptr);
  return Cptr;
}

/*****************************************************************************
  FUNCTION:  ProcessOptions()
  
  DESCRIPTION: Process the command-line options.  
******************************************************************************/
static
int
ProcessOptions (
  int   Argc,
  INT8  *Argv[]
  )
/*++

Routine Description:
  
  Process the command line options to this utility.

Arguments:

  Argc   - Standard Argc.
  Argv[] - Standard Argv.

Returns:

--*/
{
  INT8  *Cptr;
  int   FreeCwd;

  //
  // Clear out the options
  //
  memset ((INT8 *) &gGlobals, 0, sizeof (gGlobals));

  Argc--;
  Argv++;

  if (Argc == 0) {
    Usage ();
    return STATUS_ERROR;
  }
  //
  // Now process the arguments
  //
  while (Argc > 0) {

    if ((Argv[0][0] == '-') || (Argv[0][0] == '/')) {
      switch (Argv[0][1]) {
      //
      // -? or -h help option
      //
      case '?':
      case 'h':
      case 'H':
        Usage ();
        return STATUS_ERROR;

      //
      // /d macro=name
      //
      case 'd':
      case 'D':
        //
        // Skip to next arg
        //
        Argc--;
        Argv++;
        if (Argc == 0) {
          Argv--;
          Error (NULL, 0, 0, NULL, "missing macro definition with %c%c", Argv[0][0], Argv[0][1]);
          return STATUS_ERROR;
        } else {
          if (AddSymbol (Argv[0], NULL, SYM_OVERWRITE | SYM_GLOBAL) <= 0) {
            Warning (NULL, 0, 0, Argv[0], "failed to add symbol: %s");
          }
        }
        break;

      //
      // output makefile name
      //
      case 'm':
      case 'M':
        //
        // Skip to next arg
        //
        Argc--;
        Argv++;
        if (Argc == 0) {
          Argv--;
          Error (NULL, 0, 0, Argv[0], "missing output makefile name with option");
          Usage ();
          return STATUS_ERROR;
        } else {
          strcpy (gGlobals.MakefileName, Argv[0]);
        }
        break;

      //
      // Print a cross-reference file containing guid/basename/processor/fv
      //
      case 'x':
      case 'X':
        //
        // Skip to next arg
        //
        Argc--;
        Argv++;
        if (Argc == 0) {
          Argv--;
          Error (NULL, 0, 0, Argv[0], "missing cross-reference output filename with option");
          Usage ();
          return STATUS_ERROR;
        } else {
          strcpy (gGlobals.XRefFileName, Argv[0]);
        }
        break;

      //
      // GUID database file to preparse
      //
      case 'g':
      case 'G':
        //
        // Skip to next arg
        //
        Argc--;
        Argv++;
        if (Argc == 0) {
          Argv--;
          Error (NULL, 0, 0, Argv[0], "missing input GUID database filename with option");
          Usage ();
          return STATUS_ERROR;
        } else {
          strcpy (gGlobals.GuidDatabaseFileName, Argv[0]);
        }
        break;

      case 'v':
      case 'V':
        gGlobals.Verbose = 1;
        break;

      default:
        Error (NULL, 0, 0, Argv[0], "unrecognized option");
        return STATUS_ERROR;
      }
    } else {
      break;
    }

    Argc--;
    Argv++;
  }
  //
  // Must be at least one arg left
  //
  if (Argc > 0) {
    gGlobals.DscFilename = Argv[0];
  }

  if (gGlobals.DscFilename == NULL) {
    Error (NULL, 0, 0, NULL, "must specify DSC filename on command line");
    return STATUS_ERROR;
  }
  //
  // Make a global symbol for the DSC filename
  //
  AddSymbol (DSC_FILENAME, gGlobals.DscFilename, SYM_GLOBAL | SYM_FILENAME);
  //
  // If no output makefile specified, take the default
  //
  if (gGlobals.MakefileName[0] == 0) {
    strcpy (gGlobals.MakefileName, MAKEFILE_OUT_NAME);
  }
  //
  // Get the current working directory and use it for the build directory.
  // Only do this if they have not defined it on the command line. Do the
  // same for the bin dir, output dir, and library directory.
  //
  Cptr = GetSymbolValue (BUILD_DIR);
  if (Cptr == NULL) {
    Cptr    = _getcwd (NULL, 0);
    FreeCwd = 1;
    AddSymbol (BUILD_DIR, Cptr, SYM_OVERWRITE | SYM_GLOBAL | SYM_FILEPATH);
  } else {
    FreeCwd = 0;
  }

  if (FreeCwd) {
    free (Cptr);
  }

  return 0;
}

/*****************************************************************************
******************************************************************************/
static
SYMBOL *
FreeSymbols (
  SYMBOL *Syms
  )
{
  SYMBOL  *Next;
  while (Syms) {

    if (Syms->Name != NULL) {
      free (Syms->Name);
    }

    if (Syms->Value != NULL) {
      free (Syms->Value);
    }

    Next = Syms->Next;
    free (Syms);
    Syms = Next;
  }

  return Syms;
}

/*****************************************************************************
******************************************************************************/
static
int
GetSourceFileType (
  INT8 *FileName
  )
{
  INT8  *Cptr;
  int   len;
  int   i;

  len = strlen (FileName);
  if (len == 0) {
    return FILETYPE_UNKNOWN;

  }

  Cptr = FileName + len - 1;
  while ((*Cptr != '.') && (Cptr >= FileName)) {
    Cptr--;

  }

  if (*Cptr == '.') {

    for (i = 0; mFileTypes[i].Extension != NULL; i++) {
      len = strlen (mFileTypes[i].Extension);
      if (strnicmp (mFileTypes[i].Extension, Cptr, len) == 0) {
        if ((*(Cptr + len) == 0) || isspace (*(Cptr + len))) {
          return mFileTypes[i].FileType;
        }
      }
    }
  }

  return FILETYPE_UNKNOWN;
}
//
// Determine if a given file is a standard include file. If we don't know,
// then assume it's not.
//
static
int
IsIncludeFile (
  INT8 *FileName
  )
{
  INT8  *Cptr;
  int   len;
  int   i;

  len = strlen (FileName);
  if (len == 0) {
    return 0;
  }

  Cptr = FileName + len - 1;
  while ((*Cptr != '.') && (Cptr >= FileName)) {
    Cptr--;
  }

  if (*Cptr == '.') {
    //
    // Now go through the list of filename extensions and try to find
    // a match for this file extension.
    //
    for (i = 0; mFileTypes[i].Extension != NULL; i++) {
      len = strlen (mFileTypes[i].Extension);
      if (strnicmp (mFileTypes[i].Extension, Cptr, len) == 0) {
        //
        // Make sure that's all there is to the filename extension.
        //
        if ((*(Cptr + len) == 0) || isspace (*(Cptr + len))) {
          return mFileTypes[i].FileFlags & FILE_FLAG_INCLUDE;
        }
      }
    }
  }

  return 0;
}

/*****************************************************************************
******************************************************************************/
static
void
StripTrailingSpaces (
  INT8 *Str
  )
{
  INT8  *Cptr;
  Cptr = Str + strlen (Str) - 1;
  while (Cptr > Str) {
    if (isspace (*Cptr)) {
      *Cptr = 0;
      Cptr--;
    } else {
      break;
    }
  }
}

/*****************************************************************************
******************************************************************************/
static
int
GetEfiSource (
  VOID
  )
{
  INT8  *Cwd;
  INT8  *EnvValue;
  INT8  Line[MAX_PATH];

  //
  // Get the environmental variable setting of EFI_SOURCE. Hack off any
  // trailing slashes though for comparison's sake.
  //
  EnvValue = getenv (EFI_SOURCE);
  if (EnvValue != NULL) {
    if (EnvValue[strlen (EnvValue) - 1] == '\\') {
      EnvValue[strlen (EnvValue) - 1] = 0;
    }
  }
  //
  // Don't set it if the user specified it on the command line.
  //
  if (GetSymbolValue (EFI_SOURCE) != NULL) {
    return STATUS_SUCCESS;
  }
  //
  // Get the EFI_SOURCE from the build directory if you can.
  //
  Cwd = GetSymbolValue (BUILD_DIR);
  if (Cwd == NULL) {
    Error (NULL, 0, 0, NULL, "could not get current working directory from build directory");
    return STATUS_ERROR;
  }
  //
  // look for "platform" in the string. Should be something like:
  //  c:\project\efi\platform\nt32     for new directory structure
  //
  strcpy (Line, Cwd);
  Cwd = Line + strlen (Line) - 1;
  while (Cwd >= Line) {
    if (strnicmp (Cwd, PLATFORM_STR, strlen (PLATFORM_STR)) == 0) {
      *Cwd = 0;
      //
      // Emit a messsage if it's not the same as the environmental setting.
      //
      if ((EnvValue != NULL) && stricmp (EnvValue, Line)) {
        fprintf (stdout, "***************************************************************\n");
        fprintf (stdout, "* WARNING: ENVIRONMENTAL VARIABLE EFI_SOURCE DIFFERS FROM CWD *\n");
        fprintf (stdout, "***************************************************************\n");
      }

      AddSymbol (EFI_SOURCE, Line, SYM_GLOBAL | SYM_FILEPATH);
      return STATUS_SUCCESS;
    }

    Cwd--;
  }

  Error (NULL, 0, 0, NULL, "could not determine EFI_SOURCE from current working directory");
  return STATUS_ERROR;
}

void
Message (
  UINT32  PrintMask,
  INT8    *Fmt,
  ...
  )
{
  INT8    Line[MAX_LINE_LEN];
  va_list List;

  va_start (List, Fmt);
  vsprintf (Line, Fmt, List);
  if (PrintMask & gGlobals.Verbose) {
    fprintf (stdout, "%s\n", Line);
  }

  va_end (List);
}

static
void
Usage (
  VOID
  )
{
  int               i;
  static const INT8 *Help[] = {
    "Usage:  ProcessDsc {options} [Dsc Filename]",
    "    Options:",
    "       -d var=value        to define macro 'var' to 'value'",
    "       -v                  for verbose mode",
    "       -g filename         to preparse GUID listing file",
    NULL
  };
  for (i = 0; Help[i] != NULL; i++) {
    fprintf (stdout, "%s\n", Help[i]);
  }
}

/*++

Routine Description:
  
  Process the [defines] section in the DSC file.

Arguments:

  DscFile - pointer to the DSCFile class that contains the relevant info.

Returns:

  0 if not necessarily an absolute path
  1 otherwise

--*/
static
int
ProcessDSCDefinesSection (
  DSC_FILE *DscFile
  )
{
  INT8    Line[MAX_LINE_LEN];
  INT8    Line2[MAX_EXP_LINE_LEN];
  INT8    *Cptr;
  SECTION *Sect;

  //
  // Look for a [defines.common] section and process it
  //
  Sect = DSCFileFindSection (DscFile, DEFINES_SECTION_NAME);
  if (Sect == NULL) {
    return STATUS_ERROR;
  }
  //
  // Read lines while they're valid
  //
  while (DSCFileGetLine (DscFile, Line, sizeof (Line)) != NULL) {
    //
    // Expand macros on the line
    //
    if (ExpandMacros (Line, Line2, sizeof (Line2), 0)) {
      return STATUS_ERROR;
    }
    //
    // Strip the line
    //
    Cptr = StripLine (Line2);
    if (*Cptr) {
      //
      // Make the assignment
      //
      AddSymbol (Line2, NULL, SYM_OVERWRITE | SYM_GLOBAL);
    }
  }

  return STATUS_SUCCESS;
}

static
int
IsAbsolutePath (
  char    *FileName
  )
/*++

Routine Description:
  
  Determine if a given filename contains the full path information.

Arguments:

  FileName - the name of the file, with macros expanded.

Returns:

  0 if not necessarily an absolute path
  1 otherwise

--*/
{
  //
  // If the first character is a-z, and the second character is a colon, then
  // it is an absolute path.
  //
  if (isalpha (FileName[0]) && (FileName[1] == ':')) {
    return 1;
  }

  return 0;
}