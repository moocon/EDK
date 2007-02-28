/*++

Copyright (c) 2004 - 2005, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Forms.c

Abstract:

  This file contains the form processing code to the HII database.

--*/

#include "HiiDatabase.h"

CHAR16*
Ascii2Unicode (
  OUT CHAR16         *UnicodeStr,
  IN  CHAR8          *AsciiStr
  )
/*++
  
  Routine Description:

    This function converts ASCII string to Unicode string.
  
  Arguments:

    UnicodeStr     - NULL terminated Unicode output string.
    AsciieStr      - NULL terminated ASCII input string.
 
  Returns: 

    Start of the Unicode ouput string.
    
--*/

{
  CHAR16      *Str = UnicodeStr;  
  while (TRUE) {
    *(UnicodeStr++) = (CHAR16) *AsciiStr;
    if (*(AsciiStr++) == '\0') {
      return Str;
    }
  }
}

VOID
ExtractDevicePathData (
  IN     EFI_HII_DATA_TABLE   *DataTable,
  IN     UINT8                *IfrData,
  IN OUT UINT8                **ExportBufferPtr
  )
/*++

Routine Description:
  
Arguments:

Returns: 

--*/
{
  UINT8 *ExportBuffer;

  ExportBuffer = *ExportBufferPtr;

  //
  // BUGBUG - don't have devicepath data yet, setting dummy value
  //
  DataTable++;
  ExportBuffer  = (VOID *) DataTable;
  ((EFI_HII_DEVICE_PATH_PACK *) ExportBuffer)->Header.Type = EFI_HII_DEVICE_PATH;
  ((EFI_HII_DEVICE_PATH_PACK *) ExportBuffer)->Header.Length = (UINT32) (sizeof (EFI_HII_DEVICE_PATH_PACK) + sizeof (EFI_DEVICE_PATH_PROTOCOL));

  //
  // BUGBUG - part of hack - skip the Device Path Pack.....place some data
  //
  ExportBuffer  = ExportBuffer + sizeof (EFI_HII_DEVICE_PATH_PACK);

  ((EFI_DEVICE_PATH_PROTOCOL *) ExportBuffer)->Type     = EFI_END_ENTIRE_DEVICE_PATH;
  ((EFI_DEVICE_PATH_PROTOCOL *) ExportBuffer)->SubType  = EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;

  //
  // BUGBUG - still part of hack....
  //
  ExportBuffer      = ExportBuffer + sizeof (EFI_DEVICE_PATH_PROTOCOL);
  *ExportBufferPtr  = ExportBuffer;
}

VOID
ExtractVariableData (
  IN OUT EFI_HII_DATA_TABLE   *DataTable,
  IN     UINT8                *IfrData,
  IN OUT UINT8                **ExportBufferPtr
  )
