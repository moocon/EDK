/*++

Copyright 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  VfrServices.cpp

Abstract:

  Support routines for the VFR compiler
  
--*/  

#include <stdio.h>    // for FILE routines
#include <stdlib.h>   // for malloc() and free()

#include "Tiano.h"
#include "EfiUtilityMsgs.h"
#include "EfiVfr.h"
#include "VfrServices.h"

#include EFI_PROTOCOL_DEFINITION (Hii)

static const char *mSourceFileHeader[] = {
  "//",
  "//  DO NOT EDIT -- auto-generated file",
  "//",
  "//  This file is generated by the VFR compiler.",
  "//",
  NULL
};

typedef struct {
   INT8     *Name;
   INT32    Size;
} IFR_OPCODE_SIZES;

//
// Create a table that can be used to do internal checking on the IFR
// bytes we emit.
//
static const IFR_OPCODE_SIZES mOpcodeSizes[] = {
  { 0, 0 },     // invalid
  { "EFI_IFR_FORM",                       sizeof (EFI_IFR_FORM) },
  { "EFI_IFR_SUBTITLE",                   sizeof (EFI_IFR_SUBTITLE) }, 
  { "EFI_IFR_TEXT",                       -6 }, //sizeof (EFI_IFR_TEXT) }, 
  { "unused 0x04 opcode",                 0 }, // EFI_IFR_GRAPHIC_OP
  { "EFI_IFR_ONE_OF",                     sizeof (EFI_IFR_ONE_OF) }, 
  { "EFI_IFR_CHECK_BOX",                  sizeof (EFI_IFR_CHECK_BOX) }, 
  { "EFI_IFR_NUMERIC",                    sizeof (EFI_IFR_NUMERIC) }, 
  { "EFI_IFR_PASSWORD",                   sizeof (EFI_IFR_PASSWORD) }, 
  { "EFI_IFR_ONE_OF_OPTION",              sizeof (EFI_IFR_ONE_OF_OPTION) }, 
  { "EFI_IFR_SUPPRESS",                   sizeof (EFI_IFR_SUPPRESS) }, 
  { "EFI_IFR_END_FORM",                   sizeof (EFI_IFR_END_FORM) }, 
  { "EFI_IFR_HIDDEN",                     sizeof (EFI_IFR_HIDDEN) }, 
  { "EFI_IFR_END_FORM_SET",               sizeof (EFI_IFR_END_FORM_SET) }, 
  { "EFI_IFR_FORM_SET",                   sizeof (EFI_IFR_FORM_SET) }, 
  { "EFI_IFR_REF",                        sizeof (EFI_IFR_REF) }, 
  { "EFI_IFR_END_ONE_OF",                 sizeof (EFI_IFR_END_ONE_OF) }, 
  { "EFI_IFR_INCONSISTENT",               sizeof (EFI_IFR_INCONSISTENT) }, 
  { "EFI_IFR_EQ_ID_VAL",                  sizeof (EFI_IFR_EQ_ID_VAL) }, 
  { "EFI_IFR_EQ_ID_ID",                   sizeof (EFI_IFR_EQ_ID_ID) }, 
  { "EFI_IFR_EQ_ID_LIST",                 -sizeof (EFI_IFR_EQ_ID_LIST) }, 
  { "EFI_IFR_AND",                        sizeof (EFI_IFR_AND) }, 
  { "EFI_IFR_OR",                         sizeof (EFI_IFR_OR) }, 
  { "EFI_IFR_NOT",                        sizeof (EFI_IFR_NOT) }, 
  { "EFI_IFR_END_IF",                     sizeof (EFI_IFR_END_IF) }, 
  { "EFI_IFR_GRAYOUT",                    sizeof (EFI_IFR_GRAYOUT) }, 
  { "EFI_IFR_DATE",                       sizeof (EFI_IFR_DATE) / 3 }, 
  { "EFI_IFR_TIME",                       sizeof (EFI_IFR_TIME) / 3 }, 
  { "EFI_IFR_STRING",                     sizeof (EFI_IFR_STRING) }, 
  { "EFI_IFR_LABEL",                      sizeof (EFI_IFR_LABEL) }, 
  { "EFI_IFR_SAVE_DEFAULTS",              sizeof (EFI_IFR_SAVE_DEFAULTS) }, 
  { "EFI_IFR_RESTORE_DEFAULTS",           sizeof (EFI_IFR_RESTORE_DEFAULTS) }, 
  { "EFI_IFR_BANNER",                     sizeof (EFI_IFR_BANNER) },
  { "EFI_IFR_INVENTORY",                  sizeof (EFI_IFR_INVENTORY) },
  { "EFI_IFR_EQ_VAR_VAL_OP",              sizeof (EFI_IFR_EQ_VAR_VAL) },
  { "EFI_IFR_ORDERED_LIST_OP",            sizeof (EFI_IFR_ORDERED_LIST) },
  { "EFI_IFR_VARSTORE_OP",                -sizeof (EFI_IFR_VARSTORE) },
  { "EFI_IFR_VARSTORE_SELECT_OP",         sizeof (EFI_IFR_VARSTORE_SELECT) },
  { "EFI_IFR_VARSTORE_SELECT_PAIR_OP",    sizeof (EFI_IFR_VARSTORE_SELECT_PAIR) },
  { "EFI_IFR_OEM_DEFINED_OP",             -2 },
};