/*++

Routine Description:

  This function extract the EFI_HII_VARIABLE_PACK portion from the 
  each of the EFI_HII_PACKAGE_INSTANCE in HII handle database.
  
Arguments:

  DataTable       �C On input, this parameter point to the EFI_HII_DATA_TABLE structure
                    of the final data buffer for the EFI_HII_EXPORT interface. This function
                    update the NumberOfVariableData attribute.
  IfrData         - It points to a staring address of a EFI_HII_IFR_PACK structure.
  ExportBufferPtr �C On input, it points the starting address of the data buffer to 
                    host the variable pack. On output, it is the starting address
                    of data buffer for the next extraction operation.
Returns: 

  VOID
  
--*/
{
  EFI_HII_VARIABLE_PACK       *VariableContents;
  UINT8                       *ExportBuffer;
  UINTN                       Index;
  UINTN                       Index2;
  UINTN                       TempValue;
  UINTN                       TempValue2;
  EFI_FORM_CALLBACK_PROTOCOL  *FormCallback;
  EFI_PHYSICAL_ADDRESS        CallbackHandle;
  EFI_STATUS                  Status;
  CHAR16                      *String;

  FormCallback    = NULL;
  CallbackHandle  = 0;
  ExportBuffer    = *ExportBufferPtr;

  for (Index = 0; IfrData[Index] != EFI_IFR_END_FORM_SET_OP;) {
    VariableContents = (EFI_HII_VARIABLE_PACK *) ExportBuffer;

    switch (IfrData[Index]) {
    case EFI_IFR_FORM_SET_OP:
      TempValue = EFI_HII_VARIABLE;
      EfiCopyMem (&VariableContents->Header.Type, &TempValue, sizeof (UINT16));
      EfiCopyMem (&TempValue, &((EFI_IFR_FORM_SET *) &IfrData[Index])->NvDataSize, sizeof (UINT16));

      //
      // If the variable has 0 size, do not process it
      //
      if (TempValue == 0) {
        break;
      }
      //
      // Add the size of the variable pack overhead.  Later, will also add the size of the
      // name of the variable.
      //
      TempValue = TempValue + sizeof (EFI_HII_VARIABLE_PACK);

      EfiCopyMem (&VariableContents->Header.Length, &TempValue, sizeof (UINT32));
      EfiCopyMem (
        &CallbackHandle,
        &((EFI_IFR_FORM_SET *) &IfrData[Index])->CallbackHandle,
        sizeof (EFI_PHYSICAL_ADDRESS)
        );
      if (CallbackHandle != 0) {
        Status = gBS->HandleProtocol (
                        (EFI_HANDLE) (UINTN) CallbackHandle,
                        &gEfiFormCallbackProtocolGuid,
                        &FormCallback
                        );
      }
      //
      // Since we have a "Setup" variable that wasn't specified by a variable op-code
      // it will have a VariableId of 0.  All other variable op-codes will have a designation
      // of VariableId 1+
      //
      TempValue = 0;
      EfiCopyMem (&VariableContents->VariableId, &TempValue, sizeof (UINT16));
      EfiCopyMem (&VariableContents->VariableGuid, &((EFI_IFR_FORM_SET *) &IfrData[Index])->Guid, sizeof (EFI_GUID));
      TempValue = sizeof (L"Setup");
      EfiCopyMem (&VariableContents->VariableNameLength, &TempValue, sizeof (UINT32));

      //
      // Add the size of the name to the Header Length
      //
      TempValue2 = 0;
      EfiCopyMem (&TempValue2, &VariableContents->Header.Length, sizeof (UINT32));
      TempValue2 = TempValue + TempValue2;
      EfiCopyMem (&VariableContents->Header.Length, &TempValue2, sizeof (UINT32));

      ExportBuffer = ExportBuffer + sizeof (EFI_HII_VARIABLE_PACK);
      EfiCopyMem (ExportBuffer, L"Setup", sizeof (L"Setup"));
      ExportBuffer = ExportBuffer + sizeof (L"Setup");

      EfiCopyMem (&TempValue, &((EFI_IFR_FORM_SET *) &IfrData[Index])->NvDataSize, sizeof (UINT16));

      if ((FormCallback != NULL) && (FormCallback->NvRead != NULL)) {
        Status = FormCallback->NvRead (
                                 FormCallback,
                                 L"Setup",
                                 &VariableContents->VariableGuid,
                                 NULL,
                                 &TempValue,
                                 ExportBuffer
                                 );
      } else {
        Status = gRT->GetVariable (
                        L"Setup",
                        &VariableContents->VariableGuid,
                        NULL,
                        &TempValue,
                        ExportBuffer
                        );
      }

      ExportBuffer = (UINT8 *) (UINTN) (((UINTN) ExportBuffer) + TempValue);
      DataTable->NumberOfVariableData++;
      break;

    case EFI_IFR_VARSTORE_OP:
      TempValue = EFI_HII_VARIABLE;
      EfiCopyMem (&VariableContents->Header.Type, &TempValue, sizeof (UINT16));
      EfiCopyMem (&TempValue, &((EFI_IFR_VARSTORE *) &IfrData[Index])->Size, sizeof (UINT16));

      //
      // If the variable has 0 size, do not process it
      //
      if (TempValue == 0) {
        break;
      }
      //
      // Add the size of the variable pack overhead.  Later, will also add the size of the
      // name of the variable.
      //
      TempValue = TempValue + sizeof (EFI_HII_VARIABLE_PACK);

      EfiCopyMem (&VariableContents->Header.Length, &TempValue, sizeof (UINT32));
      EfiCopyMem (&VariableContents->VariableId, &((EFI_IFR_VARSTORE *) &IfrData[Index])->VarId, sizeof (UINT16));
      EfiCopyMem (&VariableContents->VariableGuid, &((EFI_IFR_VARSTORE *) &IfrData[Index])->Guid, sizeof (EFI_GUID));
      TempValue = (UINTN) ((EFI_IFR_VARSTORE *) &IfrData[Index])->Header.Length - sizeof (EFI_IFR_VARSTORE);
      TempValue = TempValue * 2;
      EfiCopyMem (&VariableContents->VariableNameLength, &TempValue, sizeof (UINT32));

      //
      // Add the size of the name to the Header Length
      //
      TempValue2 = 0;
      EfiCopyMem (&TempValue2, &VariableContents->Header.Length, sizeof (UINT32));
      TempValue2 = TempValue + TempValue2;
      EfiCopyMem (&VariableContents->Header.Length, &TempValue2, sizeof (UINT32));

      ExportBuffer  = ExportBuffer + sizeof (EFI_HII_VARIABLE_PACK);
      String        = (CHAR16 *) ExportBuffer;
      for (Index2 = 0; Index2 < TempValue / 2; Index2++) {
        ExportBuffer[Index2 * 2]      = IfrData[Index + sizeof (EFI_IFR_VARSTORE) + Index2];
        ExportBuffer[Index2 * 2 + 1]  = 0;
      }

      ExportBuffer = ExportBuffer + TempValue;

      EfiCopyMem (&TempValue, &((EFI_IFR_VARSTORE *) &IfrData[Index])->Size, sizeof (UINT16));

      if ((FormCallback != NULL) && (FormCallback->NvRead != NULL)) {
        Status = FormCallback->NvRead (
                                 FormCallback,
                                 String,
                                 &VariableContents->VariableGuid,
                                 NULL,
                                 &TempValue,
                                 ExportBuffer
                                 );
      } else {
        Status = gRT->GetVariable (
                        String,
                        &VariableContents->VariableGuid,
                        NULL,
                        &TempValue,
                        ExportBuffer
                        );
      }

      ExportBuffer = (UINT8 *) (UINTN) (((UINTN) ExportBuffer) + TempValue);
      DataTable->NumberOfVariableData++;
      break;
    }

    Index = IfrData[Index + 1] + Index;
  }
  //
  // If we have added a variable pack, add a dummy empty one to signify the end
  //
  if (ExportBuffer != *ExportBufferPtr) {
    VariableContents  = (EFI_HII_VARIABLE_PACK *) ExportBuffer;
    TempValue         = EFI_HII_VARIABLE;
    EfiCopyMem (&VariableContents->Header.Type, &TempValue, sizeof (UINT16));
    TempValue = sizeof (EFI_HII_VARIABLE_PACK);
    EfiCopyMem (&VariableContents->Header.Length, &TempValue, sizeof (UINT32));
    ExportBuffer = ExportBuffer + sizeof (EFI_HII_VARIABLE_PACK);
  }

  *ExportBufferPtr = ExportBuffer;
}

EFI_STATUS
EFIAPI
HiiExportDatabase (
  IN     EFI_HII_PROTOCOL *This,
  IN     EFI_HII_HANDLE   Handle,
  IN OUT UINTN            *BufferSize,
  OUT    VOID             *Buffer
  )