VfrOpcodeHandler::VfrOpcodeHandler (
  ) 
/*++

Routine Description:
  Constructor for the VFR opcode handling class.
  
Arguments:
  None

Returns:
  None

--*/
{ 
  mIfrBytes                       = NULL; 
  mLastIfrByte                    = NULL;
  mBytesWritten                   = 0;
  mQueuedByteCount                = 0;
  mQueuedOpcodeByteValid          = 0;
  mPrimaryVarStoreId              = 0;
  mSecondaryVarStoreId            = 0;
  mSecondaryVarStoreIdSet         = 0;
  mPrimaryVarStoreIdSet           = 0;
  mDefaultVarStoreId              = 0;
}

VOID 
VfrOpcodeHandler::SetVarStoreId (
  UINT16 VarStoreId
  )
/*++

Routine Description:
  This function is invoked by the parser when a variable is referenced in the 
  VFR. Save the variable store (and set a flag) so that we can later determine 
  if we need to emit a varstore-select or varstore-select-pair opcode.
  
Arguments:
  VarStoreId - ID of the variable store referenced in the VFR

Returns:
  None

--*/
{
  mPrimaryVarStoreId    = VarStoreId;
  mPrimaryVarStoreIdSet = 1;
}

VOID 
VfrOpcodeHandler::SetSecondaryVarStoreId (
  UINT16 VarStoreId
  )
/*++

Routine Description:
  This function is invoked by the parser when a secondary variable is 
  referenced in the VFR. Save the variable store (and set a flag) so 
  that we can later determine if we need to emit a varstore-select or 
  varstore-pair opcode.
  
Arguments:
  VarStoreId - ID of the variable store referenced in the VFR

Returns:
  None

--*/
{
  mSecondaryVarStoreId    = VarStoreId;
  mSecondaryVarStoreIdSet = 1;
}

VOID 
VfrOpcodeHandler::WriteIfrBytes (
  ) 
/*++

Routine Description:
  This function is invoked at the end of parsing. Its purpose
  is to write out all the IFR bytes that were queued up while
  parsing.
  
Arguments:
  None

Returns:
  None

--*/
{ 
  IFR_BYTE                  *Curr;
  IFR_BYTE                  *Next;
  UINT32                    Count;
  UINT32                    LineCount;
  UINT32                    PoundLines;
  UINT32                    ByteCount;
  INT8                      Line[MAX_LINE_LEN];
  INT8                      *Cptr;
  FILE                      *InFptr;
  FILE                      *OutFptr;
  UINT32                    ListFile;
  EFI_HII_IFR_PACK_HEADER   IfrHeader;
  UINT8                     *Ptr;
  FILE                      *IfrBinFptr;
  UINT32                    BytesLeftThisOpcode;
  //
  // If someone added a new opcode and didn't update our opcode sizes structure, error out.
  //
  if (sizeof(mOpcodeSizes) / sizeof (mOpcodeSizes[0]) != EFI_IFR_LAST_OPCODE + 1) {
    Error (__FILE__, __LINE__, 0, "application error", "internal IFR binary table size is incorrect");
    return;
  }
  //
  // Flush the queue
  //
  FlushQueue ();    
  //
  // If there have been any errors to this point, then skip dumping the IFR
  // binary data. This way doing an nmake again will try to build it again, and
  // the build will fail if they did not fix the problem.
  //
  if (GetUtilityStatus () != STATUS_ERROR) {
    if ((IfrBinFptr = fopen (gOptions.IfrOutputFileName, "w")) == NULL) {
      Error (PROGRAM_NAME, 0, 0, gOptions.IfrOutputFileName, "could not open file for writing");
      return;
    }
    //
    // Write the standard file header to the output file
    //
    WriteStandardFileHeader (IfrBinFptr);
    //
    // Write the structure header
    //
    fprintf (IfrBinFptr, "\nunsigned char %sBin[] = {", gOptions.VfrBaseFileName);
    //
    // Write the header
    //
    memset ((char *)&IfrHeader, 0, sizeof (IfrHeader));
    IfrHeader.Header.Type = EFI_HII_IFR;
    IfrHeader.Header.Length = mBytesWritten + sizeof (IfrHeader);    
    Ptr = (UINT8 *)&IfrHeader;
    for (Count = 0; Count < sizeof (IfrHeader); Count++, Ptr++) {
      if ((Count & 0x03) == 0) {
        fprintf (IfrBinFptr, "\n  ");
      }
      fprintf (IfrBinFptr, "0x%02X, ", *Ptr);      
    }
    //
    //
    // Write all the IFR bytes
    //
    fprintf (IfrBinFptr, "\n  // start of IFR data");
    Curr = mIfrBytes;
    Count = 0;
    while (Curr != NULL) {
      if ((Count & 0x0F) == 0) {
        fprintf (IfrBinFptr, "\n  ");
      }
      if (Curr->KeyByte != 0) {
        fprintf (IfrBinFptr, "/*%c*/ ", Curr->KeyByte);
      }
      fprintf (IfrBinFptr, "0x%02X, ", Curr->OpcodeByte);
      Count++;
      Curr = Curr->Next;
    }
    fprintf (IfrBinFptr, "\n};\n\n");
    //
    //
    // Close the file
    //
    fclose (IfrBinFptr); 
    IfrBinFptr = NULL;
  }
  //
  // Write the bytes as binary data if the user specified to do so
  //
  if ((GetUtilityStatus () != STATUS_ERROR) &&  (gOptions.CreateIfrBinFile != 0)) {
    //
    // Use the Ifr output file name with a ".hpk" extension.
    //
    for (Cptr = gOptions.IfrOutputFileName + strlen (gOptions.IfrOutputFileName) - 1;
         (*Cptr != '.') && (Cptr > gOptions.IfrOutputFileName) && (*Cptr != '\\');
         Cptr--) {
      //
      // do nothing
      //
    }
    if (*Cptr == '.') {
      strcpy (Cptr, ".hpk");
    } else {
      strcat (gOptions.IfrOutputFileName, ".hpk");
    }
    if ((IfrBinFptr = fopen (gOptions.IfrOutputFileName, "wb")) == NULL) {
      Error (PROGRAM_NAME, 0, 0, gOptions.IfrOutputFileName, "could not open file for writing");
      return;
    }
    //
    // Write the structure header
    //
    memset ((char *)&IfrHeader, 0, sizeof (IfrHeader));
    IfrHeader.Header.Type = EFI_HII_IFR;
    IfrHeader.Header.Length = mBytesWritten + sizeof (IfrHeader);    
    Ptr = (UINT8 *)&IfrHeader;
    for (Count = 0; Count < sizeof (IfrHeader); Count++, Ptr++) {
      fwrite (Ptr, 1, 1, IfrBinFptr);
    }
    //
    //
    // Write all the IFR bytes
    //
    Curr = mIfrBytes;
    Count = 0;
    while (Curr != NULL) {
      fwrite (&Curr->OpcodeByte, 1, 1, IfrBinFptr);
      Curr = Curr->Next;
    }
    //
    //
    // Close the file
    //
    fclose (IfrBinFptr); 
    IfrBinFptr = NULL;
  }
  //
  // If creating a listing file, then open the input and output files
  //
  ListFile = 0;
  if (gOptions.CreateListFile) {
    //
    // Open the input VFR file and the output list file
    //
    if ((InFptr = fopen (gOptions.PreprocessorOutputFileName, "r")) == NULL) {
      Warning (PROGRAM_NAME, 0, 0, gOptions.PreprocessorOutputFileName, "could not open file for creating a list file");
    } else {
      if ((OutFptr = fopen (gOptions.VfrListFileName, "w")) == NULL) {
        Warning (PROGRAM_NAME, 0, 0, gOptions.VfrListFileName, "could not open output list file for writing");
        fclose (InFptr);
        InFptr = NULL;
      } else {
        LineCount   = 0;
        ListFile    = 1;
        PoundLines  = 0;
        ByteCount   = 0;
      }
    }
  }
  //
  // Write the list file
  //
  if (ListFile) {
    //
    // Write out the VFR compiler version
    //
    fprintf (OutFptr, "//\n//  VFR compiler version " VFR_COMPILER_VERSION "\n//\n");
    Curr = mIfrBytes;
    while (Curr != NULL) {
      //
      // Print lines until we reach the line of the current opcode
      //
      while (LineCount < PoundLines + Curr->LineNum) {
        if (fgets (Line, sizeof (Line), InFptr) != NULL) {
          //
          // We should check for line length exceeded on the fgets(). Otherwise it
          // throws the listing file output off. Future enhancement perhaps.
          //
          fprintf (OutFptr, "%s", Line);
          if (strncmp (Line, "#line", 5) == 0) {
            PoundLines++;
          }
        }
        LineCount++;
      }
      //
      // Print all opcodes with line numbers less than where we are now
      //
      BytesLeftThisOpcode = 0;
      while ((Curr != NULL) && ((Curr->LineNum == 0) || (LineCount >= PoundLines + Curr->LineNum))) {
        if (BytesLeftThisOpcode == 0) {
          fprintf (OutFptr, ">%08X: ", ByteCount);
          if (Curr->Next != NULL) {
            BytesLeftThisOpcode = (UINT32)Curr->Next->OpcodeByte;
          }
        }
        fprintf (OutFptr, "%02X ", (UINT32)Curr->OpcodeByte);
        ByteCount++;
        BytesLeftThisOpcode--;
        if (BytesLeftThisOpcode == 0) {
          fprintf (OutFptr, "\n");
        }
        Curr = Curr->Next;
      }
    }
    //
    // Dump any remaining lines from the input file
    //
    while (fgets (Line, sizeof (Line), InFptr) != NULL) {
      fprintf (OutFptr, "%s", Line);
    }
    fclose (InFptr);
    fclose (OutFptr);
  }
  //
  // Debug code to make sure that each opcode we write out has as many
  // bytes as the IFR structure requires. If there were errors, then
  // don't do this step.
  //
  if (GetUtilityStatus () != STATUS_ERROR) {
    Curr = mIfrBytes;
    ByteCount = 0;
    while (Curr != NULL) {
      //
      // First byte is the opcode, second byte is the length
      //
      if (Curr->Next == NULL) {
        Error (__FILE__, __LINE__, 0, "application error", "last opcode written does not contain a length byte");
        break;
      }
      Count = (UINT32)Curr->Next->OpcodeByte;
      if (Count == 0) {
        Error (
          __FILE__, 
          __LINE__, 
          0, 
          "application error", 
          "opcode with 0 length specified in output at offset 0x%X", 
          ByteCount
          );
        break;
      }
      //
      // Check the length
      //
      if ((Curr->OpcodeByte > EFI_IFR_LAST_OPCODE) || (Curr->OpcodeByte == 0)) {
        Error (
          __FILE__, 
          __LINE__, 
          0, 
          "application error", 
          "invalid opcode 0x%X in output at offset 0x%X", 
          (UINT32) Curr->OpcodeByte, ByteCount
          );
      } else if (mOpcodeSizes[Curr->OpcodeByte].Size < 0) {
        //
        // For those cases where the length is variable, the size is negative, and indicates
        // the miniumum size.
        //
        if ((mOpcodeSizes[Curr->OpcodeByte].Size * -1) > Count) {
          Error (
            __FILE__, 
            __LINE__, 
            0, 
            "application error", 
            "insufficient number of bytes written for %s at offset 0x%X",
            mOpcodeSizes[Curr->OpcodeByte].Name, 
            ByteCount
            );
        }
      } else {
        //
        // Check for gaps
        //
        if (mOpcodeSizes[Curr->OpcodeByte].Size == 0) {
          Error (
            __FILE__, 
            __LINE__, 
            0, 
            "application error", 
            "invalid opcode 0x%X in output at offset 0x%X", 
            (UINT32)Curr->OpcodeByte, 
            ByteCount
            );
        } else {
          //
          // Check size
          //
          if (mOpcodeSizes[Curr->OpcodeByte].Size != Count) {
            Error (
              __FILE__, 
              __LINE__, 
              0, 
              "application error", 
              "invalid number of bytes (%d written s/b %d) written for %s at offset 0x%X",
              Count, 
              mOpcodeSizes[Curr->OpcodeByte].Size, 
              mOpcodeSizes[Curr->OpcodeByte].Name, 
              ByteCount
              );
          }
        }
      }
      //
      // Skip to next opcode
      //
      while (Count > 0) {
        ByteCount++;
        if (Curr == NULL) {
          Error (__FILE__, __LINE__, 0, "application error", "last opcode written has invalid length");
          break;
        }
        Curr = Curr->Next;
        Count--;
      }
    }
  }
}