/*++

Routine Description:
  
  This function allows a program to extract a form or form package that has 
  previously been registered with the EFI HII database.

Arguments:

Returns: 

--*/
{
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_IFR_PACK          *FormPack;
  UINT8                     *RawData;
  UINT8                     *ExportBuffer;
  EFI_HII_EXPORT_TABLE      *ExportTable;
  EFI_HII_DATA_TABLE        *DataTable;
  BOOLEAN                   InsufficientSize;
  BOOLEAN                   VariableExist;
  UINT16                    NumberOfHiiDataTables;
  UINTN                     SizeNeeded;
  UINTN                     Index;
  UINTN                     VariableSize;
  UINTN                     TempValue;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData               = EFI_HII_DATA_FROM_THIS (This);

  HandleDatabase        = HiiData->DatabaseHead;

  FormPack              = NULL;
  RawData               = NULL;
  PackageInstance       = NULL;
  InsufficientSize      = FALSE;
  NumberOfHiiDataTables = 0;
  VariableSize          = 0;
  TempValue             = 0;
  SizeNeeded            = sizeof (EFI_HII_EXPORT_TABLE);

  //
  // How many total tables are there?
  //
  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    if ((Handle != 0) && (Handle != HandleDatabase->Handle)) {
      continue;
    }

    VariableExist = FALSE;
    NumberOfHiiDataTables++;
    PackageInstance = HandleDatabase->Buffer;
    if (PackageInstance == NULL) {
      continue;
    }
    //
    // Extract Size of Export Package
    //
    SizeNeeded = SizeNeeded + PackageInstance->IfrSize 
                            + PackageInstance->StringSize
                            + sizeof (EFI_HII_DATA_TABLE)
                            + sizeof (EFI_HII_DEVICE_PATH_PACK);

    //
    // BUGBUG We aren't inserting Device path data yet
    //
    SizeNeeded = SizeNeeded + sizeof (EFI_DEVICE_PATH_PROTOCOL);

    //
    // Extract Size of Variable Data
    //
    if (PackageInstance->IfrSize > 0) {
      FormPack = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));
    } else {
      //
      // No IFR? No variable information
      //
      continue;
    }

    RawData = (UINT8 *) FormPack;

    for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
      switch (RawData[Index]) {
      case EFI_IFR_FORM_SET_OP:
        EfiCopyMem (&VariableSize, &((EFI_IFR_FORM_SET *) &RawData[Index])->NvDataSize, sizeof (UINT16));
        SizeNeeded    = SizeNeeded + VariableSize + sizeof (L"Setup") + sizeof (EFI_HII_VARIABLE_PACK);
        VariableExist = TRUE;
        break;

      case EFI_IFR_VARSTORE_OP:
        EfiCopyMem (&VariableSize, &((EFI_IFR_VARSTORE *) &RawData[Index])->Size, sizeof (UINT16));
        SizeNeeded = SizeNeeded + VariableSize + sizeof (EFI_HII_VARIABLE_PACK);
        //
        // We will be expanding the stored ASCII name to a Unicode string.  This will cause some memory overhead
        // Since the VARSTORE size already takes in consideration the ASCII size, we need to size it and add another
        // instance of it.  Essentially, 2 ASCII strings == 1 Unicode string in size.
        //
        TempValue     = (UINTN) ((EFI_IFR_VARSTORE *) &RawData[Index])->Header.Length - sizeof (EFI_IFR_VARSTORE);
        SizeNeeded    = SizeNeeded + TempValue * 2;
        VariableExist = TRUE;
        break;
      }

      Index = RawData[Index + 1] + Index;
    }
    //
    // If a variable exists for this handle, add an additional variable pack overhead to
    // indicate that we will have an extra null Variable Pack to signify the end of the Variable Packs
    //
    if (VariableExist) {
      SizeNeeded = SizeNeeded + sizeof (EFI_HII_VARIABLE_PACK);
    }
  }

  if (SizeNeeded > *BufferSize) {
    *BufferSize = SizeNeeded;
    return EFI_BUFFER_TOO_SMALL;
  }
  //
  // Zero out the incoming buffer
  //
  EfiZeroMem (Buffer, *BufferSize);

  //
  // Cast the Buffer to EFI_HII_EXPORT_TABLE
  //
  ExportTable = (EFI_HII_EXPORT_TABLE *) Buffer;

  //
  // Set the Revision for the Export Table
  //
  EfiCopyMem (&ExportTable->Revision, &gEfiHiiProtocolGuid, sizeof (EFI_GUID));

  ExportBuffer    = (UINT8 *) (UINTN) (((UINT8 *) ExportTable) + sizeof (EFI_HII_EXPORT_TABLE));
  HandleDatabase  = HiiData->DatabaseHead;

  //
  // Check numeric value against the head of the database
  //
  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    DataTable       = (EFI_HII_DATA_TABLE *) ExportBuffer;
    PackageInstance = HandleDatabase->Buffer;
    //
    // If not asking for a specific handle, export the entire database
    //
    if (Handle == 0) {
      ExportTable->NumberOfHiiDataTables = NumberOfHiiDataTables;
      EfiCopyMem (&DataTable->PackageGuid, &PackageInstance->Guid, sizeof (EFI_GUID));
      DataTable->HiiHandle        = PackageInstance->Handle;
      DataTable->DevicePathOffset = (UINT32) (sizeof (EFI_HII_DATA_TABLE));

      //
      // Start Dumping DevicePath
      //
      ExtractDevicePathData (DataTable, RawData, &ExportBuffer);

      if (((UINTN) ExportBuffer) == ((UINTN) DataTable)) {
        //
        // If there is no DevicePath information - set offset to 0 to signify the absence of data to parse
        //
        DataTable->DevicePathOffset = 0;
      }

      DataTable->VariableDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

      if (PackageInstance->IfrSize > 0) {
        FormPack  = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));

        RawData   = (UINT8 *) FormPack;
        TempValue = 0;

        //
        // Start dumping the Variable Data
        //
        ExtractVariableData (DataTable, RawData, &ExportBuffer);
        DataTable->IfrDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

        if (DataTable->VariableDataOffset == DataTable->IfrDataOffset) {
          DataTable->VariableDataOffset = 0;
        }
        //
        // Start dumping the IFR data (Note:  It is in an IFR PACK)
        //
        EfiCopyMem (ExportBuffer, &PackageInstance->IfrData, PackageInstance->IfrSize);
        ExportBuffer                = (UINT8 *) (UINTN) (((UINTN) ExportBuffer) + PackageInstance->IfrSize);
        DataTable->StringDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

        //
        // Start dumping the String data (Note:  It is in a String PACK)
        //
        if (PackageInstance->StringSize > 0) {
          RawData = (UINT8 *) (((UINTN) &PackageInstance->IfrData) + PackageInstance->IfrSize);
          EfiCopyMem (ExportBuffer, RawData, PackageInstance->StringSize);
          DataTable->DataTableSize = (UINT32) (DataTable->StringDataOffset + PackageInstance->StringSize);

          EfiCopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
          for (; TempValue != 0;) {
            DataTable->NumberOfLanguages++;
            ExportBuffer = ExportBuffer + ((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length;
            EfiCopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
          }

          ExportBuffer = ExportBuffer + sizeof (EFI_HII_STRING_PACK);
        } else {
          DataTable->StringDataOffset = 0;
        }
      } else {
        //
        // No IFR? No variable information.  If Offset is 0, means there is none.  (Hmm - this might be prunable - no strings to export if no IFR - we always have a stub)
        //
        DataTable->VariableDataOffset = 0;
        DataTable->IfrDataOffset      = 0;
        DataTable->StringDataOffset   = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

        //
        // Start dumping the String data - NOTE:  It is in String Pack form
        //
        if (PackageInstance->StringSize > 0) {
          RawData = (UINT8 *) (((UINTN) &PackageInstance->IfrData) + PackageInstance->IfrSize);
          EfiCopyMem (ExportBuffer, RawData, PackageInstance->StringSize);
          DataTable->DataTableSize = (UINT32) (DataTable->StringDataOffset + PackageInstance->StringSize);

          EfiCopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
          for (; TempValue != 0;) {
            DataTable->NumberOfLanguages++;
            ExportBuffer = ExportBuffer + ((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length;
            EfiCopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
          }

          ExportBuffer = ExportBuffer + sizeof (EFI_HII_STRING_PACK);
        } else {
          DataTable->StringDataOffset = 0;
        }
      }
    } else {
      //
      // Match the numeric value with the database entry - if matched, extract PackageInstance
      //
      if (Handle == HandleDatabase->Handle) {
        PackageInstance                     = HandleDatabase->Buffer;
        ExportTable->NumberOfHiiDataTables  = NumberOfHiiDataTables;
        DataTable->HiiHandle                = PackageInstance->Handle;
        EfiCopyMem (&DataTable->PackageGuid, &PackageInstance->Guid, sizeof (EFI_GUID));

        //
        // Start Dumping DevicePath
        //
        ExtractDevicePathData (DataTable, RawData, &ExportBuffer);
        DataTable->VariableDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

        if (PackageInstance->IfrSize > 0) {
          FormPack  = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));

          RawData   = (UINT8 *) FormPack;
          TempValue = 0;

          //
          // Start dumping the Variable Data
          //
          ExtractVariableData (DataTable, RawData, &ExportBuffer);
          DataTable->IfrDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

          if (DataTable->VariableDataOffset == DataTable->IfrDataOffset) {
            DataTable->VariableDataOffset = 0;
          }
          //
          // Start dumping the IFR data
          //
          EfiCopyMem (ExportBuffer, &PackageInstance->IfrData, PackageInstance->IfrSize);
          ExportBuffer                = (UINT8 *) (UINTN) (((UINTN) ExportBuffer) + PackageInstance->IfrSize);
          DataTable->StringDataOffset = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

          //
          // Start dumping the String data - NOTE:  It is in String Pack form
          //
          if (PackageInstance->StringSize > 0) {
            RawData = (UINT8 *) (((UINTN) &PackageInstance->IfrData) + PackageInstance->IfrSize);
            EfiCopyMem (ExportBuffer, RawData, PackageInstance->StringSize);
            DataTable->DataTableSize = (UINT32) (DataTable->StringDataOffset + PackageInstance->StringSize);

            EfiCopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
            for (; TempValue != 0;) {
              DataTable->NumberOfLanguages++;
              ExportBuffer = ExportBuffer + ((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length;
              EfiCopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
            }

            ExportBuffer = ExportBuffer + sizeof (EFI_HII_STRING_PACK);
          } else {
            DataTable->StringDataOffset = 0;
          }
        } else {
          //
          // No IFR? No variable information.  If Offset is 0, means there is none.
          //
          DataTable->VariableDataOffset = 0;
          DataTable->IfrDataOffset      = 0;
          DataTable->StringDataOffset   = (UINT32) (((UINTN) ExportBuffer) - ((UINTN) DataTable));

          //
          // Start dumping the String data - Note:  It is in String Pack form
          //
          if (PackageInstance->StringSize > 0) {
            RawData = (UINT8 *) (((UINTN) &PackageInstance->IfrData) + PackageInstance->IfrSize);
            EfiCopyMem (ExportBuffer, RawData, PackageInstance->StringSize);
            DataTable->DataTableSize = (UINT32) (DataTable->StringDataOffset + PackageInstance->StringSize);

            EfiCopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
            for (; TempValue != 0;) {
              DataTable->NumberOfLanguages++;
              ExportBuffer = ExportBuffer + ((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length;
              EfiCopyMem (&TempValue, &((EFI_HII_STRING_PACK *) ExportBuffer)->Header.Length, sizeof (UINT32));
            }

            ExportBuffer = ExportBuffer + sizeof (EFI_HII_STRING_PACK);
          } else {
            DataTable->StringDataOffset = 0;
          }
        }
        break;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiGetForms (
  IN     EFI_HII_PROTOCOL   *This,
  IN     EFI_HII_HANDLE     Handle,
  IN     EFI_FORM_ID        FormId,
  IN OUT UINT16             *BufferLength,
  OUT    UINT8              *Buffer
  )
/*++

Routine Description:
  
  This function allows a program to extract a form or form package that has 
  previously been registered with the EFI HII database.

Arguments:

Returns: 

--*/
{
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_IFR_PACK          *FormPack;
  EFI_IFR_FORM              *Form;
  EFI_IFR_OP_HEADER         *Location;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData         = EFI_HII_DATA_FROM_THIS (This);

  HandleDatabase  = HiiData->DatabaseHead;

  PackageInstance = NULL;

  //
  // Check numeric value against the head of the database
  //
  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    //
    // Match the numeric value with the database entry - if matched, extract PackageInstance
    //
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;
      break;
    }
  }
  //
  // No handle was found - error condition
  //
  if (PackageInstance == NULL) {
    return EFI_NOT_FOUND;
  }
  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
  if (PackageInstance->IfrSize > 0) {
    FormPack = (EFI_HII_IFR_PACK *) (&PackageInstance->IfrData);
  } else {
    //
    // If there is no IFR data return an error
    //
    return EFI_NOT_FOUND;
  }
  //
  // If requesting the entire Form Package
  //
  if (FormId == 0) {
    //
    // Return an error if buffer is too small
    //
    if (PackageInstance->IfrSize > *BufferLength) {
      *BufferLength = (UINT16) PackageInstance->IfrSize;
      return EFI_BUFFER_TOO_SMALL;
    }

    EfiCopyMem (Buffer, FormPack, PackageInstance->IfrSize);
    return EFI_SUCCESS;
  } else {
    FormPack  = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));
    Location  = (EFI_IFR_OP_HEADER *) FormPack;

    //
    // Look for the FormId requested
    //
    for (; Location->OpCode != EFI_IFR_END_FORM_SET_OP;) {
      switch (Location->OpCode) {
      case EFI_IFR_FORM_OP:
        Form = (EFI_IFR_FORM *) Location;

        //
        // If we found a Form Op-code and it is of the correct Id, copy it and return
        //
        if (Form->FormId == FormId) {
          if (Location->Length > *BufferLength) {
            *BufferLength = Location->Length;
            return EFI_BUFFER_TOO_SMALL;
          } else {
            for (; Location->OpCode != EFI_IFR_END_FORM_OP;) {
              EfiCopyMem (Buffer, Location, Location->Length);
              Buffer    = Buffer + Location->Length;
              Location  = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
            }

            EfiCopyMem (Buffer, Location, Location->Length);
            return EFI_SUCCESS;
          }
        }

      default:
        break;
      }
      //
      // Go to the next Op-Code
      //
      Location = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
    }
  }

  return EFI_SUCCESS;
}

//
// Helper functions to HiiGetDefaultImage()
//

STATIC
UINT8*
HiiGetDefaultImageInitPack (
  IN OUT EFI_HII_VARIABLE_PACK_LIST  *VariablePackItem,
  IN     EFI_IFR_VARSTORE            *VarStore,
  IN     UINT8                       *VarName            OPTIONAL
  )
/*++
    
  Routine Description:

    Initialize the EFI_HII_VARIABLE_PACK_LIST structure and
    prepare it ready to be used by HiiGetDefaultImagePopulateMap ().
      
  Arguments:

    VariablePackItem     - Variable Package List.
    VarStore             - IFR variable storage.
    VarName              - Variable name used to retrive the VariablePackItem.
   
  Returns: 

    Return the pointer to the Map space.
      
--*/
{
  CHAR16   *Name16;
  CHAR8    *Map;

  //
  // Set pointer the pack right after the node
  //
  VariablePackItem->VariablePack = (VOID *) (VariablePackItem + 1);

  //
  // Copy the var name from VariablePackItem. Needs ASCII->Unicode conversion.
  // If the name is not provided, extract it from the VarStore
  //
  if (NULL == VarName) {
    ASSERT (VarStore->Header.Length > sizeof (*VarStore));
    VarName  = (VOID *) (VarStore + 1);
  } else {
    ASSERT (VarStore->Header.Length == sizeof (*VarStore));
  }

  Name16 = (VOID *) (VariablePackItem->VariablePack + 1);
  Ascii2Unicode (Name16, VarName);

  //
  // Compute the other fields of the VariablePackItem
  //
  VariablePackItem->VariablePack->VariableId         = VarStore->VarId;
  EfiCopyMem (&VariablePackItem->VariablePack->VariableGuid, &VarStore->Guid, sizeof (EFI_GUID));
  VariablePackItem->VariablePack->VariableNameLength = (UINT32)((EfiStrLen (Name16) + 1) * 2);
  VariablePackItem->VariablePack->Header.Length      = sizeof (*VariablePackItem->VariablePack) 
                                                         + VariablePackItem->VariablePack->VariableNameLength
                                                         + VarStore->Size;
  //
  // Return the pointer to the Map space.
  //
  Map = (UINT8 *) Name16 + VariablePackItem->VariablePack->VariableNameLength;
  return Map;
}

STATIC
VOID
HiiGetDefaultImagePopulateMap (
  IN OUT UINT8                        *Map,  
  IN     EFI_IFR_OP_HEADER            *FormSet,
  IN     EFI_IFR_VARSTORE             *VarStore,
  IN     UINTN                        DefaultMask
  )
/*++
    
  Routine Description:

   Fill the Map with all the default values either from NV or Hii database.
      
  Arguments:

   Map         - Memory pointer to hold the default values.
   FormSet     - The starting EFI_IFR_OP_HEADER to begin retriving default values.
   VarStore    - IFR variable storage.
   DefaultMask - The mask used to get the default variable.
   
  Returns: 

   VOID
      
--*/
{
  EFI_STATUS                     Status;
  EFI_IFR_OP_HEADER              *IfrItem;
  UINT16                         VarId;
  EFI_IFR_VARSTORE_SELECT        *VarSelect;
  EFI_IFR_VARSTORE_SELECT_PAIR   *VarSelectPair;
  EFI_IFR_ONE_OF_OPTION          *OneOfOpt;
  EFI_IFR_CHECK_BOX              *CheckBox;
  EFI_IFR_NUMERIC                *Numeric;
  UINTN                          Size;
  EFI_IFR_NV_DATA                *IfrNvData;
  EFI_GUID                       Guid;

  //
  // First, check if the map exists in the NV. If so, get it from NV and exit.
  //
  Size = VarStore->Size;
  EfiCopyMem (&Guid, &VarStore->Guid, sizeof (EFI_GUID));
  if (DefaultMask == EFI_IFR_FLAG_MANUFACTURING) {
    Status = gRT->GetVariable (
                    SETUP_MANUFACTURING_OVERRIDE_NAME,
                    &Guid,
                    NULL,
                    &Size,
                    Map
                    );
  } else {
    Status = gRT->GetVariable (
                    SETUP_DEFAULT_OVERRIDE_NAME,
                    &Guid,
                    NULL,
                    &Size,
                    Map
                    );
  }
  if (!EFI_ERROR (Status)) {
    return;
  }


  VarId = 0;
  IfrItem = FormSet;
  while (EFI_IFR_END_FORM_SET_OP != IfrItem->OpCode) {
 
    //
    // Observe VarStore select.
    //
    switch (IfrItem->OpCode) {

      case EFI_IFR_VARSTORE_SELECT_OP:
        VarSelect = (VOID *) IfrItem;
        VarId = VarSelect->VarId;
        goto LabelContinue;
      case EFI_IFR_VARSTORE_SELECT_PAIR_OP:
        VarSelectPair = (VOID *) IfrItem;
        VarId = VarSelectPair->VarId;
        goto LabelContinue;
    }


    //
    // For opcodes that reference the same VarStore than current, extreact the default value to the map.
    // 
    if (VarId == VarStore->VarId) {

      IfrNvData = (VOID *) IfrItem;

      switch (IfrItem->OpCode) {

        case EFI_IFR_ONE_OF_OP:
          ASSERT (IfrNvData->QuestionId + IfrNvData->StorageWidth <= VarStore->Size);
          //
          // Get to the first EFI_IFR_ONE_OF_OPTION_OP
          //
          IfrItem = (VOID *) ((UINT8 *) IfrItem + IfrItem->Length); 
          ASSERT (EFI_IFR_ONE_OF_OPTION_OP == IfrItem->OpCode);

          OneOfOpt = (VOID *) IfrItem;
          //
          // In the worst case, the first will be the default.
          //
          EfiCopyMem (Map + IfrNvData->QuestionId, &OneOfOpt->Value, IfrNvData->StorageWidth);

          while (EFI_IFR_ONE_OF_OPTION_OP == IfrItem->OpCode) {

            OneOfOpt = (VOID *) IfrItem;
            if (DefaultMask == EFI_IFR_FLAG_MANUFACTURING) {
              if (0 != (OneOfOpt->Flags & EFI_IFR_FLAG_MANUFACTURING)) {
                //
                // In the worst case, the first will be the default.
                //
                EfiCopyMem (Map + IfrNvData->QuestionId, &OneOfOpt->Value, IfrNvData->StorageWidth);
                break;
              }
            } else {
              if (OneOfOpt->Flags & EFI_IFR_FLAG_DEFAULT) {
                //
                // In the worst case, the first will be the default.
                //
                EfiCopyMem (Map + IfrNvData->QuestionId, &OneOfOpt->Value, IfrNvData->StorageWidth);
                break;
              }
            }

            IfrItem = (VOID *) ((UINT8 *) IfrItem + IfrItem->Length);
          }
          continue;
          break;

        case EFI_IFR_CHECKBOX_OP:
          ASSERT (IfrNvData->QuestionId + IfrNvData->StorageWidth <= VarStore->Size);
          CheckBox = (VOID *) IfrItem;        
          if (DefaultMask == EFI_IFR_FLAG_MANUFACTURING) {
            if (0 != (CheckBox->Flags & EFI_IFR_FLAG_MANUFACTURING)) {
              *(UINT8 *) (Map + IfrNvData->QuestionId) = TRUE;
            }
          } else {
            if (CheckBox->Flags & EFI_IFR_FLAG_DEFAULT) {
              *(UINT8 *) (Map + IfrNvData->QuestionId) = TRUE;
            }
          }
          break;

        case EFI_IFR_NUMERIC_OP:
          ASSERT (IfrNvData->QuestionId + IfrNvData->StorageWidth <= VarStore->Size);
          Numeric = (VOID *) IfrItem;
          EfiCopyMem (Map + IfrNvData->QuestionId, &Numeric->Default, IfrNvData->StorageWidth);
          break;

        case EFI_IFR_ORDERED_LIST_OP:
        case EFI_IFR_PASSWORD_OP:
        case EFI_IFR_STRING_OP:
          //
          // No support for default falue for these opcodes.
          //
          ;
      }
    }

LabelContinue:
    IfrItem = (VOID *) ((UINT8 *) IfrItem + IfrItem->Length);
  }
}


EFI_STATUS
EFIAPI
HiiGetDefaultImage (
  IN     EFI_HII_PROTOCOL            *This,
  IN     EFI_HII_HANDLE              Handle,
  IN     UINTN                       DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST  **VariablePackList
  )
/*++
    
  Routine Description:

  This function allows a program to extract the NV Image 
  that represents the default storage image
      
  Arguments:
    This             - NULL terminated Unicode output string.
    Handle           - NULL terminated ASCII input string.
    UINTN            - Mask used to retrieve the default image.
    VariablePackList - Callee allocated, tightly-packed, link list data 
                         structure that contain all default varaible packs
                         from the Hii Database.
    
  Returns: 
    EFI_NOT_FOUND         - If Hii database does not contain any default images.
    EFI_INVALID_PARAMETER - Invalid input parameter.
    EFI_SUCCESS           - Operation successful.
      
--*/
{
  EFI_HII_HANDLE_DATABASE        *HandleDatabase;
  EFI_HII_PACKAGE_INSTANCE       *PackageInstance;
  EFI_IFR_OP_HEADER              *FormSet;
  EFI_IFR_OP_HEADER              *IfrItem;
  EFI_IFR_VARSTORE               *VarStore;
  EFI_IFR_VARSTORE               VarStoreDefault;
  UINTN                          SizeOfMaps;
  EFI_HII_VARIABLE_PACK_LIST     *PackList;
  EFI_HII_VARIABLE_PACK_LIST     *PackListNext;  
  EFI_HII_VARIABLE_PACK_LIST     *PackListLast;
  UINT8                          *Map;


  //
  // Find the IFR pack from the handle. Then get the formset from the pack.
  //
  PackageInstance = NULL;
  HandleDatabase  = (EFI_HII_DATA_FROM_THIS (This))->DatabaseHead;
  for ( ; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;
      break;
    }
  }
  if ((PackageInstance == NULL) || (PackageInstance->IfrSize < 0)) {
    return EFI_INVALID_PARAMETER;
  }
  FormSet = (VOID *) ((UINT8 *) &PackageInstance->IfrData + sizeof (EFI_HII_IFR_PACK));

  //
  // Get the sizes of all the VARSTOREs in this VFR.
  // Then allocate enough space for all of them plus all maps
  //
  SizeOfMaps = 0;
  IfrItem    = FormSet;
  while (EFI_IFR_END_FORM_SET_OP != IfrItem->OpCode) {

    if (EFI_IFR_VARSTORE_OP == IfrItem->OpCode) {
      VarStore = (VOID *) IfrItem;
      //
      // Size of the map
      //
      SizeOfMaps += VarStore->Size; 
      //
      // add the size of the string, in Unicode
      //
      SizeOfMaps += (VarStore->Header.Length - sizeof (*VarStore)) * 2; 
      //
      // Space for node
      //
      SizeOfMaps += sizeof (EFI_HII_VARIABLE_PACK);      
      //
      // Space for linked list node 
      //
      SizeOfMaps += sizeof (EFI_HII_VARIABLE_PACK_LIST); 
    }

    IfrItem = (VOID *) ((UINT8 *) IfrItem + IfrItem->Length);
  }

  //
  // If the FormSet OpCode has a non-zero NvDataSize. There is a default 
  // NvMap with ID=0, GUID that of the formset itself and "Setup" as name.
  //
  EfiZeroMem (&VarStoreDefault, sizeof (VarStoreDefault));

  if (0 != ((EFI_IFR_FORM_SET*)FormSet)->NvDataSize) {
    VarStoreDefault.Size = ((EFI_IFR_FORM_SET*) FormSet)->NvDataSize;
    VarStoreDefault.Header.OpCode = EFI_IFR_VARSTORE_OP;
    VarStoreDefault.Header.Length = sizeof (VarStoreDefault);
    EfiCopyMem (&VarStoreDefault.Guid, &((EFI_IFR_FORM_SET*) FormSet)->Guid, sizeof (EFI_GUID));
    VarStoreDefault.VarId = 0;
    //
    // Size of the map
    //
    SizeOfMaps += VarStoreDefault.Size; 
    //
    // add the size of the string
    //
    SizeOfMaps += sizeof (SETUP_MAP_NAME); 
    //
    // Space for node
    //
    SizeOfMaps += sizeof (EFI_HII_VARIABLE_PACK);      
    //
    // Space for linked list node 
    //
    SizeOfMaps += sizeof (EFI_HII_VARIABLE_PACK_LIST); 
  }

  if (0 == SizeOfMaps) {
    //
    // The IFR does not have any explicit or default map(s).
    //
    return EFI_NOT_FOUND; 
  }

  //
  // Allocate the return buffer
  //
  PackList = EfiLibAllocateZeroPool (SizeOfMaps);
  ASSERT (NULL != PackList); 

  PackListNext = PackList;
  PackListLast = PackList;

  //
  // Handle the default map first, if any.
  //
  if (0 != VarStoreDefault.Size) {

    Map = HiiGetDefaultImageInitPack (PackListNext, &VarStoreDefault, "Setup");

    HiiGetDefaultImagePopulateMap (Map, FormSet, &VarStoreDefault, DefaultMask);

    PackListNext->NextVariablePack = (VOID *) ((UINT8 *) PackListNext->VariablePack + PackListNext->VariablePack->Header.Length);
    PackListLast = PackListNext;
    PackListNext = PackListNext->NextVariablePack;
  }


  //
  // Handle the explicit varstore(s)
  //
  IfrItem = FormSet;
  while (EFI_IFR_END_FORM_SET_OP != IfrItem->OpCode) {

    if (EFI_IFR_VARSTORE_OP == IfrItem->OpCode) {

      Map = HiiGetDefaultImageInitPack (PackListNext, (VOID *) IfrItem, NULL);

      HiiGetDefaultImagePopulateMap (Map, FormSet, (VOID *) IfrItem, DefaultMask);

      PackListNext->NextVariablePack = (VOID *) ((UINT8 *) PackListNext->VariablePack + PackListNext->VariablePack->Header.Length);
      PackListLast = PackListNext;
      PackListNext = PackListNext->NextVariablePack;
    }

    IfrItem = (VOID *) ((UINT8 *) IfrItem + IfrItem->Length);
  }

  PackListLast->NextVariablePack = NULL;
  *VariablePackList = PackList;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
HiiUpdateForm (
  IN EFI_HII_PROTOCOL       *This,
  IN EFI_HII_HANDLE         Handle,
  IN EFI_FORM_LABEL         Label,
  IN BOOLEAN                AddData,
  IN EFI_HII_UPDATE_DATA    *Data
  )
/*++

Routine Description:
  This function allows the caller to update a form that has 
  previously been registered with the EFI HII database.

Arguments:
  Handle     - Hii Handle associated with the Formset to modify
  Label      - Update information starting immediately after this label in the IFR
  AddData    - If TRUE, add data.  If FALSE, remove data
  Data       - If adding data, this is the pointer to the data to add

Returns: 
  EFI_SUCCESS - Update success.
  Other       - Update fail.

--*/
{
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_IFR_PACK          *FormPack;
  EFI_IFR_OP_HEADER         *Location;
  EFI_IFR_OP_HEADER         *DataLocation;
  UINT8                     *OtherBuffer;
  UINT8                     *TempBuffer;
  UINT8                     *OrigTempBuffer;
  UINTN                     TempBufferSize;
  UINTN                     Index;

  OtherBuffer = NULL;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData         = EFI_HII_DATA_FROM_THIS (This);

  HandleDatabase  = HiiData->DatabaseHead;

  PackageInstance = NULL;

  //
  // Check numeric value against the head of the database
  //
  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    //
    // Match the numeric value with the database entry - if matched, extract PackageInstance
    //
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;
      break;
    }
  }
  //
  // No handle was found - error condition
  //
  if (PackageInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Calculate and allocate space for retrieval of IFR data
  //
  DataLocation    = (EFI_IFR_OP_HEADER *) &Data->Data;
  TempBufferSize  = (CHAR8 *) (&PackageInstance->IfrData) - (CHAR8 *) (PackageInstance);

  for (Index = 0; Index < Data->DataCount; Index++) {
    TempBufferSize += DataLocation->Length;
    DataLocation = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (DataLocation) + DataLocation->Length);
  }

  TempBufferSize += PackageInstance->IfrSize + PackageInstance->StringSize;

  TempBuffer      = EfiLibAllocateZeroPool (TempBufferSize);
  OrigTempBuffer  = TempBuffer;

  //
  // We update only packages with IFR information in it
  //
  if (PackageInstance->IfrSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  EfiCopyMem (
    TempBuffer,
    PackageInstance,
    ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER) - (CHAR8 *) (PackageInstance))
    );

  TempBuffer = TempBuffer + ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER) - (CHAR8 *) (PackageInstance));

  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
  FormPack  = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));
  Location  = (EFI_IFR_OP_HEADER *) FormPack;

  //
  // Look for the FormId requested
  //
  for (; Location->OpCode != EFI_IFR_END_FORM_SET_OP;) {
    switch (Location->OpCode) {
    case EFI_IFR_FORM_SET_OP:
      //
      // If the FormSet has an update pending, pay attention.
      //
      if (Data->FormSetUpdate) {
        ((EFI_IFR_FORM_SET *) Location)->CallbackHandle = Data->FormCallbackHandle;
      }

      EfiCopyMem (TempBuffer, Location, Location->Length);
      TempBuffer = TempBuffer + Location->Length;
      break;

    case EFI_IFR_FORM_OP:
      //
      // If the Form has an update pending, pay attention.
      //
      if (Data->FormUpdate) {
        ((EFI_IFR_FORM *) Location)->FormTitle = Data->FormTitle;
      }

      EfiCopyMem (TempBuffer, Location, Location->Length);
      TempBuffer = TempBuffer + Location->Length;
      break;

    case EFI_IFR_LABEL_OP:
      //
      // If the label does not match the requested update point, ignore it
      //
      if (((EFI_IFR_LABEL *) Location)->LabelId != Label) {
        //
        // Copy the label
        //
        EfiCopyMem (TempBuffer, Location, Location->Length);
        TempBuffer = TempBuffer + Location->Length;

        //
        // Go to the next Op-Code
        //
        Location = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
        continue;
      }

      if (AddData) {
        //
        // Copy the label
        //
        EfiCopyMem (TempBuffer, Location, Location->Length);
        TempBuffer = TempBuffer + Location->Length;

        //
        // Add the DataCount amount of opcodes to TempBuffer
        //
        DataLocation = (EFI_IFR_OP_HEADER *) &Data->Data;
        for (Index = 0; Index < Data->DataCount; Index++) {
          EfiCopyMem (TempBuffer, DataLocation, DataLocation->Length);
          ((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->IfrSize += DataLocation->Length;
          OtherBuffer = ((UINT8 *) &((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->StringSize + sizeof (UINTN));
          EfiCopyMem (OtherBuffer, &((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->IfrSize, 2);
          TempBuffer    = TempBuffer + DataLocation->Length;
          DataLocation  = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (DataLocation) + DataLocation->Length);
        }
        //
        // Go to the next Op-Code
        //
        Location = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
        continue;
      } else {
        //
        // Copy the label
        //
        EfiCopyMem (TempBuffer, Location, Location->Length);
        TempBuffer  = TempBuffer + Location->Length;
        Location    = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);

        //
        // Remove the DataCount amount of opcodes unless we run into an end of form or a label
        //
        for (Index = 0; Index < Data->DataCount; Index++) {
          //
          // If we are about to skip an end form - bail out, since that is illegal
          //
          if ((Location->OpCode == EFI_IFR_END_FORM_OP) || (Location->OpCode == EFI_IFR_LABEL_OP)) {
            break;
          }
          //
          // By skipping Location entries, we are in effect not copying what was previously there
          //
          ((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->IfrSize -= Location->Length;
          OtherBuffer = ((UINT8 *) &((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->StringSize + sizeof (UINTN));
          EfiCopyMem (OtherBuffer, &((EFI_HII_PACKAGE_INSTANCE *) OrigTempBuffer)->IfrSize, 2);
          Location = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
        }
      }

    default:
      EfiCopyMem (TempBuffer, Location, Location->Length);
      TempBuffer = TempBuffer + Location->Length;
      break;
    }
    //
    // Go to the next Op-Code
    //
    Location = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
  }
  //
  // Copy the last op-code left behind from the for loop
  //
  EfiCopyMem (TempBuffer, Location, Location->Length);

  //
  // Advance to beginning of strings and copy them
  //
  TempBuffer  = TempBuffer + Location->Length;
  Location    = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (Location) + Location->Length);
  EfiCopyMem (TempBuffer, Location, PackageInstance->StringSize);

  //
  // Free the old buffer, and assign into our database the latest buffer
  //
  gBS->FreePool (HandleDatabase->Buffer);
  HandleDatabase->Buffer = OrigTempBuffer;

  return EFI_SUCCESS;
}