VfrOpcodeHandler::~VfrOpcodeHandler(
  ) 
/*++

Routine Description:
  Destructor for the VFR opcode handler. Free up memory allocated
  while parsing the VFR script.
  
Arguments:
  None

Returns:
  None

--*/
{
  IFR_BYTE    *Curr;
  IFR_BYTE    *Next;
  //
  // Free up the IFR bytes
  //
  Curr = mIfrBytes;
  while (Curr != NULL) {
    Next = Curr->Next;
    free (Curr);
    Curr = Next;
  }
}

int 
VfrOpcodeHandler::AddOpcodeByte (
  UINT8 OpcodeByte, 
  UINT32 LineNum
  ) 
/*++

Routine Description:
  This function is invoked by the parser when a new IFR
  opcode should be emitted.
  
Arguments:
  OpcodeByte  - the IFR opcode
  LineNum     - the line number from the source file that resulted
                in the opcode being emitted.

Returns:
  0 always

--*/
{
  UINT32 Count;

  FlushQueue();
  //
  // Now add this new byte
  //
  mQueuedOpcodeByte       = OpcodeByte;
  mQueuedLineNum          = LineNum;
  mQueuedOpcodeByteValid  = 1;
  return 0;
}

VOID 
VfrOpcodeHandler::AddByte (
  UINT8 ByteVal, 
  UINT8 KeyByte
  )
/*++

Routine Description:
  This function is invoked by the parser when it determines
  that more raw IFR bytes should be emitted to the output stream.
  Here we just queue them up into an output buffer.
  
Arguments:
  ByteVal   - the raw byte to emit to the output IFR stream
  KeyByte   - a value that can be used for debug. 

Returns:
  None

--*/
{
  //
  // Check for buffer overflow
  //
  if (mQueuedByteCount > MAX_QUEUE_COUNT) {
    Error (PROGRAM_NAME, 0, 0, NULL, "opcode queue overflow");
  } else {
    mQueuedBytes[mQueuedByteCount]    = ByteVal;
    mQueuedKeyBytes[mQueuedByteCount] = KeyByte;
    mQueuedByteCount++;
  }
}

int 
VfrOpcodeHandler::FlushQueue (
  )
/*++

Routine Description:
  This function is invoked to flush the internal IFR buffer.
  
Arguments:
  None

Returns:
  0 always

--*/
{
  UINT32 Count;
  UINT32 EmitNoneOnePair;

  EmitNoneOnePair = 0;
  //
  // If the secondary varstore was specified, then we have to emit
  // a varstore-select-pair opcode, which only applies to the following
  // statement. 
  //
  if (mSecondaryVarStoreIdSet) {
    mSecondaryVarStoreIdSet = 0;
    //
    // If primary and secondary are the same as the current default
    // varstore, then we don't have to do anything.
    // Note that the varstore-select-pair only applies to the following
    // opcode.
    //
    if ((mPrimaryVarStoreId != mSecondaryVarStoreId) || (mPrimaryVarStoreId != mDefaultVarStoreId)) {
      IAddByte (EFI_IFR_VARSTORE_SELECT_PAIR_OP, 'O', mQueuedLineNum);
      IAddByte ((UINT8)sizeof (EFI_IFR_VARSTORE_SELECT_PAIR), 'L', 0);
      IAddByte ((UINT8)mPrimaryVarStoreId, 0, 0);
      IAddByte ((UINT8)(mPrimaryVarStoreId >> 8), 0, 0);
      IAddByte ((UINT8)mSecondaryVarStoreId, 0, 0);
      IAddByte ((UINT8)(mSecondaryVarStoreId >> 8), 0, 0);
    }
  } else if (mPrimaryVarStoreIdSet != 0) {
    mPrimaryVarStoreIdSet = 0;
    if (mDefaultVarStoreId != mPrimaryVarStoreId) {
      //
      // The VFR statement referenced a different variable store 
      // than the last one we reported. Insert a new varstore select 
      // statement. 
      //
      IAddByte (EFI_IFR_VARSTORE_SELECT_OP, 'O', mQueuedLineNum);
      IAddByte ((UINT8)sizeof (EFI_IFR_VARSTORE_SELECT), 'L', 0);
      IAddByte ((UINT8)mPrimaryVarStoreId, 0, 0);
      IAddByte ((UINT8)(mPrimaryVarStoreId >> 8), 0, 0);
      mDefaultVarStoreId = mPrimaryVarStoreId;
    }
  }
  //
  // Likely a new opcode is being added. Since each opcode item in the IFR has 
  // a header that specifies the size of the opcode item (which we don't
  // know until we find the next opcode in the VFR), we queue up bytes
  // until we know the size. Then we write them out. So flush the queue
  // now.
  //
  if (mQueuedOpcodeByteValid != 0) {
    // 
    // Add the previous opcode byte, the length byte, and the binary
    // data.
    //
    IAddByte (mQueuedOpcodeByte, 'O', mQueuedLineNum);
    IAddByte ((UINT8)(mQueuedByteCount + 2), 'L', 0);
    for (Count = 0; Count < mQueuedByteCount; Count++) {
      IAddByte (mQueuedBytes[Count], mQueuedKeyBytes[Count], 0);          
    }
    mQueuedByteCount = 0;
    mQueuedOpcodeByteValid = 0;
  }    
  return 0;
}

int 
VfrOpcodeHandler::IAddByte (
  UINT8   ByteVal, 
  UINT8   KeyByte, 
  UINT32  LineNum
  )
/*++

Routine Description:
  This internal function is used to add actual IFR bytes to
  the output stream. Most other functions queue up the bytes
  in an internal buffer. Once they come here, there's no
  going back.

  
Arguments:
  ByteVal   - value to write to output 
  KeyByte   - key value tied to the byte -- useful for debug
  LineNum   - line number from source file the byte resulted from

Returns:
  0 - if successful
  1 - failed due to memory allocation failure

--*/
{
  IFR_BYTE    *NewByte;
  NewByte = (IFR_BYTE *)malloc (sizeof (IFR_BYTE));
  if (NewByte == NULL) {
    return 1;
  }
  memset ((char *)NewByte, 0, sizeof (IFR_BYTE));
  NewByte->OpcodeByte = ByteVal;
  NewByte->KeyByte = KeyByte;
  NewByte->LineNum = LineNum;
  //
  // Add to the list
  //
  if (mIfrBytes == NULL) {
    mIfrBytes = NewByte;
  } else {
    mLastIfrByte->Next = NewByte;
  } 
  mLastIfrByte = NewByte;
  mBytesWritten++;
  return 0;
}

VOID 
WriteStandardFileHeader (
  FILE *OutFptr
  ) 
/*++

Routine Description:
  This function is invoked to emit a standard header to an
  output text file.
  
Arguments:
  OutFptr - file to write the header to

Returns:
  None

--*/
{
  UINT32 TempIndex;
  for (TempIndex = 0; mSourceFileHeader[TempIndex] != NULL; TempIndex++) {
    fprintf (OutFptr, "%s\n", mSourceFileHeader[TempIndex]);
  }
  //
  // Write out the VFR compiler version
  //
  fprintf (OutFptr, "//  VFR compiler version " VFR_COMPILER_VERSION "\n//\n");
}
